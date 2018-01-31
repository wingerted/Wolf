//
//  main.cpp
//  ShmAllocator
//
//  Created by Winger Cheng on 2018/1/14.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#include <iostream>
#include "allocator.h"
#include "wolf_generated.h"



int main(int argc, const char * argv[]) {
    // insert code here...
    flatbuffers::FlatBufferBuilder builder(1024);
    auto key = builder.CreateString("Winger");
    auto name = builder.CreateString("GrayWolf");
    WolfEventBuilder wolf_builder(builder);
    wolf_builder.add_id(100);
    wolf_builder.add_key(key);
    wolf_builder.add_name(name);
    wolf_builder.add_value(8.88);
    auto wolf_event_offset = wolf_builder.Finish();
    builder.Finish(wolf_event_offset);
    
    char *buf = (char*)builder.GetBufferPointer();
    int size = builder.GetSize();
    
    std::cout << "Hello, World!\n";
    Allocator allocator("/test_a");
    allocator.InitAllocator(100);
    char *mm_buf = (char *)allocator.Allocate(size);


    memcpy(mm_buf, buf, size);
    
    return 0;
}
