//
//  main.cpp
//  ShmAllocator
//
//  Created by Winger Cheng on 2018/1/14.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#include <iostream>
#include "allocator.h"



int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    Allocator allocator("/tmp/test");
    return 0;
}
