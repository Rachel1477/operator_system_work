#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <memory>

// ============= 常量定义 =============
const uint32_t DISK_SIZE = 10 * 1024 * 1024;  // 10MB 虚拟磁盘
const uint32_t BLOCK_SIZE = 4096;              // 4KB 块大小
const uint32_t INODE_SIZE = 128;               // Inode 大小
const uint32_t MAX_BLOCKS = DISK_SIZE / BLOCK_SIZE;
const uint32_t MAX_INODES = 1024;              // 最大 Inode 数量
const uint32_t MAX_FILENAME = 28;              // 文件名最大长度
const uint32_t MAX_FILE_SIZE = 1024 * 1024;    // 单个文件最大 1MB
const uint32_t DIRECT_BLOCKS = 10;             // 直接块指针数量
const uint32_t INDIRECT_BLOCKS = 1;            // 间接块指针数量

// ============= 文件类型 =============
enum FileType {
    FILE_TYPE_REGULAR = 0,  // 普通文件
    FILE_TYPE_DIRECTORY = 1 // 目录
};

// ============= 权限位定义 =============
const uint16_t PERM_READ = 0x04;   // r--
const uint16_t PERM_WRITE = 0x02;  // -w-
const uint16_t PERM_EXEC = 0x01;   // --x

// rwx权限（owner, group, others）
const uint16_t PERM_OWNER_R = PERM_READ << 6;
const uint16_t PERM_OWNER_W = PERM_WRITE << 6;
const uint16_t PERM_OWNER_X = PERM_EXEC << 6;
const uint16_t PERM_GROUP_R = PERM_READ << 3;
const uint16_t PERM_GROUP_W = PERM_WRITE << 3;
const uint16_t PERM_GROUP_X = PERM_EXEC << 3;
const uint16_t PERM_OTHER_R = PERM_READ;
const uint16_t PERM_OTHER_W = PERM_WRITE;
const uint16_t PERM_OTHER_X = PERM_EXEC;

// 默认权限
const uint16_t DEFAULT_DIR_PERM = 0755;  // rwxr-xr-x
const uint16_t DEFAULT_FILE_PERM = 0644; // rw-r--r--

// ============= 超级块 =============
struct SuperBlock {
    uint32_t magic_number;       // 魔数，用于识别文件系统
    uint32_t disk_size;          // 磁盘总大小
    uint32_t block_size;         // 块大小
    uint32_t total_blocks;       // 总块数
    uint32_t total_inodes;       // 总 Inode 数
    uint32_t free_blocks;        // 空闲块数
    uint32_t free_inodes;        // 空闲 Inode 数
    uint32_t inode_bitmap_block; // Inode 位图起始块
    uint32_t data_bitmap_block;  // 数据块位图起始块
    uint32_t inode_table_block;  // Inode 表起始块
    uint32_t data_block_start;   // 数据块起始位置
    char padding[4052];          // 填充到 4096 字节

    SuperBlock() {
        magic_number = 0x12345678;
        disk_size = DISK_SIZE;
        block_size = BLOCK_SIZE;
        total_blocks = MAX_BLOCKS;
        total_inodes = MAX_INODES;
        free_blocks = total_blocks - 10; // 预留一些块给系统
        free_inodes = total_inodes - 1;  // 根目录占用一个
        inode_bitmap_block = 1;
        data_bitmap_block = 2;
        inode_table_block = 3;
        data_block_start = 3 + (MAX_INODES * INODE_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE;
        memset(padding, 0, sizeof(padding));
    }
};

// ============= Inode 结构 =============
struct Inode {
    uint32_t inode_id;                  // Inode 编号
    uint8_t file_type;                  // 文件类型（普通文件/目录）
    uint16_t permission;                // 权限（rwx）
    uint16_t owner_id;                  // 所有者 UID
    uint32_t file_size;                 // 文件大小（字节）
    uint32_t blocks_count;              // 占用的块数
    uint32_t direct_blocks[DIRECT_BLOCKS];    // 直接块指针
    uint32_t indirect_block;            // 一级间接块指针
    uint32_t create_time;               // 创建时间
    uint32_t modify_time;               // 修改时间
    char padding[36];                   // 填充到 128 字节

    Inode() {
        inode_id = 0;
        file_type = FILE_TYPE_REGULAR;
        permission = DEFAULT_FILE_PERM;
        owner_id = 0;
        file_size = 0;
        blocks_count = 0;
        memset(direct_blocks, 0, sizeof(direct_blocks));
        indirect_block = 0;
        create_time = 0;
        modify_time = 0;
        memset(padding, 0, sizeof(padding));
    }
};

// ============= 目录项 =============
struct DirectoryEntry {
    char filename[MAX_FILENAME];  // 文件名
    uint32_t inode_id;            // 对应的 Inode 编号

