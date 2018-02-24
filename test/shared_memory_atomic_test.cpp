//
//  shared_memory_atomic_test.cpp
//  Wolf
//
//  Created by Winger Cheng on 2018/2/4.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#include <atomic>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <thread>

struct SuperBlock {
    std::atomic<int> memory_usage {0};
    std::atomic<int> max_memory_size {0};
};

class TestShmAllocator {
public:
    explicit TestShmAllocator(const std::string& shm_path):
        super_block_(nullptr),
        shm_path_(shm_path)  {
        
        this->shm_fd_ = shm_open(shm_path.c_str(),
                             O_CREAT | O_RDWR,
                             0660);
        
        fchmod(this->shm_fd_, S_IRWXU | S_IRWXG);
    };
    
    void InitAllocator(int max_memory_size){
        ftruncate(this->shm_fd_, sizeof(SuperBlock) + max_memory_size);
        this->super_block_ = (SuperBlock *)mmap(NULL,
                                                sizeof(SuperBlock),
                                                PROT_READ | PROT_WRITE,
                                                MAP_SHARED,
                                                this->shm_fd_,
                                                0);
        
        this->super_block_->max_memory_size.store(max_memory_size);
        this->super_block_->memory_usage.store(0);
        munmap(this->super_block_, sizeof(SuperBlock));
    }
    
    void TestAlloc(int src, int dst) {
        this->super_block_ = (SuperBlock *)mmap(NULL,
                                                sizeof(SuperBlock),
                                                PROT_READ | PROT_WRITE,
                                                MAP_SHARED,
                                                this->shm_fd_,
                                                0);
        while (true) {
            auto test_src = src; // test_src will be set to the value cause to fail, so set test_src to src in loop
            if (this->super_block_->memory_usage.compare_exchange_strong(test_src, dst)) {
                if (!this->super_block_->memory_usage.compare_exchange_strong(dst, test_src)) {
                    std::cout << "Test Fail" << std::endl;
                }
            } else {
                std::cout << "Src: " << src << " "
                          << "Dst: " << dst << " "
                          << "Now: " << this->super_block_->memory_usage.load()
                          << std::endl;
            }
        }
    }

    void operator()(int src, int dst) {
        this->TestAlloc(src, dst);
    }
    
private:
    SuperBlock* super_block_;
    std::string shm_path_;
    int shm_fd_;
};

int main() {
    TestShmAllocator init_alloc = TestShmAllocator("/test_atomic");
    init_alloc.InitAllocator(1000);
    
    TestShmAllocator test_alloc_10 = TestShmAllocator("/test_atomic");

    TestShmAllocator test_alloc_11 = TestShmAllocator("/test_atomic");
    
    std::thread test_thread_10(test_alloc_10, 0, 10);
    test_thread_10.detach();
    
    std::thread test_thread_11(test_alloc_11, 0, 11);
    test_thread_11.detach();

    std::cout << "..." << std::endl;
    sleep(10000000);
    return 0;
}
