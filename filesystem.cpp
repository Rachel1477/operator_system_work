#include "filesystem.h"
#include <iostream>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <iomanip>

// ============= VirtualDisk 实现 =============

VirtualDisk::VirtualDisk(const std::string& filename) : disk_filename(filename) {
    disk_file.open(disk_filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!disk_file.is_open()) {
        // 如果文件不存在，创建新文件
        disk_file.open(disk_filename, std::ios::out | std::ios::binary);
        if (disk_file.is_open()) {
            // 创建空磁盘文件
            char zero = 0;
            for (uint32_t i = 0; i < DISK_SIZE; i++) {
                disk_file.write(&zero, 1);
            }
            disk_file.close();
            // 重新以读写方式打开
            disk_file.open(disk_filename, std::ios::in | std::ios::out | std::ios::binary);
        }
    }
}

VirtualDisk::~VirtualDisk() {
    if (disk_file.is_open()) {
        disk_file.close();
    }
}

bool VirtualDisk::format() {
    if (!disk_file.is_open()) {
        return false;
    }
    
    disk_file.seekp(0);
    char zero = 0;
    for (uint32_t i = 0; i < DISK_SIZE; i++) {
        disk_file.write(&zero, 1);
    }
    disk_file.flush();
    return true;
}

bool VirtualDisk::readBlock(uint32_t block_num, char* buffer) {
    if (!disk_file.is_open() || block_num >= MAX_BLOCKS) {
        return false;
    }
    
    disk_file.seekg(block_num * BLOCK_SIZE);
    disk_file.read(buffer, BLOCK_SIZE);
    return disk_file.good();
}

bool VirtualDisk::writeBlock(uint32_t block_num, const char* buffer) {
    if (!disk_file.is_open() || block_num >= MAX_BLOCKS) {
        return false;
    }
    
    disk_file.seekp(block_num * BLOCK_SIZE);
    disk_file.write(buffer, BLOCK_SIZE);
    disk_file.flush();
    return disk_file.good();
}

bool VirtualDisk::isOpen() const {
    return disk_file.is_open();
}

// ============= FileSystem 实现 =============

FileSystem::FileSystem(const std::string& disk_file) 
    : current_user(nullptr), current_dir_inode(0), current_path("/") {
    disk = new VirtualDisk(disk_file);
    inode_bitmap.resize(MAX_INODES, false);
    data_bitmap.resize(MAX_BLOCKS, false);
}

FileSystem::~FileSystem() {
    delete disk;
}

bool FileSystem::format() {
    if (!disk->isOpen()) {
        std::cerr << "错误：磁盘未打开" << std::endl;
        return false;
    }
    
    // 格式化磁盘
    if (!disk->format()) {
        std::cerr << "错误：磁盘格式化失败" << std::endl;
        return false;
    }
    
    // 初始化超级块
    super_block = SuperBlock();
    
    // 写入超级块
    char buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);
    memcpy(buffer, &super_block, sizeof(SuperBlock));
    if (!disk->writeBlock(0, buffer)) {
        std::cerr << "错误：写入超级块失败" << std::endl;
        return false;
    }
    
    // 初始化位图
    std::fill(inode_bitmap.begin(), inode_bitmap.end(), false);
    std::fill(data_bitmap.begin(), data_bitmap.end(), false);
    
    // 根目录占用 Inode 0
    inode_bitmap[0] = true;
    super_block.free_inodes--;
    
    // 创建根目录 Inode
    Inode root_inode;
    root_inode.inode_id = 0;
    root_inode.file_type = FILE_TYPE_DIRECTORY;
    root_inode.permission = DEFAULT_DIR_PERM;
    root_inode.owner_id = 0; // root 用户
    root_inode.file_size = 0;
    root_inode.blocks_count = 0;
    root_inode.create_time = time(nullptr);
    root_inode.modify_time = root_inode.create_time;
    
    if (!writeInode(0, root_inode)) {
        std::cerr << "错误：写入根目录 Inode 失败" << std::endl;
        return false;
    }
    
    // 保存位图
    if (!saveBitmaps()) {
        std::cerr << "错误：保存位图失败" << std::endl;
        return false;
    }
    
    // 保存超级块
    if (!saveSuperBlock()) {
        std::cerr << "错误：保存超级块失败" << std::endl;
        return false;
    }
    
    // 初始化用户
    users.clear();
    addUser("root", "root", true);
    addUser("user1", "123456", false);
    addUser("user2", "123456", false);
    
    std::cout << "文件系统格式化成功！" << std::endl;
    return true;
}

