// 守护进程 main 函数
#include "daemon.hpp"

int main(int argc, char* argv[]) {
    Daemon daemon;
    daemon.run();
    return 0;
}