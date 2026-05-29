#pragma once

#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unistd.h>
#include <sys/types.h>

// Default path shown when file manager launches (set this to your desired path)
const std::filesystem::path DEFAULT_MAIN_PATH = "";
const std::string FAV_FILE = "";

// Represents a single file or folder shown as a tile in the UI
struct File {
    std::string           name;
    std::string           extension;
    std::string           type;
    std::uint64_t         size;
    std::filesystem::path path;
    bool                  isDir;
    bool                  isFav;
};

// Holds metadata/properties of a file or directory
struct Properties {
    std::filesystem::path           parentPath;
    std::filesystem::path           path;
    std::string                     name;
    std::filesystem::file_time_type lastModified;
};

// Tracks whether clipboard holds a copy or cut action
enum class ClipboardAction {
    None,
    Copy,
    Cut
};

// Holds the file path and action (copy/cut) for clipboard operations
struct Clipboard {
    std::filesystem::path path;
    ClipboardAction action = ClipboardAction::None;
};


// Core class handling all file manager operations
class FileSystem {

public:
    FileSystem();

    // Navigation
    std::filesystem::path getCurrentFilePath();
    void updatePath(std::filesystem::path newPath);
    void toParent();

    // File listing
    std::vector<File> getAllFileInCurrentDir();
    std::string typeOfFile(const std::filesystem::directory_entry& entry);

    // File operations
    bool createDirectory(std::string name);
    bool renamePath(std::string oldname, std::string newName);
    bool deletePath(std::string name);

    // Clipboard
    void copy(const std::string name);
    void cut(const std::string name);
    bool paste();

    // Favourites
    bool ToggleFav(std::filesystem::path path);

    // Properties
    Properties getProperties(std::filesystem::path path);

    // Open with external app (platform-specific, not yet implemented)
    bool openWith(const std::filesystem::path& app, const std::filesystem::path& file);

private:
    std::filesystem::path mainFilePath;
    std::unordered_set<std::filesystem::path> isFav;
    Clipboard clipboard;

    bool isFileFav(std::filesystem::path path);
    bool loadFav();
    bool saveFav(std::filesystem::path path);
    bool removeFav(const std::filesystem::path& path);
};