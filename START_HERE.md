# 🚀 开始使用多用户文件系统

## 快速开始（3步）

### 1️⃣ 编译项目
```bash
make
```

### 2️⃣ 运行程序
```bash
./myfs
```

### 3️⃣ 初始化系统
```
format
yes
mount
login
root
root
```

## 🎉 恭喜！你已经成功启动了文件系统！

现在你可以：
- 输入 `help` 查看所有可用命令
- 输入 `mkdir test` 创建目录
- 输入 `touch file.txt` 创建文件
- 输入 `ls` 查看文件列表

## 📚 想了解更多？

### 快速入门
👉 阅读 [QUICKSTART.md](QUICKSTART.md)

### 查看示例
👉 阅读 [EXAMPLES.md](EXAMPLES.md)

### 完整文档
👉 阅读 [INDEX.md](INDEX.md) 查看所有文档

### 运行演示
```bash
./test_demo.sh
```

## 🎯 默认账户

| 用户名 | 密码 | 权限 |
|--------|------|------|
| root | root | 超级管理员 |
| user1 | 123456 | 普通用户 |
| user2 | 123456 | 普通用户 |

## 💡 常用命令速查

```bash
# 系统命令
help        # 帮助
info        # 系统信息
exit        # 退出

# 文件操作
ls          # 列出文件
cd <dir>    # 切换目录
mkdir <dir> # 创建目录
touch <file># 创建文件
cat <file>  # 查看文件
write <file># 写入文件

# 权限管理
chmod 755 <file>    # 修改权限
```

## 🔧 遇到问题？

### 无法编译？
确保安装了 g++ 和 make：
```bash
sudo apt-get install build-essential
```

### 无法运行？
检查是否有执行权限：
```bash
chmod +x myfs
```

### 需要重新开始？
```bash
make distclean
make
./myfs
```

## 📖 文档结构

```
START_HERE.md           ← 你在这里！
├── QUICKSTART.md       ← 快速入门指南
├── EXAMPLES.md         ← 详细使用示例
├── README.md           ← 完整项目文档
├── ARCHITECTURE.md     ← 技术架构设计
├── PROJECT_SUMMARY.md  ← 项目总结报告
└── INDEX.md            ← 文档索引导航
```

## 🌟 项目特色

- ✅ 完整的文件系统实现
- ✅ 多用户权限控制
- ✅ 并发读写保护
- ✅ 类Linux命令界面
- ✅ 详细的技术文档

## 🎓 适合人群

- 📚 操作系统课程学生
- 💻 系统编程学习者
- 🔍 文件系统研究者
- 🚀 C++项目实践者

## 📞 获取帮助

1. 查看 [QUICKSTART.md](QUICKSTART.md) 的故障排除
2. 查看 [EXAMPLES.md](EXAMPLES.md) 的常见问题
3. 阅读源代码注释
4. 查看 [ARCHITECTURE.md](ARCHITECTURE.md) 的技术细节

---

**准备好了吗？让我们开始吧！**

```bash
make && ./myfs
```

**祝你使用愉快！** 🎉

