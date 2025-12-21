# 系统架构设计文档

## 1. 总体架构

### 1.1 分层架构

本文件系统采用经典的分层架构设计，从上到下分为五层：

```
┌─────────────────────────────────────────────┐
│  Layer 5: 用户界面层 (Shell)                 │
│  - 命令解析                                  │
│  - 用户交互                                  │
│  - 结果展示                                  │
├─────────────────────────────────────────────┤
│  Layer 4: 用户管理与权限控制层               │
│  - 用户认证                                  │
│  - 权限验证                                  │
│  - 所有者检查                                │
├─────────────────────────────────────────────┤
│  Layer 3: 文件系统逻辑层 (FileSystem)        │
│  - 文件/目录操作                             │
│  - 路径解析                                  │
│  - Inode 管理                                │
│  - 空间分配                                  │
├─────────────────────────────────────────────┤
│  Layer 2: 并发控制层                         │
│  - 读写锁                                    │
│  - 打开文件表                                │
│  - 并发同步                                  │
├─────────────────────────────────────────────┤
│  Layer 1: 虚拟磁盘层 (VirtualDisk)           │
│  - 块读写                                    │
│  - 磁盘格式化                                │
│  - 物理存储                                  │
└─────────────────────────────────────────────┘
```

### 1.2 模块关系图

```
┌──────────┐
│  Shell   │
└────┬─────┘
     │ uses
     ▼
┌──────────────┐      ┌─────────────┐
│  FileSystem  │◄─────│    User     │
└──────┬───────┘      └─────────────┘
       │ uses
       ├──────────────┐
       ▼              ▼
┌──────────────┐  ┌──────────────┐
│ VirtualDisk  │  │ OpenFileEntry│
└──────────────┘  └──────────────┘
       │
       ▼
┌──────────────┐
│  disk.bin    │ (物理文件)
└──────────────┘
```

## 2. 核心数据结构

### 2.1 SuperBlock（超级块）

**作用：** 存储文件系统的全局元数据

**结构：**
```cpp
struct SuperBlock {
    uint32_t magic_number;       // 0x12345678 (魔数)
    uint32_t disk_size;          // 10MB
    uint32_t block_size;         // 4096 字节
    uint32_t total_blocks;       // 2560 块
    uint32_t total_inodes;       // 1024 个
    uint32_t free_blocks;        // 空闲块数
    uint32_t free_inodes;        // 空闲 Inode 数
    uint32_t inode_bitmap_block; // Block 1
    uint32_t data_bitmap_block;  // Block 2
    uint32_t inode_table_block;  // Block 3
    uint32_t data_block_start;   // Block N
    char padding[4052];          // 填充到 4096
};
```

**存储位置：** Block 0

**设计考虑：**
- 魔数用于识别文件系统类型
- 记录所有关键区域的位置
- 大小正好为一个块（4096 字节）

### 2.2 Inode（索引节点）

**作用：** 存储文件/目录的元数据和数据块索引

**结构：**
```cpp
struct Inode {
    uint32_t inode_id;                     // Inode 编号
    uint8_t file_type;                     // 0=文件, 1=目录
    uint16_t permission;                   // rwxrwxrwx
    uint16_t owner_id;                     // 所有者 UID
    uint32_t file_size;                    // 字节数
    uint32_t blocks_count;                 // 占用块数
    uint32_t direct_blocks[10];            // 直接块指针
    uint32_t indirect_block;               // 间接块指针
    uint32_t create_time;                  // 创建时间戳
    uint32_t modify_time;                  // 修改时间戳
    char padding[36];                      // 填充到 128
};
```

**大小：** 128 字节

**寻址能力：**
- 直接块：10 × 4KB = 40KB
- 间接块：(4KB / 4) × 4KB = 4MB
- 总计：约 4MB（当前实现仅支持直接块）

