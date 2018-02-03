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
    Allocator allocator("/Users/wingerted/test_shared_memory_dat");
    allocator.InitAllocator(100);
    char *mm_buf = (char *)allocator.Allocate(size);


    memcpy(mm_buf, buf, size);

    
    
    auto wolf = GetWolfEvent(mm_buf);
    std::cout << wolf->id() << std::endl;
    
    flatbuffers::FlatBufferBuilder builder2(1024);
    WolfEventBuilder wolf_builder2(builder);
    wolf_builder2.add_id(200);
    wolf_builder2.add_key(key);
    wolf_builder2.add_name(name);
    wolf_builder2.add_value(28.88);
    auto wolf_event_offset2 = wolf_builder.Finish();
    builder.Finish(wolf_event_offset2);
    char *buf2 = (char*)builder.GetBufferPointer();
    int size2 = builder.GetSize();
    memcpy(mm_buf, buf2, size2);
    
    std::cout << wolf->id() << std::endl;
    return 0;
}
