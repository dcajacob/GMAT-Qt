#define GMAT_EVENT_DIAGNOSTIC_EXPORTS
#include "EventSpiceDiagnostics.hpp"

void GmatEventBodvrd(const char *body, const char *item, SpiceInt size, SpiceInt *count, SpiceDouble *values)
{
    bodvrd_c(body, item, size, count, values);
}
SpiceBoolean GmatEventFailed() { return failed_c(); }
void GmatEventKtotal(const char *kind, SpiceInt *count) { ktotal_c(kind, count); }
void GmatEventKdata(SpiceInt index, const char *kind, SpiceInt fileSize, SpiceInt typeSize,
    SpiceInt sourceSize, SpiceChar *file, SpiceChar *type, SpiceChar *source,
    SpiceInt *handle, SpiceBoolean *found)
{
    kdata_c(index, kind, fileSize, typeSize, sourceSize, file, type, source, handle, found);
}
void GmatEventStr2et(const char *value, SpiceDouble *et) { str2et_c(value, et); }
SpiceDouble GmatEventUnitim(SpiceDouble value, const char *from, const char *to)
{
    return unitim_c(value, from, to);
}
void GmatEventSpkezr(const char *target, SpiceDouble et, const char *frame, const char *aberration,
    const char *observer, SpiceDouble *state, SpiceDouble *lightTime)
{
    spkezr_c(target, et, frame, aberration, observer, state, lightTime);
}
void GmatEventPxform(const char *from, const char *to, SpiceDouble et, SpiceDouble matrix[3][3])
{
    pxform_c(from, to, et, matrix);
}
