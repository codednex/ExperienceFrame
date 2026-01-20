#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <fstream>
#include <windows.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct AppConfig {
    std::string url = "./index.html";
    int distributedMode = 0;
    int allowUnsafe = 1;
    std::vector<std::string> lockedURL;
};

void EnsureConfigExists() {
    std::ifstream checkFile("config.json");
    if (checkFile.is_open()) {
        checkFile.close();
        return; 
    }

    json j;
    j["url"] = "./index.html";
    j["distributedMode"] = 0;
    j["allowUnsafe"] = 1;
    // Przykład filtrów w nowym pliku
    j["lockedURL"] = {"*.doubleclick.net", "facebook.com/*", "!google.com"}; 
    
    std::ofstream out("config.json");
    if (out.is_open()) {
        out << j.dump(4);
        out.close();
    }
}

std::wstring GetFormattedUrl(std::string target) {
    if (target.find("http://") == 0 || target.find("https://") == 0 || target.find("about:") == 0) {
        int len = MultiByteToWideChar(CP_UTF8, 0, target.c_str(), -1, NULL, 0);
        std::wstring wstr(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, target.c_str(), -1, &wstr[0], len);
        return wstr;
    } 
    char fullPath[MAX_PATH];
    if (GetFullPathNameA(target.c_str(), MAX_PATH, fullPath, NULL) == 0) return L"about:blank";
    std::string sPath(fullPath);
    for(auto &c : sPath) if(c == '\\') c = '/';
    std::string finalUrl = "file:///" + sPath;
    int len = MultiByteToWideChar(CP_UTF8, 0, finalUrl.c_str(), -1, NULL, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, finalUrl.c_str(), -1, &wstr[0], len);
    return wstr;
}

AppConfig LoadSettings() {
    EnsureConfigExists(); 
    AppConfig cfg;
    std::ifstream file("config.json");
    if (!file.is_open()) return cfg;
    json data;
    try {
        file >> data;
        cfg.url = data.value("url", "./index.html");
        cfg.distributedMode = data.value("distributedMode", 0);
        cfg.allowUnsafe = data.value("allowUnsafe", 1);
        cfg.lockedURL = data.value("lockedURL", std::vector<std::string>{});
    } catch(...) {}
    return cfg;
}
#endif