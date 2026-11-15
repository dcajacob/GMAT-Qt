#include "Subscriber.hpp"
#include "Variable.hpp"
#include "VariableWrapper.hpp"
#include "Function.hpp"
#include "BaseException.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <iostream>
#include <stdexcept>
class ScalarOutputProbe : public Subscriber
{
public:
 ScalarOutputProbe():Subscriber("ScalarOutputProbe","Probe"){}
 GmatBase *Clone()const override{return new ScalarOutputProbe(*this);}
 void attach(ElementWrapper *wrapper){yWrapperObjectNames={"Count"};yParamWrappers={wrapper};}
 ElementWrapper *binding(){return yParamWrappers.at(0);}
};
static void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
int main()
{
 try {
  Variable count("Count");count.SetReal(0);ScalarOutputProbe live;
  auto *wrapper=new VariableWrapper;wrapper->SetDescription("Count");require(wrapper->SetRefObject(&count)&&!wrapper->OwnsRefObject(),"Live wrapper should borrow Count");live.attach(wrapper);
  auto frame=live.SuspendFunctionBindings();require(live.binding()!=wrapper&&live.binding()->GetRefObject()==&count&&!live.binding()->OwnsRefObject(),"Suspended live wrapper lost borrowed identity");
  count.SetReal(7);require(live.binding()->EvaluateReal()==7,"Suspended shared wrapper copied stale scalar");
  require(frame->Restore()&&live.binding()==wrapper&&live.binding()->EvaluateReal()==7,"Outer binding restoration lost live scalar");frame.reset();
  ScalarOutputProbe owned;auto *value=wrapper->Clone();require(value&&value->OwnsRefObject()&&value->GetRefObject()!=&count,"Owned value control missing independent reference");owned.attach(value);
  auto privateFrame=owned.SuspendFunctionBindings();require(owned.binding()!=value&&owned.binding()->GetRefObject()!=value->GetRefObject()&&owned.binding()->OwnsRefObject(),"Owned wrapper snapshot became borrowed");
  owned.binding()->SetReal(99);require(owned.binding()->EvaluateReal()==99&&value->EvaluateReal()==7&&count.EvaluateReal()==7,"Owned inner value mutated enclosing or live scalar");
  require(privateFrame->Restore()&&owned.binding()==value&&owned.binding()->EvaluateReal()==7,"Owned outer value was not restored");privateFrame.reset();
  const QJsonObject proof{{"status","passed"},{"borrowed_scalar_identity_preserved",true},{"live_value",7},{"owned_inner_value",99},{"owned_outer_value",7},{"owned_wrapper_isolation_preserved",true},{"original_wrapper_pointers_restored",true},{"new_requirement_credit",0}};
  const auto output=qgetenv("GMAT_QT_ACCEPTANCE_DIR");if(!output.isEmpty()){QFile f(QString::fromUtf8(output)+"/proof.json");require(f.open(QIODevice::WriteOnly),"Proof write failed");f.write(QJsonDocument(proof).toJson());}
  std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<e.what()<<std::endl;}return 1;
}
