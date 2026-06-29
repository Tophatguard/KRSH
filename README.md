# KRSH
A very basic but customizable shell
-----------------------------------
this is the example of a plugin
#include "krsh_plugin.h"
#include <iostream>

extern "C" {

    // 1. Tell krsh who this plugin is and what command triggers it
    PluginInfo get_plugin_info() {
        PluginInfo info;
        info.name = "Plugin Test";
        info.description = "a test plugin to make sure that plugins work";
        info.command_trigger = "plgntst"; // Typing 'testplugin' runs this
        return info;
    }

    // 2. The core logic that runs instantly inside your shell's RAM space
    void execute_plugin(const std::vector<std::string>& args) {
        std::cout << "\n\033[1;32m🙏 SUCCESS: g++ cooperated perfectly!\033[0m" << std::endl;
        std::cout << "The .so plugin architecture is working natively in-process." << std::endl;

        // Show that argument arrays are passing smoothly between the binaries
        if (args.size() > 1) {
            std::cout << "Extra arguments captured by plugin: ";
            for (size_t i = 1; i < args.size(); ++i) {
                std::cout << "\033[1;36m" << args[i] << "\033[0m ";
            }
            std::cout << std::endl;
        } else {
            std::cout << "No arguments passed to the plugin, try passing some!";
        }
        std::cout << std::endl;
    }

}
