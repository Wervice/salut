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
#include <iterator>
#include <random>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <tuple>
#include <unistd.h>
#include <vector>
using namespace std;

vector<string> split(string a, char delim) {
  vector<string> result;
  string left = a;

  while (left.find(delim) != std::string::npos) {
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

string center_y(string a, int h, bool fill) {
  vector<string> lines = split(a, '\n');
  string result;
  int padding = (h - lines.size()) / 2 + 1;
  if (padding <= 0) {
    return result;
  }
  result.append(string(padding, '\n'));
  result.append(a);

  if (fill) {
    result.append(string(padding, '\n'));
  }

  return result;
}

void clear_screen() { std::cout << "\033[2J\033[1;1H"; }

string format_options(vector<tuple<string, string, string, string>> options) {
  string result;
  int break_c = 0;

  for (tuple<string, string, string, string> el : options) {
    string break_pad;
    break_c++;
    if (break_c == 2) {
      break_pad = "\n";
      break_c = 0;
    } else {
      break_pad = "\t";
    }
    result.append(fmt::format("{} {} [:{}]{}", get<1>(el), get<0>(el),
                              get<2>(el), break_pad));
  }

  return result;
}

int getch() {
  char ch;
  struct termios oldattr;
  struct termios newattr;

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
  if (c == RED) {
    code = 31;
  } else if (c == GREEN) {
    code = 32;
  } else if (c == YELLOW) {
    code = 33;
  } else if (c == BLUE) {
    code = 34;
  } else if (c == MAGENTA) {
    code = 35;
  } else if (c == CYAN) {
    code = 36;
  } else if (c == WHITE) {
    code = 0;
  }

  return fmt::format("\033[{}m{}\033[0m", code, s);
}

void quit() {
  clear_screen();
  int ret = system("clear");
  if (ret == -1) {
    std::cerr << "Error: clear command failed!\n";
    exit(1);
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  bool qt = false;
  if (argc > 1) {
    if (strcmp(argv[1], "--quick-tap") == 0) {
      qt = true;
    }
  }

  string ascii_art =
      "██╗    ██╗███████╗██╗      ██████╗ ██████╗ ███╗   ███╗███████╗\n██║    "
      "██║██╔════╝██║     ██╔════╝██╔═══██╗████╗ ████║██╔════╝\n██║ █╗ "
      "██║█████╗  ██║     ██║     ██║   ██║██╔████╔██║█████╗  "
      "\n██║███╗██║██╔══╝  ██║     ██║     ██║   ██║██║╚██╔╝██║██╔══╝  "
      "\n╚███╔███╔╝███████╗███████╗╚██████╗╚██████╔╝██║ ╚═╝ ██║███████╗\n "
      "╚══╝╚══╝ ╚══════╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝";

  char prefix = ':';

  string username = getenv("USER");
  string pwd = std::filesystem::current_path().string();
  string hostname;
  string os_icon;
  ifstream hostname_file;
  hostname_file.open("/etc/hostname");
  getline(hostname_file, hostname);
  ifstream os_release_file;
  os_release_file.open("/etc/os-release");
  string os_release_line;
  getline(os_release_file, os_release_line);
  while (split(os_release_line, '=')[0] != "ID") {
    getline(os_release_file, os_release_line);
  }
  string id = split(os_release_line, '=')[1];
  if (id == "arch") {
    os_icon = colorize(" ", BLUE);
  } else if (id == "debian") {
    os_icon = colorize(" ", RED);
  } else if (id == "ubuntu") {
    os_icon = colorize("󰕈 ", YELLOW);
  } else if (id == "fedora") {
    os_icon = colorize(" ", BLUE);
  } else if (id == "nixos") {
    os_icon = colorize(" ", BLUE);
  } else {
    os_icon = colorize(" ", YELLOW);
  }
  // Tuple values:
  // 1) The name of the program/command as it will be displayed
  // 2) The nerd fonts icon to use for this program
  // 3) The command you need to enter to run the program
  // 4) The actual command that will be ran.
  //	This uses a ' ' seperated string with up to 5 arguments
  //	You can add more in the `execlp` down bellow.
  vector<tuple<string, string, string, string>> options = {
      make_tuple("Neovim", " ", "nz", "nvim"),
      make_tuple("Fastfetch", os_icon, "ft", "fastfetch"),
      make_tuple("Zsh", "$ ", "zs", "zsh"),
      make_tuple("Btop", " ", "bp", "btop"),
  };

  static random_device rd;
  static mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, WHITE);
  Color rand_color = static_cast<Color>(dis(gen));

  pwd.replace(0, strlen(getenv("HOME")), "~");
  string subtitle = fmt::format("  {}     {}  {}{}", username, pwd,
                                os_icon, hostname);
  string exit_directory;

  std::cout << "\033[32m";
  string screen = fmt::format(
      "{}\n{}\n{}\n{}", colorize(center_x(ascii_art, w.ws_col), rand_color),
      colorize(center_x(fmt::format("Press {} to keep open", prefix), w.ws_col),
               MAGENTA),
      colorize(center_x(subtitle, w.ws_col + 10), WHITE),
      colorize(center_x(format_options(options), w.ws_col), WHITE));
  std::cout << center_y(screen, w.ws_row, true);
  std::cout << "\033[0m";

  if (!qt) {
    char p = getch();
    string c_inp;
    if (p != prefix) {
      quit();
    }
  }

  screen = fmt::format(
      "{}\n{}\n{}", colorize(center_x(ascii_art, w.ws_col), rand_color),
      colorize(center_x(subtitle, w.ws_col + 10), WHITE),
      colorize(center_x(format_options(options), w.ws_col), WHITE));
  std::cout << center_y(screen, w.ws_row, true);

  bool immediate = qt;
  while (true) {
    string i;
    if (!immediate) {
	  std::cout << ":";
      cin >> i;
    } else {
      i = getch();
      immediate = false;
    }

    if (i == "h") {
      clear_screen();
      string msg = "Salut (\033[32mfrench: Hi; /saly/\033[0m) is a terminal "
                   "greeter application\n"
                   "It provides a clean welome screen when launched and let's "
                   "you quickly launch\n"
                   "your most important applications.\n\n"
                   "\033[31mClose this message with\033[0m \t:main\n"
                   "\033[31mOpen this message with\033[0m \t:h\n"
                   "\033[31mQuit with\033[0m \t\t\t:q\n";
      std::cout << center_y(center_x(msg, w.ws_col), w.ws_row, true);
    } else if (i == "q") {
      quit();
    } else if (i == "main") {
      std::cout << "\033[33m";
      std::cout << center_y(screen, w.ws_row, true);
      std::cout << "\033[0m";
    } else {
      for (tuple<string, string, string, string> el : options) {
        if (get<2>(el) == i) {
          clear_screen();
          int ret = std::system("clear");
          if (ret == -1) {
            std::cerr << "Error: clear command failed!\n";
            exit(1);
          }
          string v = get<3>(el);
          std::vector<string> argv = split(v, ' ');
          std::vector<char *> fitting;
          for (string arg : argv) {
            char *non_const = (char *)strdup(arg.c_str());
            fitting.push_back(non_const);
          }
          fitting.push_back(nullptr);
          execvp(argv[0].c_str(), fitting.data());
        }
      }
	  if (qt) {
		clear_screen();
		system("clear");
		exit(0);
	  }
    }
  }
}