**设计考虑：**
- 大小为 2 的幂，便于计算
- 每个块可存储 32 个 Inode (4096 / 128)
- 支持扩展到间接块和多级间接块

### 2.3 DirectoryEntry（目录项）

**作用：** 建立文件名到 Inode 的映射

**结构：**
```cpp
struct DirectoryEntry {
    char filename[28];  // 文件名（最长 27 字符）
    uint32_t inode_id;  // 对应的 Inode 编号
};
```

**大小：** 32 字节

**设计考虑：**
- 固定大小便于计算和管理
- 每个块可存储 128 个目录项 (4096 / 32)
- 文件名长度限制为 27 字符（加上 '\0'）

### 2.4 Bitmap（位图）

**作用：** 管理 Inode 和数据块的分配状态

**实现：**
```cpp
std::vector<bool> inode_bitmap;  // 1024 位
std::vector<bool> data_bitmap;   // 2560 位
```

**存储：**
- Inode 位图：Block 1（需要 128 字节）
- 数据块位图：Block 2（需要 320 字节）

**操作：**
- 分配：查找第一个为 false 的位，设为 true
- 释放：将对应位设为 false
- 时间复杂度：O(n)，可优化为 O(1)

## 3. 磁盘布局

### 3.1 整体布局

```
┌─────────────────────────────────────────────┐
│ Block 0: SuperBlock (4KB)                   │
├─────────────────────────────────────────────┤
│ Block 1: Inode Bitmap (4KB)                 │
├─────────────────────────────────────────────┤
│ Block 2: Data Block Bitmap (4KB)            │
├─────────────────────────────────────────────┤
│ Block 3-35: Inode Table (128KB)             │
│   - 1024 Inodes × 128 bytes                 │
│   - 32 Inodes per block                     │
├─────────────────────────────────────────────┤
│ Block 36-2559: Data Blocks (~10MB)          │
│   - 用户数据存储区                           │
│   - 目录内容存储区                           │
└─────────────────────────────────────────────┘
```

### 3.2 空间分配

| 区域 | 块数 | 大小 | 占比 |
|------|------|------|------|
| SuperBlock | 1 | 4KB | 0.04% |
| Inode Bitmap | 1 | 4KB | 0.04% |
| Data Bitmap | 1 | 4KB | 0.04% |
| Inode Table | 33 | 132KB | 1.29% |
| Data Blocks | 2524 | ~10MB | 98.59% |
| **总计** | **2560** | **10MB** | **100%** |

### 3.3 Inode 表布局

```
Block 3:
  [Inode 0] [Inode 1] ... [Inode 31]

Block 4:
  [Inode 32] [Inode 33] ... [Inode 63]

...

Block 35:
  [Inode 992] [Inode 993] ... [Inode 1023]
```

**计算 Inode 位置：**
```cpp
block_num = inode_table_block + (inode_id * 128) / 4096
offset = (inode_id * 128) % 4096
```

## 4. 关键算法

### 4.1 路径解析算法

**功能：** 将路径字符串转换为 Inode 编号

**算法：**
```
function findInodeByPath(path):
    if path is empty:
        return current_dir_inode
    
    if path starts with '/':
        inode_id = 0  # 从根目录开始
        path = path[1:]
    else:
        inode_id = current_dir_inode
    
    for each component in split(path, '/'):
        if component is '.' or empty:
            continue
        
        entries = readDirectory(inode_id)
        found = false
        
        for each entry in entries:
            if entry.filename == component:
                inode_id = entry.inode_id
                found = true
                break
        
        if not found:
            return INVALID
    
    return inode_id
```

**时间复杂度：** O(d × n)
- d: 路径深度
- n: 每个目录的平均文件数

### 4.2 空间分配算法

**Inode 分配：**
```
function allocateInode():
    for i from 0 to MAX_INODES:
        if not inode_bitmap[i]:
            inode_bitmap[i] = true
            free_inodes--
            saveBitmaps()
            return i
    return INVALID
```

