#ifndef KRSH_PLUGIN_H
#define KRSH_PLUGIN_H

#include <string>
#include <vector>

// 1. Data blueprint: What the plugin must tell krsh about itself
struct PluginInfo {
    std::string name;             // e.g., "System Monitor"
    std::string command_trigger;  // e.g., "sysmon" (what the user types to run it)
    std::string description;      // e.g., "Displays system resource usage in real-time"
};

// 2. The code contract: Every plugin MUST export these two exact functions.
// We use extern "C" to stop C++ from hiding/mangling the function names!
extern "C" {
    PluginInfo get_plugin_info();
    void execute_plugin(const std::vector<std::string>& args);
}

#endif
