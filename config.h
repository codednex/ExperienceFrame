#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct AppConfig {
    std::string url = "./example.html";
    int distributedMode = 0;
    std::vector<std::string> lockedURL;
    bool bypassEnabled = false;
};

// Wildcard matching
bool IsUrlBlocked(std::string pattern, std::string url) {
    if (pattern == "*") return true;
    if (pattern.find("*.") == 0) {
        std::string dom = pattern.substr(2);
        return url.length() >= dom.length() && url.compare(url.length()-dom.length(), dom.length(), dom) == 0;
    }
    return url.find(pattern) != std::string::npos;
}

AppConfig LoadSettings() {
    AppConfig cfg;
    const std::string filename = "config.json";
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        // Generate default config.json if not exists
        json j;
        j["url"] = "./example.html";
        j["distributedMode"] = 0;
        j["lockedURL"] = json::array({"*.doubleclick.net", "*.ads.com"});
        
        std::ofstream out(filename);
        out << j.dump(4);
        out.close();
        return cfg;
    }

    json data;
    file >> data;
    cfg.url = data.value("url", "./example.html");
    cfg.distributedMode = data.value("distributedMode", 0);
    cfg.lockedURL = data.value("lockedURL", std::vector<std::string>{});

    if (!cfg.url.empty() && cfg.url[0] == '!') {
        cfg.bypassEnabled = true;
        cfg.url.erase(0, 1);
    }
    return cfg;
}

#endif