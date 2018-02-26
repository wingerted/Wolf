//
//  skiplist.cpp
//  Wolf
//
//  Created by Winger Cheng on 2018/2/17.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//
#include <string>
#include <atomic>
#include "shm_manager.h"
#include "random.h"

class DataIndex {
    int n;
};

class SkipList {
private:
    // 这部分Node需要放在共享内存中, 所以需要是POD类型
    // TODO:: 共享内存中最大的Node数量受限于Node的size和next中存储的索引值的上限,即如果next使用long则无法做无锁mark
    // 所以会有2的32次方的索引上限, 可以考虑替换成于当前节点的相对索引来提高上限
    class Node {
    public:
        Node(int k, long data_shm_index, long self_shm_index);
        void SetNext(int n, long node_offset);
        long Next(int n);
    public:
        int key;
        long data_shm_index_;
        long self_shm_index_;
    private:
        // Array of length equal to the node height.  next_[0] is lowest level link.
        std::atomic<long> next_[12]; // 这里使用了一个数组连续内存存储的特性, 这里是申明了一个长度为1的数组, 但实际可以更长
    };
public:
    SkipList(ShmManager* shm_manager);
    Node* NewNode(int key, long data_shm_index, int height);
    void Insert(int key, long data_shm_index);
    Node* FindGreaterOrEqual(int key, Node** prev);
    int RandomHeight();
    int GetMaxHeight();
    Node* GetNodeByIndex(long self_shm_index);
private:
    Random rnd_;
    ShmManager* shm_manager_;
    Node* head_;
    const int kMaxHeight {12};
    std::atomic<int> max_height_ {1};
};

SkipList::SkipList(ShmManager* shm_manager): shm_manager_(shm_manager),
                                             rnd_(0xdeadbeef) {
    this->head_ = this->NewNode(0, 0, kMaxHeight);
    for (int i = 0; i < kMaxHeight; i++) {
        head_->SetNext(i, 0);
    }
}

SkipList::Node* SkipList::NewNode(int key, long data_shm_index, int height) {
    long node_shm_index;
    char* node_buffer;
    std::tie(node_shm_index, node_buffer) = this->shm_manager_->Allocate(sizeof(Node) + sizeof(std::atomic<int>) *
                                                                       (height - 1));
    return new (node_buffer) Node(key, data_shm_index, node_shm_index);
}

int SkipList::GetMaxHeight() {
    return this->max_height_.load();
}

int SkipList::RandomHeight() {
    // Increase height with probability 1 in kBranching
    static const unsigned int kBranching = 4;
    int height = 1;
    while (height < kMaxHeight && ((rnd_.Next() % kBranching) == 0)) {
        height++;
    }
    assert(height > 0);
    assert(height <= kMaxHeight);
    return height;
}

SkipList::Node* SkipList::GetNodeByIndex(long self_shm_index) {
    return (Node*)(this->shm_manager_->GetBufferByIndex(self_shm_index));
}

SkipList::Node* SkipList::FindGreaterOrEqual(int key, Node** prev) {
    Node* x = head_;
    int level = GetMaxHeight() - 1;
    while (true) {
        long next_index = x->Next(level);
        Node* next_node = this->GetNodeByIndex(next_index);
        if (next_index != 0 && next_node->key < key) {
            // Keep searching in this list
            x = next_node;
        } else {
            if (prev != NULL) prev[level] = x;
            if (level == 0) {
                return next_node;
            } else {
                // Switch to next list
                level--;
            }
        }
    }
}

void SkipList::Insert(int key, long data_shm_index) {
    Node* prev[kMaxHeight];
//    auto first = std::chrono::system_clock::now();
    Node* x = FindGreaterOrEqual(key, prev);
//    auto second = std::chrono::system_clock::now();
//    std::cout
//    << " Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(second - first).count() << " us"
//    << std::endl;
//
//    // Our data structure does not allow duplicate insertion
//    assert(x == NULL || !Equal(key, x->key));
//    first = std::chrono::system_clock::now();
    int height = RandomHeight();
//    second = std::chrono::system_clock::now();
//    std::cout
//    << " Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(second - first).count() << " us"
//    << std::endl;
//
//    first = std::chrono::system_clock::now();
    if (height > GetMaxHeight()) {
        for (int i = GetMaxHeight(); i < height; i++) {
            prev[i] = head_;
        }
        this->max_height_.store(height);
    }
//    second = std::chrono::system_clock::now();
//    std::cout
//    << " Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(second - first).count() << " us"
//    << std::endl;
//
//    first = std::chrono::system_clock::now();
    x = this->NewNode(key, data_shm_index, height);
    for (int i = 0; i < height; i++) {
        // NoBarrier_SetNext() suffices since we will add a barrier when
        // we publish a pointer to "x" in prev[i].
        x->SetNext(i, prev[i]->Next(i));
        prev[i]->SetNext(i, x->self_shm_index_);
    }
//    second = std::chrono::system_clock::now();
//    std::cout
//    << " Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(second - first).count() << " us"
//    << std::endl;
}

SkipList::Node::Node(int k, long data_shm_index, long self_shm_index) : key(k),
                                                                        data_shm_index_(data_shm_index),
                                                                        self_shm_index_(self_shm_index) { }

long SkipList::Node::Next(int n) {
    return this->next_[n].load();
}

void SkipList::Node::SetNext(int n, long node_offset) {
    this->next_[n].store(node_offset);
}

