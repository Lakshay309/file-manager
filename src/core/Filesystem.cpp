#include "Filesystem.hpp"
#include <unistd.h>


//  Constructor & Private Helpers


FileSystem::FileSystem() {
    this->mainFilePath = DEFAULT_MAIN_PATH;
    loadFav();
}

// Returns true if the given path is marked as favourite
bool FileSystem::isFileFav(std::filesystem::path path) {
    return isFav.count(path);
}

// Loads saved favourites from the FAV_FILE on disk into memory
bool FileSystem::loadFav() {
    isFav.clear();

    // If the favourites file doesn't exist, create it
    if (!std::filesystem::exists(FAV_FILE)) {
        std::ofstream createFile(FAV_FILE);
        if (!createFile.is_open()) {
            return false;
        }
        return true;
    }

    std::ifstream file(FAV_FILE);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        isFav.insert(std::filesystem::path(line));
    }

    return true;
}

// Adds a path to favourites and saves the updated list to disk
bool FileSystem::saveFav(std::filesystem::path path) {
    try {
        if (!std::filesystem::exists(path)) return false;

        isFav.insert(path);

        std::ofstream file(FAV_FILE);
        if (!file.is_open()) return false;

        for (const auto& fav : isFav) {
            file << fav.string() << '\n';
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// Removes a path from favourites and saves the updated list to disk
bool FileSystem::removeFav(const std::filesystem::path& path) {
    try {
        auto it = isFav.find(path);
        if (it == isFav.end()) return false;

        isFav.erase(it);

        std::ofstream file(FAV_FILE);
        if (!file.is_open()) return false;

        for (const auto& fav : isFav) {
            file << fav.string() << '\n';
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}


//  Navigation


// Returns the directory the file manager is currently showing
std::filesystem::path FileSystem::getCurrentFilePath() {
    return mainFilePath;
}

// Changes the current directory to the given path
void FileSystem::updatePath(std::filesystem::path newPath) {
    this->mainFilePath = newPath;
}

// Goes up one level to the parent directory
void FileSystem::toParent() {
    if (!mainFilePath.has_parent_path()) return;
    mainFilePath = mainFilePath.parent_path();
}


//  File Listing


// Returns all files and folders inside the current directory
std::vector<File> FileSystem::getAllFileInCurrentDir() {
    std::vector<File> allFiles;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(this->mainFilePath)) {

            allFiles.emplace_back();
            File& file = allFiles.back();

            file.name  = entry.path().filename().string();
            file.path  = entry.path();
            file.isDir = entry.is_directory();

            if (!file.isDir) {
                file.extension = entry.path().extension().string();
                file.type      = typeOfFile(entry);
                file.size      = entry.file_size();
            } else {
                file.extension = "";
                file.type      = typeOfFile(entry);
                file.size      = 0;
            }

            file.isFav = isFileFav(file.path);
        }
        return allFiles;

    } catch (const std::exception&) {
        return allFiles;  // Return whatever was collected before the error
    }
}

// Returns a human-readable string for the type of file/directory/symlink
std::string FileSystem::typeOfFile(const std::filesystem::directory_entry& entry) {
    auto type = entry.symlink_status().type();

    switch (type) {
        case std::filesystem::file_type::regular:   return "File";
        case std::filesystem::file_type::directory: return "Directory";
        case std::filesystem::file_type::symlink:   return "Symlink";
        default:                                    return "Unknown";
    }
}


//  File Operations


// Creates a new folder with the given name inside the current directory
bool FileSystem::createDirectory(std::string name) {
    return std::filesystem::create_directories(mainFilePath / name);
}

// Renames a file or folder inside the current directory
bool FileSystem::renamePath(std::string oldname, std::string newName) {
    try {
        std::filesystem::rename(mainFilePath / oldname, mainFilePath / newName);
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

// Deletes a file or folder (including its contents if it's a directory)
bool FileSystem::deletePath(std::string name) {
    try {
        if (std::filesystem::is_directory(mainFilePath / name)) {
            std::filesystem::remove_all(mainFilePath / name);
        } else {
            std::filesystem::remove(mainFilePath / name);
        }
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}


//  Clipboard (Copy / Cut / Paste)

// Marks a file for copying
void FileSystem::copy(const std::string name) {
    clipboard.path   = mainFilePath / name;
    clipboard.action = ClipboardAction::Copy;
}

// Marks a file for cutting (moving)
void FileSystem::cut(const std::string name) {
    clipboard.path   = mainFilePath / name;
    clipboard.action = ClipboardAction::Cut;
}

// Pastes the clipboard file into the current directory
bool FileSystem::paste() {
    if (clipboard.action == ClipboardAction::None) 
        return false;

    auto target = mainFilePath / clipboard.path.filename();

    try {
        // Don't paste if destination already exists or source == destination
        // if (std::filesystem::exists(target) ||
        //     (std::filesystem::exists(clipboard.path) &&
        //     std::filesystem::equivalent(clipboard.path, target))) {
        //     return false;
        // }
        if (std::filesystem::exists(target)) return false;
        if (clipboard.path == target)        return false;


        if (clipboard.action == ClipboardAction::Copy) {
            if (std::filesystem::is_directory(clipboard.path)) {
                std::filesystem::copy(
                    clipboard.path, target,
                    std::filesystem::copy_options::recursive
                );
            } else {
                std::filesystem::copy_file(clipboard.path, target);
            }

        } else if (clipboard.action == ClipboardAction::Cut) {
            std::filesystem::rename(clipboard.path, target);
            clipboard.action = ClipboardAction::None;
            clipboard.path.clear();
        }

        return true;

    } catch (const std::exception&) {
        return false;
    }
}


//  Favourites


// Toggles the favourite status of a file (directories cannot be favourited)
bool FileSystem::ToggleFav(std::filesystem::path path) {
    if (std::filesystem::is_directory(path)) return false;

    if (isFileFav(path)) {
        return removeFav(path);
    } else {
        return saveFav(path);
    }
}


//  Properties


// Returns metadata for a given file or folder path
Properties FileSystem::getProperties(std::filesystem::path path) {
    return Properties{
        .parentPath  = path.has_parent_path() ? path.parent_path() : path,
        .path        = path,
        .name        = path.filename().string(),
        .lastModified = std::filesystem::last_write_time(path),
    };
}



// ─────────────────────────────────────────────
//  Open With
// ─────────────────────────────────────────────

// Opens a file using the specified application.
// Each platform has its own block — add Windows and macOS implementations below when ready.
bool FileSystem::openWith(const std::filesystem::path& app, const std::filesystem::path& file) {

    // Basic sanity checks — same for all platforms
    if (!std::filesystem::exists(app)) {
        return false;  // App doesn't exist
    }
    if (!std::filesystem::exists(file)) {
        return false;  // File doesn't exist
    }

    #if defined(__linux__)

    pid_t pid =fork();
    if(pid<0){
        return false;
    }
    if(pid == 0){
        execl(app.c_str(), app.c_str(), file.c_str(),nullptr);
        _exit(1);
    }

    return true;

    #elif defined(_WIN32) || defined(_WIN64) 

    // ── Windows ──────────────────────────────────────────────────────────────
    // TODO: Use ShellExecuteW or CreateProcess
    // Example skeleton:
    //
    // HINSTANCE result = ShellExecuteW(
    //     NULL,                        // parent window handle
    //     L"open",                     // action
    //     app.wstring().c_str(),       // application path
    //     file.wstring().c_str(),      // file passed as argument
    //     NULL,                        // working directory (NULL = current)
    //     SW_SHOWNORMAL                // how to show the window
    // );
    // return ((INT_PTR)result > 32);   // ShellExecute returns > 32 on success
    return false;  // Remove this line once implemented

    #elif defined(__APPLE__)
    // ── macOS ────────────────────────────────────────────────────────────────
    // TODO: Use the 'open' command with -a flag
    // Example skeleton:
    //
    // std::string command = "open -a \"" + app.string() + "\" \"" + file.string() + "\"";
    // int result = std::system(command.c_str());
    // return (result == 0);
    return false;  // Remove this line once implemented

    #else
    // ── Unsupported Platform ─────────────────────────────────────────────────
    return false;
    #endif
}


#if defined (__linux__)

#elif defined

#endif
