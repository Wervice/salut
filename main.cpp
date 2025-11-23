#include "utfcpp-4.0.6/source/utf8/checked.h"
#include <asm-generic/ioctls.h>
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

// ==============================
// UTILITY FUNCTIONS
// ==============================

vector<string> split(const string& str, char delim) {
    vector<string> result;
    string remaining = str;
    while (remaining.find(delim) != string::npos) {
        size_t index = remaining.find_first_of(delim);
        result.push_back(remaining.substr(0, index));
        remaining = remaining.substr(index + 1);
    }
    result.push_back(remaining);
    return result;
}

string center_x(string a, int w) {
    vector<string> lines = split(a, '\n');
    string result;

    int len = utf8::distance(lines[0].begin(), lines[0].end());
    int padding = (w - len) / 2;
    if (padding <= 0) {
        return result;
    }
    string padstr = string(padding, ' ');
    for (string line : lines) {
        result.append(padstr);
        result.append(line);
        result.append("\n");
    }

    return result;
}

string center_y(const string& str, int height, bool fill = true) {
    vector<string> lines = split(str, '\n');
    string result;

    int padding = (height - lines.size()) / 2 + 1;
    if (padding <= 0) return str;

    result.append(string(padding, '\n') + str);
    if (fill) result.append(string(padding, '\n'));

    return result;
}

void clear_screen() {
    cout << "\033[2J\033[1;1H";
}

// ==============================
// PROGRAM STRUCTURE
// ==============================

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

//  Parse configuration supporting both older array format and newer object with "programs"
vector<Program> parse_config(const json& config) {
    vector<Program> programs;
    if (config.is_object() && config.contains("programs") && config["programs"].is_array()) {
        for (const auto& item : config["programs"]) {
            programs.push_back(make_program(item.value("name", string("unknown")),
                                            item.value("icon", string("")),
                                            item.value("shortcut", string("")),
                                            item.value("command", string(""))));
        }
    } else if (config.is_array()) {
        for (const auto& item : config) {
            //  older format: array of program objects
            programs.push_back(make_program(item.value("name", string("unknown")),
                                            item.value("icon", string("")),
                                            item.value("shortcut", string("")),
                                            item.value("command", string(""))));
        }
    }
    return programs;
}

string format_options(const vector<Program>& options, const string& separator) {
    string result;
    int counter = 0;

    for (const auto& el : options) {
        counter++;
        string sep = (counter == 2) ? "\n" : separator;
        if (counter == 2) counter = 0;
        result.append(fmt::format("{} {} [:{}]{}", el.icon, el.name, el.shortcut, sep));
    }
    return result;
}

// ==============================
// TERMINAL INPUT
// ==============================

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

// ==============================
// COLOR UTILITIES
// ==============================

enum Color { RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, RANDOM };

string colorize(const string& str, int code) {
    return fmt::format("\033[{}m{}\033[0m", code, str);
}

void quit() {
    clear_screen();
    exit(0);
}

// ==============================
// MAIN PROGRAM
// ==============================

