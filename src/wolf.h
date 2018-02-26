//
// Created by Winger Cheng on 2018/2/26.
//

#pragma once

#include <string>
#include "core/area/shm_area.h"

#include "core/container/lockfree_skiplist.h"
#include "core/container/lockfree_linklist.h"
#include "core/container/shm_manager.h"

template <typename Data>
class Wolf{
public:
    Wolf(const std::string& index_path,
         uint32_t index_max_size = 0,
         bool index_reset_flag = false,
         const std::string& data_path = "",
         uint32_t data_max_size = 0,
         bool data_reset_flag = false): index_path_(index_path),
                                        index_max_size_(index_max_size),
                                        index_reset_flag_(index_reset_flag),
                                        data_path_(data_path),
                                        data_max_size_(data_max_size),
                                        data_reset_flag_(data_reset_flag) {
        auto* allocator = new area::ShmArea(index_path, index_max_size, index_reset_flag);
        auto* shm_manager = new container::ShmManager(allocator);

        if (data_path.empty()) {
            this->inner_data_ = new container::LockFreeSkipList<Data>(shm_manager, index_reset_flag);
        } else {
            this->index_ = new container::LockFreeSkipList<uint32_t>(shm_manager, index_reset_flag);
            this->data_area_ = new area::ShmArea(data_path, data_max_size, data_reset_flag);
        }
    }

    void Add(const Data& data) {
        if (this->inner_data_ != nullptr) {
            this->inner_data_->Add(data);
        } else {
//            auto* buffer = this->data_area_->Allocate(sizeof(Data));
//            memcpy(buffer, &data, sizeof(Data));
//
//            auto offset = (uint32_t)(buffer - this->data_area_->MemoryStart());
            //this->index_->Add(offset);
        }
    }

//    void Find(const Data& data) {
//        this->inner_data_->FindLessThan(data);
//    }

private:
    const std::string index_path_;
    uint32_t index_max_size_;
    bool index_reset_flag_;
    const std::string data_path_;
    uint32_t data_max_size_;
    bool data_reset_flag_;
    container::LockFreeSkipList<Data>* inner_data_ = nullptr;
    container::LockFreeSkipList<uint32_t>* index_ = nullptr;
    area::ShmArea* data_area_ = nullptr;
};
