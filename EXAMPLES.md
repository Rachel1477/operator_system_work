# 使用示例

本文档提供了多用户文件系统的详细使用示例。

## 示例 1：基本文件操作

```bash
# 启动程序
./myfs

# 格式化文件系统
format
yes

# 挂载文件系统
mount

# 登录 root 用户
login
root
root

# 创建目录
mkdir documents
mkdir pictures

# 查看目录
ls

# 进入 documents 目录
cd documents

# 查看当前路径
pwd

# 创建文件
touch readme.txt
touch notes.txt

# 写入文件内容
write readme.txt
This is a readme file.
It contains important information.
EOF

# 读取文件内容
cat readme.txt

# 查看文件详细信息
ls

# 返回根目录
cd /

# 退出
exit
```

**预期输出：**
```
权限        UID   大小      修改时间          名称
----------------------------------------------------------------------
drwxr-xr-x  0     0         2025-12-21 10:30  documents/
drwxr-xr-x  0     0         2025-12-21 10:30  pictures/
```

## 示例 2：多用户权限测试

### 步骤 1：root 创建私有文件

```bash
./myfs
mount
login
root
root

# 创建私有文件
touch private.txt
write private.txt
This is a private file.
Only root can read it.
EOF

# 设置为只有所有者可读写
chmod 600 private.txt

# 查看权限
ls

# 登出
logout
```

**预期输出：**
```
权限        UID   大小      修改时间          名称
----------------------------------------------------------------------
-rw-------  0     45        2025-12-21 10:35  private.txt
```

### 步骤 2：user1 尝试访问

```bash
# 登录 user1
login
user1
123456

# 尝试读取（应该失败）
cat private.txt

# 登出
logout
```

**预期输出：**
```
错误：没有读权限
```

### 步骤 3：root 修改权限

```bash
# 重新登录 root
login
root
root

# 修改为所有人可读
chmod 644 private.txt

# 登出
logout
```

### 步骤 4：user1 再次尝试

```bash
# 登录 user1
login
user1
123456

# 现在可以读取了
cat private.txt

# 尝试写入（应该失败）
write private.txt
This should fail.
EOF
```

**预期输出：**
```
This is a private file.
Only root can read it.

错误：没有写权限
```

## 示例 3：目录权限

```bash
./myfs
mount
login
root
root

# 创建目录
mkdir secret

# 设置目录权限为 700（只有所有者可访问）
chmod 700 secret

# 进入目录
cd secret

# 创建文件
touch secret_file.txt

# 返回上级
cd /

# 登出
logout

# 以 user1 登录
login
user1
123456

# 尝试进入目录（应该失败）
cd secret
```

**预期输出：**
```
错误：没有执行权限
```

## 示例 4：文件修改和查看

```bash
./myfs
mount
login
root
root

# 创建文件
touch log.txt

# 第一次写入
write log.txt
Log entry 1: System started
Log entry 2: User logged in
EOF

# 查看内容
cat log.txt

# 修改内容（覆盖）
write log.txt
Log entry 1: System started
Log entry 2: User logged in
Log entry 3: File modified
EOF

# 再次查看
cat log.txt

# 查看文件信息
ls
```

**预期输出：**
```
第一次 cat:
Log entry 1: System started
Log entry 2: User logged in

第二次 cat:
Log entry 1: System started
Log entry 2: User logged in
Log entry 3: File modified
```

## 示例 5：复杂目录结构

```bash
./myfs
mount
login
root
root

# 创建项目目录结构
mkdir project
cd project

mkdir src
mkdir docs
mkdir tests

cd src
touch main.cpp
touch utils.cpp

write main.cpp
#include <iostream>

int main() {
    std::cout << "Hello World" << std::endl;
    return 0;
}
EOF

cd /project/docs
touch README.md

write README.md
# Project Documentation

This is the main documentation file.
EOF

cd /project
ls

cd /
pwd
```

**预期输出：**
```
权限        UID   大小      修改时间          名称
----------------------------------------------------------------------
drwxr-xr-x  0     0         2025-12-21 10:40  src/
drwxr-xr-x  0     0         2025-12-21 10:40  docs/
drwxr-xr-x  0     0         2025-12-21 10:40  tests/
```

## 示例 6：权限数字含义

### 常用权限组合

| 权限 | 数字 | 说明 | 适用场景 |
|------|------|------|----------|
| rwxrwxrwx | 777 | 所有人全部权限 | 临时文件、测试 |
| rwxr-xr-x | 755 | 所有者全部，其他人读和执行 | 目录、可执行文件 |
| rw-rw-rw- | 666 | 所有人读写 | 共享数据文件 |
| rw-r--r-- | 644 | 所有者读写，其他人只读 | 普通文件 |
| rw------- | 600 | 只有所有者可读写 | 私密文件 |
| rwx------ | 700 | 只有所有者可访问 | 私密目录 |

### 测试不同权限

```bash
./myfs
mount
login
root
root

# 创建测试文件
touch test1.txt
touch test2.txt
touch test3.txt

# 设置不同权限
chmod 777 test1.txt
chmod 644 test2.txt
chmod 600 test3.txt

# 查看权限
ls
```

**预期输出：**
```
权限        UID   大小      修改时间          名称
----------------------------------------------------------------------
-rwxrwxrwx  0     0         2025-12-21 10:45  test1.txt
-rw-r--r--  0     0         2025-12-21 10:45  test2.txt
-rw-------  0     0         2025-12-21 10:45  test3.txt
```

