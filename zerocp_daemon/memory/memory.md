MemoryManager (内存管理器)
    │
    ├─ MemPool #1 (内存池 1) ← 管理 128 字节的 Chunk
    │    │
    │    ├─ MpmcLoFFLi (管理区)
    │    │    └─ Index_t[] (索引数组)
    │    │
    │    └─ Chunk Array (数据区)
    │         ├─ Chunk 0 (块 0)
    │         ├─ Chunk 1 (块 1)
    │         ├─ Chunk 2 (块 2)
    │         └─ ... (共 10000 个)
    │
    ├─ MemPool #2 (内存池 2) ← 管理 1024 字节的 Chunk
    │    │
    │    ├─ MpmcLoFFLi (管理区)
    │    │    └─ Index_t[] (索引数组)
    │    │
    │    └─ Chunk Array (数据区)
    │         ├─ Chunk 0
    │         ├─ Chunk 1
    │         └─ ... (共 5000 个)
    │
    └─ MemPool #3 (内存池 3) ← 管理 4096 字节的 Chunk
         │
         ├─ MpmcLoFFLi (管理区)
         │    └─ Index_t[] (索引数组)
         │
         └─ Chunk Array (数据区)
              ├─ Chunk 0
              ├─ Chunk 1
              └─ ... (共 1000 个)

