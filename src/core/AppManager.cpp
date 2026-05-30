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
    for(const auto& app : cachedApps_){
        fprintf(stderr, "APP: [%s] exec=[%s] path=[%s]\n",
            app.name.c_str(),
            app.executable.c_str(),
            app.execPath.string().c_str());
    }
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

        const char* home = getenv("HOME");
    std::filesystem::path homeDir = home ? home : "";
 
    const std::vector<std::filesystem::path> desktopDirs = {
        "/usr/local/share/applications",
        homeDir / ".local/share/applications",
        "/var/lib/flatpak/exports/share/applications", 
        homeDir / ".local/share/flatpak/exports/share/applications",
        "/var/lib/snapd/desktop/applications",      
        homeDir / "snap",     
        "/var/lib/snapd/desktop/applications/",
        "/snap",                               
        "/usr/share/applications",
    };


    for(const auto& dir:desktopDirs){
        if(std::filesystem::exists(dir) && std::filesystem::is_directory(dir)){
            scanDesktopFiles(dir);
        }
    }

    // Remove duplicates by executable name
    // Sort by executable so std::unique can find all duplicates
std::sort(cachedApps_.begin(), cachedApps_.end(), [](const AppInfo& a, const AppInfo& b){
    return a.executable < b.executable;
});
cachedApps_.erase(
    std::unique(cachedApps_.begin(), cachedApps_.end(), [](const AppInfo& a, const AppInfo& b){
        return a.executable == b.executable;
    }),
    cachedApps_.end()
);
// Re-sort by name for display
std::sort(cachedApps_.begin(), cachedApps_.end(), [](const AppInfo& a, const AppInfo& b){
    return a.name < b.name;
});

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
        for (const auto& entry : std::filesystem::recursive_directory_iterator(
                dir, std::filesystem::directory_options::skip_permission_denied)) {
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
            
            while(ss>>token){
                if(token[0]=='%') continue; //place holder ignored
                if(first){
                    exec = token; // First token is executable
                    first = false;
                }
            }
            app.executable = std::filesystem::path(exec).filename().string();

            app.execPath = resolveExecutable(exec);
            if(app.execPath.empty()){
                app.execPath = resolveSnapExecutable(exec);
            }
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
        auto snapWrapper = std::filesystem::path("/snap/bin") / p.filename();
        if(std::filesystem::exists(snapWrapper)){
            return snapWrapper;
        }
        return std::filesystem::exists(p) ? p : std::filesystem::path{};
    }
    
    auto snapCandidate = std::filesystem::path("/snap/bin") / exec;
    if(std::filesystem::exists(snapCandidate)){
        return snapCandidate;
    }

    // Search PATH environment variable
    const char* pathEnv = getenv("PATH");
    if (!pathEnv) return {};

    std::istringstream ss(pathEnv);
    std::string dir;

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


std::filesystem::path AppManager::resolveSnapExecutable(const std::string& exec) {
    if(exec.empty()) return {};
 
    // 1. Snap wrapper scripts live in /snap/bin — fastest check
    auto snapBinPath = std::filesystem::path("/snap/bin") / exec;
    if(std::filesystem::exists(snapBinPath)){
        return snapBinPath;
    }
 
    // 2. Walk each installed snap package and check its inner binary tree
    std::filesystem::path snapRoot("/snap");
    if(!std::filesystem::exists(snapRoot) || !std::filesystem::is_directory(snapRoot)){
        return {};
    }
 
    try {
        for(const auto& snapEntry : std::filesystem::directory_iterator(snapRoot)){
            if(!snapEntry.is_directory()) continue;
 
            std::string snapName = snapEntry.path().filename().string();
 
            // Skip meta-directories used by snapd itself
            if(snapName == "bin" || snapName == "core" || snapName == "core18" ||
               snapName == "core20" || snapName == "core22" || snapName == "snapd"){
                continue;
            }
 
            // /snap/<name>/current is a symlink to the active revision
            auto currentDir = snapEntry.path() / "current";
            if(!std::filesystem::exists(currentDir)) continue;
 
            // Common binary locations inside a snap
            const std::vector<std::filesystem::path> searchPaths = {
                currentDir / "usr" / "bin",
                currentDir / "bin",
                currentDir / "usr" / "local" / "bin",
            };
 
            for(const auto& searchDir : searchPaths){
                auto candidate = searchDir / exec;
                if(std::filesystem::exists(candidate)){
                    return candidate;
                }
            }
        }
    } catch(const std::exception&) {
        // /snap may not be readable without elevated permissions — ignore
    }
 
    return {};
}


#endif // __linux__

