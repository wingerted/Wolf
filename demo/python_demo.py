import WolfEvent as wf
import flatbuffers
import mmap

mmap_file = open("/Users/wingerted/test_shared_memory_dat", "rb")

mmap_obj = mmap.mmap(mmap_file.fileno(), 0, prot=mmap.PROT_READ)

#x = memoryview(mmap_obj)
x = buffer(mmap_obj)

wolf = wf.WolfEvent.GetRootAsWolfEvent(x, 8)

import time
while True:
    print(wolf.Id())
    print(wolf.Name())
    time.sleep(1)