bool FileSystem::mount() {
    if (!disk->isOpen()) {
        std::cerr << "错误：磁盘未打开" << std::endl;
        return false;
    }
    
    if (!loadSuperBlock()) {
        std::cerr << "错误：加载超级块失败" << std::endl;
        return false;
    }
    
    if (super_block.magic_number != 0x12345678) {
        std::cerr << "错误：无效的文件系统，请先格式化" << std::endl;
        return false;
    }
    
    if (!loadBitmaps()) {
        std::cerr << "错误：加载位图失败" << std::endl;
        return false;
    }
    
    // 初始化用户（实际项目中应该从磁盘读取）
    users.clear();
    addUser("root", "root", true);
    addUser("user1", "123456", false);
    addUser("user2", "123456", false);
    
    current_dir_inode = 0; // 根目录
    current_path = "/";
    
    std::cout << "文件系统挂载成功！" << std::endl;
    return true;
}

bool FileSystem::loadSuperBlock() {
    char buffer[BLOCK_SIZE];
    if (!disk->readBlock(0, buffer)) {
        return false;
    }
    memcpy(&super_block, buffer, sizeof(SuperBlock));
    return true;
}

bool FileSystem::saveSuperBlock() {
    char buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);
    memcpy(buffer, &super_block, sizeof(SuperBlock));
    return disk->writeBlock(0, buffer);
}

bool FileSystem::loadBitmaps() {
    char buffer[BLOCK_SIZE];
    
    // 读取 Inode 位图
    if (!disk->readBlock(super_block.inode_bitmap_block, buffer)) {
        return false;
    }
    for (uint32_t i = 0; i < MAX_INODES; i++) {
        inode_bitmap[i] = (buffer[i / 8] & (1 << (i % 8))) != 0;
    }
    
    // 读取数据块位图
    if (!disk->readBlock(super_block.data_bitmap_block, buffer)) {
        return false;
    }
    for (uint32_t i = 0; i < MAX_BLOCKS; i++) {
        data_bitmap[i] = (buffer[i / 8] & (1 << (i % 8))) != 0;
    }
    
    return true;
}

bool FileSystem::saveBitmaps() {
    char buffer[BLOCK_SIZE];
    
    // 保存 Inode 位图
    memset(buffer, 0, BLOCK_SIZE);
    for (uint32_t i = 0; i < MAX_INODES; i++) {
        if (inode_bitmap[i]) {
            buffer[i / 8] |= (1 << (i % 8));
        }
    }
    if (!disk->writeBlock(super_block.inode_bitmap_block, buffer)) {
        return false;
    }
    
    // 保存数据块位图
    memset(buffer, 0, BLOCK_SIZE);
    for (uint32_t i = 0; i < MAX_BLOCKS; i++) {
        if (data_bitmap[i]) {
            buffer[i / 8] |= (1 << (i % 8));
        }
    }
    if (!disk->writeBlock(super_block.data_bitmap_block, buffer)) {
        return false;
    }
    
    return true;
}

uint32_t FileSystem::allocateInode() {
    for (uint32_t i = 0; i < MAX_INODES; i++) {
        if (!inode_bitmap[i]) {
            inode_bitmap[i] = true;
            super_block.free_inodes--;
            saveBitmaps();
            saveSuperBlock();
            return i;
        }
    }
    return UINT32_MAX; // 没有空闲 Inode
}

void FileSystem::freeInode(uint32_t inode_id) {
    if (inode_id < MAX_INODES && inode_bitmap[inode_id]) {
        inode_bitmap[inode_id] = false;
        super_block.free_inodes++;
        saveBitmaps();
        saveSuperBlock();
    }
}

uint32_t FileSystem::allocateDataBlock() {
    for (uint32_t i = super_block.data_block_start; i < MAX_BLOCKS; i++) {
        if (!data_bitmap[i]) {
            data_bitmap[i] = true;
            super_block.free_blocks--;
            saveBitmaps();
            saveSuperBlock();
            return i;
        }
    }
    return UINT32_MAX; // 没有空闲数据块
}

void FileSystem::freeDataBlock(uint32_t block_id) {
    if (block_id < MAX_BLOCKS && data_bitmap[block_id]) {
        data_bitmap[block_id] = false;
        super_block.free_blocks++;
        saveBitmaps();
        saveSuperBlock();
    }
}

bool FileSystem::readInode(uint32_t inode_id, Inode& inode) {
    if (inode_id >= MAX_INODES) {
        return false;
    }
    
    uint32_t block_num = super_block.inode_table_block + (inode_id * INODE_SIZE) / BLOCK_SIZE;
    uint32_t offset = (inode_id * INODE_SIZE) % BLOCK_SIZE;
    
    char buffer[BLOCK_SIZE];
    if (!disk->readBlock(block_num, buffer)) {
        return false;
    }
    
    memcpy(&inode, buffer + offset, sizeof(Inode));
    return true;
}

