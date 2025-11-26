# Salut
## A beautiful terminal greeter 🌸

[![Build and Release Debian Package](https://github.com/fauzymadani/salut/actions/workflows/build.yml/badge.svg)](https://github.com/fauzymadani/salut/actions/workflows/build.yml)

Salut is a simple yet beautiful terminal greeter.   
To use it, simply attach the path of the salut binary to the top of your shell config file.
Salut will then launch, everytime you open your shell.

<div align="center">
<img src="screenshot.png" width="500" alt="Salut in action" />
</div>

### Installation
Salut does not come pre-built for any distributions.   
You have to build it your self.

1. Clone this repository
```bash
git clone https://github.com/Wervice/salut
```
2. Compile and run the source code
- a) Using GCC
```bash
g++ -o salut -O3 main.cpp -lfmt -I./include && ./salut
```
- b) Using Clang
```bash
clang++ -o salut -O3 main.cpp -lfmt -I./include  && ./salut
```
- c) If you are on NixOS
```bash
nix-shell
```
- d) If you are on macOS
```bash
brew install fmt
brew install nlohmann-json
g++ main_for_mac.cpp -std=c++17 -I/opt/homebrew/include -L/opt/homebrew/lib -lfmt -o salut && ./salut
```

3. Copy the executable somewhere else
I recommend moving the executable into a folder in your `$PATH`, but you can choose any location you like.
4. Add `salut` binary path to your config file
Depending on your shell, your config file may be called:
- `~/.bashrc`
- `~/.zshrc`
- `~/.config/fish/config.fish`
5. Salut will now launch every time you open a new shell

### Usage
To keep salut open, you need to press `:` one time, before you can do anything.
If you do not press `:`, the splash screen will instantly close at the first key press and leave your default shell.
Salut uses a command line at the bottom of your terminal for inputs. You do not need to enter the `:` when writing commands.
You run the programs in the list by entering their corresponding command.
By default the following commands are implemented:
|Command|Action|
|-------|------|
|`q`    |Exits salut|
|`h`    |Shows help|
|`main` |Returns to main screen|
|`nv`   |Opens neovim|
|`ft `  |Opens fastfetch|
|`bs`   |Starts bash|
|`bp`   |Opens btop|

### Configuration
Salut is configured using a JSON file located at:
```
~/.config/salut/config.json
```
You can set your own applications and preferences by editing this file.
#### Example format:

```jsonc
{
"command": "zsh",   // The actual shell command that gets executed
"icon": "$ ",       // A Nerd Font or ASCII icon shown before the name
"name": "Zsh",      // Display name shown in the menu
"shortcut": "zs"    // The key sequence typed to run this program
},
```
Each entry defines an application with a display name, icon, shortcut, and command.

> [!NOTE]
> salut will not display links or ascii art if the terminal is to small.

#### Flags
- `--quick-tap`  
  When enabled, salut will only listen to one key press before quitting. This way, you can skip the prefix and press a key to immediately launch any command you want.

### The name
The name of `salut` comes from the french "salut", which means "Hi".
It is pronouced without the t in the end.