int main(int argc, char* argv[]) {
    // Reported by PhoenixAceVFX in this PR: https://github.com/Wervice/salut/pull/10
    // and later modified
    // -------
    if (getenv("DISPLAY") == nullptr) {
        return 0;  // Exit silently if running from a display manager
    }
    // -------
    // The block above was intentionally saved (commented) per your request.

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    bool qt = (argc > 1 && strcmp(argv[1], "--quick-tap") == 0);

    const char* xdg_config = getenv("XDG_CONFIG_HOME");
    string config_dir = (xdg_config != nullptr) ? string(xdg_config) + "/salut" : string(getenv("HOME")) + "/.config/salut";
    filesystem::create_directories(config_dir);

    string config_path = config_dir + "/config.json";
    string ascii_file_path = config_dir + "/custom-ascii-art.txt";

    const string default_ascii =
        "██╗    ██╗███████╗██╗      ██████╗ ██████╗ ███╗   ███╗███████╗\n"
        "██║    ██║██╔════╝██║     ██╔════╝██╔═══██╗████╗ ████║██╔════╝\n"
        "██║ █╗ ██║█████╗  ██║     ██║     ██║   ██║██╔████╔██║█████╗  \n"
        "██║███╗██║██╔══╝  ██║     ██║     ██║   ██║██║╚██╔╝██║██╔══╝  \n"
        "╚███╔███╔╝███████╗███████╗╚██████╗╚██████╔╝██║ ╚═╝ ██║███████╗\n"
        " ╚══╝╚══╝ ╚══════╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝";

    if (!filesystem::exists(ascii_file_path)) {
        ofstream ascii_file(ascii_file_path);
        ascii_file << default_ascii;
    }

    json config;
    if (filesystem::exists(config_path)) {
        ifstream in_file(config_path);
        try {
            in_file >> config;
        } catch (...) {
            config = json::object();
        }
    } else {
        config = {
            {"separator_subtitle", " "},
            {"separator_commands", "\t"},
            {"color_art", "RANDOM"},
            {"color_subtitle", "RANDOM"},
            {"color_commands", "RANDOM"},
            {"programs", {
                { {"name", "Neovim"}, {"icon", " "}, {"shortcut", "nv"}, {"command", "nvim"} },
                { {"name", "Fastfetch"}, {"icon", " "}, {"shortcut", "ft"}, {"command", "fastfetch"} },
                { {"name", "Bash"}, {"icon", " "}, {"shortcut", "bs"}, {"command", "bash"} },
                { {"name", "Btop"}, {"icon", " "}, {"shortcut", "bp"}, {"command", "btop"} }
            }}
        };
        ofstream out_file(config_path);
        out_file << config.dump(4);
    }

    string ascii_art;
    ifstream ascii_file(ascii_file_path);
    ascii_art.assign((istreambuf_iterator<char>(ascii_file)), istreambuf_iterator<char>());

    vector<Program> options = parse_config(config);
    string separator_subtitle = config.value("separator_subtitle", " ");
    string separator_commands = config.value("separator_commands", "\t");

    //  Linux-specific: try to detect the distro from /etc/os-release and set icon
    string os_icon;
    string hostname = "unknown";
    ifstream hostname_file("/etc/hostname");
    if (hostname_file.is_open() && !hostname_file.eof()) {
        getline(hostname_file, hostname);
    }

    string id;
    ifstream os_release_file("/etc/os-release");
    if (os_release_file.is_open()) {
        string line;
        while (getline(os_release_file, line)) {
            if (line.rfind("ID=", 0) == 0) {
                id = line.substr(3);
                break;
            }
        }
    }

    auto colorize_icon = [&](const string& icon, int code) {
        return fmt::format("\033[{}m{}\033[0m", code, icon);
    };

    if (id == "arch") {
        os_icon = " ";
    } else if (id == "debian") {
        os_icon = " ";
    } else if (id == "ubuntu") {
        os_icon = "󰕈 ";
    } else if (id == "fedora") {
        os_icon = " ";
    } else if (id == "nixos") {
        os_icon = " ";
    } else if (id == "linuxmint") {
        os_icon = "󰣭 ";
    } else if (id == "gentoo") {
        os_icon = " ";
    } else if (id == "\"endeavouros\"" || id == "endeavouros") {
        os_icon = " ";
    } else {
        os_icon = " ";
    }

    //  Color parsing helpers (reuse RANDOM option)
    auto parse_color = [](const string& cstr) -> Color {
        if (cstr == "RED") return RED;
        if (cstr == "GREEN") return GREEN;
        if (cstr == "YELLOW") return YELLOW;
        if (cstr == "BLUE") return BLUE;
        if (cstr == "MAGENTA") return MAGENTA;
        if (cstr == "CYAN") return CYAN;
        if (cstr == "WHITE") return WHITE;
        return RANDOM;
    };

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(31, 36);
    int random_color_code = dis(gen);

    auto get_color_code = [&](const string& cstr) -> int {
        Color c = parse_color(cstr);
        if (c == RANDOM) return random_color_code;
        switch(c) {
            case RED: return 31;
            case GREEN: return 32;
            case YELLOW: return 33;
            case BLUE: return 34;
            case MAGENTA: return 35;
            case CYAN: return 36;
            case WHITE: return 0;
            default: return 0;
        }
    };

    int color_art_code = get_color_code(config.value("color_art", "RANDOM"));
    int color_subtitle_code = get_color_code(config.value("color_subtitle", "RANDOM"));
    int color_commands_code = get_color_code(config.value("color_commands", "RANDOM"));

    string username = getenv("USER") ? getenv("USER") : "unknown";
    string pwd = filesystem::current_path().string();
    const char* home_env = getenv("HOME");
    if (home_env != nullptr) {
        pwd.replace(0, strlen(home_env), "~");
    }

    string subtitle = fmt::format(
        "{0}   {2} {0}   {3} {0} {1} {4} {0}",
        separator_subtitle,
        os_icon,
        username,
        pwd,
        hostname
    );

    string screen = fmt::format(
        "{}\n{}\n{}",
        colorize(center_x(ascii_art, w.ws_col), color_art_code),
        colorize(center_x(subtitle, w.ws_col), color_subtitle_code),
        colorize(center_x(format_options(options, separator_commands), w.ws_col), color_commands_code)
    );

    cout << center_y(screen, w.ws_row, true);
    cout << "\033[0m";

    if (!qt) {
        char p = getch();
        if (p != ':') quit();
    }

    while (true) {
        string input;
        if (!qt) {
            cout << ":";
            cin >> input;
        } else {
            input = string(1, getch());
            qt = false;
        }

        if (input == "h") {
            clear_screen();
            string msg =
                "Salut is a terminal greeter application\n\n"
                "Close this message with :main\n"
                "Open this message with :h\n"
                "Quit with :q\n";
            cout << colorize(center_y(center_x(msg, w.ws_col), w.ws_row, true), color_art_code);
        } else if (input == "q") {
            quit();
        } else if (input == "main") {
            cout << "\033[33m";
            cout << center_y(screen, w.ws_row, true);
            cout << "\033[0m";
        } else {
            for (auto& el : options) {
                if (el.shortcut == input) {
                    clear_screen();
                    vector<string> argv = split(el.command, ' ');
                    vector<char*> exec_args;
                    for (const string& arg : argv) exec_args.push_back(strdup(arg.c_str()));
                    exec_args.push_back(nullptr);
                    execvp(argv[0].c_str(), exec_args.data());
                }
            }
        }
    }

    return 0;
}
