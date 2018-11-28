#include "resourcemanager.hpp"
#include <fstream>
#include <iostream>

ResourceManager::ResourceManager()
{
    //ctor
}

ResourceManager::~ResourceManager()
{
    for(auto it: resBlocks)
    {
        if(it.second->count == 0)
            delete it.second;
        else
            it.second->managed = false;
    }
}

bool ResourceManager::registerType(const std::string &name, Constructor constructor, Destructor destructor)
{
    if(types.find(name) != types.end() || !constructor || !destructor)
        return false;
    Type f;
    f.construct = constructor;
    f.destruct = destructor;
    types[name] = f;
    return true;
}

Resource ResourceManager::getResource(const std::string &name, const std::string &type, bool copiable, Constructor overrideConstruct)
{
    auto it = types.find(type);
    if(it == types.end())
        return Resource();

    if(copiable && resBlocks.find(name) != resBlocks.end())
        return Resource(resBlocks[name]);

    void* ptr = nullptr;
    if(overrideConstruct)
        ptr = overrideConstruct(name, *this);
    else
        ptr = it->second.construct(name, *this);
    if(!ptr)
        return Resource();

    ResBlock* block = new ResBlock();
    block->destruct = it->second.destruct;
    block->data = ptr;
    block->count = 0;
    block->loaded = true;
    block->parent = this;
    block->managed = true;
    block->copiable = copiable;
    block->name = name;

    addResource(block->name, (ResBlock*)block);
    return Resource(block);
}

File ResourceManager::getFile(const std::string &path)
{
    if(fileBlocks.find(path) != fileBlocks.end())
        return File(fileBlocks[path]);

    FileBlock* block = new FileBlock();
    block->count = 0;
    block->loaded = true;
    block->parent = this;
    block->managed = true;
    block->name = path;

    block->error = !getFileBinary(path, block->bin, block->size);
    if(block->error)
    {
        delete block;
        return File();
    }
    addFile(path, (FileBlock*)block);
    return File(block);
}

File ResourceManager::getFile(const std::string &pack, const std::string &path)
{
    if(fileBlocks.find(path) != fileBlocks.end())
        return File(fileBlocks[path]);

    FileBlock* block = new FileBlock();
    block->count = 0;
    block->loaded = true;
    block->parent = this;
    block->managed = true;
    block->name = path;

    block->error = !getFileBinary(pack, path, block->bin, block->size);
    if(block->error)
    {
        delete block;
        return File();
    }
    addFile(path, (FileBlock*)block);
    return File(block);
}

void ResourceManager::lock(const std::string &name, bool file)
{
    if(file)
    {
        auto it = fileBlocks.find(name);
        if(it != fileBlocks.end())
            it->second->count++;
    }
    else
    {
        auto it = resBlocks.find(name);
        if(it != resBlocks.end())
            it->second->count++;
    }
}

void ResourceManager::unlock(const std::string &name, bool file)
{
    if(file)
    {
        auto it = fileBlocks.find(name);
        if(it != fileBlocks.end() && it->second->count > 0)
            it->second->count--;
    }
    else
    {
        auto it = resBlocks.find(name);
        if(it != resBlocks.end() && it->second->count > 0)
            it->second->count--;
    }
}

void ResourceManager::garbageCollection()
{
    for(auto it = resTrashbin.begin(); it != resTrashbin.end(); )
    {
        ResBlock* block = *it;
        if(block && block->count == 0)
        {
            std::string id = block->name;
            if(!block->copiable)
                id += "_" + std::to_string((size_t)block);
            auto jt = resBlocks.find(id);
            if(jt != resBlocks.end())
            {
                resBlocks.erase(jt);
                if(block->destruct)
                    block->destruct(block);
                delete block;
            }
        }
        it = resTrashbin.erase(it);
    }
    for(auto it = fileTrashbin.begin(); it != fileTrashbin.end(); )
    {
        FileBlock* block = *it;
        if(block)
        {
            auto jt = fileBlocks.find(block->name);
            if(jt != fileBlocks.end())
            {
                if(block->count == 0)
                {
                    fileBlocks.erase(jt);
                    delete [] block->bin;
                    delete block;
                }
            }
        }
        it = fileTrashbin.erase(it);
    }
}

bool ResourceManager::readPack(const std::string &name)
{
    std::ifstream f(name, std::ios::in | std::ios::binary);
    if(!f)
        return false;
    RPHeader pack;
    size_t tmp, count;

    f.read((char*)&tmp, 4);
    if(tmp != MAGIC_VER)
        return false;
    f.read((char*)&count, 4);

    for(size_t i = 0; i < count; ++i)
    {
        if(!f.good())
            return false;
        RFHeader header;
        header.parent = name;
        f.read((char*)&header.size, 4);
        f.read((char*)&tmp, 4);
        std::string fn;
        if(tmp > 0)
        {
            char c[tmp+1];
            f.read(c, tmp);
            c[tmp] = '\0';
            fn = std::string(c);
        }
        header.start = f.tellg();
        f.seekg(f.tellg() + (std::streampos)header.size);
        pack.files[fn] = header;
    }
    if(!f.good())
        return false;
    packs[name] = pack;
    return true;
}

