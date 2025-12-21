#include "shell.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

Shell::Shell(FileSystem* filesystem) : fs(filesystem), running(false) {
}

Shell::~Shell() {
}

std::vector<std::string> Shell::parseCommand(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

void Shell::printPrompt() {
    if (fs->getCurrentUser()) {
        std::cout << fs->getCurrentUser()->username << "@myfs:" 
                  << fs->getCurrentPath() << "$ ";
    } else {
        std::cout << "login: ";
    }
}

std::string Shell::getInput() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}

void Shell::run() {
    running = true;
    
    std::cout << "=====================================" << std::endl;
    std::cout << "  欢迎使用多用户文件系统 v1.0" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "输入 'help' 查看可用命令" << std::endl;
    std::cout << std::endl;
    
    while (running) {
        printPrompt();
        std::string input = getInput();
        
        if (input.empty()) {
            continue;
        }
        
        processCommand(input);
    }
}

bool Shell::processCommand(const std::string& input) {
    std::vector<std::string> tokens = parseCommand(input);
    
    if (tokens.empty()) {
        return true;
    }
    
    std::string cmd = tokens[0];
    
    // 将命令转换为小写
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    
    if (cmd == "help") {
        cmdHelp();
    } else if (cmd == "format") {
        cmdFormat();
    } else if (cmd == "mount") {
        cmdMount();
    } else if (cmd == "login") {
        cmdLogin();
    } else if (cmd == "logout") {
        cmdLogout();
    } else if (cmd == "ls") {
        cmdLs(tokens);
    } else if (cmd == "cd") {
        cmdCd(tokens);
    } else if (cmd == "pwd") {
        cmdPwd();
    } else if (cmd == "mkdir") {
        cmdMkdir(tokens);
    } else if (cmd == "touch") {
        cmdTouch(tokens);
    } else if (cmd == "rm") {
        cmdRm(tokens);
    } else if (cmd == "rmdir") {
        cmdRmdir(tokens);
    } else if (cmd == "cat") {
        cmdCat(tokens);
    } else if (cmd == "write") {
        cmdWrite(tokens);
    } else if (cmd == "chmod") {
        cmdChmod(tokens);
    } else if (cmd == "chown") {
        cmdChown(tokens);
    } else if (cmd == "info") {
        cmdInfo();
    } else if (cmd == "exit" || cmd == "quit") {
        cmdExit();
    } else {
        std::cout << "未知命令: " << cmd << std::endl;
        std::cout << "输入 'help' 查看可用命令" << std::endl;
    }
    
    return true;
}

void Shell::cmdHelp() {
    std::cout << "\n可用命令：\n" << std::endl;
    std::cout << "系统管理：" << std::endl;
    std::cout << "  format              - 格式化文件系统" << std::endl;
    std::cout << "  mount               - 挂载文件系统" << std::endl;
    std::cout << "  info                - 显示文件系统信息" << std::endl;
    std::cout << "  exit/quit           - 退出系统" << std::endl;
    std::cout << std::endl;
    
    std::cout << "用户管理：" << std::endl;
    std::cout << "  login               - 用户登录" << std::endl;
    std::cout << "  logout              - 用户登出" << std::endl;
    std::cout << std::endl;
    
    std::cout << "文件操作：" << std::endl;
    std::cout << "  ls [path]           - 列出目录内容" << std::endl;
    std::cout << "  cd <path>           - 切换目录" << std::endl;
    std::cout << "  pwd                 - 显示当前路径" << std::endl;
    std::cout << "  mkdir <name>        - 创建目录" << std::endl;
    std::cout << "  touch <name>        - 创建文件" << std::endl;
    std::cout << "  rm <name>           - 删除文件" << std::endl;
    std::cout << "  rmdir <name>        - 删除目录" << std::endl;
    std::cout << "  cat <file>          - 查看文件内容" << std::endl;
    std::cout << "  write <file>        - 写入文件（交互式）" << std::endl;
    std::cout << std::endl;
    
    std::cout << "权限管理：" << std::endl;
    std::cout << "  chmod <mode> <file> - 修改文件权限（如: chmod 755 file.txt）" << std::endl;
    std::cout << "  chown <uid> <file>  - 修改文件所有者（仅root）" << std::endl;
    std::cout << std::endl;
    
    std::cout << "提示：" << std::endl;
    std::cout << "  - 默认用户: root/root, user1/123456, user2/123456" << std::endl;
    std::cout << "  - 权限格式: rwxrwxrwx (所有者/组/其他)" << std::endl;
    std::cout << std::endl;
}

void Shell::cmdFormat() {
    std::cout << "警告：格式化将清除所有数据！" << std::endl;
    std::cout << "确认格式化？(yes/no): ";
    
    std::string confirm;
    std::getline(std::cin, confirm);
    
    if (confirm == "yes" || confirm == "y") {
        if (fs->format()) {
            std::cout << "文件系统格式化完成" << std::endl;
        } else {
            std::cout << "格式化失败" << std::endl;
        }
    } else {
        std::cout << "取消格式化" << std::endl;
    }
}

