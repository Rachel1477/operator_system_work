# Makefile for Multi-User File System

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -pthread
TARGET = myfs
OBJECTS = main.o filesystem.o shell.o

# 默认目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "编译完成！运行 ./$(TARGET) 启动文件系统"

# 编译 main.cpp
main.o: main.cpp filesystem.h shell.h
	$(CXX) $(CXXFLAGS) -c main.cpp

# 编译 filesystem.cpp
filesystem.o: filesystem.cpp filesystem.h
	$(CXX) $(CXXFLAGS) -c filesystem.cpp

# 编译 shell.cpp
shell.o: shell.cpp shell.h filesystem.h
	$(CXX) $(CXXFLAGS) -c shell.cpp

# 清理编译文件
clean:
	rm -f $(OBJECTS) $(TARGET) disk.bin
	@echo "清理完成"

# 清理所有文件（包括磁盘文件）
distclean: clean
	rm -f disk.bin
	@echo "完全清理完成"

# 运行程序
run: $(TARGET)
	./$(TARGET)

# 帮助信息
help:
	@echo "可用的 make 命令："
	@echo "  make          - 编译项目"
	@echo "  make clean    - 清理编译文件"
	@echo "  make distclean- 完全清理（包括磁盘文件）"
	@echo "  make run      - 编译并运行"
	@echo "  make help     - 显示帮助信息"

.PHONY: all clean distclean run help