**数据块分配：**
```
function allocateDataBlock():
    for i from data_block_start to MAX_BLOCKS:
        if not data_bitmap[i]:
            data_bitmap[i] = true
            free_blocks--
            saveBitmaps()
            return i
    return INVALID
```

**优化方向：**
- 使用空闲链表
- 记住上次分配位置
- 使用位操作加速查找

### 4.3 权限检查算法

**算法：**
```
function checkPermission(inode, required_perm):
    if current_user is root:
        return true
    
    if inode.owner_id == current_user.uid:
        # 检查所有者权限（高 3 位）
        owner_perm = (inode.permission >> 6) & 0x07
        return (owner_perm & required_perm) == required_perm
    
    # 检查其他用户权限（低 3 位）
    other_perm = inode.permission & 0x07
    return (other_perm & required_perm) == required_perm
```

**权限位布局：**
```
15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 \_____未使用_____/  \__所有者_/ \__组_/ \__其他_/
                      r  w  x   r  w  x  r  w  x
```

## 5. 并发控制机制

### 5.1 读者-写者问题

**问题描述：**
- 多个进程可能同时访问同一文件
- 读操作不修改数据，可以并发
- 写操作修改数据，必须互斥

**解决方案：** 读者优先策略

### 5.2 OpenFileEntry 结构

```cpp
struct OpenFileEntry {
    uint32_t inode_id;              // 文件标识
    int reader_count;                // 当前读者数
    bool is_writing;                 // 写标志
    std::mutex mutex;                // 互斥锁
    std::condition_variable cv;      // 条件变量
};
```

### 5.3 读锁算法

```
function acquireReadLock(inode_id):
    lock(open_files_mutex)
    if inode_id not in open_files:
        create new OpenFileEntry
    entry = open_files[inode_id]
    unlock(open_files_mutex)
    
    lock(entry.mutex)
    while entry.is_writing:
        wait(entry.cv)
    entry.reader_count++
    unlock(entry.mutex)

function releaseReadLock(inode_id):
    lock(open_files_mutex)
    entry = open_files[inode_id]
    unlock(open_files_mutex)
    
    lock(entry.mutex)
    entry.reader_count--
    if entry.reader_count == 0:
        notify_all(entry.cv)
    unlock(entry.mutex)
```

### 5.4 写锁算法

```
function acquireWriteLock(inode_id):
    lock(open_files_mutex)
    if inode_id not in open_files:
        create new OpenFileEntry
    entry = open_files[inode_id]
    unlock(open_files_mutex)
    
    lock(entry.mutex)
    while entry.reader_count > 0 or entry.is_writing:
        wait(entry.cv)
    entry.is_writing = true
    unlock(entry.mutex)

function releaseWriteLock(inode_id):
    lock(open_files_mutex)
    entry = open_files[inode_id]
    unlock(open_files_mutex)
    
    lock(entry.mutex)
    entry.is_writing = false
    notify_all(entry.cv)
    unlock(entry.mutex)
```

### 5.5 并发场景分析

**场景 1：两个读者**
```
时间 | 读者1          | 读者2          | 状态
-----|---------------|---------------|------------------
t1   | acquireRead   |               | readers=1
t2   | reading...    | acquireRead   | readers=2
t3   | reading...    | reading...    | readers=2
t4   | releaseRead   | reading...    | readers=1
t5   |               | releaseRead   | readers=0
```

**场景 2：读者和写者**
```
时间 | 读者          | 写者           | 状态
-----|--------------|---------------|------------------
t1   | acquireRead  |               | readers=1
t2   | reading...   | acquireWrite  | readers=1, 写者等待
t3   | reading...   | waiting...    | readers=1, 写者等待
t4   | releaseRead  | waiting...    | readers=0, 唤醒写者
t5   |              | writing...    | is_writing=true
t6   |              | releaseWrite  | is_writing=false
```

