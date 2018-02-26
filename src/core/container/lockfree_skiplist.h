//
//  skiplist.cpp
//  Wolf
//
//  Created by Winger Cheng on 2018/2/17.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//
// Key必须是POD类型
#pragma once

#include <string>
#include <atomic>
#include <random>
#include <chrono>

#include "comparator.h"
#include "shm_manager.h"
#include "random.h"

namespace container{

template<typename Key, class Comparator = DefaultComparator<Key>>
class LockFreeSkipList {
public:
    const static int kMaxHeight {12};
private:
    // 这部分Node需要放在共享内存中, 所以需要是POD类型
    // TODO:: 共享内存中最大的Node数量受限于Node的size和next中存储的索引值的上限,即如果next使用long则无法做无锁mark
    // 所以会有2的32次方的索引上限, 可以考虑替换成于当前节点的相对索引来提高上限
    struct Node;
    struct Ref;
public:
    LockFreeSkipList(ShmManager* shm_manager,
                     bool reset = false,
                     Comparator cmp = DefaultComparator<Key>());
    
    bool Add(Key key);
    bool Remove(Key key);
    Node* FindLessThan(Key key);
    
private:
    int RandomHeight();
    Node* NewNode(Key key, int height);
    Node* GetNodeByOffset(uint32_t offset);
    bool FindWindow(Key key,
                    Node** first_nodes,
                    Node** second_nodes);
    bool FindWindowInner(Node* start_node,
                         Key key,
                         Node** first_nodes,
                         Node** second_nodes,
                         bool* error_flag);
private:
    //std::minstd_rand rnd_;
    Random rnd_;
    Comparator compare_;
    ShmManager* shm_manager_;
    LockFreeSkipList<Key, Comparator>::Node* head_;
};

template<typename Key, class Comparator>
struct LockFreeSkipList<Key, Comparator>::Ref {
    Ref() = default;
    Ref(uint32_t offset, bool mark): next_offset(offset), mark_removed(mark) {}
    Ref(Node* node) {
        if (node != nullptr) {
            next_offset = node->self_offset;
        }
    }
    uint32_t next_offset {0};
    bool mark_removed {false};
    char padding[3] {0}; //这里需要手工初始化字节对齐的字节
};

template<typename Key, class Comparator>
struct LockFreeSkipList<Key, Comparator>::Node {
    Node(int in_key, uint32_t in_offset, int in_height): height(in_height),
                                                         key(in_key),
                                                         self_offset(in_offset) {
        for (int i=0; i<kMaxHeight; ++i) {
            this->ref[i] = {0, false};
        }
    }

    void SetRef(int in_height, Ref in_ref) {
        ref[in_height].store(in_ref);
    }
    
    std::atomic<Ref>* GetRef(int in_height) {
        return &(ref[in_height]);
    }

