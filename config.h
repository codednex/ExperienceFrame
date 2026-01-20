#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <fstream>
#include <windows.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct AppConfig {
    std::string url = "./example.html";
    int distributedMode = 0;
    std::vector<std::string> lockedURL;
    bool bypassEnabled = false;
};

// Formatuje URL: jeśli to nie www, zamienia na ścieżkę absolutną file:///
std::wstring GetFormattedUrl(std::string target) {
    std::string finalUrl = target;
    
    // Sprawdzenie czy to nie jest adres internetowy
    if (target.find("http://") != 0 && target.find("https://") != 0) {
        char fullPath[MAX_PATH];
        if (target.find("./") == 0) {
            GetFullPathNameA(target.c_str(), MAX_PATH, fullPath, NULL);
            finalUrl = "file:///" + std::string(fullPath);
        }
    }

    int len = MultiByteToWideChar(CP_UTF8, 0, finalUrl.c_str(), -1, NULL, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, finalUrl.c_str(), -1, &wstr[0], len);
    return wstr;
}

bool IsUrlBlocked(std::string pattern, std::string url) {
    if (pattern == "*") return true;
    return url.find(pattern) != std::string::npos;
}

AppConfig LoadSettings() {
    AppConfig cfg;
    std::ifstream file("config.json");
    if (!file.is_open()) {
        json j;
        j["url"] = "./example.html";
        j["distributedMode"] = 0;
        j["lockedURL"] = {"*.doubleclick.net"};
        std::ofstream out("config.json");
        out << j.dump(4);
        return cfg;
    }
    json data;
    try {
        file >> data;
        cfg.url = data.value("url", "./example.html");
        cfg.distributedMode = data.value("distributedMode", 0);
        cfg.lockedURL = data.value("lockedURL", std::vector<std::string>{});
        
        if (!cfg.url.empty() && cfg.url[0] == '!') {
            cfg.bypassEnabled = true;
            cfg.url.erase(0, 1);
        }
    } catch(...) {}
    return cfg;
}
#endif