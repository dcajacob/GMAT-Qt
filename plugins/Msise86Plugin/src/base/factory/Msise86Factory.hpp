// GMAT optional NASA MSIS-86 factory. Apache-2.0.
#pragma once
#include "Factory.hpp"
#include "Msise86Atmosphere.hpp"
class Msise86Factory : public Factory
{
public:
   Msise86Factory() : Factory(Gmat::ATMOSPHERE) { creatables.push_back("MSISE86"); }
   StringArray GetListOfCreatableObjects(const std::string &body="") override
   { return body.empty() || body=="Earth" ? creatables : StringArray{}; }
   GmatBase *CreateObject(const std::string &type,const std::string &name="") override
   { return CreateAtmosphereModel(type,name); }
   AtmosphereModel *CreateAtmosphereModel(const std::string &type,const std::string &name="") override
   { return type=="MSISE86" ? new Msise86Atmosphere(name) : nullptr; }
};
