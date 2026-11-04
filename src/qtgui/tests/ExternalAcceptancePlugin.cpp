// Test-only external GMAT plug-in. Never linked into the GUI or enabled in
// the shipped startup. Its affine result is deliberately independently known.
#include "Factory.hpp"
#include "Hardware.hpp"
#include "HardwareException.hpp"
#include "GmatCommand.hpp"
#include "CommandException.hpp"
#include "MessageInterface.hpp"
#include <cmath>
#include <sstream>

class AcceptanceProbe final : public Hardware
{
public:
   explicit AcceptanceProbe(const std::string &name) : Hardware(Gmat::HARDWARE,"AcceptanceProbe",name)
   {
      objectTypeNames.push_back("AcceptanceProbe");
      parameterCount=ProbeCount;
      for(Integer i=Input;i<ProbeCount;++i)parameterWriteOrder.push_back(i);
   }
   AcceptanceProbe(const AcceptanceProbe &other):Hardware(other),input(other.input),gain(other.gain),bias(other.bias)
   {
      // GmatBase intentionally does not copy parameterWriteOrder; each level
      // reconstructs its own entries. Preserve the provider's fields on clones.
      parameterCount=ProbeCount;
      for(Integer i=Input;i<ProbeCount;++i)parameterWriteOrder.push_back(i);
   }
   GmatBase *Clone() const override {return new AcceptanceProbe(*this);}
   void Copy(const GmatBase *source) override
   {
      const auto *probe=dynamic_cast<const AcceptanceProbe *>(source);
      if(!probe)throw HardwareException("AcceptanceProbe copy requires its own type");
      const auto name=GetName();*this=*probe;SetName(name);
   }
   std::string GetParameterText(Integer id) const override
   {return id==Input?"Input":id==Gain?"Gain":id==Bias?"Bias":Hardware::GetParameterText(id);}
   Integer GetParameterID(const std::string &name) const override
   {return name=="Input"?Input:name=="Gain"?Gain:name=="Bias"?Bias:Hardware::GetParameterID(name);}
   Gmat::ParameterType GetParameterType(Integer id) const override
   {return id>=Input&&id<ProbeCount?Gmat::REAL_TYPE:Hardware::GetParameterType(id);}
   std::string GetParameterTypeString(Integer id) const override
   {return id>=Input&&id<ProbeCount?"Real":Hardware::GetParameterTypeString(id);}
   bool IsParameterReadOnly(Integer id) const override
   {return id>=Input&&id<ProbeCount?false:Hardware::IsParameterReadOnly(id);}
   using Hardware::GetRealParameter;
   using Hardware::SetRealParameter;
   Real GetRealParameter(Integer id) const override
   {return id==Input?input:id==Gain?gain:id==Bias?bias:Hardware::GetRealParameter(id);}
   Real SetRealParameter(Integer id,Real value) override
   {
      if(id<Input||id>=ProbeCount)return Hardware::SetRealParameter(id,value);
      if(!std::isfinite(value)||(id==Input&&std::abs(value)>100)||
         (id==Gain&&(value<=0||value>8))||(id==Bias&&std::abs(value)>10))
         throw HardwareException("AcceptanceProbe requires finite Input in [-100,100], Gain in (0,8], Bias in [-10,10]");
      if(id==Input)input=value;else if(id==Gain)gain=value;else bias=value;
      return value;
   }
   Real Evaluate() const {return gain*input+bias;}
private:
   enum {Input=HardwareParamCount,Gain,Bias,ProbeCount};
   Real input=1,gain=2,bias=0;
};

class AcceptanceEvaluate final : public GmatCommand
{
public:
   AcceptanceEvaluate():GmatCommand("AcceptanceEvaluate"){}
   GmatBase *Clone() const override {return new AcceptanceEvaluate(*this);}
   DEFAULT_TO_NO_CLONES
   bool RenameRefObject(UnsignedInt,const std::string &oldName,const std::string &newName) override
   {if(probeName==oldName)probeName=newName;if(resultName==oldName)resultName=newName;return true;}
   bool InterpretAction() override
   {
      auto text=generatingString;const auto comment=text.find('%');if(comment!=std::string::npos)text.erase(comment);
      for(auto &c:text)if(c==';')c=' ';
      std::istringstream stream(text);std::string command,extra;
      if(!(stream>>command>>probeName>>resultName)||command!="AcceptanceEvaluate"||(stream>>extra))
         throw CommandException("AcceptanceEvaluate requires a probe and a Variable result");
      return true;
   }
   const StringArray &GetRefObjectNameArray(UnsignedInt type) override
   {
      names.clear();
      if(type==Gmat::UNKNOWN_OBJECT||type==Gmat::HARDWARE)names.push_back(probeName);
      if(type==Gmat::UNKNOWN_OBJECT||type==Gmat::PARAMETER)names.push_back(resultName);
      return names;
   }
   bool Initialize() override
   {
      if(!GmatCommand::Initialize())return false;
      probe=dynamic_cast<AcceptanceProbe *>(FindObject(probeName));result=FindObject(resultName);
      if(!probe||!result||!result->IsOfType(Gmat::VARIABLE))
         throw CommandException("AcceptanceEvaluate needs AcceptanceProbe and Variable objects");
      return true;
   }
   bool Execute() override
   {
      if(!probe||!result)throw CommandException("AcceptanceEvaluate was not initialized");
      result->SetRealParameter("Value",probe->Evaluate());BuildCommandSummary(true);return true;
   }
private:
   std::string probeName,resultName;StringArray names;
   AcceptanceProbe *probe=nullptr;GmatBase *result=nullptr;
};

class ProbeFactory final : public Factory
{
public:
   ProbeFactory():Factory(Gmat::HARDWARE){creatables.push_back("AcceptanceProbe");}
   GmatBase *CreateObject(const std::string &type,const std::string &name="") override
   {return CreateHardware(type,name);}
   Hardware *CreateHardware(const std::string &type,const std::string &name="") override
   {return type=="AcceptanceProbe"?new AcceptanceProbe(name):nullptr;}
};
class EvaluateFactory final : public Factory
{
public:
   EvaluateFactory():Factory(Gmat::COMMAND){creatables.push_back("AcceptanceEvaluate");}
   GmatBase *CreateObject(const std::string &type,const std::string &name="") override
   {return CreateCommand(type,name);}
   GmatCommand *CreateCommand(const std::string &type,const std::string &name="") override
   {return type=="AcceptanceEvaluate"?new AcceptanceEvaluate:nullptr;}
};
#if defined(_WIN32)
#define ACCEPTANCE_PLUGIN_EXPORT __declspec(dllexport)
#else
#define ACCEPTANCE_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif
extern "C" {
ACCEPTANCE_PLUGIN_EXPORT Integer GetFactoryCount(){return 2;}
ACCEPTANCE_PLUGIN_EXPORT Factory *GetFactoryPointer(Integer index)
{return index==0?static_cast<Factory *>(new ProbeFactory):index==1?static_cast<Factory *>(new EvaluateFactory):nullptr;}
ACCEPTANCE_PLUGIN_EXPORT void SetMessageReceiver(MessageReceiver *receiver)
{MessageInterface::SetMessageReceiver(receiver);}
}