bool FileSystem::writeInode(uint32_t inode_id, const Inode& inode) {
    if (inode_id >= MAX_INODES) {
        return false;
    }
    
    uint32_t block_num = super_block.inode_table_block + (inode_id * INODE_SIZE) / BLOCK_SIZE;
    uint32_t offset = (inode_id * INODE_SIZE) % BLOCK_SIZE;
    
    char buffer[BLOCK_SIZE];
    if (!disk->readBlock(block_num, buffer)) {
        return false;
    }
    
    memcpy(buffer + offset, &inode, sizeof(Inode));
    return disk->writeBlock(block_num, buffer);
}

bool FileSystem::readInodeData(const Inode& inode, char* buffer, uint32_t size) {
    if (size > inode.file_size) {
        size = inode.file_size;
    }
    
    uint32_t bytes_read = 0;
    char block_buffer[BLOCK_SIZE];
    
    // 读取直接块
    for (uint32_t i = 0; i < DIRECT_BLOCKS && bytes_read < size; i++) {
        if (inode.direct_blocks[i] == 0) break;
        
        if (!disk->readBlock(inode.direct_blocks[i], block_buffer)) {
            return false;
        }
        
        uint32_t to_read = std::min(BLOCK_SIZE, size - bytes_read);
        memcpy(buffer + bytes_read, block_buffer, to_read);
        bytes_read += to_read;
    }
    
    return true;
}

bool FileSystem::writeInodeData(Inode& inode, const char* buffer, uint32_t size) {
    if (size > MAX_FILE_SIZE) {
        std::cerr << "错误：文件大小超过限制" << std::endl;
        return false;
    }
    
    uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (blocks_needed > DIRECT_BLOCKS) {
        std::cerr << "错误：文件过大（当前仅支持直接块）" << std::endl;
        return false;
    }
    
    // 释放旧的数据块
    for (uint32_t i = 0; i < DIRECT_BLOCKS; i++) {
        if (inode.direct_blocks[i] != 0) {
            freeDataBlock(inode.direct_blocks[i]);
            inode.direct_blocks[i] = 0;
        }
    }
    
    // 分配新的数据块并写入数据
    uint32_t bytes_written = 0;
    char block_buffer[BLOCK_SIZE];
    
    for (uint32_t i = 0; i < blocks_needed; i++) {
        uint32_t block_id = allocateDataBlock();
        if (block_id == UINT32_MAX) {
            std::cerr << "错误：磁盘空间不足" << std::endl;
            return false;
        }
        
        inode.direct_blocks[i] = block_id;
        
        memset(block_buffer, 0, BLOCK_SIZE);
        uint32_t to_write = std::min(BLOCK_SIZE, size - bytes_written);
        memcpy(block_buffer, buffer + bytes_written, to_write);
        
        if (!disk->writeBlock(block_id, block_buffer)) {
            return false;
        }
        
        bytes_written += to_write;
    }
    
    inode.file_size = size;
    inode.blocks_count = blocks_needed;
    inode.modify_time = time(nullptr);
    
    return true;
}

std::vector<DirectoryEntry> FileSystem::readDirectory(uint32_t dir_inode_id) {
    std::vector<DirectoryEntry> entries;
    
    Inode dir_inode;
    if (!readInode(dir_inode_id, dir_inode)) {
        return entries;
    }
    
    if (dir_inode.file_type != FILE_TYPE_DIRECTORY) {
        return entries;
    }
    
    if (dir_inode.file_size == 0) {
        return entries;
    }
    
    char* buffer = new char[dir_inode.file_size];
    if (!readInodeData(dir_inode, buffer, dir_inode.file_size)) {
        delete[] buffer;
        return entries;
    }
    
    uint32_t entry_count = dir_inode.file_size / sizeof(DirectoryEntry);
    DirectoryEntry* dir_entries = reinterpret_cast<DirectoryEntry*>(buffer);
    
    for (uint32_t i = 0; i < entry_count; i++) {
        entries.push_back(dir_entries[i]);
    }
    
    delete[] buffer;
    return entries;
}

bool FileSystem::addDirectoryEntry(uint32_t dir_inode_id, const std::string& name, uint32_t inode_id) {
    Inode dir_inode;
    if (!readInode(dir_inode_id, dir_inode)) {
        return false;
    }
    
    if (dir_inode.file_type != FILE_TYPE_DIRECTORY) {
        return false;
    }
    
    // 检查是否已存在同名文件
    std::vector<DirectoryEntry> entries = readDirectory(dir_inode_id);
    for (const auto& entry : entries) {
        if (std::string(entry.filename) == name) {
            std::cerr << "错误：文件已存在" << std::endl;
            return false;
        }
    }
    
    // 添加新目录项
    DirectoryEntry new_entry(name.c_str(), inode_id);
    entries.push_back(new_entry);
    
    // 写回目录数据
    uint32_t new_size = entries.size() * sizeof(DirectoryEntry);
    char* buffer = new char[new_size];
    memcpy(buffer, entries.data(), new_size);
    
    bool result = writeInodeData(dir_inode, buffer, new_size);
    delete[] buffer;
    
    if (result) {
        writeInode(dir_inode_id, dir_inode);
    }
    
    return result;
}

