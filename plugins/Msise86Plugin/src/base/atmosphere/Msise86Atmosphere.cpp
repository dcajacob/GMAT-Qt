// GMAT optional NASA MSIS-86 atmosphere adapter. Apache-2.0.
#include "Msise86Atmosphere.hpp"
#include "GmatBaseException.hpp"
#include <typeinfo>
#include "TimeSystemConverter.hpp"
#include "CelestialBody.hpp"
#include <cmath>
#include <mutex>

extern "C" {
void gmat_m86_gts5_(int *,float *,float *,float *,float *,float *,float *,float *,float *,int *,float *,float *);
void gmat_m86_tselec_(float *);
}
namespace { std::mutex modelMutex; }

Msise86Atmosphere::Msise86Atmosphere(const std::string &name)
   : AtmosphereModel("MSISE86", name) {}

GmatBase *Msise86Atmosphere::Clone() const
{
   return new Msise86Atmosphere(*this);
}

bool Msise86Atmosphere::Density(Real *position, Real *density, Real epoch, Integer count)
{
   if (!mCentralBody)
      throw AtmosphereException("Central body pointer not set in MSISE86 model.");
   if (mCentralBody->GetName() != "Earth")
      throw AtmosphereException("MSISE86 is an Earth atmosphere model.");
   if (!position || !density || count < 0 || !std::isfinite(epoch))
      throw AtmosphereException("Invalid MSISE86 state, output buffer, count or epoch.");
   // The original model retains Fortran COMMON and cached local state. Its
   // symbols are isolated from MSISE90/NRLMSISE00; serialize its own callers.
   std::lock_guard<std::mutex> guard(modelMutex);
   const Real utc = theTimeConverter->Convert(epoch, TimeSystemConverter::A1MJD,
      TimeSystemConverter::UTCMJD, GmatTimeConstants::JD_JAN_5_1941);
   GetInputs(utc);
   float switches[25];
   for (float &value : switches) value = 1.0f;
   // As in GMAT's other MSIS adapters, use the daily Ap value supplied by
   // AtmosphereModel (including its documented Kp-to-Ap conversion).
   gmat_m86_tselec_(switches);
   int date=yd, mass=48;
   float seconds=static_cast<float>(sod), flux=static_cast<float>(f107),
         average=static_cast<float>(f107a), magnetic[7];
   for (int i=0;i<7;++i) magnetic[i]=static_cast<float>(ap[i]);
   for (Integer i=0;i<count;++i)
   {
      for (int j=0;j<6;++j)
         if (!std::isfinite(position[6*i+j]))
            throw AtmosphereException("Nonfinite MSISE86 spacecraft state.");
      CalculateGeodetics(position+6*i, epoch, true);
      if (!std::isfinite(geoHeight) || !std::isfinite(geoLat) || !std::isfinite(geoLong) || geoHeight < 90.0)
         throw AtmosphereException("MSISE86 requires a finite geodetic state at or above 90 km altitude.");
      float altitude=static_cast<float>(geoHeight),latitude=static_cast<float>(geoLat),
            longitude=static_cast<float>(geoLong),localTime=static_cast<float>(sod/3600.0+geoLong/15.0),
            species[8]={},temperature[2]={};
      gmat_m86_gts5_(&date,&seconds,&altitude,&latitude,&longitude,&localTime,
                    &average,&flux,magnetic,&mass,species,temperature);
      // Original output is g/cm^3; GMAT consumes kg/m^3.
      const Real value=static_cast<Real>(species[5])*1000.0;
      if (!std::isfinite(value) || value < 0.0)
         throw AtmosphereException("MSISE86 returned an invalid atmospheric density.");
      density[i]=value;
   }
   return true;
}

// Reset/copy through the inherited polymorphic contract without slicing.
void Msise86Atmosphere::Copy(const GmatBase *original)
{
   const auto *source = dynamic_cast<const Msise86Atmosphere*>(original);
   if (!source || typeid(*source) != typeid(Msise86Atmosphere) ||
       typeid(*this) != typeid(Msise86Atmosphere))
      throw GmatBaseException("Cannot copy incompatible object into Msise86Atmosphere");
   if (source == this)
      return;
   operator=(*source);
}