**场景 3：两个写者**
```
时间 | 写者1         | 写者2         | 状态
-----|--------------|--------------|------------------
t1   | acquireWrite |              | is_writing=true
t2   | writing...   | acquireWrite | 写者2等待
t3   | writing...   | waiting...   | 写者2等待
t4   | releaseWrite | waiting...   | 唤醒写者2
t5   |              | writing...   | is_writing=true
t6   |              | releaseWrite | is_writing=false
```

## 6. 文件操作流程

### 6.1 创建文件流程

```
createFile(filename)
  │
  ├─► 检查用户是否登录
  │
  ├─► 检查当前目录写权限
  │
  ├─► 分配新 Inode
  │     └─► allocateInode()
  │           └─► 查找空闲位图
  │
  ├─► 初始化 Inode
  │     ├─► 设置文件类型
  │     ├─► 设置权限 (644)
  │     ├─► 设置所有者
  │     └─► 设置时间戳
  │
  ├─► 写入 Inode 到磁盘
  │     └─► writeInode()
  │
  └─► 添加目录项
        └─► addDirectoryEntry()
              ├─► 读取目录内容
              ├─► 检查重名
              ├─► 添加新项
              └─► 写回目录
```

### 6.2 读文件流程

```
readFile(filename)
  │
  ├─► 检查用户是否登录
  │
  ├─► 查找文件 Inode
  │     └─► findInodeByPath()
  │
  ├─► 检查读权限
  │     └─► checkPermission(PERM_READ)
  │
  ├─► 获取读锁
  │     └─► acquireReadLock()
  │
  ├─► 读取数据
  │     └─► readInodeData()
  │           ├─► 遍历直接块
  │           ├─► 读取每个块
  │           └─► 拼接数据
  │
  └─► 释放读锁
        └─► releaseReadLock()
```

### 6.3 写文件流程

```
writeFile(filename, content)
  │
  ├─► 检查用户是否登录
  │
  ├─► 查找文件 Inode
  │     └─► findInodeByPath()
  │
  ├─► 检查写权限
  │     └─► checkPermission(PERM_WRITE)
  │
  ├─► 获取写锁
  │     └─► acquireWriteLock()
  │
  ├─► 释放旧数据块
  │     └─► freeDataBlock()
  │
  ├─► 分配新数据块
  │     └─► allocateDataBlock()
  │
  ├─► 写入数据
  │     └─► writeInodeData()
  │           ├─► 计算需要的块数
  │           ├─► 分配块
  │           ├─► 写入每个块
  │           └─► 更新 Inode
  │
  └─► 释放写锁
        └─► releaseWriteLock()
```

### 6.4 删除文件流程

```
removeFile(filename)
  │
  ├─► 检查用户是否登录
  │
  ├─► 查找文件 Inode
  │     └─► findInodeByPath()
  │
  ├─► 检查写权限
  │     └─► checkPermission(PERM_WRITE)
  │
  ├─► 释放数据块
  │     └─► 遍历所有块
  │           └─► freeDataBlock()
  │
  ├─► 从目录中删除
  │     └─► removeDirectoryEntry()
  │
  └─► 释放 Inode
        └─► freeInode()
```

## 7. 性能分析

### 7.1 时间复杂度

| 操作 | 时间复杂度 | 说明 |
|------|-----------|------|
| 创建文件 | O(n + m) | n=目录项数, m=位图大小 |
| 删除文件 | O(n + k) | n=目录项数, k=文件块数 |
| 读文件 | O(k) | k=文件块数 |
| 写文件 | O(k + m) | k=文件块数, m=位图大小 |
| 查找文件 | O(d × n) | d=路径深度, n=平均目录项数 |
| 列出目录 | O(n) | n=目录项数 |

### 7.2 空间复杂度

