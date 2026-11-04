// GMAT optional NASA MSIS-86 atmosphere adapter. Apache-2.0.
#ifndef Msise86Atmosphere_hpp
#define Msise86Atmosphere_hpp
#include "AtmosphereModel.hpp"

class Msise86Atmosphere : public AtmosphereModel
{
public:
   explicit Msise86Atmosphere(const std::string &name = "");
   GmatBase *Clone() const override;
   void Copy(const GmatBase *original) override;
   bool Density(Real *position, Real *density,
                Real epoch = GmatTimeConstants::MJD_OF_J2000,
                Integer count = 1) override;
};
#endif
