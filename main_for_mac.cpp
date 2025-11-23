#include "utfcpp-4.0.6/source/utf8/checked.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <tuple>
#include <unistd.h>
#include <vector>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

vector<string> split(string a, char delim) {
    vector<string> result;
    string left = a;
    while (left.find(delim) != string::npos) {
        int index = left.find_first_of(delim);
        result.push_back(left.substr(0, index));
        left = left.substr(index + 1, left.length());
    }
    result.push_back(left);
    return result;
}

string center_x(string a, int w) {
    vector<string> lines = split(a, '\n');
    string result;
    int len = utf8::distance(lines[0].begin(), lines[0].end());
    int padding = (w - len) / 2;
    if (padding <= 0) return a;
    string padstr(padding, ' ');
    for (string line : lines) {
        result.append(padstr + line + "\n");
    }
    return result;
}

string center_y(string a, int h, bool fill) {
    vector<string> lines = split(a, '\n');
    string result;
    int padding = (h - lines.size()) / 2 + 1;
    if (padding <= 0) return a;
    result.append(string(padding, '\n') + a);
    if (fill) result.append(string(padding, '\n'));
    return result;
}

void clear_screen() { cout << "\033[2J\033[1;1H"; }

struct Program {
    string name;
    string icon;
    string shortcut;
    string command;
};

Program make_program(string name, string icon, string shortcut, string command) {
    return Program{name, icon, shortcut, command};
}

void to_json(json& j, const Program& p) {
    j = json{{"name", p.name}, {"icon", p.icon}, {"shortcut", p.shortcut}, {"command", p.command}};
}

vector<Program> parse_config(json config) {
    vector<Program> programs;
    for (auto& item : config) {
        programs.push_back(make_program(item["name"], item["icon"], item["shortcut"], item["command"]));
    }
    return programs;
}

string format_options(vector<Program> options) {
    string result;
    int break_c = 0;
    for (auto el : options) {
        break_c++;
        string break_pad = (break_c == 2) ? "\n" : "\t";
        if (break_c == 2) break_c = 0;
        result.append(fmt::format("{} {} [:{}]{}", el.icon, el.name, el.shortcut, break_pad));
    }
    return result;
}

int getch() {
    char ch;
    struct termios oldattr, newattr;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~ICANON;
    newattr.c_lflag &= ~ECHO;
    newattr.c_cc[VMIN] = 1;
    newattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}

enum Color { RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

string colorize(string s, Color c) {
    int code;
    switch (c) {
        case RED: code = 31; break;
        case GREEN: code = 32; break;
        case YELLOW: code = 33; break;
        case BLUE: code = 34; break;
        case MAGENTA: code = 35; break;
        case CYAN: code = 36; break;
        case WHITE: code = 0; break;
    }
    return fmt::format("\033[{}m{}\033[0m", code, s);
}

void quit() {
    clear_screen();
    exit(0);
}

int main(int argc, char *argv[]) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    bool qt = (argc > 1 && strcmp(argv[1], "--quick-tap") == 0);

    string ascii_art =
        "██╗    ██╗███████╗██╗      ██████╗ ██████╗ ███╗   ███╗███████╗\n"
        "██║    ██║██╔════╝██║     ██╔════╝██╔═══██╗████╗ ████║██╔════╝\n"
        "██║ █╗ ██║█████╗  ██║     ██║     ██║   ██║██╔████╔██║█████╗  \n"
        "██║███╗██║██╔══╝  ██║     ██║     ██║   ██║██║╚██╔╝██║██╔══╝  \n"
        "╚███╔███╔╝███████╗███████╗╚██████╗╚██████╔╝██║ ╚═╝ ██║███████╗\n"
        " ╚══╝╚══╝ ╚══════╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝";

    char prefix = ':';
    string username = getenv("USER") ? getenv("USER") : "unknown";
    string pwd = filesystem::current_path().string();
    pwd.replace(0, strlen(getenv("HOME")), "~");

    char hostname_buf[256];
    gethostname(hostname_buf, 256);
    string hostname(hostname_buf);

    string os_icon = colorize(" ", BLUE); // macOS icon

    vector<Program> default_options = {
        make_program("Neovim", " ", "nz", "nvim"),
        make_program("Terminal", os_icon, "tt", "open -a Terminal"),
        make_program("Zsh", "$ ", "zs", "zsh"),
    };

    const char* xdg_config = getenv("XDG_CONFIG_HOME");
    string config_path = (xdg_config != nullptr) ? string(xdg_config) + "/salut/config.json"
                                                 : string(getenv("HOME")) + "/.config/salut/config.json";

    vector<Program> options;
    filesystem::path config_file_path(config_path);

    if (filesystem::exists(config_file_path)) {
        try {
            ifstream in_file(config_path);
            json config = json::parse(in_file);
            options = parse_config(config);
        } catch (const exception& e) {
            cerr << "Error reading config file: " << e.what() << endl;
            options = default_options;
        }
    } else {
        filesystem::create_directories(config_file_path.parent_path());
        options = default_options;
        json j = default_options;
        ofstream out_file(config_path);
        out_file << j.dump(4);
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, WHITE);
    Color rand_color = static_cast<Color>(dis(gen));

    string subtitle = fmt::format("  {}     {}  {}{}", username, pwd, os_icon, hostname);

    string screen = fmt::format(
        "{}\n{}\n{}\n{}", 
        colorize(center_x(ascii_art, w.ws_col), rand_color),
        colorize(center_x(fmt::format("Press {} to keep open", prefix), w.ws_col), MAGENTA),
        colorize(center_x(subtitle, w.ws_col + 10), WHITE),
        colorize(center_x(format_options(options), w.ws_col), WHITE)
    );

    cout << center_y(screen, w.ws_row, true);
    cout << "\033[0m";

    if (!qt) {
        char p = getch();
        if (p != prefix) quit();
    }

    while (true) {
        string i;
        if (!qt) {
            cout << ":";
            cin >> i;
        } else {
            i = string(1, getch());
            qt = false;
        }

        if (i == "h") {
            clear_screen();
            string msg = "Salut (macOS edition) is a terminal greeter\n\n"
                         "Close this message with :main\n"
                         "Open this message with :h\n"
                         "Quit with :q\n";
            cout << center_y(center_x(msg, w.ws_col), w.ws_row, true);
        } else if (i == "q") {
            quit();
        } else if (i == "main") {
            cout << "\033[33m";
            cout << center_y(screen, w.ws_row, true);
            cout << "\033[0m";
        } else {
            for (auto el : options) {
                if (el.shortcut == i) {
                    clear_screen();
                    vector<string> argv = split(el.command, ' ');
                    vector<char*> fitting;
                    for (string arg : argv) fitting.push_back(strdup(arg.c_str()));
                    fitting.push_back(nullptr);
                    execvp(argv[0].c_str(), fitting.data());
                }
            }
        }
    }
}