bool FileSystem::removeDirectoryEntry(uint32_t dir_inode_id, const std::string& name) {
    Inode dir_inode;
    if (!readInode(dir_inode_id, dir_inode)) {
        return false;
    }
    
    if (dir_inode.file_type != FILE_TYPE_DIRECTORY) {
        return false;
    }
    
    std::vector<DirectoryEntry> entries = readDirectory(dir_inode_id);
    auto it = std::remove_if(entries.begin(), entries.end(),
        [&name](const DirectoryEntry& entry) {
            return std::string(entry.filename) == name;
        });
    
    if (it == entries.end()) {
        std::cerr << "错误：文件不存在" << std::endl;
        return false;
    }
    
    entries.erase(it, entries.end());
    
    // 写回目录数据
    if (entries.empty()) {
        dir_inode.file_size = 0;
        dir_inode.blocks_count = 0;
        for (uint32_t i = 0; i < DIRECT_BLOCKS; i++) {
            if (dir_inode.direct_blocks[i] != 0) {
                freeDataBlock(dir_inode.direct_blocks[i]);
                dir_inode.direct_blocks[i] = 0;
            }
        }
        return writeInode(dir_inode_id, dir_inode);
    }
    
    uint32_t new_size = entries.size() * sizeof(DirectoryEntry);
    char* buffer = new char[new_size];
    memcpy(buffer, entries.data(), new_size);
    
    bool result = writeInodeData(dir_inode, buffer, new_size);
    delete[] buffer;
    
    if (result) {
        writeInode(dir_inode_id, dir_inode);
    }
    
    return result;
}

uint32_t FileSystem::findInodeByPath(const std::string& path) {
    if (path.empty()) {
        return current_dir_inode;
    }
    
    uint32_t inode_id = current_dir_inode;
    
    if (path[0] == '/') {
        inode_id = 0; // 从根目录开始
    }
    
    if (path == "/" || path == ".") {
        return inode_id;
    }
    
    // 解析路径
    std::string path_copy = path;
    if (path_copy[0] == '/') {
        path_copy = path_copy.substr(1);
    }
    
    std::istringstream iss(path_copy);
    std::string component;
    
    while (std::getline(iss, component, '/')) {
        if (component.empty() || component == ".") {
            continue;
        }
        
        if (component == "..") {
            // 处理父目录（简化实现，未完全支持）
            continue;
        }
        
        // 在当前目录查找
        std::vector<DirectoryEntry> entries = readDirectory(inode_id);
        bool found = false;
        
        for (const auto& entry : entries) {
            if (std::string(entry.filename) == component) {
                inode_id = entry.inode_id;
                found = true;
                break;
            }
        }
        
        if (!found) {
            return UINT32_MAX; // 路径不存在
        }
    }
    
    return inode_id;
}

bool FileSystem::checkPermission(const Inode& inode, uint16_t required_perm) {
    if (!current_user) {
        return false;
    }
    
    // root 用户拥有所有权限
    if (current_user->is_root) {
        return true;
    }
    
    // 检查所有者权限
    if (inode.owner_id == current_user->uid) {
        uint16_t owner_perm = (inode.permission >> 6) & 0x07;
        return (owner_perm & required_perm) == required_perm;
    }
    
    // 检查其他用户权限
    uint16_t other_perm = inode.permission & 0x07;
    return (other_perm & required_perm) == required_perm;
}

void FileSystem::acquireReadLock(uint32_t inode_id) {
    std::unique_lock<std::mutex> lock(open_files_mutex);
    
    if (open_files.find(inode_id) == open_files.end()) {
        open_files[inode_id] = std::make_shared<OpenFileEntry>();
        open_files[inode_id]->inode_id = inode_id;
    }
    
    auto& entry = open_files[inode_id];
    std::unique_lock<std::mutex> file_lock(entry->mutex);
    lock.unlock();
    
    // 等待直到没有写者
    entry->cv.wait(file_lock, [&entry] { return !entry->is_writing; });
    entry->reader_count++;
}

void FileSystem::releaseReadLock(uint32_t inode_id) {
    std::unique_lock<std::mutex> lock(open_files_mutex);
    
    if (open_files.find(inode_id) == open_files.end()) {
        return;
    }
    
    auto entry = open_files[inode_id];
    lock.unlock();  // 先释放 open_files_mutex
    
    std::lock_guard<std::mutex> file_lock(entry->mutex);
    entry->reader_count--;
    if (entry->reader_count == 0) {
        entry->cv.notify_all(); // 唤醒等待的写者
    }
}

