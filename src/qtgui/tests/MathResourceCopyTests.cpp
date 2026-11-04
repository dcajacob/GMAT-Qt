#include "FormationAcceptance.hpp"
#include "FactoryManager.hpp"
#include "MathNode.hpp"
#include "Add.hpp"
#include "Parameter.hpp"
#include <memory>
using namespace formaccept;
struct DerivedAdd : Add { using Add::Add; };

static QJsonObject state(MathNode *node)
{
   QJsonObject row{{"name",QString::fromStdString(node->GetName())},{"type",QString::fromStdString(node->GetTypeName())},{"element_type",node->GetElementType()},{"number",node->IsNumber()},{"function",node->IsFunction()},{"function_input",node->IsFunctionInput()},{"global",node->IsGlobal()},{"local",node->IsLocal()}};
   if (node->GetElementType()==Gmat::REAL_TYPE) row["value"]=node->GetRealValue();
   else if (node->GetElementType()==Gmat::RMATRIX_TYPE)
   { const auto &m=node->GetMatrixValue(); require(m.IsSized(),"Active Copy matrix is unsized"); row["rows"]=m.GetNumRows(); row["columns"]=m.GetNumColumns(); QJsonArray values; for (int i=0;i<m.GetNumRows();++i) for (int j=0;j<m.GetNumColumns();++j) values.append(m(i,j)); row["value"]=values; }
   return row;
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("MathResourceCopy");
   try
   {
      require(argc==3 && acceptanceWidgetPlatform(),"Supply math Copy startup and catalog"); Rig rig(QFileInfo(argv[1]).absoluteFilePath()); require(rig.window.buildScript(),"Math Copy baseline Build failed");
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object(); require(catalog["cases"].toArray().size()==44,"Math Copy candidate omitted");
      const auto text=rig.source->toPlainText(); const int history=rig.source->document()->availableUndoSteps(); const auto configured=Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT);
      std::unique_ptr<Parameter> variable(FactoryManager::Instance()->CreateParameter("Variable","Mismatch")); require(bool(variable),"Math Copy incompatible control absent");
      const Rmatrix first(2,3,2.0,-3.0,5.5,7.0,11.0,13.0),second(1,2,-11.0,17.0); QJsonArray rows,failed;
      for (const auto &entry:catalog["cases"].toArray()) for (bool matrix:{false,true})
      {
         const auto spec=entry.toObject(); const auto type=spec["type"].toString(); auto make=[&](const QString &kind,const char *name) { return std::unique_ptr<MathNode>(dynamic_cast<MathNode*>(FactoryManager::Instance()->CreateObject(Gmat::MATH_NODE,kind.toStdString(),name))); };
         auto source=make(type,"CopySource"),destination=make(type,"CopyDestination"),different=make(type=="MathElement"?"Add":"MathElement","DifferentMath"); require(source && destination && different,"Math Copy native constructor absent");
         if (matrix) source->SetMatrixValue(first); else source->SetRealValue(17.25);
         source->SetNumberFlag(!matrix); source->SetFunctionInputFlag(!source->IsFunction()); destination->SetMatrixValue(second); destination->SetIsGlobal(true);
         const auto before=state(source.get()); require(before["type"]==spec["native_type"] && before["value"]==(matrix?QJsonValue(QJsonArray{2,-3,5.5,7,11,13}):QJsonValue(17.25)),"Math Copy source differs from independent literals/type");
         QJsonObject row{{"factory_type",type},{"native_type",spec["native_type"]},{"matrix",matrix},{"source",before}};
         try
         {
            destination->Copy(source.get()); auto expected=before; expected["name"]="CopyDestination"; expected["global"]=true; row["copied"]=state(destination.get()); require(state(destination.get())==expected,"Polymorphic math Copy lost active state,flags or destination identity");
            std::unique_ptr<GmatBase> object(destination->Clone()); auto *clone=dynamic_cast<MathNode*>(object.get()); require(clone && state(clone)==expected,"Copied math Clone differs"); clone->SetRealValue(-123); require(state(destination.get())==expected && state(source.get())==before,"Copied math Clone aliases operands");
            const auto self=state(destination.get()); destination->Copy(destination.get()); require(state(destination.get())==self,"Math self-Copy changed state"); int rejected=0;
            for (const GmatBase *bad:{static_cast<const GmatBase*>(nullptr),static_cast<const GmatBase*>(variable.get()),static_cast<const GmatBase*>(different.get())})
            { bool threw=false; try { destination->Copy(bad); } catch (BaseException &) { threw=true; } require(threw && state(destination.get())==self,"Math incompatible Copy was accepted or mutated destination"); ++rejected; }
            require(state(source.get())==before,"Polymorphic math Copy mutated source"); row["source_unchanged"]=true; row["invalid_rejections"]=rejected; row["clone_independent"]=true; row["self_unchanged"]=true; row["passed"]=true;
         }
         catch (BaseException &e) { row["passed"]=false; row["error"]=QString::fromStdString(e.GetFullMessage()); failed.append(type+(matrix?":matrix":":scalar")); }
         catch (std::exception &e) { row["passed"]=false; row["error"]=e.what(); failed.append(type+(matrix?":matrix":":scalar")); }
         row["source_after"]=state(source.get()); row["destination_after"]=state(destination.get()); rows.append(row); rig.save("copy-progress.json",QJsonDocument(rows).toJson());
      }
      require(rows.size()==88,"Math polymorphic Copy control omitted"); int derivedRejected=0;
      if (failed.isEmpty())
      {
         Add exact("Exact"); DerivedAdd derived("Derived");
         for (auto pair:{std::make_pair(static_cast<GmatBase*>(&exact),static_cast<const GmatBase*>(&derived)),std::make_pair(static_cast<GmatBase*>(&derived),static_cast<const GmatBase*>(&exact))})
         { bool threw=false; try { pair.first->Copy(pair.second); } catch (BaseException &) { threw=true; } require(threw,"Math Copy sliced an unsupported derived type"); ++derivedRejected; }
      }
      require(rig.source->toPlainText()==text && rig.source->document()->availableUndoSteps()==history && configured==Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT),"Unmanaged math Copy checks changed GUI configuration/source/history"); acceptanceView(rig.window);
      const QJsonObject proof{{"checkpoint","math-resource-polymorphic-copy-active-state"},{"outcome",failed.isEmpty()?"passed":"failed"},{"factory_spellings",44},{"rows",rows},{"failed_controls",failed},{"invalid_rejections",3*(rows.size()-failed.size())},{"unsupported_derived_rejections",derivedRejected},{"scope","All 44 spellings with scalar/matrix active state,independent literal values,flags,destination name/global identity,source/clone independence,self-Copy and null/Variable/different-math rejection. Constructed uninitialized nodes only;child/argument graph ownership,configured FunctionRunner execution,all-state/provider/scientific qualification remain open."}};
      rig.save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return failed.isEmpty()?0:1;
   }
   catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
   catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
