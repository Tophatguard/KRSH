#include <ncurses.h>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <unistd.h>
#include <filesystem>
#include <cerrno>
#include <fstream>
#include <termios.h>
#include <vector>
#include <sys/wait.h>
#include <dlfcn.h>
#include "krsh_plugin.h"
#include <algorithm>

// hey if you wanna make a plugin it's in the readme, but basically you need to make a .so file with the two functions get_plugin_info() and execute_plugin() and put it in ~/.config/krsh/plugins/

using namespace std;
namespace fs = filesystem;

typedef PluginInfo (*GetInfoFunc)();
typedef void (*ExecuteFunc)(const std::vector<std::string>&);

struct LoadedPlugin {
    void* handle;
    PluginInfo info;
    ExecuteFunc execute_ptr; 
};

vector<LoadedPlugin> global_plugins;

void loadFolderPlugins() {
    char* home = getenv("HOME");
    if (!home) return;

    fs::path plugin_dir = fs::path(home) / ".config/krsh/plugins";
    if (!fs::exists(plugin_dir)) {
        fs::create_directories(plugin_dir);
        return;
    }

    for (const auto& entry : fs::directory_iterator(plugin_dir)) {
        if (entry.path().extension() == ".so") {
            void* handle = dlopen(entry.path().c_str(), RTLD_NOW);
            if (!handle) {
                cerr << "krsh plugin loader error: " << dlerror() << "\n";
                continue;
            }

            GetInfoFunc get_info = (GetInfoFunc)dlsym(handle, "get_plugin_info");
            ExecuteFunc exec_func = (ExecuteFunc)dlsym(handle, "execute_plugin");
            if (!get_info || !exec_func) {
                cerr << "krsh error: Plugin " << entry.path().filename() 
                     << " is missing extern \"C\" bindings: " << dlerror() << "\n";
                dlclose(handle);
                continue;
            }
            LoadedPlugin plugin;
            plugin.handle = handle;
            plugin.info = get_info();
            plugin.execute_ptr = exec_func;

            global_plugins.push_back(plugin);
        }
    }
}

vector<char *> tokenize(const char* cmd) {
    vector<char *> arguments;
    size_t length = strlen(cmd) + 1;
    char *cmd_copy = new char[length];
    strcpy(cmd_copy, cmd);
    size_t len = strlen(cmd_copy);
    while (len > 0 && (cmd_copy[len - 1] == '\n' || 
                       cmd_copy[len - 1] == '\r' || 
                       cmd_copy[len - 1] == ' '  || 
                       cmd_copy[len - 1] == '\t')) {
        cmd_copy[len - 1] = '\0';
        len--;
    }

    char *argument = strtok(cmd_copy, " ");
    while (argument != nullptr) {
        char *arg_copy = new char[strlen(argument) + 1];
        strcpy(arg_copy, argument);
        arguments.push_back(arg_copy);
        argument = strtok(nullptr, " ");
    }
    delete[] cmd_copy;
    arguments.push_back(nullptr);
    return arguments;
}

void executeCommand(const char* cmd) {
    vector<char *> args = tokenize(cmd);
    if (args.empty() || args[0] == nullptr) {
        for (char* arg : args) delete[] arg;
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        cerr << "FATAL: failed to fork a new process: " << strerror(errno) << endl;
        for (char* arg : args) delete[] arg;
        return;
    }

    if (pid == 0) {
        execvp(args[0], args.data());
        cerr << "FATAL: execvp failed: " << strerror(errno) << endl;
        for (char* arg : args) delete[] arg;
        exit(EXIT_FAILURE);
    }

    int status;
    waitpid(pid, &status, 0);
    for (char* arg : args) delete[] arg;
}

void pluginMode() {
    initscr();
    int MaxX, MaxY;
    getmaxyx(stdscr, MaxY, MaxX);
    WINDOW *plugins = newwin(MaxY - 1, MaxX, 0, 0);
    refresh();
    box(plugins, 0, 0);
    
    wprintw(plugins, " === KRSH ACTIVE INSTALLED PLUGINS ===\n\n");
    if (global_plugins.empty()) {
        wprintw(plugins, "  No plugins found. Drop .so profiles into ~/.config/krsh/plugins/\n");
    } else {
        for (size_t i = 0; i < global_plugins.size(); ++i) {
            wprintw(plugins, "  [%lu] %s (Trigger Command: '%s')\n", 
                    i + 1, 
                    global_plugins[i].info.name.c_str(), 
                    global_plugins[i].info.command_trigger.c_str());
            wprintw(plugins, "   |--%s\n", global_plugins[i].info.description.c_str());
        }
    }
    wprintw(plugins, "\n Press any key to return to mode selector...");
    wrefresh(plugins);
    getch();
    endwin();
}

