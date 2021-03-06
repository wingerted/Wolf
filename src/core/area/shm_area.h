//
//  shm_allocator.h
//  ShmArea
//
//  Created by Winger Cheng on 2018/1/14.
//  Copyright © 2018年 Winger Cheng. All rights reserved.
//

#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <atomic>
#include <exception>
#include <string>
#include <cassert>

namespace area {

struct SuperBlock {
  std::atomic<uint32_t> memory_usage{0};
  std::atomic<uint32_t> max_memory_size{0};
};

class ShmArea {
 public:
  ShmArea(const std::string &shm_path,
          uint32_t max_memory_size,
          bool force_reset = false);
  char *Allocate(uint32_t bytes);
  uint32_t MemoryUsage() const;
  char *MemoryStart() const;
  char *GetMemoryBuffer(uint32_t offset);

 private:
  uint32_t Init(uint32_t max_memory_size, bool force_reset = false);

 private:
  int shm_fd_;
  char *begin_alloc_ptr_;
  SuperBlock *super_block_;
};

}  // namespace area