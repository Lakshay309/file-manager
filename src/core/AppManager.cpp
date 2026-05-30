#include "AppManager.hpp"
#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

AppManager::AppManager(){
    // currently app load lazily on first use not in constructor
    // AppManager::refresh(); if we want to force reload
}


// Public API

// Returns all installed apps loads from disk on first call cached after
std::vector<AppInfo> AppManager::getAllApps(){
    if(!loaded_) loadApps();
    return cachedApps_;
}

std::vector<AppInfo> AppManager::getAppForFile(const std::filesystem::path& filesPath){
    if(!loaded_) loadApps();

    #if defined (__linux__)

    std::string mime = getMimeTypeForFile(filesPath);

    if(mime.empty()) return cachedApps_;

    std::vector<AppInfo> matching;
    for(const auto& app: cachedApps_){
        for(const auto& supported:app.mimeTypes){
            if(supported==mime){
                matching.push_back(app);
                break;
            }
        }
    }
    // if nothing matched then return all apps
    return matching.empty()?cachedApps_:matching;
    #elif defined(_WIN32) || defined(_WIN64)
        // TODO: Filter by file extension using registry associations
        return cachedApps_;
    
    #elif defined(__APPLE__)
        // TODO: Use LSCopyApplicationURLsForURL to find capable apps
        return cachedApps_;
    
    #else
        return cachedApps_;
    #endif
}

// called after installing/uninstalling apps
void AppManager::refresh() {
    cachedApps_.clear();
    loaded_ = false;
    loadApps();
}

void AppManager::loadApps(){
    loaded_=true;

    #if defined(__linux__)
    // Ref: https://specifications.freedesktop.org/desktop-entry-spec/latest/

    const std::vector<std::filesystem::path> desktopDirs={
        "/usr/share/applications",          // System-wide apps (most common)
        "/usr/local/share/applications",    // Locally compiled/installed apps
        std::filesystem::path(getenv("HOME") ? getenv("HOME") : "") / ".local/share/applications", // User-installed apps
        "/var/lib/flatpak/exports/share/applications",  // Flatpak apps
        std::filesystem::path(getenv("HOME") ? getenv("HOME") : "") / ".local/share/flatpak/exports/share/applications", // User Flatpak
        "/snap/bin",                        // Snap apps (executables, not .desktop)
    };

    for(const auto& dir:desktopDirs){
        if(std::filesystem::exists(dir) && std::filesystem::is_directory(dir)){
            scanDesktopFiles(dir);
        }
    }

    // Remove duplicates by executable name
    std::sort(cachedApps_.begin(),cachedApps_.end(),[](const AppInfo& a,const AppInfo& b){
        return a.name<b.name;
    });
    cachedApps_.erase(
        std::unique(cachedApps_.begin(),cachedApps_.end(),[](const AppInfo& a, const AppInfo& b){
            return a.executable==b.executable;
        }),
        cachedApps_.end()
    );

    #elif defined(_WIN32) || defined(_WIN64)
        // TODO: Windows — read installed apps from:
        //   HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths
        //   HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths
        //   Or enumerate Start Menu .lnk shortcuts

    #elif defined(__APPLE__)
        // TODO: macOS — scan /Applications directory recursively,
        //   read each app's Info.plist for CFBundleExecutable, CFBundleName,
        //   and CFBundleDocumentTypes for supported file types
    #endif
}


#if defined(__linux__)

void AppManager::scanDesktopFiles(const std::filesystem::path& dir) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() == ".desktop") {
                AppInfo app = parseDesktopFile(entry.path());

                // Only add if it's a valid GUI app with an executable
                if (!app.name.empty() && !app.execPath.empty()) {
                    cachedApps_.push_back(std::move(app));
                }
            }
        }
    } catch (const std::exception&) {
        // Skip directories we can't read (permissions etc.)
    }
}


// Parses a single .desktop file into an AppInfo struct
// .desktop files are simple key=value text files, one section at a time
// Example:
//   [Desktop Entry]
//   Name=VLC Media Player
//   Exec=vlc %U
//   Icon=vlc
//   MimeType=video/mp4;audio/mpeg;
AppInfo AppManager::parseDesktopFile(const std::filesystem::path& path) {
    AppInfo app;

    std::ifstream file(path);
    if(!file.is_open()) return app;

    bool inDesktopEntry = false;
    std::string line;

    while(std::getline(file,line)){
        // trim white space
        line.erase(0,line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t\r\n")+1);
        // skip empty or comment lines
        if(line.empty() || line[0]=='#') continue;

        // Section header like [Desktop Entry] or [Desktop Action New]
        if(line[0]=='['){
            inDesktopEntry=(line == "[Desktop Entry]");
            continue;
        }

        // Only care about [Desktop Entry] section
        if(!inDesktopEntry) continue;

        // split on first ==
        auto eq = line.find('=');
        if(eq==std::string::npos) continue;

        std::string key = line.substr(0,eq);
        std::string value = line.substr(eq+1);

        if (key == "Name"){
            app.name = value;
        } 
        else if (key == "Exec"){
            // Exec= can have placeholders like %f %u %F %U — strip them
            // They are replaced by the launcher with actual file paths at runtime
            std::string exec;
            std::istringstream ss(value);
            std::string token;
            bool first = true;
            // FIXME: doubt here 
            while(ss>>token){
                if(token[0]=='%') continue; //place holder ignored
                if(first){
                    exec = token; // First token is executable
                    first = false;
                }
            }
            app.executable = std::filesystem::path(exec).filename().string();

            app.execPath = resolveExecutable(exec);
        }
        else if (key == "Icon"){
            app.icon = value;
        }
        else if (key == "MimeType") {
            std::istringstream ss(value);
            std::string mime;

            while (std::getline(ss, mime, ';')) {
                if (!mime.empty()) {
                    app.mimeTypes.push_back(mime);
                }
            }
        }
        else if(key == "NoDisplay" && value =="true"){
            return AppInfo{};
        }
        else if(key == "Type" && value !="Application"){
            return AppInfo{};
        } 
    }

    return app;
}

// Resolves an executable name to its full path
// e.g. "vlc" → "/usr/bin/vlc"
// If already an absolute path, returns as-is
std::filesystem::path AppManager::resolveExecutable(const std::string& exec) {
    if(exec.empty()) return {};

    std::filesystem::path p(exec);

    if (p.is_absolute()){
        return std::filesystem::exists(p)? p : std::filesystem::path{}; 
    }

    // Search PATH environment variable
    const char* pathEnv = getenv("PATH");
    if (!pathEnv) return {};

    std::istringstream ss(pathEnv);
    std::string dir;
    // FIXME: check this also 
    while (std::getline(ss, dir, ':')) {
        auto candidate = std::filesystem::path(dir) / exec;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {}; 
}

// Uses the 'file' command to detect MIME type of a file
// e.g. /home/user/video.mp4 → "video/mp4"
std::string AppManager::getMimeTypeForFile(const std::filesystem::path& filePath) {
    try {
        std::string command = "file --mime-type -b \"" + filePath.string() + "\" 2>/dev/null";
    
        FILE* pipe = popen(command.c_str(), "r");
        if(!pipe) return {};
    
        char buffer[128];
        std::string result;
        while(fgets(buffer,sizeof(buffer),pipe)){
            result +=buffer;
        }
        pclose(pipe);
    
        result.erase(result.find_last_not_of("\t\r\n")+1);
        return result;
    
    } catch (std::exception&) {
        // error 
        return {};
    }
}

#endif // __linux__