void FileSystem::acquireWriteLock(uint32_t inode_id) {
    std::unique_lock<std::mutex> lock(open_files_mutex);
    
    if (open_files.find(inode_id) == open_files.end()) {
        open_files[inode_id] = std::make_shared<OpenFileEntry>();
        open_files[inode_id]->inode_id = inode_id;
    }
    
    auto& entry = open_files[inode_id];
    std::unique_lock<std::mutex> file_lock(entry->mutex);
    lock.unlock();
    
    // 等待直到没有读者和写者
    entry->cv.wait(file_lock, [&entry] { 
        return entry->reader_count == 0 && !entry->is_writing; 
    });
    entry->is_writing = true;
}

void FileSystem::releaseWriteLock(uint32_t inode_id) {
    std::unique_lock<std::mutex> lock(open_files_mutex);
    
    if (open_files.find(inode_id) == open_files.end()) {
        return;
    }
    
    auto entry = open_files[inode_id];
    lock.unlock();  // 先释放 open_files_mutex
    
    std::lock_guard<std::mutex> file_lock(entry->mutex);
    entry->is_writing = false;
    entry->cv.notify_all(); // 唤醒所有等待者
}

// ============= 用户管理 =============

bool FileSystem::addUser(const std::string& username, const std::string& password, bool is_root) {
    // 检查用户名是否已存在
    for (const auto& pair : users) {
        if (pair.second.username == username) {
            std::cerr << "错误：用户 " << username << " 已存在" << std::endl;
            return false;
        }
    }

    // 找到一个未被占用的最小 UID
    uint16_t uid = 0;
    while (users.find(uid) != users.end()) {
        ++uid;
    }

    users[uid] = User(uid, username, password, is_root);
    return true;
}

bool FileSystem::login(const std::string& username, const std::string& password) {
    for (auto& pair : users) {
        if (pair.second.username == username && pair.second.password == password) {
            current_user = &pair.second;
            std::cout << "用户 " << username << " 登录成功！" << std::endl;
            return true;
        }
    }
    std::cerr << "错误：用户名或密码错误" << std::endl;
    return false;
}

void FileSystem::logout() {
    if (current_user) {
        std::cout << "用户 " << current_user->username << " 退出登录" << std::endl;
        current_user = nullptr;
    }
}

// ============= 文件操作 =============

bool FileSystem::createFile(const std::string& filename) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return false;
    }
    
    // 检查父目录写权限
    Inode dir_inode;
    if (!readInode(current_dir_inode, dir_inode)) {
        return false;
    }
    
    if (!checkPermission(dir_inode, PERM_WRITE)) {
        std::cerr << "错误：没有写权限" << std::endl;
        return false;
    }
    
    // 分配新 Inode
    uint32_t new_inode_id = allocateInode();
    if (new_inode_id == UINT32_MAX) {
        std::cerr << "错误：Inode 已用完" << std::endl;
        return false;
    }
    
    // 创建文件 Inode
    Inode file_inode;
    file_inode.inode_id = new_inode_id;
    file_inode.file_type = FILE_TYPE_REGULAR;
    file_inode.permission = DEFAULT_FILE_PERM;
    file_inode.owner_id = current_user->uid;
    file_inode.file_size = 0;
    file_inode.blocks_count = 0;
    file_inode.create_time = time(nullptr);
    file_inode.modify_time = file_inode.create_time;
    
    if (!writeInode(new_inode_id, file_inode)) {
        freeInode(new_inode_id);
        return false;
    }
    
    // 添加到目录
    if (!addDirectoryEntry(current_dir_inode, filename, new_inode_id)) {
        freeInode(new_inode_id);
        return false;
    }
    
    std::cout << "文件 " << filename << " 创建成功" << std::endl;
    return true;
}

bool FileSystem::createDirectory(const std::string& dirname) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return false;
    }
    
    // 检查父目录写权限
    Inode dir_inode;
    if (!readInode(current_dir_inode, dir_inode)) {
        return false;
    }
    
    if (!checkPermission(dir_inode, PERM_WRITE)) {
        std::cerr << "错误：没有写权限" << std::endl;
        return false;
    }
    
    // 分配新 Inode
    uint32_t new_inode_id = allocateInode();
    if (new_inode_id == UINT32_MAX) {
        std::cerr << "错误：Inode 已用完" << std::endl;
        return false;
    }
    
    // 创建目录 Inode
    Inode new_dir_inode;
    new_dir_inode.inode_id = new_inode_id;
    new_dir_inode.file_type = FILE_TYPE_DIRECTORY;
    new_dir_inode.permission = DEFAULT_DIR_PERM;
    new_dir_inode.owner_id = current_user->uid;
    new_dir_inode.file_size = 0;
    new_dir_inode.blocks_count = 0;
    new_dir_inode.create_time = time(nullptr);
    new_dir_inode.modify_time = new_dir_inode.create_time;
    
    if (!writeInode(new_inode_id, new_dir_inode)) {
        freeInode(new_inode_id);
        return false;
    }
    
    // 添加到父目录
    if (!addDirectoryEntry(current_dir_inode, dirname, new_inode_id)) {
        freeInode(new_inode_id);
        return false;
    }
    
    std::cout << "目录 " << dirname << " 创建成功" << std::endl;
    return true;
}

