//
// Created by Winger Cheng on 2018/2/24.
//

#include "shm_area.h"

namespace area {
    ShmArea::ShmArea(const std::string &shm_path,
                     uint32_t max_memory_size,
                     bool force_reset): begin_alloc_ptr_(nullptr),
                                        super_block_(nullptr) {
        this->shm_fd_ = open(shm_path.c_str(), O_CREAT | O_RDWR, 0660);

        fchmod(this->shm_fd_, S_IRWXU | S_IRWXG);

        uint32_t memory_file_size = this->Init(max_memory_size, force_reset);
        // 共享内存文件已经初始化完毕, 进行成员变量填充
        char *shared_mem = (char *)mmap(NULL,
                                        memory_file_size,
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED,
                                        this->shm_fd_,
                                        0);
        this->super_block_ = (SuperBlock *)shared_mem;
        this->begin_alloc_ptr_ = shared_mem + sizeof(SuperBlock);
    };

    char* ShmArea::Allocate(uint32_t bytes) {
        assert(bytes > 0);
        uint32_t current_memory_usage = this->super_block_->memory_usage.load();
        uint32_t max_memory_size = this->super_block_->max_memory_size.load();

        if (bytes > max_memory_size - current_memory_usage) {
            throw ShmAreaException(bytes, current_memory_usage, max_memory_size);
        }

        // 对共享内存中存储的memory_usage进行CAS操作,
        // 如果之前获取的current_memory_usage与当前共享内存中的值不一致
        // 则共享内存在此期间已经对外分配过, 需要重新load current_memory_usage的值,
        // 再次进行CAS操作, 以保证获取到bytes字节不会被别的线程/进程使用
        while (!this->super_block_->memory_usage.compare_exchange_strong(
                current_memory_usage, current_memory_usage + bytes)) {
            current_memory_usage = this->super_block_->memory_usage.load();
        }

        // 对于同一块共享内存来说begin_alloc_ptr永远不变, 所以current_memory_usage +
        // begin_alloc_ptr可以确定一块内存的分配点
        return this->begin_alloc_ptr_ + current_memory_usage;
    }

    uint32_t ShmArea::MemoryUsage() const {
        return this->super_block_->memory_usage.load();
    }

    char* ShmArea::MemoryStart() const {
        return this->begin_alloc_ptr_;
    }

    // Init函数可以多进程/线程调用, 但是实际对SharedMemory的Init操作只会被执行一次
    // force_reset很危险, 可以直接初始化SuperBlock中的参数,
    // 无论这块共享内存是否初始化过
    uint32_t ShmArea::Init(uint32_t max_memory_size, bool force_reset) {
        assert(max_memory_size > 0);
        // 对共享内存文件上锁, lockf基于POSIX是线程安全的上锁方式
        ::lockf(this->shm_fd_, F_LOCK, 0);

        // 为了获取当前文件的大小
        struct stat file_stat;
        if (::fstat(this->shm_fd_, &file_stat) == -1) {
            /* TODO: check the value of errno */
        }

        uint32_t init_memory_file_size = (uint32_t)file_stat.st_size;

        // 如果文件大小小于SuperBlock的大小, 则认为这块共享内存文件没有初始化过
        if (force_reset || init_memory_file_size < sizeof(SuperBlock)) {
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
}