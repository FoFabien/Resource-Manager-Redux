#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <map>
#include <set>
#include <string>
#include <memory>
#include <functional>
#include <vector>

// for include issues with resource.hpp
class ResourceManager;
class ResBlock;
class FileBlock;

// callback used for resources
typedef std::function<void*(const std::string &name, ResourceManager &rm)> Constructor; // must return a pointer to your data/resource/whatever (return null if error)
typedef std::function<void(ResBlock *block)> Destructor; // must delete your data
struct Type // a type just keep track of the constructor and destructor
{
    Constructor construct = nullptr;
    Destructor destruct = nullptr;
};

#include "resource.hpp"

// file pack data structures
#define MAGIC_VER 0x31303252 //"R201", first to be checked
#define WPACK // define to compile the writePack function
struct RFHeader // file header
{
    std::streampos start = 0;
    size_t size = 0;
    std::string parent;
};

struct RPHeader // pack header
{
    std::map<std::string, RFHeader> files; // just contain all file headers
};

// resource manager
class ResourceManager
{
    public:
        ResourceManager();
        ~ResourceManager(); // if resources are still allocated somewhere, they won't be deleted (it's recommended to delete the manager once all resources are free)

        // resource and file creation
        bool registerType(const std::string &name, Constructor constructor, Destructor destructor); // register a resource type with specific callbacks
        Resource getResource(const std::string &name, const std::string &type, bool copiable, Constructor overrideConstruct = nullptr); // get the specific resource. constructor can be overrided. if copiable is false, a new instance is created in memory (the resource object is still copiable but this function won't return this specific instance ever again)
        File getFile(const std::string &path); // return a file (check the error flag for errors)
        File getFile(const std::string &pack, const std::string &path); // same but from a file pack

        // resource and file management
        void lock(const std::string &name, bool file = false); // increase the counter of the resource/file by 1. useful to keep it loaded
        void unlock(const std::string &name, bool file = false); // but don't forget to call this one if you used lock, once you are done
        void garbageCollection(); // will delete unused instances. to be called once in a while

        // file pack management
        bool readPack(const std::string &name); // load a pack in memory
        #ifdef WPACK
        static bool writePack(const std::string &name, const std::string &folder); // create a pack on the disk (Windows only)
        #endif
        void removePack(const std::string &name); // remove a pack from the memory
        bool patchPack(const std::string &base, const std::string &patch); // content of patch will be added to/overwrite base
        std::vector<std::string> searchInPacks(const std::string &name); // search packs containing this file name
        RFHeader getFileHeader(const std::string &pack, const std::string &name);

        // inner workings (keeping it public for new child classes)
        bool getFileBinary(const std::string &pack, const std::string &path, char* &bin, size_t &size); // load the binary data of a file from a pack
        bool getFileBinary(const std::string &path, char* &bin, size_t &size); // load the binary data of a file from the disk
        void addResource(const std::string &name, ResBlock* block); // register the new Resource Block
        void addFile(const std::string &name, FileBlock* block); // register the new File Block
        void trashResource(ResBlock* block); // add to the trashbin
        void trashFile(FileBlock* block); // add to the trashbin

    protected:
        // data
        std::map<std::string, RPHeader> packs; // loaded packs
        std::map<std::string, Type> types; // registered types
        std::map<std::string, ResBlock*> resBlocks; // registered resource blocks
        std::map<std::string, FileBlock*> fileBlocks; // registered file blocks
        std::set<ResBlock*> resTrashbin; // resource block trashbin (will be processed by garbageCollection())
        std::set<FileBlock*> fileTrashbin; // resource file trashbin (will be processed by garbageCollection())
};

#endif // RESOURCE MANAGER_HPP
