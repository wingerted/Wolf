//
// Created by Winger Cheng on 2018/2/24.
//

#include "shm_manager.h"

namespace container {
    ShmManager::ShmManager(area::ShmArea *allocator) : allocator_(allocator) {}

    std::pair<uint32_t, char *> ShmManager::Allocate(int bytes) {
        char *buffer = this->allocator_->Allocate(bytes);

        return {(uint32_t) (buffer - this->allocator_->MemoryStart()), buffer};
    }

    char *ShmManager::GetBufferByOffset(uint32_t offset) {
        return this->allocator_->MemoryStart() + offset;
    }
}