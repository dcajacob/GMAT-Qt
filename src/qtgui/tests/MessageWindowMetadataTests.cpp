#include "MessageWindow.hpp"
#include "BaseException.hpp"
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){
 try{
  MessageWindow value("MetadataProbe");const auto precision=value.GetParameterID("Precision");
  require(value.GetParameterText(precision)=="Precision","MessageWindow precision name lookup is out of bounds");
  require(value.GetParameterType(precision)==Gmat::INTEGER_TYPE&&value.GetParameterTypeString(precision)=="Integer","MessageWindow precision type metadata is wrong");
  for(int id=0;id<value.GetParameterCount();++id){const auto name=value.GetParameterText(id);require(value.GetParameterID(name)==id,"MessageWindow parameter name/id roundtrip changed");require(value.GetParameterTypeString(id)==GmatBase::PARAM_TYPE_STRING[value.GetParameterType(id)],"MessageWindow type description disagrees with actual type");}
  require(value.GetIntegerParameter(precision)==10,"Default precision changed");require(value.SetIntegerParameter(precision,17)==17&&value.GetIntegerParameter(precision)==17,"Positive precision semantics changed");require(value.SetIntegerParameter(precision,0)==17&&value.GetIntegerParameter(precision)==17,"Invalid precision changed preserved value");
  std::cout<<"PASS exact Precision name/type, all parameter name/type roundtrips and unchanged10/17/nonpositive precision behavior\n";return 0;
 }catch(BaseException&e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
