//
//  comparator.cpp
//  Wolf
//
//  Created by Winger Cheng on 2018/2/17.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#pragma once
namespace container{
    
template<typename Key>
struct DefaultComparator {
    int operator()(const Key& left, const Key& right) const {
        if (left < right) {
            return -1;
        } else if (left > right) {
            return 1;
        } else {
            return 0;
        }
    }
};

}