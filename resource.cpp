#include "resource.hpp"

BaseRes::BaseRes()
{

}

BaseRes::~BaseRes()
{

}

bool BaseRes::loaded()
{
    return block->loaded;
}

size_t BaseRes::count()
{
    return block->count;
}

bool BaseRes::isManaged()
{
    return block->managed;
}

/////////////////////////////////////////////////////////////////////////

Resource::Resource()
{
    block = new ResBlock();
    ((ResBlock*)block)->destruct = nullptr;
    ((ResBlock*)block)->data = nullptr;
    block->count = 1;
    block->loaded = false;
    block->parent = nullptr;
    block->managed = true;
    ((ResBlock*)block)->copiable = false;
}

Resource::Resource(const std::string &name, void* ptr, Destructor destruct, ResourceManager& parent, bool copiable)
{
    block = new ResBlock();
    ((ResBlock*)block)->destruct = destruct;
    ((ResBlock*)block)->data = ptr;
    block->count = 1;
    block->loaded = true;
    block->parent = &parent;
    block->managed = true;
    ((ResBlock*)block)->copiable = copiable;
    block->name = name;

    parent.addResource(block->name, (ResBlock*)block);
}

Resource::Resource(ResBlock* b)
{
    if(b)
    {
        block = b;
        block->count++;
    }
    else
    {
        block = new ResBlock();
        ((ResBlock*)block)->destruct = nullptr;
        ((ResBlock*)block)->data = nullptr;
        block->count = 1;
        block->loaded = false;
        block->parent = nullptr;
        block->managed = true;
        ((ResBlock*)block)->copiable = false;
    }
}

Resource::Resource(const Resource &res)
{
    if(res.block->loaded)
    {
        block = res.block;
        block->count++;
    }
    else
    {
        block = new ResBlock();
        ((ResBlock*)block)->destruct = nullptr;
        ((ResBlock*)block)->data = nullptr;
        block->count = 1;
        block->loaded = false;
        block->parent = nullptr;
        block->managed = true;
        ((ResBlock*)block)->copiable = false;
    }
}

Resource Resource::operator = (const Resource& res)
{
    return Resource(res);
}

Resource::~Resource()
{
    if(block->loaded)
    {
        block->count--;
        if(block->count == 0)
        {
            if(!block->managed)
            {
                if(((ResBlock*)block)->destruct)
                    ((ResBlock*)block)->destruct((ResBlock*)block);
                delete ((ResBlock*)block);
            }
            else if(block->count == 0)
                block->parent->trashResource((ResBlock*)block);
        }
    }
    else delete ((ResBlock*)block);
}

void* Resource::get()
{
    return ((ResBlock*)block)->data;
}

/////////////////////////////////////////////////////////////////////////

File::File()
{
    block = new FileBlock();
    ((FileBlock*)block)->bin = nullptr;
    ((FileBlock*)block)->size = 0;
    ((FileBlock*)block)->error = true;
    block->count = 1;
    block->loaded = false;
    block->parent = nullptr;
    block->managed = true;
}

File::File(const std::string &name, ResourceManager& parent)
{
    block = new FileBlock();
    block->count = 1;
    block->loaded = true;
    block->parent = &parent;
    block->managed = true;
    block->name = name;

    ((FileBlock*)block)->error = !(parent.getFileBinary(name, ((FileBlock*)block)->bin, ((FileBlock*)block)->size));

    parent.addFile(name, (FileBlock*)block);
}

File::File(const std::string &pack, const std::string &name, ResourceManager& parent)
{
    block = new FileBlock();
    block->count = 1;
    block->loaded = true;
    block->parent = &parent;
    block->managed = true;
    block->name = name;

    ((FileBlock*)block)->error = !(parent.getFileBinary(pack, name, ((FileBlock*)block)->bin, ((FileBlock*)block)->size));

    parent.addFile(name, (FileBlock*)block);
}

File::File(FileBlock* f)
{
    if(f)
    {
        block = f;
        block->count++;
    }
    else
    {
        block = new FileBlock();
        ((FileBlock*)block)->bin = nullptr;
        ((FileBlock*)block)->size = 0;
        ((FileBlock*)block)->error = true;
        block->count = 1;
        block->loaded = false;
        block->parent = nullptr;
        block->managed = true;
    }
}

File::File(const File &fil)
{
    if(fil.block->loaded)
    {
        block = fil.block;
        block->count++;
    }
    else
    {
        block = new FileBlock();
        ((FileBlock*)block)->bin = nullptr;
        ((FileBlock*)block)->size = 0;
        ((FileBlock*)block)->error = true;
        block->count = 1;
        block->loaded = false;
        block->parent = nullptr;
        block->managed = true;
    }
}

File File::operator = (const File& fil)
{
    return File(fil);
}

File::~File()
{
    if(block->loaded)
    {
        block->count--;
        if(block->count == 0)
        {
            if(!block->managed)
            {
                delete [] ((FileBlock*)block)->bin;
                delete ((FileBlock*)block);
            }
            else if(block->count == 0)
                block->parent->trashFile((FileBlock*)block);
        }
    }
    else delete ((ResBlock*)block);
}

char* File::get()
{
    return ((FileBlock*)block)->bin;
}

size_t File::size()
{
    return ((FileBlock*)block)->size;
}

bool File::error()
{
    return ((FileBlock*)block)->error;
}

///////////////////////////////////////////////////////////////
FileStream::FileStream()
{

}

FileStream::~FileStream()
{

}

bool FileStream::open(const std::string &name)
{
    f.open(name, std::ios::in | std::ios::binary | std::ios::ate);
    if(!f)
        return false;
    s = f.tellg();
    f.seekg(f.beg);
    pos = 0;
    off = 0;
    return true;
}

bool FileStream::open(const std::string &pack, const std::string &name, ResourceManager &rm)
{
    RFHeader rf = rm.getFileHeader(pack, name);
    if(rf.start == -1)
        return false;

    f.open(rf.parent, std::ios::in | std::ios::binary);
    if(!f)
        return false;
    s = rf.size;
    f.seekg(rf.start);
    pos = 0;
    off = rf.start;
    return true;
}

void FileStream::close()
{
    f.close();
}

std::streamsize FileStream::read(char* buf, const size_t &n)
{
    if(!f)
        return -1;
    size_t left = s - pos;
    if(left < n)
    {
        f.read(buf, left);
        return left;
    }
    else
    {
        f.read(buf, n);
        return n;
    }
}

void FileStream::seek(std::streampos p)
{
    if(f && p >= 0 && p < s)
    {
        f.seekg(p + off);
        pos = f.tellg() - off;
    }

}

std::streampos FileStream::tell()
{
    if(!f)
        return -1;
    return f.tellg() - off;
}

size_t FileStream::size()
{
    return s;
}