bool FileSystem::removeFile(const std::string& filename) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return false;
    }
    
    // 查找文件
    uint32_t file_inode_id = findInodeByPath(filename);
    if (file_inode_id == UINT32_MAX) {
        std::cerr << "错误：文件不存在" << std::endl;
        return false;
    }
    
    Inode file_inode;
    if (!readInode(file_inode_id, file_inode)) {
        return false;
    }

    // 如果文件正在被写入，则禁止删除（跨进程保护）
    if (file_inode.state == FILE_STATE_WRITING) {
        std::cerr << "错误：文件正在被写入，暂时无法删除" << std::endl;
        return false;
    }
    
    // 检查权限
    if (!checkPermission(file_inode, PERM_WRITE)) {
        std::cerr << "错误：没有删除权限" << std::endl;
        return false;
    }
    
    // 释放数据块
    for (uint32_t i = 0; i < DIRECT_BLOCKS; i++) {
        if (file_inode.direct_blocks[i] != 0) {
            freeDataBlock(file_inode.direct_blocks[i]);
        }
    }
    
    // 从目录中删除
    if (!removeDirectoryEntry(current_dir_inode, filename)) {
        return false;
    }
    
    // 释放 Inode
    freeInode(file_inode_id);
    
    std::cout << "文件 " << filename << " 删除成功" << std::endl;
    return true;
}

bool FileSystem::removeDirectory(const std::string& dirname) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return false;
    }
    
    // 查找目录
    uint32_t dir_inode_id = findInodeByPath(dirname);
    if (dir_inode_id == UINT32_MAX) {
        std::cerr << "错误：目录不存在" << std::endl;
        return false;
    }
    
    Inode dir_inode;
    if (!readInode(dir_inode_id, dir_inode)) {
        return false;
    }
    
    if (dir_inode.file_type != FILE_TYPE_DIRECTORY) {
        std::cerr << "错误：不是目录" << std::endl;
        return false;
    }
    
    // 检查目录是否为空
    std::vector<DirectoryEntry> entries = readDirectory(dir_inode_id);
    if (!entries.empty()) {
        std::cerr << "错误：目录不为空" << std::endl;
        return false;
    }
    
    // 检查权限
    if (!checkPermission(dir_inode, PERM_WRITE)) {
        std::cerr << "错误：没有删除权限" << std::endl;
        return false;
    }
    
    // 从父目录中删除
    if (!removeDirectoryEntry(current_dir_inode, dirname)) {
        return false;
    }
    
    // 释放 Inode
    freeInode(dir_inode_id);
    
    std::cout << "目录 " << dirname << " 删除成功" << std::endl;
    return true;
}

bool FileSystem::writeFile(const std::string& filename, const std::string& content) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return false;
    }
    
    // 查找文件
    uint32_t file_inode_id = findInodeByPath(filename);
    if (file_inode_id == UINT32_MAX) {
        std::cerr << "错误：文件不存在" << std::endl;
        return false;
    }
    
    Inode file_inode;
    if (!readInode(file_inode_id, file_inode)) {
        return false;
    }
    
    // 检查权限
    if (!checkPermission(file_inode, PERM_WRITE)) {
        std::cerr << "错误：没有写权限" << std::endl;
        return false;
    }

    // 先尝试基于磁盘状态的跨进程写锁
    if (!beginWrite(file_inode_id)) {
        // 另一个进程正在写同一文件
        return false;
    }

    // 获取进程内写锁（非交互式场景直接在这里加锁）
    acquireWriteLock(file_inode_id);

    // 写入数据
    bool result = writeInodeData(file_inode, content.c_str(), content.size());
    if (result) {
        writeInode(file_inode_id, file_inode);
        std::cout << "文件写入成功" << std::endl;
    }

    // 释放进程内写锁
    releaseWriteLock(file_inode_id);

    // 释放跨进程写锁（将状态恢复为 AVAILABLE）
    endWrite(file_inode_id);

    return result;
}

