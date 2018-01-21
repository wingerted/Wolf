//
//  shm_allocator.h
//  ShmAllocator
//
//  Created by Winger Cheng on 2018/1/14.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#ifndef shm_allocator_h
#define shm_allocator_h

#include <atomic>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

struct SuperBlock {
    std::atomic<int> memory_usage {0};
    std::atomic<int> max_memory_size {0};
};

class Allocator {
public:
    explicit Allocator(const std::string& shm_path): super_block_(nullptr),
                                                     alloc_ptr_(nullptr),
                                                     shm_path_(shm_path)  {
        this->shm_fd_ = shm_open(shm_path.c_str(),
                                 O_CREAT | O_RDWR,
                                 0660);
        fchmod(this->shm_fd_, S_IRWXU | S_IRWXG);
    };
    
    void InitAllocator(){
        this->super_block_ = (SuperBlock *)mmap(NULL,
                                              sizeof(SuperBlock),
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED,
                                              this->shm_fd_,
                                              0);
        int max_memory_size = this->super_block_->max_memory_size.load();
        
        munmap(this->super_block_, sizeof(SuperBlock));
        
        char* shared_mem = (char*)mmap(NULL,
                                       max_memory_size,
                                       PROT_READ | PROT_WRITE,
                                       MAP_SHARED,
                                       this->shm_fd_,
                                       0);
        
        this->super_block_ = (SuperBlock*)shared_mem;
        
        this->alloc_ptr_ = shared_mem + sizeof(SuperBlock);
    }
    
    char* Allocate(int bytes) {
        assert(bytes > 0);
        int current_memory_usage = this->super_block_->memory_usage.load();
        int max_memory_size = this->super_block_->max_memory_size.load();
        if (current_memory_usage + bytes < max_memory_size) {
            return nullptr;
        }
        
        while(!this->super_block_->memory_usage.compare_exchange_strong(current_memory_usage,
                                                                        current_memory_usage + bytes)) {
            current_memory_usage = this->super_block_->memory_usage.load();
        }
        
        return alloc_ptr_ + current_memory_usage - bytes;
    }
    
    void Reset() {
        // TODO:: 这里应该给到SuperBlock一个引用计数,然后这里置Flag让其他内存分配都失败, 然后减引用计数, 最后reset
        this->super_block_->max_memory_size = 0;
        this->super_block_->memory_usage = 0;
    }
    
    int MemoryUsage() const {
        return this->super_block_->memory_usage.load();
    }
private:
    SuperBlock* super_block_;
    char* alloc_ptr_;
    std::string shm_path_;
    int shm_fd_;
};

#endif /* shm_allocator_h */