    int height;
    Key key;
    uint32_t self_offset;
    std::atomic<Ref> ref[kMaxHeight];
};

template<typename Key, class Comparator>
LockFreeSkipList<Key, Comparator>::LockFreeSkipList(ShmManager* shm_manager,
                                                    bool reset,
                                                    Comparator cmp):
    //rnd_(static_cast<unsigned int>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))),
    rnd_(0xdeadbeef),
    compare_(cmp),
    shm_manager_(shm_manager) {
        
    if (reset) {
        this->head_ = this->NewNode(0, kMaxHeight);
        for (int i = 0; i < kMaxHeight; i++) {
            this->head_->SetRef(i, {0, false});
        }
    } else {
        this->head_ = (Node*) this->shm_manager_->GetBufferByOffset(0);
    }

}

template<typename Key, class Comparator>
auto LockFreeSkipList<Key, Comparator>::GetNodeByOffset(uint32_t offset) -> Node*{
    if (offset == 0) {
        return nullptr;
    } else {
        return (Node*) this->shm_manager_->GetBufferByOffset(offset);
    }
}
    
template<typename Key, class Comparator>
auto LockFreeSkipList<Key, Comparator>::NewNode(Key key, int height) -> Node* {
    uint32_t node_offset;
    char* node_buffer;
    std::tie(node_offset, node_buffer) = this->shm_manager_->Allocate(sizeof(Node));
    return new (node_buffer) Node(key, node_offset, height);
}

template<typename Key, class Comparator>
bool LockFreeSkipList<Key, Comparator>::FindWindow(Key key,
                                                   Node** first_nodes,
                                                   Node** second_nodes) {
    while (true) {
        bool error_flag = false;
        bool succ = this->FindWindowInner(head_, key, first_nodes, second_nodes, &error_flag);
        if (error_flag) {
            continue;
        } else {
            return succ;
        }
    }
}

template<typename Key, class Comparator>
auto LockFreeSkipList<Key, Comparator>::FindLessThan(Key key) -> Node* {
    Node* pred = this->head_;
    for (int i=kMaxHeight-1; i>=0; --i) {
        Node* curr = this->GetNodeByOffset(pred->GetRef(i)->load().next_offset);
        while (true) {
            if (curr == nullptr) {
                break;
            } else {
                while (curr->GetRef(i)->load().mark_removed) {
                    curr = this->GetNodeByOffset(curr->GetRef(i)->load().next_offset);
                    if (curr == nullptr) {
                        break;
                    }
                }
                if (curr == nullptr) {
                    break;
                }
                if (this->compare_(curr->key, key) < 0) {
                    pred = curr;
                    curr = this->GetNodeByOffset(curr->GetRef(i)->load().next_offset);
                } else {
                    break;
                }
            }
        }
    }
    
    return pred;
}
    
template<typename Key, class Comparator>
bool LockFreeSkipList<Key, Comparator>::FindWindowInner(Node* start_node,
                                                        Key key,
                                                        Node** first_nodes,
                                                        Node** second_nodes,
                                                        bool* error_flag) {
    Node* pred = start_node;
    Node* curr = nullptr;
    for (int height = kMaxHeight-1; height >= 0; height--) {
        curr = this->GetNodeByOffset(pred->GetRef(height)->load().next_offset);
        while (true) {
            if (curr == nullptr) {
                first_nodes[height] = pred;
                second_nodes[height] = nullptr;
                break;
            }

            Node* next = this->GetNodeByOffset(curr->GetRef(height)->load().next_offset);
            while (curr->GetRef(height)->load().mark_removed) {  // mark_removed是单向的,一旦被标记,不会被改回去
                Ref ref_to_curr = {curr->self_offset, false};
                Ref ref_to_next = {next};
                if (!pred->GetRef(height)->compare_exchange_strong(ref_to_curr, ref_to_next)) {
                    // 用两个nullptr表示查找的物理清除过程中CAS操作失败, 需要从头查找并进行物理清除
                    *error_flag = true;
                    return false;
                } else if (next == nullptr) {
                    // next 节点不存在, 遍历删除结束
                    break;
                } else {
                    curr = next;
                    next = this->GetNodeByOffset(curr->GetRef(height)->load().next_offset);
                }
            }
            if (this->compare_(curr->key, key) >= 0) {
                break;
            } else {
                pred = curr;
                curr = next;
            }
        }
        first_nodes[height] = pred;
        second_nodes[height] = curr;
    }
    
    *error_flag = false; // 这里已经进行到最底层,说明这段遍历删除没有CAS失败
    return curr == nullptr ? false : (curr->key == key); // 这里已经进行到最底层, 如果找到key则返回true
}

template<typename Key, class Comparator>
int LockFreeSkipList<Key, Comparator>::RandomHeight() {
    
    static const unsigned int kBranching = 4;
    int height = 1;
    while (height < kMaxHeight && ((rnd_.Next() % kBranching) == 0)) {
        height++;
    }
    assert(height > 0);
    assert(height <= kMaxHeight);
    return height;
//
//    std::uniform_int_distribution<unsigned> u(0, kMaxHeight - 1);
//    return u(this->rnd_);
}

template<typename Key, class Comparator>
bool LockFreeSkipList<Key, Comparator>::Add(Key key) {
    int height = this->RandomHeight();

    Node* first_nodes[kMaxHeight];
    Node* second_nodes[kMaxHeight];
    
    while (true) {
//        auto first = std::chrono::system_clock::now();
        bool found = this->FindWindow(key, first_nodes, second_nodes);
//        auto second = std::chrono::system_clock::now();
//        std::cout
//        << " Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(second - first).count() << " us"
//        << std::endl;
        if (found) {
            // 不支持重复的Key
            return false;
        } else {
            Node* new_node = this->NewNode(key, height);
            for (int i=0; i<height; ++i) {
                Ref ref_to_second = {second_nodes[i]};
                new_node->SetRef(i, ref_to_second);
            }
            
            Ref ref_to_second_bottom = {second_nodes[0]};
            Ref ref_to_new_node_bottom = {new_node->self_offset, false};
            if (!first_nodes[0]->GetRef(0)->compare_exchange_strong(ref_to_second_bottom,
                                                                    ref_to_new_node_bottom)){
                continue;
            }
            for (int i=1; i<height; ++i) {
                while (true) {
                    Node* first = first_nodes[i];
                    Node* second = second_nodes[i];
                    Ref ref_to_new_node = {new_node->self_offset, false};
                    Ref ref_to_second = {second};
                    if (first->GetRef(i)->compare_exchange_strong(ref_to_second,
                                                                  ref_to_new_node)) {
                        break;
                    } else {
                        this->FindWindow(key, first_nodes, second_nodes);
                    }
                }
            }
            return true;
        }
    }
}
    
template<typename Key, class Comparator>
bool LockFreeSkipList<Key, Comparator>::Remove(Key key) {
    Node* first_nodes[kMaxHeight];
    Node* second_nodes[kMaxHeight];
    while (true) {
        bool found = this->FindWindow(key, first_nodes, second_nodes);
        if (!found) {
            return false;
        } else {
            Node* node_need_remove = second_nodes[0];
            for (int i=node_need_remove->height - 1; i>=1; --i) {
                Ref node_need_remove_expect_ref = node_need_remove->GetRef(i)->load();
                Ref node_need_remove_exchange_ref = node_need_remove_expect_ref;
                node_need_remove_exchange_ref.mark_removed = true;
                while (!node_need_remove->GetRef(i)->load().mark_removed) {
                    node_need_remove->GetRef(i)->compare_exchange_strong(node_need_remove_expect_ref,
                                                                         node_need_remove_exchange_ref);
                    node_need_remove_expect_ref = node_need_remove->GetRef(i)->load();
                }
            }
            Ref node_need_remove_expect_ref = node_need_remove->GetRef(0)->load();
            Ref node_need_remove_exchange_ref = node_need_remove_expect_ref;
            node_need_remove_exchange_ref.mark_removed = true;
            while (true) {
                bool succ = node_need_remove->GetRef(0)->compare_exchange_strong(node_need_remove_expect_ref,
                                                                                 node_need_remove_exchange_ref);
                node_need_remove_expect_ref = node_need_remove->GetRef(0)->load();
                if (succ) {
                    this->FindWindow(key, first_nodes, second_nodes);
                    return true;
                } else if (node_need_remove_expect_ref.mark_removed) {
                    return false;
                }
            }
        }
    }

} 
}
