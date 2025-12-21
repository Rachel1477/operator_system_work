#include "filesystem.h"
#include "shell.h"
#include <iostream>

int main(int /* argc */, char* /* argv */[]) {
    // 设置 UTF-8 输出（Linux 系统通常默认支持）
    std::cout << "初始化文件系统..." << std::endl;
    
    // 创建文件系统实例
    FileSystem fs("disk.bin");
    
    // 创建 Shell
    Shell shell(&fs);
    
    // 运行 Shell
    shell.run();
    
    return 0;
}