bool FileSystem::beginWrite(uint32_t inode_id) {
    // 从磁盘读取 inode 的最新状态
    Inode inode;
    if (!readInode(inode_id, inode)) {
        return false;
    }

    // 检查文件是否已经被其他进程占用（跨进程写锁）
    if (inode.state == FILE_STATE_WRITING) {
        std::cerr << "错误：文件正在被其他进程写入，请稍后再试" << std::endl;
        return false;
    }

    // 抢占写锁：设置状态为 WRITING 并写回磁盘
    inode.state = FILE_STATE_WRITING;
    if (!writeInode(inode_id, inode)) {
        std::cerr << "错误：无法获取文件写锁" << std::endl;
        return false;
    }

    return true;
}

void FileSystem::endWrite(uint32_t inode_id) {
    // 从磁盘读取 inode
    Inode inode;
    if (!readInode(inode_id, inode)) {
        return;
    }

    // 释放写锁：设置状态为 AVAILABLE 并写回磁盘
    inode.state = FILE_STATE_AVAILABLE;
    writeInode(inode_id, inode);
}

bool FileSystem::lockFileForWrite(const std::string& filename) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return false;
    }

    // 查找文件
    uint32_t file_inode_id = findInodeByPath(filename);
    if (file_inode_id == UINT32_MAX) {
        std::cerr << "错误：文件不存在" << std::endl;
        return false;
    }

    Inode file_inode;
    if (!readInode(file_inode_id, file_inode)) {
        return false;
    }

    // 检查写权限
    if (!checkPermission(file_inode, PERM_WRITE)) {
        std::cerr << "错误：没有写权限" << std::endl;
        return false;
    }

    // 尝试获取跨进程写锁（基于磁盘状态）
    if (!beginWrite(file_inode_id)) {
        return false;
    }

    // 获取进程内写锁（如果已有写者或读者，这里会阻塞，直到可以写）
    acquireWriteLock(file_inode_id);
    return true;
}

void FileSystem::unlockFileForWrite(const std::string& filename) {
    // 查找文件（如果文件已被删除则直接返回）
    uint32_t file_inode_id = findInodeByPath(filename);
    if (file_inode_id == UINT32_MAX) {
        return;
    }
    
    // 释放进程内写锁
    releaseWriteLock(file_inode_id);
    
    // 释放跨进程写锁（将磁盘状态改回 AVAILABLE）
    endWrite(file_inode_id);
}

bool FileSystem::writeFileLocked(const std::string& filename, const std::string& content) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return false;
    }

    // 查找文件
    uint32_t file_inode_id = findInodeByPath(filename);
    if (file_inode_id == UINT32_MAX) {
        std::cerr << "错误：文件不存在" << std::endl;
        return false;
    }

    Inode file_inode;
    if (!readInode(file_inode_id, file_inode)) {
        return false;
    }

    // 再次检查写权限（防御性编程）
    if (!checkPermission(file_inode, PERM_WRITE)) {
        std::cerr << "错误：没有写权限" << std::endl;
        return false;
    }

    bool result = writeInodeData(file_inode, content.c_str(), content.size());
    if (result) {
        writeInode(file_inode_id, file_inode);
        std::cout << "文件写入成功" << std::endl;
    }

    return result;
}

std::string FileSystem::readFile(const std::string& filename) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return "";
    }
    
    // 查找文件
    uint32_t file_inode_id = findInodeByPath(filename);
    if (file_inode_id == UINT32_MAX) {
        std::cerr << "错误：文件不存在" << std::endl;
        return "";
    }
    
    Inode file_inode;
    if (!readInode(file_inode_id, file_inode)) {
        return "";
    }

    // 如果文件正在被写入，则禁止读取（跨进程保护 cat）
    if (file_inode.state == FILE_STATE_WRITING) {
        std::cerr << "错误：文件正在被写入，暂时无法读取" << std::endl;
        return "";
    }
    
    // 检查权限
    if (!checkPermission(file_inode, PERM_READ)) {
        std::cerr << "错误：没有读权限" << std::endl;
        return "";
    }
    
    // 获取读锁
    acquireReadLock(file_inode_id);
    
    // 读取数据
    char* buffer = new char[file_inode.file_size + 1];
    memset(buffer, 0, file_inode.file_size + 1);
    
    if (!readInodeData(file_inode, buffer, file_inode.file_size)) {
        delete[] buffer;
        releaseReadLock(file_inode_id);
        return "";
    }
    
    std::string content(buffer, file_inode.file_size);
    delete[] buffer;
    
    // 释放读锁
    releaseReadLock(file_inode_id);
    
    return content;
}

