#!/bin/bash

# 多用户文件系统演示脚本

echo "======================================"
echo "  多用户文件系统演示"
echo "======================================"
echo ""

# 清理旧的磁盘文件
if [ -f "disk.bin" ]; then
    echo "清理旧的磁盘文件..."
    rm -f disk.bin
fi

echo "启动文件系统..."
echo ""

# 创建测试命令文件
cat > test_commands.txt << 'EOF'
format
yes
mount
login
root
root
help
info
mkdir documents
mkdir pictures
ls
cd documents
pwd
touch readme.txt
touch notes.txt
ls
write readme.txt
This is a test file.
It demonstrates the multi-user file system.
Created by root user.
EOF
cat readme.txt
chmod 644 readme.txt
ls
cd /
pwd
ls
info
logout
login
user1
123456
ls
cd documents
cat readme.txt
touch user1_file.txt
write user1_file.txt
This file is created by user1.
EOF
cat user1_file.txt
ls
cd /
logout
exit
EOF

echo "运行测试命令..."
./myfs < test_commands.txt

echo ""
echo "======================================"
echo "  演示完成！"
echo "======================================"
echo ""
echo "你可以手动运行 ./myfs 来交互式使用文件系统"

# 清理测试命令文件
rm -f test_commands.txt

