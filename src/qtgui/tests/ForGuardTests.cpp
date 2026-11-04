#include "For.hpp"
#include "Variable.hpp"
#include "NumberWrapper.hpp"
#include "VariableWrapper.hpp"
#include "BaseException.hpp"
#include <cfenv>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
class ForProbe final : public For
{
public:
 void prepare(Variable &index,double start,double step,double end) {
  indexName=index.GetName(); startName=std::to_string(start); endName=std::to_string(end); incrName=std::to_string(step);
  auto *variable=new VariableWrapper; variable->SetDescription(indexName); require(variable->SetRefObject(&index),"For probe index reference failed"); require(SetElementWrapper(variable,indexName),"For probe index binding failed");
  for (const auto &item:{std::pair<std::string,double>{startName,start},{endName,end},{incrName,step}}) {
   auto *number=new NumberWrapper; number->SetDescription(item.first); require(number->SetReal(item.second)&&SetElementWrapper(number,item.first),"For probe numeric binding failed");
  }
 }
 bool evaluate() {
  // Keep the floating-point probe independent of message-renderer arithmetic.
  const bool original=writeInvalidLoopMessage; writeInvalidLoopMessage=false;
  try { const bool result=StillLooping(); writeInvalidLoopMessage=original; return result; }
  catch (...) { writeInvalidLoopMessage=original; throw; }
 }
};

int main()
{
 try {
  struct Case { const char *name; double start,step,end; bool looping; double index; };
  for (const auto &c:{Case{"zero",1,0,3,false,-99},Case{"wrong-direction",3,.5,1,false,-99},Case{"finite",2,1,4,true,2}}) {
   Variable index("Index"); index.SetReal(-99); ForProbe probe; probe.prepare(index,c.start,c.step,c.end);
   require(std::feclearexcept(FE_ALL_EXCEPT)==0,"Cannot clear For floating-point probe flags"); const bool looping=probe.evaluate(); const int flags=std::fetestexcept(FE_DIVBYZERO|FE_INVALID);
   std::cout<<"FOR_GUARD case="<<c.name<<" flags="<<flags<<" looping="<<looping<<" index="<<index.GetReal()<<std::endl;
   require(flags==0,"For guard raised divide-by-zero or invalid floating-point operation before rejecting an empty loop");
   require(looping==c.looping&&std::abs(index.GetReal()-c.index)<1e-12,"For guard changed documented loop/index behavior");
  }
  return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
