#ifndef SHELL_H
#define SHELL_H

#include "filesystem.h"
#include <string>
#include <vector>

class Shell {
private:
    FileSystem* fs;
    bool running;
    
    // 命令解析
    std::vector<std::string> parseCommand(const std::string& input);
    
    // 命令处理函数
    void cmdHelp();
    void cmdFormat();
    void cmdMount();
    void cmdLogin();
    void cmdLogout();
    void cmdLs(const std::vector<std::string>& args);
    void cmdCd(const std::vector<std::string>& args);
    void cmdPwd();
    void cmdMkdir(const std::vector<std::string>& args);
    void cmdTouch(const std::vector<std::string>& args);
    void cmdRm(const std::vector<std::string>& args);
    void cmdRmdir(const std::vector<std::string>& args);
    void cmdCat(const std::vector<std::string>& args);
    void cmdWrite(const std::vector<std::string>& args);
    void cmdChmod(const std::vector<std::string>& args);
    void cmdChown(const std::vector<std::string>& args);
    void cmdAddUser();
    void cmdInfo();
    void cmdExit();
    
    // 辅助函数
    void printPrompt();
    std::string getInput();

public:
    Shell(FileSystem* filesystem);
    ~Shell();
    
    void run();
    bool processCommand(const std::string& input);
};

#endif // SHELL_H