void Shell::cmdMount() {
    if (fs->mount()) {
        std::cout << "文件系统挂载完成" << std::endl;
    } else {
        std::cout << "挂载失败" << std::endl;
    }
}

void Shell::cmdLogin() {
    std::string username, password;
    
    std::cout << "用户名: ";
    std::getline(std::cin, username);
    
    std::cout << "密码: ";
    std::getline(std::cin, password);
    
    fs->login(username, password);
}

void Shell::cmdLogout() {
    fs->logout();
}

void Shell::cmdLs(const std::vector<std::string>& args) {
    std::string path = ".";
    if (args.size() > 1) {
        path = args[1];
    }
    
    auto entries = fs->listDirectory(path);
    
    if (entries.empty()) {
        std::cout << "(空目录)" << std::endl;
        return;
    }
    
    std::cout << std::left;
    std::cout << std::setw(12) << "权限"
              << std::setw(6) << "UID"
              << std::setw(10) << "大小"
              << std::setw(18) << "修改时间"
              << "名称" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    for (const auto& pair : entries) {
        std::string type = (pair.second.file_type == FILE_TYPE_DIRECTORY) ? "d" : "-";
        std::string perm = fs->permissionToString(pair.second.permission);
        
        // 修改时间
        char time_str[20];
        struct tm* timeinfo = localtime((time_t*)&pair.second.modify_time);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", timeinfo);
        
        std::cout << std::setw(1) << type
                  << std::setw(11) << perm
                  << std::setw(6) << pair.second.owner_id
                  << std::setw(10) << pair.second.file_size
                  << std::setw(18) << time_str;
        
        // 目录名称用不同颜色显示
        if (pair.second.file_type == FILE_TYPE_DIRECTORY) {
            std::cout << "\033[1;34m" << pair.first << "/\033[0m" << std::endl;
        } else {
            std::cout << pair.first << std::endl;
        }
    }
}

void Shell::cmdCd(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "用法: cd <path>" << std::endl;
        return;
    }
    
    fs->changeDirectory(args[1]);
}

void Shell::cmdPwd() {
    std::cout << fs->getCurrentPath() << std::endl;
}

void Shell::cmdMkdir(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "用法: mkdir <name>" << std::endl;
        return;
    }
    
    fs->createDirectory(args[1]);
}

void Shell::cmdTouch(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "用法: touch <name>" << std::endl;
        return;
    }
    
    fs->createFile(args[1]);
}

void Shell::cmdRm(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "用法: rm <name>" << std::endl;
        return;
    }
    
    fs->removeFile(args[1]);
}

void Shell::cmdRmdir(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "用法: rmdir <name>" << std::endl;
        return;
    }
    
    fs->removeDirectory(args[1]);
}

void Shell::cmdCat(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "用法: cat <file>" << std::endl;
        return;
    }
    
    std::string content = fs->readFile(args[1]);
    if (!content.empty()) {
        std::cout << content << std::endl;
    }
}

void Shell::cmdWrite(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "用法: write <file>" << std::endl;
        return;
    }
    
    std::cout << "请输入文件内容（输入 EOF 结束）：" << std::endl;
    
    std::string content;
    std::string line;
    
    while (true) {
        std::getline(std::cin, line);
        if (line == "EOF") {
            break;
        }
        content += line + "\n";
    }
    
    fs->writeFile(args[1], content);
}

void Shell::cmdChmod(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cout << "用法: chmod <mode> <file>" << std::endl;
        std::cout << "示例: chmod 755 file.txt" << std::endl;
        return;
    }
    
    // 将八进制字符串转换为整数
    uint16_t mode;
    std::istringstream iss(args[1]);
    iss >> std::oct >> mode;
    
    fs->changePermission(args[2], mode);
}

void Shell::cmdChown(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cout << "用法: chown <uid> <file>" << std::endl;
        return;
    }
    
    uint16_t uid = std::stoi(args[1]);
    fs->changeOwner(args[2], uid);
}

void Shell::cmdInfo() {
    std::cout << "\n文件系统信息：\n" << std::endl;
    std::cout << "磁盘大小:     " << (DISK_SIZE / 1024 / 1024) << " MB" << std::endl;
    std::cout << "块大小:       " << BLOCK_SIZE << " 字节" << std::endl;
    std::cout << "总块数:       " << MAX_BLOCKS << std::endl;
    std::cout << "总 Inode 数:  " << MAX_INODES << std::endl;
    
    if (fs->getCurrentUser()) {
        std::cout << "\n当前用户:     " << fs->getCurrentUser()->username 
                  << " (UID: " << fs->getCurrentUser()->uid << ")" << std::endl;
        std::cout << "当前目录:     " << fs->getCurrentPath() << std::endl;
    }
    std::cout << std::endl;
}

void Shell::cmdExit() {
    std::cout << "感谢使用，再见！" << std::endl;
    running = false;
}

