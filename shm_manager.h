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
#include "allocator.h"


class ShmManager {
public:
    ShmManager(Allocator* allocator);
    std::pair<long, char*> Allocate(int bytes);
    char* GetBufferByIndex(long index);
private:
    Allocator* allocator_;
};

ShmManager::ShmManager(Allocator* allocator): allocator_(allocator) {}

std::pair<long, char*> ShmManager::Allocate(int bytes) {
    char* buffer = this->allocator_->Allocate(bytes);
    
    return {(long)(buffer - this->allocator_->MemoryStart()), buffer};
}

char* ShmManager::GetBufferByIndex(long index) {
    return this->allocator_->MemoryStart() + index;
}


#endif /* shm_manager_h */
