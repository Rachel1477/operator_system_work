# 快速入门指南

## 快速开始

### 1. 编译项目
```bash
make
```

### 2. 运行程序
```bash
./myfs
```

### 3. 初始化文件系统

首次使用需要格式化：

```
format
yes
mount
```

### 4. 登录用户

默认账户：
- **root** / root （超级管理员）
- **user1** / 123456
- **user2** / 123456

```
login
root
root
```

## 常用命令速查

### 系统命令
```bash
help        # 查看帮助
info        # 查看系统信息
exit        # 退出系统
```

### 文件操作
```bash
ls          # 列出当前目录
pwd         # 显示当前路径
cd <dir>    # 切换目录
mkdir <dir> # 创建目录
touch <file># 创建文件
rm <file>   # 删除文件
rmdir <dir> # 删除目录
```

### 文件读写
```bash
cat <file>  # 查看文件内容
write <file># 写入文件（输入内容后输入 EOF 结束）
```

### 权限管理
```bash
chmod 755 <file>    # 修改权限
chown <uid> <file>  # 修改所有者（仅root）
```

## 完整示例

```bash
# 1. 启动并初始化
./myfs
format
yes
mount

# 2. 登录
login
root
root

# 3. 创建目录和文件
mkdir projects
cd projects
touch readme.txt

# 4. 写入内容
write readme.txt
Hello, this is my file system!
This is a test.
EOF

# 5. 查看内容
cat readme.txt

# 6. 查看文件列表
ls

# 7. 修改权限
chmod 644 readme.txt

# 8. 退出
exit
```

## 运行演示脚本

我们提供了一个自动演示脚本：

```bash
./test_demo.sh
```

这个脚本会自动执行一系列操作，展示文件系统的各项功能。

## 权限说明

权限使用八进制表示：

| 数字 | 权限 | 说明 |
|------|------|------|
| 7    | rwx  | 读、写、执行 |
| 6    | rw-  | 读、写 |
| 5    | r-x  | 读、执行 |
| 4    | r--  | 只读 |
| 3    | -wx  | 写、执行 |
| 2    | -w-  | 只写 |
| 1    | --x  | 只执行 |
| 0    | ---  | 无权限 |

**示例：**
- `755` = rwxr-xr-x（所有者全部权限，其他人读和执行）
- `644` = rw-r--r--（所有者读写，其他人只读）
- `600` = rw-------（只有所有者可读写）

## 多用户测试

### 测试权限控制

1. **以 root 创建私有文件：**
```bash
login
root
root
touch private.txt
chmod 600 private.txt
write private.txt
This is private!
EOF
logout
```

2. **以 user1 尝试访问：**
```bash
login
user1
123456
cat private.txt    # 应该失败（权限不足）
logout
```

3. **root 修改权限：**
```bash
login
root
root
chmod 644 private.txt
logout
```

4. **user1 再次尝试：**
```bash
login
user1
123456
cat private.txt    # 现在可以读取了
```

## 并发测试

虽然在单进程环境下难以完全展示并发功能，但代码中已经实现了读写锁机制：

- **读读允许**：多个用户可以同时读取同一文件
- **读写互斥**：当有人在读时，写操作会等待
- **写写互斥**：当有人在写时，其他写操作会等待

在实际多线程或多进程环境中，这些机制会自动生效。

## 故障排除

### 问题：无法格式化
**解决：** 确保没有其他程序正在使用 disk.bin 文件

### 问题：挂载失败
**解决：** 先执行 `format` 命令格式化文件系统

### 问题：权限不足
**解决：** 
1. 检查当前登录用户
2. 使用 `ls` 查看文件权限
3. 如需修改权限，以文件所有者或 root 身份登录

### 问题：目录不为空无法删除
**解决：** 先删除目录中的所有文件，再删除目录

## 清理

### 清理编译文件
```bash
make clean
```

### 完全清理（包括磁盘文件）
```bash
make distclean
```

## 下一步

- 阅读 [README.md](README.md) 了解详细的技术实现
- 查看源代码了解各个模块的实现细节
- 尝试扩展功能（如符号链接、文件缓存等）

## 技术支持

如有问题，请查看：
1. 代码注释
2. README.md 文档
3. 源代码实现

祝使用愉快！

