#ifndef SpiceKernelFileRetirement_hpp
#define SpiceKernelFileRetirement_hpp

#include "SpiceInterface.hpp"
#include "UtilityException.hpp"
#include "MessageInterface.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <exception>

namespace GmatSpiceKernelFiles
{
// Generated files must not be rewritten or removed while a native furnish
// still refers to them. A failed unload remains an error even if its recovery
// happened to remove this file from the pool.
inline void Unload(SpiceInterface *reader, const std::string &file)
{
   if (file.empty()) return;
   if (!reader) throw UtilityException("No SPICE reader available to retire generated kernel \"" + file + "\".\n");
   if (!reader->IsLoaded(file)) return;
   const bool unloaded = reader->UnloadKernel(file);
   const bool stillLoaded = reader->IsLoaded(file);
   if (!unloaded) throw UtilityException("SPICE reported an unload failure for generated kernel \"" + file + "\".\n");
   if (stillLoaded) throw UtilityException("Generated kernel \"" + file + "\" is still loaded after unload.\n");
}

inline void Retire(SpiceInterface *reader, const std::string &file, bool removeFile = true)
{
   Unload(reader, file);
   if (!removeFile || file.empty()) return;
   if (std::remove(file.c_str()) != 0)
   {
      const int error = errno;
      if (error != ENOENT)
         throw UtilityException("Cannot remove generated kernel \"" + file + "\": " + std::strerror(error) + "\n");
   }
}

// Destruction logs failures and retains files whose retirement is uncertain.
// Logging failures cannot turn this cleanup helper into a throwing destructor.
inline bool TryRetire(SpiceInterface *reader, const std::string &file, bool removeFile = true) noexcept
{
   try { Retire(reader, file, removeFile); return true; }
   catch (BaseException &error)
   {
      try { MessageInterface::ShowMessage("*** WARNING *** Generated kernel retirement failed for %s: %s\n", file.c_str(), error.GetFullMessage().c_str()); }
      catch (...) {}
   }
   catch (const std::exception &error)
   {
      try { MessageInterface::ShowMessage("*** WARNING *** Generated kernel retirement failed for %s: %s\n", file.c_str(), error.what()); }
      catch (...) {}
   }
   catch (...)
   {
      try { MessageInterface::ShowMessage("*** WARNING *** Generated kernel retirement failed for %s\n", file.c_str()); }
      catch (...) {}
   }
   return false;
}
}

#endif
