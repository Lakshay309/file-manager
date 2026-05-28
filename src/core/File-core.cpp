#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include<vector>
#include<unordered_set>

using namespace std;
using namespace std::filesystem;

//  this is the default path that fileManager will show when launched
const std::filesystem::path DEFAULT_MAIN_PATH ="";
const string FAV_FILE="";

//  File struct give the structure that will be give to frontend in the form of tile 
struct File {
    std::string           name;
    std::string           extension;
    std::string           type;
    std::uint64_t         size;
    std::filesystem::path path;
    bool                  isDir;
    bool                  isFav;
};

//  properties of a dir 
struct Properties {
    std::filesystem::path           parentPath;
    std::filesystem::path           path;
    std::string                     name;
    std::filesystem::file_time_type lastModified;
};

//  enum for the clipboard cut or copy
enum class ClipboardAction {
    None,
    Copy,
    Cut
};

//  struct that will help in copy and cut a file 
struct Clipboard {
    std::filesystem::path path;
    ClipboardAction action = ClipboardAction::None;
};


//  this is the main class that is responsible for the core functionality of the file manager
class FileSystem {
    
    private:
        std::filesystem::path mainFilePath;
        // we store the isFav inmemory 
        std::unordered_set<std::filesystem::path> isFav;

        Clipboard clipboard;
        // give user ability to check if a file is fav or not
        bool isFileFav(std::filesystem::path path){
            return isFav.count(path);
        }

        // TODO we are going to load the isFav from file
        bool loadFav(){
            // loads the fav file
            isFav.clear();
            if(!std::filesystem::exists(FAV_FILE)){
                std::ofstream createFile(FAV_FILE);
                if(!createFile.is_open()){
                    return false;
                }
                return true;
            }
            std::ifstream file(FAV_FILE);
            if(!file.is_open()){
                return false;
            }
            std::string line;
            while(std::getline(file,line)){
                if(line.empty()){
                    continue;
                }
                isFav.insert(std::filesystem::path(line));
            }
            return true;
        }
        
        // TODO can improve fav file saving 
        bool saveFav(std::filesystem::path path){
            try{
                if(!std::filesystem::exists(path)){
                    return false;
                }
                isFav.insert(path);

                std::ofstream file(FAV_FILE);
                if(!file.is_open()){
                    return false;
                }
                for (const auto& fav : isFav)
                {
                    file << fav.string() << '\n';
                }

                return true;
            }
            catch (const std::exception&)
            {
                return false;
            }
        }

        bool removeFav(const std::filesystem::path& path){
            try
            {
                auto it = isFav.find(path);

                if (it == isFav.end())
                {
                    return false;
                }

                isFav.erase(it);

                std::ofstream file(FAV_FILE);

                if (!file.is_open())
                {
                    return false;
                }

                for (const auto& fav : isFav)
                {
                    file << fav.string() << '\n';
                }

                return true;
            }
            catch (const std::exception&)
            {
                return false;
            }
        }

        
    public:
        
        // Constructor load the mainFilePath and loadFav
        FileSystem(){
            this->mainFilePath=DEFAULT_MAIN_PATH;
            // while(!loadFav());
            loadFav();
        }
    
        //  give user the current file path
        std::filesystem::path getCurrentFilePath(){
            return mainFilePath;
        }

        //  give user all the files and directory inside the current directory
        std::vector<File> getAllFileInCurrentDir(){
            std::vector<File> allFiles;
            try {
                for(const auto& entry : std::filesystem::directory_iterator(this->mainFilePath)){
                    
                    allFiles.emplace_back();
                    File& file = allFiles.back();
    
                    file.name = entry.path().filename().string();
                    file.path = entry.path();
                    file.isDir = entry.is_directory();
                    if(!file.isDir){
                        file.extension = entry.path().extension().string();
                        file.type = typeOfFile(entry);
                        file.size = entry.file_size();
                    }
                    else{
                        file.extension = "";  // file is a dir
                        file.type = typeOfFile(entry);
                        file.size = 0;
                    }
                    
                    file.isFav = isFileFav(file.path);
                    
                }
                return allFiles;
            
            } catch (const std::exception& e) {
                return allFiles;
            }
        }
        
        // toggle the fav 
        bool ToggleFav(std::filesystem::path path){
            // directory cannot be fav
            if(std::filesystem::is_directory(path)) return false;
            
            // remove fav
            if(isFileFav(path)){
                return removeFav(path);
            }else{ // add fav
                return saveFav(path);
            }

        }

        // give the type of file in user understanding format
        std::string typeOfFile(const std::filesystem::directory_entry& entry){
            auto type = entry.symlink_status().type();

            switch(type){
                case std::filesystem::file_type::regular:
                    return "File";

                case std::filesystem::file_type::directory:
                    return "Directory";
                
                    case std::filesystem::file_type::symlink:
                    return "Symlink";
                
                    default:
                    return "Unknown";
            }

            return "Unknown";
        }

        // updates the mainPath
        void updatePath(std::filesystem::path newPath){
            this->mainFilePath = newPath;
        }

        // create new folder
        bool createDirectory(std::string name){
            return std::filesystem::create_directories(mainFilePath/name);
        }

        // get properties
        Properties getProperties(std::filesystem::path path){
            return Properties{
                .parentPath= path.has_parent_path()?path.parent_path():path,
                .path=path,
                .name=path.filename().string(),
                .lastModified=std::filesystem::last_write_time(path),
            };
        }

        // rename a file or dir
        bool renamePath(std::string oldname, std::string newName){
            try{
                std::filesystem::rename(mainFilePath/oldname,mainFilePath/newName);
                return true;
            }catch(const std::filesystem::filesystem_error&){
                return false;
            }
        }
        // delete file or dir 
        bool deletePath(std::string name){
            try{
                if(std::filesystem::is_directory(mainFilePath/name)){
                    std::filesystem::remove_all(mainFilePath/name);
                }else{
                    std::filesystem::remove(mainFilePath/name);
                }
                return true;
            }catch(const std::filesystem::filesystem_error&){
                return false;
            }
        }

        // copy
        void copy(const string name){
            clipboard.path = mainFilePath/name;
            clipboard.action = ClipboardAction::Copy;
        }
        // cut 
        void cut(const string name){
            clipboard.path = mainFilePath/name;
            clipboard.action = ClipboardAction::Cut;
        }
        // paste
        bool paste(){
            if(clipboard.action == ClipboardAction::None){
                return false;
            }
            auto target = mainFilePath/clipboard.path.filename();
            //  either there is same file in the folder or we are not changing the place 
            try{
                if(
                    std::filesystem::exists(target) || (std::filesystem::exists(clipboard.path) &&std::filesystem::equivalent(clipboard.path, target))
                    ){
                    return false;
                }
                if(clipboard.action == ClipboardAction::Copy){
                    if(std::filesystem::is_directory(clipboard.path)){
                        std::filesystem::copy(
                            clipboard.path,
                            target,std::filesystem::copy_options::recursive
                        );
                    }else{
                        std::filesystem::copy_file(
                            clipboard.path,
                            target
                        );
                    }
                }else if(clipboard.action == ClipboardAction::Cut){
                    std::filesystem::rename(
                        clipboard.path,
                        target
                    );
                    clipboard.action = ClipboardAction::None;
                    clipboard.path.clear();
                }
                return true;

            }catch(const std::exception& e){
                return false;
            }
        }

        // TODO

        // open terminal 

        // open with option

        // ?compress
};
