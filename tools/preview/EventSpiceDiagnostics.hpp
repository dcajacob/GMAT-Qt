#pragma once
#include "SpiceUsr.h"

#if defined(_WIN32)
#if defined(GMAT_EVENT_DIAGNOSTIC_EXPORTS)
#define GMAT_EVENT_DIAGNOSTIC_API __declspec(dllexport)
#else
#define GMAT_EVENT_DIAGNOSTIC_API __declspec(dllimport)
#endif
#else
#define GMAT_EVENT_DIAGNOSTIC_API
#endif

// These test-only exports keep diagnostic calls in the engine's SPICE pool.
extern "C" {
GMAT_EVENT_DIAGNOSTIC_API void GmatEventBodvrd(const char *, const char *, SpiceInt, SpiceInt *, SpiceDouble *);
GMAT_EVENT_DIAGNOSTIC_API SpiceBoolean GmatEventFailed();
GMAT_EVENT_DIAGNOSTIC_API void GmatEventKtotal(const char *, SpiceInt *);
GMAT_EVENT_DIAGNOSTIC_API void GmatEventKdata(SpiceInt, const char *, SpiceInt, SpiceInt, SpiceInt,
    SpiceChar *, SpiceChar *, SpiceChar *, SpiceInt *, SpiceBoolean *);
GMAT_EVENT_DIAGNOSTIC_API void GmatEventStr2et(const char *, SpiceDouble *);
GMAT_EVENT_DIAGNOSTIC_API SpiceDouble GmatEventUnitim(SpiceDouble, const char *, const char *);
GMAT_EVENT_DIAGNOSTIC_API void GmatEventSpkezr(const char *, SpiceDouble, const char *, const char *,
    const char *, SpiceDouble *, SpiceDouble *);
GMAT_EVENT_DIAGNOSTIC_API void GmatEventPxform(const char *, const char *, SpiceDouble, SpiceDouble [3][3]);
}

#if !defined(GMAT_EVENT_DIAGNOSTIC_EXPORTS)
#define bodvrd_c GmatEventBodvrd
#define failed_c GmatEventFailed
#define ktotal_c GmatEventKtotal
#define kdata_c GmatEventKdata
#define str2et_c GmatEventStr2et
#define unitim_c GmatEventUnitim
#define spkezr_c GmatEventSpkezr
#define pxform_c GmatEventPxform
#endif
