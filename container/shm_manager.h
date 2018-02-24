//
//  shared_memory_manager.h
//  Wolf
//
//  Created by Winger Cheng on 2018/2/17.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#pragma once

#include <utility>
#include "area/shm_area.h"

namespace container {

class ShmManager {
public:
    ShmManager(area::ShmArea* allocator);
    std::pair<uint32_t, char*> Allocate(int bytes);
    char* GetBufferByIndex(uint32_t index);
private:
    area::ShmArea* allocator_;
};

ShmManager::ShmManager(area::ShmArea* allocator): allocator_(allocator) {}

std::pair<uint32_t, char*> ShmManager::Allocate(int bytes) {
    char* buffer = this->allocator_->Allocate(bytes);
    
    return {(uint32_t)(buffer - this->allocator_->MemoryStart()), buffer};
}

char* ShmManager::GetBufferByIndex(uint32_t index) {
    return this->allocator_->MemoryStart() + index;
}

}