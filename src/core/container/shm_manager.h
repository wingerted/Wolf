//
//  shared_memory_manager.h
//  Wolf
//
//  Created by Winger Cheng on 2018/2/17.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#pragma once

#include <utility>
#include "core/area/shm_area.h"

namespace container {

class ShmManager {
public:
    ShmManager(area::ShmArea* allocator);
    std::pair<uint32_t, char*> Allocate(int bytes);
    char* GetBufferByOffset(uint32_t offset);
private:
    area::ShmArea* allocator_;
};

}