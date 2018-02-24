//
//  lockfree_linked_list.h
//  Wolf
//
//  Created by Winger Cheng on 2018/2/18.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#ifndef lockfree_linked_list_h
#define lockfree_linked_list_h

#include <atomic>
#include <utility>
#include "shm_manager.h"

namespace linklist {
template<typename Key>
struct DefaultComparator {
    int operator()(const Key& a, const Key& b) const {
        if (a < b) {
            return -1;
        } else if (a > b) {
            return +1;
        } else {
            return 0;
        }
    }
};

template<typename Key, class Comparator = DefaultComparator<Key>>
class LockFreeLinkList {
public:
    struct Ref;
    struct Node;
public:
    LockFreeLinkList(ShmManager* shm_manager, bool reset = false, Comparator cmp = DefaultComparator<Key>());
    Node* FindLessThan(Key key);
    bool Add(Key key);
    bool Remove(Key key);
private:
    Node* GetNodeByOffset(int offset);
    Node* NewNode(Key key);
    
    std::pair<Node*, Node*> FindWindow(Key key);
    std::pair<Node*, Node*> FindWindowInner(Node* start_node, Key key);
private:
    ShmManager* shm_manager_;
    Comparator compare_;
    Node* head_;
};

template<typename Key, class Comparator>
struct LockFreeLinkList<Key, Comparator>::Ref {
    int next_offset;
    bool mark_removed;
    char padding[3] {0}; // 这里需要字节对齐, 否则在做CAS操作时会出现对未初始化字节比较的情况
};

template<typename Key, class Comparator>
struct LockFreeLinkList<Key, Comparator>::Node {
    Node(int offset, Key k): self_offset(offset), key(k) {}
    Key key;
    int self_offset;
    std::atomic<Ref> ref;
};

template<typename Key, class Comparator>
LockFreeLinkList<Key, Comparator>::LockFreeLinkList(ShmManager* shm_manager,
                                                    bool reset, Comparator cmp): shm_manager_(shm_manager),
                                                                                 compare_(cmp){
    if (reset) {
        this->head_ = this->NewNode(0);
        this->head_->ref = {0, false};
    } else {
        this->head_ = (Node*)this->shm_manager_->GetBufferByIndex(0);
    }
}

template<typename Key, class Comparator>
auto LockFreeLinkList<Key, Comparator>::NewNode(Key key) -> Node* {
    char* node_buffer;
    int node_offset;
    std::tie(node_offset, node_buffer) = this->shm_manager_->Allocate(sizeof(Node));
    Node* node = new (node_buffer) Node(node_offset, key);
    return node;
}

template<typename Key, class Comparator>
auto LockFreeLinkList<Key, Comparator>::GetNodeByOffset(int offset) -> Node*{
    if (offset == 0) {
        return nullptr;
    } else {
        return (Node*)this->shm_manager_->GetBufferByIndex(offset);
    }
}

template<typename Key, class Comparator>
auto LockFreeLinkList<Key, Comparator>::FindWindowInner(Node* start_node, Key key) -> std::pair<Node*, Node*> {
    Node* pred = start_node;
    Node* curr = this->GetNodeByOffset(pred->ref.load().next_offset);
    while (true) {
        if (curr == nullptr) {
            return {pred, nullptr};
        }
        
        Ref pred_ref = pred->ref;
        Node* next = this->GetNodeByOffset(curr->ref.load().next_offset);
        while (curr->ref.load().mark_removed) {
            Ref curr_ref = curr->ref; // 这个Ref需要提供给CAS操作, 必须是左值
            if (!pred->ref.compare_exchange_strong(pred_ref, curr_ref)) {
                // 用两个nullptr表示查找的物理清除过程中CAS操作失败, 需要从头查找并进行物理清除
                return {nullptr, nullptr};
            } else if (next == nullptr) {
                // next 节点不存在, 遍历结束
                break;
            } else {
                curr = next;
                next = this->GetNodeByOffset(curr->ref.load().next_offset);
            }
        }
        if (this->compare_(curr->key, key) >= 0) {
            return {pred, curr};
        } else {
            pred = curr;
            curr = next;
        }
    }
}

template<typename Key, class Comparator>
auto LockFreeLinkList<Key, Comparator>::FindWindow(Key key) -> std::pair<Node*, Node*> {
    while (true) {
        Node* window_first;
        Node* window_second;
        std::tie(window_first, window_second) = this->FindWindowInner(head_, key);
        if (window_first == nullptr && window_second == nullptr) {
            continue;
        } else {
            return {window_first, window_second};
        }
    }
}

template<typename Key, class Comparator>
auto LockFreeLinkList<Key, Comparator>::FindLessThan(Key key) -> Node* {
    Node* pred = this->head_;
    Node* curr = this->GetNodeByOffset(pred->ref.load().next_offset);
    
    while (true) {
        if (curr == nullptr || this->compare_(curr->key, key) >= 0) {
            return pred;
        } else {
            Node* next = this->GetNodeByOffset(curr->ref.load().next_offset);
            pred = curr;
            curr = next;
        }
    }
}

template<typename Key, class Comparator>
bool LockFreeLinkList<Key, Comparator>::Add(Key key) {
    while (true) {
        Node* window_first;
        Node* window_second;
        std::tie(window_first, window_second) = this->FindWindow(key);
        Node* pred = window_first;
        Node* curr = window_second;
        Ref pred_ref = pred->ref;
    
        if (pred == nullptr ||
            (curr != nullptr && curr->key == key)) {
            return false;
        } else {
            Node* node = this->NewNode(key);
            if (curr == nullptr) {
                node->ref = {0, false};
            } else {
                node->ref = {curr->self_offset, false};
            }
            
            Ref next = {node->self_offset, false};
            if (pred->ref.compare_exchange_strong(pred_ref, next)) {
                return true;
            }
        }
    }
}

template<typename Key, class Comparator>
bool LockFreeLinkList<Key, Comparator>::Remove(Key key) {
    while (true) {
        Node* window_first;
        Node* window_second;
        std::tie(window_first, window_second) = this->FindWindow(key);
        Node* pred = window_first;
        Node* curr = window_second;
        if (curr == nullptr || curr->key != key) {
            return false;
        } else {
            Ref curr_ref = curr->ref;
            Ref curr_next = {curr->ref.load().next_offset, true};
            if (!curr->ref.compare_exchange_strong(curr_ref, curr_next)) {
                continue;
            }
            Ref pred_ref = pred->ref;
            pred->ref.compare_exchange_strong(pred_ref, curr_ref);
            
            return true;
        }
    }
}
}
#endif /* lockfree_linked_list_h */
