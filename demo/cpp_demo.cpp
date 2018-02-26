//
//  main.cpp
//  ShmAllocator
//
//  Created by Winger Cheng on 2018/1/14.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#include <iostream>
#include <chrono>
#include <map>
#include <set>
//#include "core/area/shm_area.h"
//
//#include "core/container/lockfree_skiplist.h"
//#include "core/container/lockfree_linklist.h"
//#include "core/container/shm_manager.h"
// #include <folly/ConcurrentSkipList.h>

// using namespace folly;

#include "wolf.h"


int main(int argc, const char * argv[]) {

    Wolf<int> wolf_home("/Users/wingerted/test_shared_memory_dat", 10000000, true);
    wolf_home.Add(1);


//    // insert code here...
//    flatbuffers::FlatBufferBuilder builder(1024);
//    auto key = builder.CreateString("Winger");
//    auto name = builder.CreateString("GrayWolf");
//    WolfEventBuilder wolf_builder(builder);
//    wolf_builder.add_id(100);
//    wolf_builder.add_key(key);
//    wolf_builder.add_name(name);
//    wolf_builder.add_value(8.88);
//    auto wolf_event_offset = wolf_builder.Finish();
//    builder.Finish(wolf_event_offset);
//
//    char *buf = (char*)builder.GetBufferPointer();
//    int size = builder.GetSize();
//
//    std::cout << "Hello, World!\n";
//     ShmAllocator allocator = ShmAllocator("/Users/wingerted/test_shared_memory_dat", UINT32_MAX - 10000000, true);
//     ShmManager* shm_manager = new ShmManager(&allocator);

    
// //
//     SkipList list1(shm_manager);
//     auto first = std::chrono::system_clock::now();
//     for (int i=0; i<20000; ++i) {
//         list1.Insert(i, i);
//     }
//     auto second = std::chrono::system_clock::now();
//     std::cout
//     << " Time: " << std::chrono::duration_cast<std::chrono::microseconds>(second - first).count() << " us"
//     << std::endl;


//     typedef ConcurrentSkipList<int> SkipListT;
//     std::shared_ptr<SkipListT> sl(SkipListT::createInstance(24));
//     {
//         // It's usually good practice to hold an accessor only during
//         // its necessary life cycle (but not in a tight loop as
//         // Accessor creation incurs ref-counting overhead).
//         //
//         // Holding it longer delays garbage-collecting the deleted
//         // nodes in the list.
//         SkipListT::Accessor accessor(sl);
//         first = std::chrono::system_clock::now();
//         for (int i=0; i<20000; ++i) {

//             accessor.insert(i);

//         }
//         second = std::chrono::system_clock::now();
//         std::cout
//         << " Time: " << std::chrono::duration_cast<std::chrono::microseconds>(second - first).count() << " us"
//         << std::endl;
//     }
    
    
    
    
    
    area::ShmArea allocator2 = area::ShmArea("/Users/wingerted/test_shared_memory_dat2", 10000000, true);
    container::ShmManager* shm_manager2 = new container::ShmManager(&allocator2);

//
    auto first = std::chrono::system_clock::now();

    container::LockFreeSkipList<int> list(shm_manager2, true);
    for (int i=0; i<20000; ++i) {
        //auto first = std::chrono::system_clock::now();
        list.Add(i);
        //auto second = std::chrono::system_clock::now();
//        std::cout
//            << "Index: " << i
//            << " Time: " << std::chrono::duration_cast<std::chrono::microseconds>(second - first).count() << " us"
//            << std::endl;
    }

    auto second = std::chrono::system_clock::now();
    std::cout
    << " Time: " << std::chrono::duration_cast<std::chrono::microseconds>(second - first).count() << " us"
    << std::endl;
////
//
    std::set<int> rb_map;
    first = std::chrono::system_clock::now();
    for (int i=0; i<20000; ++i) {
//        auto first = std::chrono::system_clock::now();
        rb_map.insert(i);
//        auto second = std::chrono::system_clock::now();
//        std::cout
//        << "Index: " << i
//        << " Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(second - first).count() << " us"
//        << std::endl;
    }
    second = std::chrono::system_clock::now();
    std::cout
    << " Time: " << std::chrono::duration_cast<std::chrono::microseconds>(second - first).count() << " us"
    << std::endl;
////    auto first = std::chrono::system_clock::now();
////    list.Add(1000000);
////    auto second = std::chrono::system_clock::now();
////    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(second - first).count() << std::endl;
//
//
//
//    //auto x = list.FindLessThan(6);
//    //auto x = list.FindLessThan(1);
//
//
//
//
//
//    //allocator.InitAllocator(100);
////    char *mm_buf = (char *)allocator.Allocate(size);
////
////
////    memcpy(mm_buf, buf, size);
//
////
////
////    auto wolf = GetWolfEvent(mm_buf);
////    std::cout << wolf->id() << std::endl;
////
////    flatbuffers::FlatBufferBuilder builder2(1024);
////    WolfEventBuilder wolf_builder2(builder);
////    wolf_builder2.add_id(200);
////    wolf_builder2.add_key(key);
////    wolf_builder2.add_name(name);
////    wolf_builder2.add_value(28.88);
////    auto wolf_event_offset2 = wolf_builder.Finish();
////    builder.Finish(wolf_event_offset2);
////    char *buf2 = (char*)builder.GetBufferPointer();
////    int size2 = builder.GetSize();
////    memcpy(mm_buf, buf2, size2);
////
////    std::cout << wolf->id() << std::endl;
    return 0;
}
