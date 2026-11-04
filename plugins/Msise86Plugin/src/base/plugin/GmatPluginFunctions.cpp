// GMAT optional NASA MSIS-86 plugin entry points. Apache-2.0.
#include "Msise86Factory.hpp"
#include "MessageInterface.hpp"
#if defined(_WIN32)
#define MSI86_EXPORT __declspec(dllexport)
#else
#define MSI86_EXPORT __attribute__((visibility("default")))
#endif
extern "C" {
MSI86_EXPORT Integer GetFactoryCount() { return 1; }
MSI86_EXPORT Factory *GetFactoryPointer(Integer index) { return index==0 ? new Msise86Factory : nullptr; }
MSI86_EXPORT void SetMessageReceiver(MessageReceiver *receiver) { MessageInterface::SetMessageReceiver(receiver); }
}
