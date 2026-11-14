#include "Moderator.hpp"
#include "Propagate.hpp"
#include "StopCondition.hpp"
#include "Spacecraft.hpp"
#include "Variable.hpp"
#include "VariableWrapper.hpp"
#include "Array.hpp"
#include "ArrayWrapper.hpp"
#include "ArrayElementWrapper.hpp"
#include "NumberWrapper.hpp"
#include "BaseException.hpp"
extern "C" {
#include "CInterfaceFunctions.h"
}
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

static void require(bool value,const std::string &message) {if(!value)throw std::runtime_error(message);}
static QJsonObject read(const QString &path) {
   QFile file(path);require(file.open(QIODevice::ReadOnly),"Native scalar oracle absent");
   return QJsonDocument::fromJson(file.readAll()).object();
}
static GmatBase *runtime(const std::string &name) {
   auto *object=Moderator::Instance()->GetInternalObject(name);require(object,"Runtime object absent: "+name);return object;
}
static Propagate *propagate() {
   std::set<GmatCommand*> seen;
   for(auto *command=Moderator::Instance()->GetFirstCommand(1);command;command=command->GetNextWhileExecuting()) {
      require(seen.insert(command).second,"Unexpected top-level command cycle");
      if(auto *p=dynamic_cast<Propagate*>(command))return p;
   }
   throw std::runtime_error("Runtime Propagate absent");
}
static std::vector<StopCondition*> conditions() {
   std::vector<StopCondition*> result;
   for(auto *object:propagate()->GetRefObjectArray(Gmat::STOP_CONDITION)) {
      auto *condition=dynamic_cast<StopCondition*>(object);require(condition,"Native condition type lost");result.push_back(condition);
   }
   require(!result.empty(),"Native conditions absent");return result;
}
static void reject(const std::string &name,const std::function<void()> &f,QJsonArray &records) {
   std::string diagnostic;
   try {f();}catch(BaseException &error) {diagnostic=error.GetFullMessage();}
   require(!diagnostic.empty(),"Invalid scalar accepted: "+name);
   records.append(QJsonObject{{"case",QString::fromStdString(name)},{"diagnostic",QString::fromStdString(diagnostic)}});
}
static void clocks(const QJsonObject &expected,QJsonArray &observations) {
   const auto initial=expected["base_epoch"].toDouble();
   auto stops=conditions();const auto wanted=expected["conditions"].toArray();
   require(stops.size()==static_cast<size_t>(wanted.size()),"Condition cardinality differs");
   for(int i=0;i<wanted.size();++i) {
      auto *condition=stops[static_cast<size_t>(i)];const auto row=wanted[i].toObject();
      require(condition->GetStringParameter("StopVar")==row["lhs"].toString().toStdString(),"Wrong native stopping operand");
      require(condition->UsesUserScalar()==row["user_scalar"].toBool(),"Wrong scalar classification");
      const auto base=condition->GetRealParameter("BaseEpoch");
      require(base==initial,"Goal owner shifted a condition's epoch clock");
      const auto value=condition->EvaluateStopValue();
      require(std::isfinite(value)&&std::abs(value-row["value"].toDouble())<=2e-6,"Wrong measured scalar value");
      observations.append(QJsonObject{{"kind","bound-condition"},{"index",i},{"lhs",row["lhs"]},{"base_epoch",base},{"value",value},{"user_scalar",condition->UsesUserScalar()}});
   }
   for(const auto entry:expected["spacecraft"].toArray()) {
      const auto row=entry.toObject();const auto name=row["name"].toString().toStdString();
      auto *craft=dynamic_cast<Spacecraft*>(runtime(name));require(craft,"Expected spacecraft missing");
      double epoch=0;require(GetSpacecraftEpoch(name.c_str(),&epoch)==0,"C epoch read failed");
      require(std::abs(epoch-row["epoch"].toDouble())*86400<=2e-6,"Independent stopping epoch differs");
      const auto state=craft->GetCartesianState();const auto wantedState=row["state"].toArray();QJsonArray actual;
      require(wantedState.size()==6,"Invalid independent state oracle");
      for(int i=0;i<6;++i) {
         require(std::isfinite(state[i])&&std::abs(state[i]-wantedState[i].toDouble())<=(i<3?2e-5:3e-8),"Independent stopping state differs");
         actual.append(state[i]);
      }
      observations.append(QJsonObject{{"kind","spacecraft"},{"name",row["name"]},{"epoch",epoch},{"state",actual}});
   }
}
static void wrappers(QJsonArray &observations,QJsonArray &rejections) {
   auto stops=conditions();require(stops.size()==3,"Wrapper fixture conditions absent");
   auto *value=dynamic_cast<Variable*>(runtime("Value"));auto *index=dynamic_cast<Variable*>(runtime("Index"));
   auto *grid=dynamic_cast<Array*>(runtime("Grid"));require(value&&index&&grid,"Bound scalar resources absent");
   auto *scalar=stops[0];auto *element=stops[1];
   require(scalar->GetStopParameter()==value&&element->GetStopParameter()==grid,"Scalar metadata uses another native instance");
   require(scalar->UsesUserScalar()&&element->UsesUserScalar(),"Measured scalars not classified");
   for(double wanted:{2.,7.,-3.,730.}) {
      value->SetReal(wanted);require(scalar->EvaluateStopValue()==wanted,"Variable wrapper did not use its bound instance");
      require(scalar->GetStopDifference()==999-wanted,"Variable difference differs");
      observations.append(QJsonObject{{"kind","bound-variable"},{"value",wanted},{"difference",999-wanted}});
   }
   value->SetReal(2);
   for(double invalid:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}) {
      reject(std::isnan(invalid)?"variable-NaN":invalid>0?"variable-positive-infinity":"variable-negative-infinity",[&]{value->SetReal(invalid);scalar->EvaluateStopValue();},rejections);
      value->SetReal(2);require(scalar->EvaluateStopValue()==2,"Variable did not recover after rejection");
   }
   for(double selected:{1.,2.,1.}) {
      index->SetReal(selected);const double wanted=selected==1?3:9;
      require(element->EvaluateStopValue()==wanted,"Array wrapper did not evaluate its current index");
      require(element->GetStopDifference()==999-wanted,"Array difference differs");
      observations.append(QJsonObject{{"kind","bound-array-index"},{"index",selected},{"value",wanted}});
   }
   for(double invalid:{0.,-1.,3.,1.5,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}) {
      const std::string label=std::isnan(invalid)?"NaN":std::isinf(invalid)?"infinity":std::to_string(invalid);
      reject("array-row-"+label,[&]{index->SetReal(invalid);element->EvaluateStopValue();},rejections);
      index->SetReal(1);require(element->EvaluateStopValue()==3,"Index error did not recover");
   }
   index->SetReal(2);grid->SetRealParameter("SingleValue",13,1,1);
   require(element->EvaluateStopValue()==13,"Selected native cell update was ignored");
   grid->SetRealParameter("SingleValue",9,1,1);
   reject("array-cell-NaN",[&]{grid->SetRealParameter("SingleValue",std::numeric_limits<double>::quiet_NaN(),1,1);element->EvaluateStopValue();},rejections);
   grid->SetRealParameter("SingleValue",9,1,1);index->SetReal(1);
   {
      Variable replacement("Value");replacement.SetReal(17);
      VariableWrapper wrapper;wrapper.SetDescription("Value");require(wrapper.SetRefObject(value),"Independent Variable wrapper absent");
      StopCondition rebound("RebindValue","",nullptr,value,999);rebound.SetLhsWrapper(&wrapper);
      require(rebound.EvaluateStopValue()==2,"Independent Variable wrapper has wrong initial value");
      require(rebound.SetStopParameter(&replacement)&&rebound.EvaluateStopValue()==17,"Variable wrapper failed to rebind");
      require(scalar->EvaluateStopValue()==2&&scalar->GetStopParameter()==value,"Independent rebind changed original Variable condition");
      observations.append(QJsonObject{{"kind","variable-rebind"},{"value",17},{"original",2}});
   }
   {
      Array replacement("Grid");replacement.SetSize(2,2);
      replacement.SetRealParameter("SingleValue",13,0,1);replacement.SetRealParameter("SingleValue",19,1,1);
      ArrayElementWrapper wrapper;wrapper.SetDescription("Grid(Index,2)");
      auto *row=new VariableWrapper;row->SetDescription("Index");require(row->SetRefObject(index),"Independent row binding failed");
      auto *column=new NumberWrapper;column->SetDescription("2");
      wrapper.SetRow(row);wrapper.SetColumn(column);require(wrapper.SetRefObject(grid),"Independent Array binding failed");
      StopCondition rebound("RebindArray","",nullptr,grid,999);rebound.SetLhsWrapper(&wrapper);
      require(rebound.EvaluateStopValue()==3,"Independent Array wrapper has wrong initial value");
      require(rebound.SetStopParameter(&replacement),"Array rebind failed");
      for(double selected:{1.,2.}) {
         index->SetReal(selected);const double wanted=selected==1?13:19;
         require(rebound.EvaluateStopValue()==wanted,"Rebound Array lost element/index semantics");
         require(element->EvaluateStopValue()==(selected==1?3:9),"Array rebind changed original condition");
         observations.append(QJsonObject{{"kind","array-rebind"},{"index",selected},{"value",wanted}});
      }
      index->SetReal(1);
   }
   {
      ArrayWrapper whole;whole.SetDescription("Grid");require(whole.SetRefObject(grid),"Whole-array setup failed");
      StopCondition invalid;invalid.SetStopParameter(grid);invalid.SetLhsWrapper(&whole);
      reject("whole-array-is-not-scalar",[&]{invalid.EvaluateStopValue();},rejections);
      StopCondition empty;reject("missing-stopping-value",[&]{empty.EvaluateStopValue();},rejections);
   }
   {
      Variable lhs("Finite");lhs.SetReal(1);VariableWrapper wrapper;wrapper.SetDescription("Finite");wrapper.SetRefObject(&lhs);
      NumberWrapper goal;goal.SetDescription("10");StopCondition condition("FiniteCheck","",nullptr,&lhs,10);
      condition.SetLhsWrapper(&wrapper);condition.SetRhsWrapper(&goal);require(condition.Initialize(),"Finite condition initialization failed");
      for(double invalid:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}) {
         reject(std::isnan(invalid)?"goal-NaN":invalid>0?"goal-positive-infinity":"goal-negative-infinity",[&]{goal.SetReal(invalid);condition.GetStopDifference();},rejections);
         goal.SetReal(10);require(condition.GetStopDifference()==9,"Goal rejection did not recover");
      }
      reject("finite-subtraction-overflow",[&]{lhs.SetReal(-1e308);goal.SetReal(1e308);condition.GetStopDifference();},rejections);
      lhs.SetReal(1);goal.SetReal(10);require(condition.GetStopDifference()==9,"Overflow rejection did not recover");
   }
}
static void reinitialize(const std::string &mode,QJsonArray &observations) {
   auto *original=conditions().front();
   require(mode=="reinitialize-angle"?original->IsCyclicParameter():original->IsTimeCondition(),"Original native metadata absent");
   const auto originalMultiplier=original->GetTimeMultiplier();
   Variable value("Scalar");value.SetReal(730);
   VariableWrapper wrapper;wrapper.SetDescription("Scalar");wrapper.SetRefObject(&value);
   NumberWrapper goal;goal.SetDescription("10");
   StopCondition rebound(*original),fresh("Fresh","",nullptr,&value,10);
   for(auto *condition:{&rebound,&fresh}) {
      condition->SetStringParameter("StopVar","Scalar");condition->SetLhsString("Scalar");condition->SetRhsString("10");
      condition->SetLhsWrapper(&wrapper);condition->SetRhsWrapper(&goal);condition->SetStopParameter(&value);
      require(condition->Initialize(),"Scalar reinitialization failed");
   }
   observations.append(QJsonObject{{"kind","reinitialization"},{"original_multiplier",originalMultiplier},{"rebound_multiplier",rebound.GetTimeMultiplier()},{"fresh_multiplier",fresh.GetTimeMultiplier()},{"rebound_difference",rebound.GetStopDifference()},{"fresh_difference",fresh.GetStopDifference()},{"rebound_cyclic",rebound.IsCyclicParameter()},{"rebound_time",rebound.IsTimeCondition()}});
   require(!rebound.IsCyclicParameter()&&!rebound.IsTimeCondition()&&rebound.UsesUserScalar(),"Scalar retained cyclic/time classification");
   require(rebound.GetStopDifference()==-720&&rebound.GetStopDifference()==fresh.GetStopDifference(),"Scalar retained angular wrapping");
   require(rebound.GetTimeMultiplier()==fresh.GetTimeMultiplier()&&fresh.GetTimeMultiplier()==1,"Scalar retained old time conversion metadata");
   require(mode=="reinitialize-angle"?original->IsCyclicParameter():original->IsTimeCondition(),"Reinitialization changed original condition");
}
int main(int argc,char **argv) {
   QCoreApplication app(argc,argv);QJsonArray observations,rejections;std::string error;
   try {
      require(argc==5,"Expected script, mode, oracle and proof paths");const std::string mode=argv[2];
      require(StartGmat()==0,"Native engine initialization failed");
      std::ifstream maps("/proc/self/maps");require(maps.good(),"Native image map absent");std::string line;
      while(std::getline(maps,line)){const auto pos=line.find('/');if(pos!=std::string::npos)std::cout<<"NASA_LOADED "<<line.substr(pos)<<'\n';}
      require(LoadAndRunScript(argv[1])==0,std::string("Native script failed: ")+getLastMessage());
      if(mode=="wrappers"||mode.find("reinitialize-")==0)require(FindOdeModel("FM")>=1000,"Native prepared model absent");
      if(mode=="wrappers")wrappers(observations,rejections);
      else if(mode.find("reinitialize-")==0)reinitialize(mode,observations);
      else clocks(read(argv[3]),observations);
   }catch(BaseException &e){error=e.GetFullMessage();}catch(std::exception &e){error=e.what();}
   const QJsonObject proof{{"checkpoint","native-scalar-stop-boundaries"},{"outcome",error.empty()?"passed":"failed"},{"mode",argc>2?argv[2]:""},{"observations",observations},{"rejections",rejections},{"error",QString::fromStdString(error)},{"GUI_executions",0},{"new_requirement_credit",0}};
   if(argc==5){QFile file(argv[4]);if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(proof).toJson());else error="Cannot retain native proof";}
   std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';
   if(!error.empty())std::cerr<<error<<'\n';
   return error.empty()?0:1;
}
