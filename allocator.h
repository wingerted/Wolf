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
#include <unistd.h>

struct SuperBlock {
    std::atomic<int> memory_usage {0};
    std::atomic<int> max_memory_size {0};
};

class Allocator {
public:
    static Allocator& GetInstance(const std::string& shm_path,
                                  int max_memory_size = 0,
                                  bool force_reset = false) {
        static Allocator instance(shm_path, max_memory_size, force_reset);
        return instance;
    }
    
    char* Allocate(int bytes) {
        assert(bytes > 0);
        int current_memory_usage = this->super_block_->memory_usage.load();
        int max_memory_size = this->super_block_->max_memory_size.load();
        if (current_memory_usage + bytes > max_memory_size) {
            return nullptr;
        }
        
        // 对共享内存中存储的memory_usage进行CAS操作, 如果之前获取的current_memory_usage与当前共享内存中的值不一致
        // 则共享内存在此期间已经对外分配过, 需要重新load current_memory_usage的值, 再次进行CAS操作,
        // 以保证获取到bytes字节不会被别的线程/进程使用
        while(!this->super_block_->memory_usage.compare_exchange_strong(current_memory_usage,
                                                                        current_memory_usage + bytes)) {
            current_memory_usage = this->super_block_->memory_usage.load();
        }
        
        // 对于同一块共享内存来说begin_alloc_ptr永远不变, 所以current_memory_usage + begin_alloc_ptr可以确定一块内存的分配点
        return this->begin_alloc_ptr_ + current_memory_usage;
    }
    
    int MemoryUsage() const {
        return this->super_block_->memory_usage.load();
    }
    
    char* MemoryStart() const {
        return this->begin_alloc_ptr_;
    }
    
private:
    Allocator(const std::string& shm_path, int max_memory_size = 0, bool force_reset = false):
    super_block_(nullptr),
    begin_alloc_ptr_(nullptr) {
        
        this->shm_fd_ = open(shm_path.c_str(),
                             O_CREAT | O_RDWR,
                             0660);
        
        fchmod(this->shm_fd_, S_IRWXU | S_IRWXG);
        
        int memory_file_size = this->Init(max_memory_size, force_reset);
        // 共享内存文件已经初始化完毕, 进行成员变量填充
        char* shared_mem = (char*)mmap(NULL,
                                       memory_file_size,
                                       PROT_READ | PROT_WRITE,
                                       MAP_SHARED,
                                       this->shm_fd_,
                                       0);
        this->super_block_ = (SuperBlock*)shared_mem;
        this->begin_alloc_ptr_ = shared_mem + sizeof(SuperBlock);
    };
    
    // Init函数可以多进程/线程调用, 但是实际对SharedMemory的Init操作只会被执行一次
    // force_reset很危险, 可以直接初始化SuperBlock中的参数, 无论这块共享内存是否初始化过
    int Init(int max_memory_size, bool force_reset = false){
        // 对共享内存文件上锁, lockf基于POSIX是线程安全的上锁方式
        ::lockf(this->shm_fd_, F_LOCK, 0);
        
        // 为了获取当前文件的大小
        struct stat file_stat;
        if (::fstat(this->shm_fd_, &file_stat) == -1) {
            /* TODO: check the value of errno */
        }
        
        int init_memory_file_size = (int)file_stat.st_size;
        
        // 如果文件大小小于SuperBlock的大小, 则认为这块共享内存文件没有初始化过
        if (max_memory_size > 0 &&
            (force_reset || init_memory_file_size < sizeof(SuperBlock))) {
            init_memory_file_size = sizeof(SuperBlock) + max_memory_size;
            ::ftruncate(this->shm_fd_, init_memory_file_size);
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
        
        // 成员变量的填充不需要对文件进行操作, 所以在这里就可以解锁
        ::lockf(this->shm_fd_, F_ULOCK, 0);
        
        return init_memory_file_size;
    }

private:
    int shm_fd_;
    char* begin_alloc_ptr_;
    SuperBlock* super_block_;
};

#endif /* shm_allocator_h */
