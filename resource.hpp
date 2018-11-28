#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include <fstream>
#include <sstream>

class Resource;
class File;
#include "resourcemanager.hpp"

// various data structures and classes
// base ones SHOULDN'T be instanciated

struct BaseBlock
{
    size_t count; // usage counter
    bool loaded; // true if it's valid
    bool managed; // true if ResourceManager is alive and valid
    ResourceManager* parent; // the block is registered to this ResourceManager
    std::string name; // block id
};

class BaseRes
{
    public:
        virtual ~BaseRes();
        virtual bool loaded(); // check the block loaded flag
        virtual size_t count(); // return the block counter
        virtual bool isManaged(); // return true if it's linked to a ResourceManager instance
    protected:
        BaseRes();
        BaseBlock *block;
};


// resource class
struct ResBlock: public BaseBlock
{
    Destructor destruct; // destructor callback (constructor isn't saved because we already called it at this point in time)
    void* data; // pointer to the data
    bool copiable;
};

class Resource: public BaseRes
{
    public:
        Resource();
        Resource(const std::string &name, void* ptr, Destructor destruct, ResourceManager& parent, bool copiable);
        Resource(ResBlock* b);
        Resource(const Resource &res);
        Resource operator = (const Resource& res);
        ~Resource();
        void* get(); // return data
        template <class T> T* getData() // return data but casted (to simplify the syntax). be careful with this thing
        {
            return (T*)(((ResBlock*)block)->data);
        }
};

// file class
struct FileBlock: public BaseBlock
{
    char* bin; // binary file content (a char array)
    size_t size; // size of the array
    bool error; // true if the loading failed
};

class File: public BaseRes
{
    public:
        File();
        File(const std::string &name, ResourceManager& parent);
        File(const std::string &pack, const std::string &name, ResourceManager& parent);
        File(FileBlock* f);
        File(const File &fil);
        File operator = (const File& fil);
        ~File();
        char* get(); // return the array
        size_t size(); // return the size
        bool error(); // return the error flag
};

// to do: streaming class ?
class FileStream
{
    public:
        FileStream();
        virtual ~FileStream();
        virtual bool open(const std::string &name);
        virtual bool open(const std::string &pack, const std::string &name, ResourceManager &rm);
        virtual void close();
        virtual std::streamsize read(char* buf, const size_t &n);
        virtual void seek(std::streampos p);
        virtual std::streampos tell();
        virtual size_t size();
    protected:
        std::ifstream f;
        std::streampos off;
        std::streampos pos;
        size_t s;
};

#endif // RESOURCE_HPP
