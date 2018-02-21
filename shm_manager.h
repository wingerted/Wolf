//
//  shared_memory_manager.h
//  Wolf
//
//  Created by Winger Cheng on 2018/2/17.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#ifndef shm_manager_h
#define shm_manager_h

#include <utility>
#include "shm_allocator.h"


class ShmManager {
public:
    ShmManager(ShmAllocator* allocator);
    std::pair<uint32_t, char*> Allocate(int bytes);
    char* GetBufferByIndex(uint32_t index);
private:
    ShmAllocator* allocator_;
};

ShmManager::ShmManager(ShmAllocator* allocator): allocator_(allocator) {}

std::pair<uint32_t, char*> ShmManager::Allocate(int bytes) {
    char* buffer = this->allocator_->Allocate(bytes);
    
    return {(uint32_t)(buffer - this->allocator_->MemoryStart()), buffer};
}

char* ShmManager::GetBufferByIndex(uint32_t index) {
    return this->allocator_->MemoryStart() + index;
}


#endif /* shm_manager_h */
