# Wolf -- a simple shared memory container

## Module

### Area
Area is the interface easy to create or load a shared memory buffer. 
It can be use in multi thread or process.
To be simple and fast, when it allocate a buffer it will never use the buffer again.

##### usage: 
~~~~
 #include "area/shm_area.h"
 
 struct TestNode {
    int test_int;
    double test_double;
    char[10] test_char_array;
 };
 
 auto shm = area::ShmArea("test_shared_memory_dat2", // shared memory file path 
                          1000,                      // shared memory max size
                          true);                     // reset shared memory flag
                          
 TestNode* buffer = (TestNode*)shm.Allocate(50);     // allocate memory just like malloc
~~~~ 


### Container
Container if a set of structure base on area. 
For more easy to use, it's designed as template.
Easy to use is the most important !

1. #### LockFreeSkipList
   
   Design for multi thread and process, it can be use by multi writer and multi reader.
   The algorithm is from *The Art of Multiprocessor Programming By Maurice Herlihy, Nir Shavit*
   Since use lock in shared memory is very very difficult so LockFreeSkipList is implement by atomic library from C++11
   current random.h is copy from *leveldb*.
   
   Atomic library can be used in shared memory and you can test it by the code shared_memory_atomic_test.cpp in test folder.
   
   ##### usage:
   ~~~~
   #include "area/shm_area.h"
   #include "container/shm_manager.h"
   #include "container/lockfree_skiplist.h"
   
   area::ShmArea area = area::ShmArea("test_shared_memory_dat", 10000000, true);
   // shm_manager for conatiner just for the interface can be easy to use
   container::ShmManager* shm_manager = new container::ShmManager(&area); 
   container::LockFreeSkipList<int> list(shm_manager, true);
   
   for (int i=0; i<20000; ++i) {
           list.Add(i);
   }
   
   list.FindLessThan(9999);
   
   ~~~~
