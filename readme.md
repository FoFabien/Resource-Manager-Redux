# Resource Manager  
  
Rewrite of [my other resource manager](https://github.com/FoFabien/Resource-Manager) (keeping it up for archiving's sake) from scratch,still in C++11.  
Way simplier to use.  
Removed the minimalistic XOR encryption on data pack files, for now.  
  
# How-to  
  
  - Register a Resource Type using registerType(name, constructor, destructor).  
  - Load a data pack file, if any, with readPack(filename).  
  - Use getResource(name, typename) to load/retrieve a resource object instance. The data is staying alive as long as the object, and its copies, are alive. Then, call get() or getData() to retrieve the pointer to your resource.  
  - call garbageCollection() once in a while to clean up.  
  
  - You can also use the FileStream object to stream a file from the disk or a data pack (this one requires a ResourceManager object with the pack loaded).  
  
  - There are more features available (patching a pack with another pack in memory, for example).  
  
# To do  
  
  - Clean up / Optimization.  
  - Examples.  
  - More features ?  