#ifdef WPACK
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stack>
bool ResourceManager::writePack(const std::string &name, const std::string &folder)
{
    DIR *dir;
    struct dirent *ent;
    std::stack<std::string> folders;
    std::stack<std::string> files;

    std::string basepath = folder;
    if(!folder.empty() && folder.back() != '/')
        basepath += "/";

    folders.push(basepath);

    while(!folders.empty())
    {
        std::string current = folders.top();
        folders.pop();
        if((dir = opendir (current.c_str())) != NULL)
        {
            while((ent = readdir (dir)) != NULL)
            {
                if(std::string(ent->d_name) != "." && std::string(ent->d_name) != "..")
                {
                    struct stat s;
                    std::string fpath = current + ent->d_name;
                    if(stat(fpath.c_str(), &s) == 0)
                    {
                        if(s.st_mode & S_IFDIR)
                            folders.push(fpath + "/");
                        else if(s.st_mode & S_IFREG)
                            files.push(fpath);
                    }
                    else return false;
                }
            }
            closedir (dir);
        }
        else return false;
    }

    std::ofstream fo(name, std::ios::out | std::ios::trunc | std::ios::binary);
    if(!fo)
        return false;

    size_t tmp = MAGIC_VER;
    std::streampos size;
    char c;
    fo.write((char*)&tmp, 4);
    tmp = files.size();
    fo.write((char*)&tmp, 4);

    while(!files.empty())
    {
        std::string fullpath = files.top();
        std::string current = fullpath.substr(basepath.size());
        files.pop();

        std::ifstream fi(fullpath, std::ios::in | std::ios::binary | std::ios::ate);
        if(!fi)
            return false;

        size = fi.tellg();
        fi.seekg(fi.beg);
        fo.write((char*)&size, 4);

        tmp = current.size();
        fo.write((char*)&tmp, 4);
        fo.write(current.c_str(), tmp);

        for(size_t i = 0; i < size; ++i)
        {
            fi.read(&c, 1);
            fo.write(&c, 1);
            if(!fo || !fi)
                return false;
        }
    }
    return true;
}
#endif

void ResourceManager::removePack(const std::string &name)
{
    auto it = packs.find(name);
    if(it != packs.end())
        packs.erase(it);
}

bool ResourceManager::patchPack(const std::string &base, const std::string &patch)
{
    garbageCollection();

    auto it = packs.find(base);
    if(it == packs.end())
        return false;
    auto jt = packs.find(patch);
    if(jt == packs.end())
        return false;

    for(auto kt: jt->second.files)
        it->second.files[kt.first] = kt.second;

    return true;
}

std::vector<std::string> ResourceManager::searchInPacks(const std::string &name)
{
    std::vector<std::string> results;
    for(auto it: packs)
        if(it.second.files.find(name) != it.second.files.end())
            results.push_back(it.first);
    return results;
}

RFHeader ResourceManager::getFileHeader(const std::string &pack, const std::string &name)
{
    if(packs.find(pack) != packs.end())
    {
        auto it = packs[pack].files.find(name);
        if(it == packs[pack].files.end())
        {
            RFHeader rf;
            rf.start = -1;
            return rf;
        }
        return it->second;
    }
    else
    {
        RFHeader rf;
        rf.start = -1;
        return rf;
    }
}

bool ResourceManager::getFileBinary(const std::string &pack, const std::string &path, char* &bin, size_t &size)
{
    if(packs[pack].files.find(path) != packs[pack].files.end())
    {
        RFHeader &ref = packs[pack].files[path];
        std::ifstream f(ref.parent, std::ios::in | std::ios::binary);
        if(!f)
            return false;
        size = ref.size;
        f.seekg(ref.start);
        if(size > 0)
        {
            bin = new char[size];
            f.read(bin, size);
        }
        return true;
    }
    return false;
}

bool ResourceManager::getFileBinary(const std::string &path, char* &bin, size_t &size)
{
    std::ifstream f(path, std::ios::in | std::ios::binary | std::ios::ate);
    if(!f)
        return false;
    std::streampos pos = f.tellg();
    if(pos < 0)
        return false;
    size = (size_t)pos;
    if(size > 0)
    {
        bin = new char[size];
        f.seekg(f.beg);
        f.read(bin, size);
    }
    return true;
}

void ResourceManager::addResource(const std::string &name, ResBlock* block)
{
    if(block)
    {
        std::string id = name;
        if(!block->copiable)
            id += "_" + std::to_string((size_t)block);
        auto it = resBlocks.find(id);
        if(it != resBlocks.end())
        {
            if(it->second->count == 0)
                delete it->second;
            else
                it->second->managed = false;
        }
        resBlocks[id] = block;
    }
}

void ResourceManager::addFile(const std::string &name, FileBlock* block)
{
    if(block)
    {
        auto it = fileBlocks.find(name);
        if(it != fileBlocks.end())
        {
            if(it->second->count == 0)
                delete it->second;
            else
                it->second->managed = false;
        }
        fileBlocks[name] = block;
    }
}

void ResourceManager::trashResource(ResBlock* block)
{
    resTrashbin.insert(block);
}

void ResourceManager::trashFile(FileBlock* block)
{
    fileTrashbin.insert(block);
}