    DirectoryEntry() {
        memset(filename, 0, MAX_FILENAME);
        inode_id = 0;
    }

    DirectoryEntry(const char* name, uint32_t id) {
        strncpy(filename, name, MAX_FILENAME - 1);
        filename[MAX_FILENAME - 1] = '\0';
        inode_id = id;
    }
};

// ============= 用户信息 =============
struct User {
    uint16_t uid;
    std::string username;
    std::string password;
    bool is_root;

    User() : uid(0), is_root(false) {}
    User(uint16_t id, const std::string& name, const std::string& pwd, bool root = false)
        : uid(id), username(name), password(pwd), is_root(root) {}
};

// ============= 打开文件表项 =============
struct OpenFileEntry {
    uint32_t inode_id;
    int reader_count;          // 当前读者数量
    bool is_writing;           // 是否有写者
    std::mutex mutex;          // 保护本结构的互斥锁
    std::condition_variable cv; // 条件变量

    OpenFileEntry() : inode_id(0), reader_count(0), is_writing(false) {}
};

// ============= 虚拟磁盘类 =============
class VirtualDisk {
private:
    std::string disk_filename;
    std::fstream disk_file;

public:
    VirtualDisk(const std::string& filename);
    ~VirtualDisk();

    bool format();  // 格式化磁盘
    bool readBlock(uint32_t block_num, char* buffer);
    bool writeBlock(uint32_t block_num, const char* buffer);
    bool isOpen() const;
};

// ============= 文件系统类 =============
class FileSystem {
private:
    VirtualDisk* disk;
    SuperBlock super_block;
    std::vector<bool> inode_bitmap;    // Inode 位图
    std::vector<bool> data_bitmap;     // 数据块位图
    
    // 用户管理
    std::map<uint16_t, User> users;    // UID -> User
    User* current_user;                // 当前登录用户
    
    // 当前工作目录
    uint32_t current_dir_inode;        // 当前目录的 Inode 编号
    std::string current_path;          // 当前路径
    
    // 打开文件表（用于并发控制）
    std::map<uint32_t, std::shared_ptr<OpenFileEntry>> open_files;
    std::mutex open_files_mutex;

    // 内部辅助函数
    bool loadSuperBlock();
    bool saveSuperBlock();
    bool loadBitmaps();
    bool saveBitmaps();
    
    uint32_t allocateInode();
    void freeInode(uint32_t inode_id);
    uint32_t allocateDataBlock();
    void freeDataBlock(uint32_t block_id);
    
    bool readInode(uint32_t inode_id, Inode& inode);
    bool writeInode(uint32_t inode_id, const Inode& inode);
    
    bool readInodeData(const Inode& inode, char* buffer, uint32_t size);
    bool writeInodeData(Inode& inode, const char* buffer, uint32_t size);
    
    bool addDirectoryEntry(uint32_t dir_inode_id, const std::string& name, uint32_t inode_id);
    bool removeDirectoryEntry(uint32_t dir_inode_id, const std::string& name);
    std::vector<DirectoryEntry> readDirectory(uint32_t dir_inode_id);
    
    uint32_t findInodeByPath(const std::string& path);
    bool checkPermission(const Inode& inode, uint16_t required_perm);
    
    // 并发控制
    void acquireReadLock(uint32_t inode_id);
    void releaseReadLock(uint32_t inode_id);
    void acquireWriteLock(uint32_t inode_id);
    void releaseWriteLock(uint32_t inode_id);

public:
    FileSystem(const std::string& disk_file);
    ~FileSystem();

    // 初始化和格式化
    bool format();
    bool mount();
    
    // 用户管理
    bool addUser(const std::string& username, const std::string& password, bool is_root = false);
    bool login(const std::string& username, const std::string& password);
    void logout();
    User* getCurrentUser() const { return current_user; }
    
    // 文件操作
    bool createFile(const std::string& filename);
    bool createDirectory(const std::string& dirname);
    bool removeFile(const std::string& filename);
    bool removeDirectory(const std::string& dirname);
    
    bool writeFile(const std::string& filename, const std::string& content);
    std::string readFile(const std::string& filename);
    
    // 目录操作
    bool changeDirectory(const std::string& path);
    std::vector<std::pair<std::string, Inode>> listDirectory(const std::string& path = ".");
    std::string getCurrentPath() const { return current_path; }
    
    // 权限管理
    bool changePermission(const std::string& filename, uint16_t new_perm);
    bool changeOwner(const std::string& filename, uint16_t new_owner);
    
    // 辅助函数
    std::string getFileInfo(const Inode& inode);
    std::string permissionToString(uint16_t perm);
};

#endif // FILESYSTEM_H