## 示例 7：chown 命令（仅 root）

```bash
./myfs
mount
login
root
root

# 创建文件
touch file.txt

# 查看所有者
ls

# 修改所有者为 user1 (UID=1)
chown 1 file.txt

# 再次查看
ls

# 登出
logout

# 以 user1 登录
login
user1
123456

# 现在 user1 拥有这个文件
write file.txt
Now I own this file!
EOF

cat file.txt
```

**预期输出：**
```
第一次 ls:
-rw-r--r--  0     0         2025-12-21 10:50  file.txt

第二次 ls:
-rw-r--r--  1     0         2025-12-21 10:50  file.txt

cat 输出:
Now I own this file!
```

## 示例 8：系统信息查看

```bash
./myfs
mount
login
root
root

# 查看系统信息
info

# 创建一些文件和目录
mkdir dir1
mkdir dir2
touch file1.txt
touch file2.txt

# 再次查看系统信息
info
```

**预期输出：**
```
文件系统信息：

磁盘大小:     10 MB
块大小:       4096 字节
总块数:       2560
总 Inode 数:  1024

当前用户:     root (UID: 0)
当前目录:     /
```

## 示例 9：错误处理

### 尝试删除不存在的文件
```bash
rm nonexistent.txt
```
**输出：** `错误：文件不存在`

### 尝试删除非空目录
```bash
mkdir mydir
cd mydir
touch file.txt
cd /
rmdir mydir
```
**输出：** `错误：目录不为空`

### 尝试创建重复文件
```bash
touch test.txt
touch test.txt
```
**输出：** `错误：文件已存在`

### 尝试访问无权限文件
```bash
# 以 root 创建
touch private.txt
chmod 600 private.txt
logout

# 以 user1 访问
login
user1
123456
cat private.txt
```
**输出：** `错误：没有读权限`

## 示例 10：完整工作流程

这是一个完整的使用场景，模拟实际工作：

```bash
# 1. 启动和初始化
./myfs
format
yes
mount

# 2. root 用户设置系统
login
root
root

# 3. 创建用户工作目录
mkdir /home
cd /home
mkdir user1_home
mkdir user2_home

# 4. 创建共享目录
cd /
mkdir /shared
chmod 755 /shared

# 5. 在共享目录创建文档
cd /shared
touch team_doc.txt
write team_doc.txt
Team Project Documentation
=========================

Project: Multi-User File System
Status: In Development
EOF

chmod 644 team_doc.txt

# 6. 切换到 user1
logout
login
user1
123456

# 7. user1 工作
cd /home/user1_home
touch my_work.txt
write my_work.txt
My personal work notes.
This is private.
EOF

chmod 600 my_work.txt

# 8. 查看共享文档
cd /shared
cat team_doc.txt

# 9. 查看目录结构
cd /
ls
cd /home
ls

# 10. 退出
logout
exit
```

## 并发测试示例

虽然在单进程环境下难以完全展示，但代码已实现并发控制。

### 理论上的并发场景

**场景 1：多个读者**
```
终端 1: cat large_file.txt  (读者1)
终端 2: cat large_file.txt  (读者2) ✓ 同时进行
终端 3: cat large_file.txt  (读者3) ✓ 同时进行
```

**场景 2：读写互斥**
```
终端 1: cat large_file.txt  (读者)
终端 2: write large_file.txt (写者) ✗ 等待读者完成
```

**场景 3：写写互斥**
```
终端 1: write file.txt (写者1)
终端 2: write file.txt (写者2) ✗ 等待写者1完成
```

## 提示和技巧

### 1. 快速查看帮助
```bash
help
```

### 2. 查看当前位置
```bash
pwd
```

### 3. 查看系统状态
```bash
info
```

### 4. 权限快速参考
- 7 = 读+写+执行 (4+2+1)
- 6 = 读+写 (4+2)
- 5 = 读+执行 (4+1)
- 4 = 只读

### 5. 常用权限
- 目录：755 (rwxr-xr-x)
- 普通文件：644 (rw-r--r--)
- 私密文件：600 (rw-------)

### 6. 文件写入技巧
写入多行内容时，输入 `EOF` 结束输入。

### 7. 权限检查
使用 `ls` 命令查看详细的文件权限信息。

## 常见问题

**Q: 如何清空文件内容？**
A: 使用 `write` 命令，然后直接输入 `EOF`。

**Q: 如何修改文件内容？**
A: 使用 `write` 命令会覆盖原有内容。

**Q: 忘记当前在哪个目录？**
A: 使用 `pwd` 命令查看当前路径。

**Q: 如何查看所有可用命令？**
A: 输入 `help` 命令。

**Q: 权限不足怎么办？**
A: 
1. 检查文件所有者（使用 `ls`）
2. 以文件所有者或 root 身份登录
3. 使用 `chmod` 修改权限

**Q: 如何重新开始？**
A: 
```bash
exit
make distclean  # 删除所有文件
make            # 重新编译
./myfs
format          # 重新格式化
```

## 总结

本文档提供了丰富的使用示例，涵盖了：
- ✅ 基本文件操作
- ✅ 多用户权限管理
- ✅ 目录结构创建
- ✅ 权限修改和测试
- ✅ 错误处理
- ✅ 完整工作流程

通过这些示例，你可以全面了解多用户文件系统的功能和使用方法。

更多信息请参考：
- [README.md](README.md) - 详细技术文档
- [QUICKSTART.md](QUICKSTART.md) - 快速入门指南
- [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) - 项目总结