void terminalMode(string autotermm = "true") {
    string input;
    char *home = getenv("HOME");
    if (!home) cerr << "Error: HOME environment variable not set" << endl;

    stringstream config_path;
    config_path << home << "/.config/krsh/krsh.conf";
    ifstream config(config_path.str().c_str());
    string line;
    int cline = 0, symbol = 1, dir = 2, ontop = 3, user = 4;
    string symboll, dirr, ontopp, userr;
    
    while (getline(config, line)) {
        cline++;
        if (cline == symbol) {
            size_t colonPos = line.find('=');
            if (colonPos != string::npos) symboll = line.substr(colonPos + 2);
        } else if (cline == dir) {
            size_t colonPos = line.find('=');
            if (colonPos != string::npos) dirr = line.substr(colonPos + 2);
        } else if (cline == ontop) {
            size_t colonPos = line.find('=');
            if (colonPos != string::npos) ontopp = line.substr(colonPos + 2);
        } else if (cline == user) {
            size_t colonPos = line.find('=');
            if (colonPos != string::npos) userr = line.substr(colonPos + 2);
        }
    }

    if (autotermm == "false") cout << "terminal mode\n";

    while (1) {
        if (dirr == "true")  cout << fs::current_path().string();
        if (userr == "true")  cout << "@" << getenv("USER");
        if (ontopp == "true") cout << "\n";
        cout << symboll;
        
        getline(cin, input);
        
        if (input == "exit") {
            break;
        } else if (input == "close") {
            exit(0);
        } else if (input.substr(0, 2) == "cd" && (input.size() == 2 || input[2] == ' ')) {
            string path = (input.size() > 3) ? input.substr(3) : "";
            if (path.empty()) { path = getenv("HOME"); }
            if (chdir(path.c_str()) != 0) cerr << "cd: " << strerror(errno) << endl;
            continue;
        } else if (input == "clear") {
            cout << "\033[2J\033[1;1H";
            continue;
        } 

        vector<char*> tokens = tokenize(input.c_str());
        bool handled_by_plugin = false;

        if (!tokens.empty() && tokens[0] != nullptr) {
            vector<string> cpp_args;
            for (size_t i = 0; tokens[i] != nullptr; ++i) {
                cpp_args.push_back(string(tokens[i]));
            }

            if (!cpp_args.empty()) {
                for (const auto& plugin : global_plugins) {
                    if (cpp_args[0] == plugin.info.command_trigger) {
                        
                        plugin.execute_ptr(cpp_args);
                        
                        handled_by_plugin = true;
                        break;
                    }
                }
            }
        }

        for (char* arg : tokens) delete[] arg;

        if (handled_by_plugin) continue;

        if (!input.empty()) {
            if (input == "ls") {
                input = "ls --color=auto";
            } else if (input.substr(0, 3) == "ls ") {
                input = "ls --color=auto " + input.substr(3);
            }
            executeCommand(input.c_str());
        }
    }
}

void modeSelector() {
    string input;
    char *home = getenv("HOME");
    if (!home) {
        cerr << "Error: HOME environment variable not set" << endl;
    }

    stringstream config_path;
    config_path << home << "/.config/krsh/krsh.conf";
    ifstream config(config_path.str().c_str());
    string line;
    int cline = 0;
    int tline = 1;
    int autoterm = 5;
    string extractedVariable;
    string autotermm;
    while (getline(config, line)) {
	cline++;
	if (cline == tline) {
    		size_t colonPos = line.find('=');
    		if (colonPos != std::string::npos) {
        		extractedVariable = line.substr(colonPos + 2);
    		}
	} else if (cline == autoterm) {
		        size_t colonPos = line.find('=');
		        if (colonPos != string::npos) {
			        autotermm = line.substr(colonPos + 2);
		        }
	}
    }
    if (autotermm == "true") {
        cout << "automatically entering terminal mode\n";
        terminalMode(autotermm);
    }
    cout << "mode selector\n";
    while (1) {
        cout << extractedVariable;
        getline(cin, input);
        
        if (input == "listmodes") {
            cout << "terminal mode\nplugin mode\n";
        } else if (input == "mode") {
            cout << "mode name\n>> ";
            getline(cin, input);
            if (input == "terminal mode") {
                terminalMode();
            } else if (input == "plugin mode") {
		        pluginMode();
	        } else {
                cout << "no mode recognized\n";
            }
        } else if (input == "exit") {
            exit(0);
            break;
        } else if (input == "clear") {
            cout << "\033[2J\033[1;1H";
        } else if (input.empty()) {
            cout << "no command recognized\n";
        } else if (input == "list") {
            cout << "mode - use this to select a mode\n"
                 << "listmodes - use this to list all modes\n"
                 << "exit - use this to exit the program\n"
                 << "clear - use this to clear the screen\n";
        } else if (input == "refreshplugins") {
            loadFolderPlugins();
        }
    }
}

int main() {
    loadFolderPlugins();
    char *home = getenv("HOME");
    if (!home) {
        cerr << "Error: HOME environment variable not set" << endl;
        return 1;
    }

    stringstream config_path;
    config_path << home << "/.config/krsh/krsh.conf";
    fs::create_directories(fs::path(home) / ".config/krsh/plugins");
    if (!fs::exists(config_path.str())) {
	cout << "A config file does not exist, generating one.\n";
        fs::create_directories(fs::path(config_path.str()).parent_path());
        
        ofstream config_file(config_path.str());
        if (config_file) {
            config_file << "symbol = # \n"
                        << "showDirectory = true\n"
	    		        << "directoryOnTop = false\n"
			            << "showUser = false\n"
                        << "autoOpenTerminalMode = false\n"
                        << "showWelcome = true\n";

	cout << "Done generating!\n";
    cout << "the path to the config file is: " << config_path.str() << endl;
        } else {
            cerr << "Error: Failed to create config file" << endl;
            return 1;
        }
    }

    ifstream config(config_path.str().c_str());
    string line;
    int cline = 0;
    int showWelcome = 6;
    string showWelcomee;

    while (getline(config, line)) {
	cline++;
	if (cline == showWelcome) {
    		size_t colonPos = line.find('=');
    		if (colonPos != std::string::npos) {
        		showWelcomee = line.substr(colonPos + 2);
    		}
	}
    
    }

    if (showWelcomee == "false") {
        modeSelector();
    }

    cout << "------------------------------------------\n"
         << "| Welcome to krsh!                       |\n"
         << "| The lightweight command line interface |\n"
         << "------------------------------------------\n"
         << "\n- now type \"list\" to get a list of all commands!\n";
    
    modeSelector();
    return 0;
}