bool FileSystem::changeDirectory(const std::string& path) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return false;
    }
    
    uint32_t target_inode_id;
    
    if (path == "..") {
        // 简化实现：不支持完整的 .. 导航
        std::cerr << "提示：当前简化实现不完全支持 .." << std::endl;
        return false;
    }
    
    target_inode_id = findInodeByPath(path);
    if (target_inode_id == UINT32_MAX) {
        std::cerr << "错误：目录不存在" << std::endl;
        return false;
    }
    
    Inode target_inode;
    if (!readInode(target_inode_id, target_inode)) {
        return false;
    }
    
    if (target_inode.file_type != FILE_TYPE_DIRECTORY) {
        std::cerr << "错误：不是目录" << std::endl;
        return false;
    }
    
    // 检查执行权限
    if (!checkPermission(target_inode, PERM_EXEC)) {
        std::cerr << "错误：没有执行权限" << std::endl;
        return false;
    }
    
    current_dir_inode = target_inode_id;
    
    // 更新当前路径
    if (path[0] == '/') {
        current_path = path;
    } else if (current_path == "/") {
        current_path = "/" + path;
    } else {
        current_path = current_path + "/" + path;
    }
    
    return true;
}

std::vector<std::pair<std::string, Inode>> FileSystem::listDirectory(const std::string& path) {
    std::vector<std::pair<std::string, Inode>> result;
    
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return result;
    }
    
    uint32_t dir_inode_id = current_dir_inode;
    if (path != ".") {
        dir_inode_id = findInodeByPath(path);
        if (dir_inode_id == UINT32_MAX) {
            std::cerr << "错误：目录不存在" << std::endl;
            return result;
        }
    }
    
    Inode dir_inode;
    if (!readInode(dir_inode_id, dir_inode)) {
        return result;
    }
    
    // 检查读权限
    if (!checkPermission(dir_inode, PERM_READ)) {
        std::cerr << "错误：没有读权限" << std::endl;
        return result;
    }
    
    std::vector<DirectoryEntry> entries = readDirectory(dir_inode_id);
    for (const auto& entry : entries) {
        Inode inode;
        if (readInode(entry.inode_id, inode)) {
            result.push_back({std::string(entry.filename), inode});
        }
    }
    
    return result;
}

bool FileSystem::changePermission(const std::string& filename, uint16_t new_perm) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return false;
    }
    
    uint32_t inode_id = findInodeByPath(filename);
    if (inode_id == UINT32_MAX) {
        std::cerr << "错误：文件不存在" << std::endl;
        return false;
    }
    
    Inode inode;
    if (!readInode(inode_id, inode)) {
        return false;
    }
    
    // 只有所有者和 root 可以修改权限
    if (!current_user->is_root && inode.owner_id != current_user->uid) {
        std::cerr << "错误：只有所有者可以修改权限" << std::endl;
        return false;
    }
    
    inode.permission = new_perm;
    inode.modify_time = time(nullptr);
    
    if (!writeInode(inode_id, inode)) {
        return false;
    }
    
    std::cout << "权限修改成功" << std::endl;
    return true;
}

bool FileSystem::changeOwner(const std::string& filename, uint16_t new_owner) {
    if (!current_user) {
        std::cerr << "错误：请先登录" << std::endl;
        return false;
    }
    
    // 只有 root 可以修改所有者
    if (!current_user->is_root) {
        std::cerr << "错误：只有 root 可以修改所有者" << std::endl;
        return false;
    }
    
    uint32_t inode_id = findInodeByPath(filename);
    if (inode_id == UINT32_MAX) {
        std::cerr << "错误：文件不存在" << std::endl;
        return false;
    }
    
    Inode inode;
    if (!readInode(inode_id, inode)) {
        return false;
    }
    
    inode.owner_id = new_owner;
    inode.modify_time = time(nullptr);
    
    if (!writeInode(inode_id, inode)) {
        return false;
    }
    
    std::cout << "所有者修改成功" << std::endl;
    return true;
}

std::string FileSystem::permissionToString(uint16_t perm) {
    std::string result;
    
    // Owner
    result += (perm & 0400) ? 'r' : '-';
    result += (perm & 0200) ? 'w' : '-';
    result += (perm & 0100) ? 'x' : '-';
    
    // Group
    result += (perm & 0040) ? 'r' : '-';
    result += (perm & 0020) ? 'w' : '-';
    result += (perm & 0010) ? 'x' : '-';
    
    // Others
    result += (perm & 0004) ? 'r' : '-';
    result += (perm & 0002) ? 'w' : '-';
    result += (perm & 0001) ? 'x' : '-';
    
    return result;
}

std::string FileSystem::getFileInfo(const Inode& inode) {
    std::ostringstream oss;
    
    // 文件类型
    oss << (inode.file_type == FILE_TYPE_DIRECTORY ? 'd' : '-');
    
    // 权限
    oss << permissionToString(inode.permission);
    
    // 所有者 UID
    oss << " " << std::setw(4) << inode.owner_id;
    
    // 文件大小
    oss << " " << std::setw(8) << inode.file_size;
    
    // 修改时间
    char time_str[20];
    struct tm* timeinfo = localtime((time_t*)&inode.modify_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", timeinfo);
    oss << " " << time_str;
    
    return oss.str();
}

