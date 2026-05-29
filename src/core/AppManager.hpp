#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct AppInfo {
    std::string              name;
    std::string              executable;
    std::filesystem::path    execPath;
    std::string              icon;
    std::vector<std::string> mineTypes;
};

class AppManager{
    public:
        AppManager();

            // Returns all installed apps found on the system
            std::vector<AppInfo> getAllApps();

            // Return only apps that can open the given file (filtered by MINE Type)
            std::vector<AppInfo> getAppForFile(const std::filesystem::path& filepath);

            // Refresh the internal app list
            void refresh();
    
    private:
        std::vector<AppInfo> cachedApps_; // Cached so we don't re-scan every call

        bool loaded_ = false;

        void loadApps();

        #if defined (__linux__)
            //  linux apps are described by .desktop files in well-known directories
            void scanDesktopFiles(const std::filesystem::path& dirpath);

            AppInfo parseDesktopFile(const std::filesystem::path& path);

            std::filesystem::path resolveExecutable(const std::string& exec);

            std::string getMimeTypeForFile(const std::filesystem::path& filePath);
        #endif

        #if defined(_WIN32) || defined(_WIN64)
            // TODO: Windows — scan registry / Start Menu
            // void scanRegistry();
        #endif
        
        #if defined(__APPLE__)
            // TODO: macOS — scan /Applications, parse Info.plist
            // void scanApplicationsFolder();
        #endif
};