| 数据结构 | 空间占用 |
|---------|---------|
| SuperBlock | 4KB |
| Inode Bitmap | 128 字节 (1024 bits) |
| Data Bitmap | 320 字节 (2560 bits) |
| Inode Table | 128KB (1024 × 128) |
| 每个文件 | 128B + 数据大小 |
| 每个目录项 | 32 字节 |

### 7.3 性能瓶颈

1. **位图查找：** O(n) 线性查找
   - **优化：** 使用空闲链表或跳表

2. **路径解析：** 多次目录查找
   - **优化：** 实现路径缓存

3. **目录查找：** 线性搜索
   - **优化：** 使用哈希表或 B 树

4. **磁盘 I/O：** 每次操作都写磁盘
   - **优化：** 实现块缓存和延迟写入

## 8. 可扩展性设计

### 8.1 支持更大文件

**当前限制：** 40KB（10 个直接块）

**扩展方案：**
```
一级间接块：1024 个块指针 → 4MB
二级间接块：1024 × 1024 个块指针 → 4GB
三级间接块：1024 × 1024 × 1024 个块指针 → 4TB
```

### 8.2 支持更多文件

**当前限制：** 1024 个 Inode

**扩展方案：**
- 动态 Inode 分配
- 增大 Inode 表区域
- 使用 Inode 链表

### 8.3 支持用户组

**扩展结构：**
```cpp
struct Inode {
    // ... 现有字段
    uint16_t group_id;  // 添加组 ID
    // ...
};
```

**权限检查：**
```cpp
// 添加组权限检查
if (inode.group_id == current_user.gid) {
    group_perm = (inode.permission >> 3) & 0x07;
    return (group_perm & required_perm) == required_perm;
}
```

### 8.4 支持符号链接

**扩展结构：**
```cpp
enum FileType {
    FILE_TYPE_REGULAR = 0,
    FILE_TYPE_DIRECTORY = 1,
    FILE_TYPE_SYMLINK = 2  // 添加符号链接类型
};

struct Inode {
    // ... 现有字段
    char link_target[100];  // 链接目标路径
    // ...
};
```

## 9. 安全性考虑

### 9.1 权限检查

- ✅ 每次文件操作前检查权限
- ✅ root 用户拥有超级权限
- ✅ 所有者可以修改自己文件的权限

### 9.2 输入验证

- ✅ 文件名长度限制
- ✅ 路径合法性检查
- ✅ 权限值范围检查

### 9.3 资源限制

- ✅ 文件大小限制（1MB）
- ✅ 文件名长度限制（27 字符）
- ✅ Inode 数量限制（1024）

### 9.4 并发安全

- ✅ 使用互斥锁保护共享数据
- ✅ 读写锁防止数据竞争
- ✅ 条件变量避免忙等待

## 10. 测试策略

### 10.1 单元测试

- Inode 分配和释放
- 数据块分配和释放
- 权限检查逻辑
- 路径解析算法

### 10.2 集成测试

- 文件创建、读写、删除
- 目录操作
- 权限管理
- 用户切换

### 10.3 并发测试

- 多个读者同时读取
- 读写互斥测试
- 写写互斥测试

### 10.4 压力测试

- 创建大量文件
- 写入大文件
- 深层目录结构
- 磁盘空间耗尽

## 11. 总结

本文件系统采用了经典的 Unix 文件系统设计思想，实现了：

1. **完整的分层架构**
   - 清晰的模块划分
   - 良好的接口设计

2. **高效的数据结构**
   - Inode 索引机制
   - 位图空间管理
   - 目录项映射

3. **健壮的并发控制**
   - 读写锁机制
   - 条件变量同步
   - 线程安全保证

4. **灵活的权限系统**
   - Unix 风格权限
   - 多用户支持
   - 所有者控制

5. **良好的可扩展性**
   - 支持间接块
   - 支持用户组
   - 支持符号链接

这是一个高质量的教学项目，既满足了课程要求，又具有实际应用价值。

---

**文档版本：** 1.0  
**最后更新：** 2025年12月21日

