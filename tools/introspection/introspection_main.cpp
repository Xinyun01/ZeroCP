#include "introspection/introspection_server.hpp"
#include "introspection/introspection_client.hpp"
#include <ncurses.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <atomic>

using namespace zero_copy::introspection;

// 全局标志用于处理信号
std::atomic<bool> g_should_exit(false);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_should_exit.store(true);
    }
}

/**
 * @brief 格式化字节数
 */
std::string formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = bytes;
    
    while (size >= 1024 && unit_index < 4) {
        size /= 1024;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unit_index];
    return oss.str();
}

/**
 * @brief TUI 界面类
 */
class IntrospectionTUI {
public:
    IntrospectionTUI() : current_view_(0), show_help_(false) {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        nodelay(stdscr, TRUE);
        
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_RED, COLOR_BLACK);
            init_pair(2, COLOR_GREEN, COLOR_BLACK);
            init_pair(3, COLOR_YELLOW, COLOR_BLACK);
            init_pair(4, COLOR_BLUE, COLOR_BLACK);
            init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
            init_pair(6, COLOR_CYAN, COLOR_BLACK);
        }
    }

    ~IntrospectionTUI() {
        endwin();
    }

    void run(IntrospectionClient& client) {
        while (!g_should_exit.load()) {
            SystemMetrics metrics;
            if (client.getMetrics(metrics)) {
                drawInterface(metrics);
            }

            handleInput();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

private:
    void drawInterface(const SystemMetrics& metrics) {
        clear();

        // 标题栏
        attron(COLOR_PAIR(5));
        mvprintw(0, 0, "╔════════════════════════════════════════════════════════════════════════════╗");
        mvprintw(1, 0, "║           Zero Copy Framework - Introspection Tool                         ║");
        mvprintw(2, 0, "╚════════════════════════════════════════════════════════════════════════════╝");
        attroff(COLOR_PAIR(5));

        // 时间戳
        attron(COLOR_PAIR(6));
        auto time_t = std::chrono::system_clock::to_time_t(metrics.timestamp);
        auto tm = *std::localtime(&time_t);
        mvprintw(3, 0, "更新时间: %04d-%02d-%02d %02d:%02d:%02d", 
                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                 tm.tm_hour, tm.tm_min, tm.tm_sec);
        attroff(COLOR_PAIR(6));

        // 根据当前视图绘制内容
        switch (current_view_) {
            case 0: drawOverview(metrics); break;
            case 1: drawProcessView(metrics); break;
            case 2: drawConnectionView(metrics); break;
            case 3: drawSystemView(metrics); break;
        }

        // 状态栏
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        attron(COLOR_PAIR(4));
        mvprintw(max_y - 2, 0, "视图: [1]概览 [2]进程 [3]连接 [4]系统 | [h]帮助 [q]退出");
        attroff(COLOR_PAIR(4));

        if (show_help_) {
            drawHelp();
        }

        refresh();
    }

    void drawOverview(const SystemMetrics& metrics) {
        int row = 5;
        
        // 内存信息
        attron(COLOR_PAIR(2));
        mvprintw(row++, 0, "═══ 内存使用情况 ═══");
        attroff(COLOR_PAIR(2));
        
        mvprintw(row++, 2, "内存使用率: %.1f%%", metrics.memory.memory_usage_percent);
        
        // 进度条
        int bar_width = 60;
        int used_width = (int)(metrics.memory.memory_usage_percent / 100.0 * bar_width);
        mvprintw(row, 2, "[");
        for (int i = 0; i < bar_width; ++i) {
            if (i < used_width) {
                attron(COLOR_PAIR(metrics.memory.memory_usage_percent > 80 ? 1 : 2));
            } else {
                attron(COLOR_PAIR(6));
            }
            addch('█');
        }
        addch(']');
        row++;

        attron(COLOR_PAIR(6));
        mvprintw(row++, 2, "总内存:   %s", formatBytes(metrics.memory.total_memory).c_str());
        mvprintw(row++, 2, "已使用:   %s", formatBytes(metrics.memory.used_memory).c_str());
        mvprintw(row++, 2, "空闲:     %s", formatBytes(metrics.memory.free_memory).c_str());
        row++;

        // 系统负载
        attron(COLOR_PAIR(2));
        mvprintw(row++, 0, "═══ 系统负载 ═══");
        attroff(COLOR_PAIR(2));
        
        attron(COLOR_PAIR(6));
        mvprintw(row++, 2, "CPU核心数: %u", metrics.load.cpu_count);
        mvprintw(row++, 2, "CPU使用率: %.1f%%", metrics.load.cpu_usage_percent);
        mvprintw(row++, 2, "负载: %.2f (1m) %.2f (5m) %.2f (15m)", 
                 metrics.load.load_1min, metrics.load.load_5min, metrics.load.load_15min);
    }

    void drawProcessView(const SystemMetrics& metrics) {
        int row = 5;
        
        attron(COLOR_PAIR(2));
        mvprintw(row++, 0, "═══ 进程列表 (前15个) ═══");
        attroff(COLOR_PAIR(2));
        
        attron(COLOR_PAIR(5));
        mvprintw(row++, 2, "%-8s %-20s %-12s %-8s %s", 
                 "PID", "名称", "内存", "CPU%", "状态");
        attroff(COLOR_PAIR(5));

        int max_processes = std::min(15, (int)metrics.processes.size());
        for (int i = 0; i < max_processes; ++i) {
            const auto& proc = metrics.processes[i];
            attron(COLOR_PAIR(i % 2 == 0 ? 2 : 6));
            mvprintw(row++, 2, "%-8u %-20.20s %-12s %-8.1f %s", 
                     proc.pid,
                     proc.name.c_str(),
                     formatBytes(proc.memory_usage).c_str(),
                     proc.cpu_usage,
                     proc.state.c_str());
        }

        if (metrics.processes.size() > 15) {
            attron(COLOR_PAIR(3));
            mvprintw(row++, 2, "... 还有 %zu 个进程", metrics.processes.size() - 15);
        }
    }

    void drawConnectionView(const SystemMetrics& metrics) {
        int row = 5;
        
        attron(COLOR_PAIR(2));
        mvprintw(row++, 0, "═══ 网络连接 (前12个) ═══");
        attroff(COLOR_PAIR(2));
        
        attron(COLOR_PAIR(5));
        mvprintw(row++, 2, "%-22s %-22s %-8s %s", 
                 "本地地址", "远程地址", "协议", "状态");
        attroff(COLOR_PAIR(5));

        int max_conns = std::min(12, (int)metrics.connections.size());
        for (int i = 0; i < max_conns; ++i) {
            const auto& conn = metrics.connections[i];
            attron(COLOR_PAIR(i % 2 == 0 ? 2 : 6));
            mvprintw(row++, 2, "%-22.22s %-22.22s %-8s %s", 
                     conn.local_address.c_str(),
                     conn.remote_address.c_str(),
                     conn.protocol.c_str(),
                     conn.state.c_str());
        }

        if (metrics.connections.size() > 12) {
            attron(COLOR_PAIR(3));
            mvprintw(row++, 2, "... 还有 %zu 个连接", metrics.connections.size() - 12);
        }
    }

    void drawSystemView(const SystemMetrics& metrics) {
        drawOverview(metrics);
    }

    void drawHelp() {
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        int start_y = max_y / 4;
        int start_x = max_x / 4;
        
        attron(COLOR_PAIR(5));
        mvprintw(start_y, start_x, "╔═══════════════════════════════════╗");
        mvprintw(start_y + 1, start_x, "║           帮助信息                 ║");
        mvprintw(start_y + 2, start_x, "╚═══════════════════════════════════╝");
        attroff(COLOR_PAIR(5));
        
        attron(COLOR_PAIR(6));
        mvprintw(start_y + 4, start_x + 2, "1-4: 切换视图");
        mvprintw(start_y + 5, start_x + 2, "h:   显示/隐藏帮助");
        mvprintw(start_y + 6, start_x + 2, "q:   退出程序");
        mvprintw(start_y + 7, start_x + 2, "r:   刷新数据");
        attroff(COLOR_PAIR(6));
    }

    void handleInput() {
        int ch = getch();
        
        switch (ch) {
            case '1': current_view_ = 0; break;
            case '2': current_view_ = 1; break;
            case '3': current_view_ = 2; break;
            case '4': current_view_ = 3; break;
            case 'h':
            case 'H': show_help_ = !show_help_; break;
            case 'q':
            case 'Q': g_should_exit.store(true); break;
        }
    }

private:
    int current_view_;
    bool show_help_;
};

/**
 * @brief 显示帮助信息
 */
void showHelp() {
    std::cout << "Zero Copy Framework - Introspection Tool\n";
    std::cout << "==========================================\n\n";
    std::cout << "用法: introspection [选项]\n\n";
    std::cout << "选项:\n";
    std::cout << "  -h, --help              显示此帮助信息\n";
    std::cout << "  -i, --interval <ms>     设置更新间隔 (默认: 1000ms)\n";
    std::cout << "  -p, --process <name>    过滤进程名称 (可多次使用)\n";
    std::cout << "  -c, --connection <port> 过滤连接端口 (可多次使用)\n\n";
    std::cout << "示例:\n";
    std::cout << "  introspection                        # 启动默认监控\n";
    std::cout << "  introspection -i 500                # 500ms更新间隔\n";
    std::cout << "  introspection -p nginx -p apache    # 只监控nginx和apache\n";
    std::cout << "  introspection -c 80 -c 443          # 只监控80和443端口\n\n";
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 解析命令行参数
    IntrospectionConfig config;
    bool show_help_flag = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            show_help_flag = true;
        } else if (arg == "-i" || arg == "--interval") {
            if (i + 1 < argc) {
                config.update_interval_ms = std::stoul(argv[++i]);
            }
        } else if (arg == "-p" || arg == "--process") {
            if (i + 1 < argc) {
                config.process_filter.push_back(argv[++i]);
            }
        } else if (arg == "-c" || arg == "--connection") {
            if (i + 1 < argc) {
                config.connection_filter.push_back(std::stoul(argv[++i]));
            }
        }
    }

    if (show_help_flag) {
        showHelp();
        return 0;
    }

    try {
        // 创建服务端和客户端
        auto server = std::make_shared<IntrospectionServer>();
        IntrospectionClient client;

        // 启动服务端
        if (!server->start(config)) {
            std::cerr << "错误: 无法启动监控服务" << std::endl;
            return 1;
        }

        // 连接客户端到服务端
        if (!client.connectLocal(server)) {
            std::cerr << "错误: 无法连接到监控服务" << std::endl;
            return 1;
        }

        // 启动 TUI 界面
        IntrospectionTUI tui;
        tui.run(client);

        // 清理
        client.disconnect();
        server->stop();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}

