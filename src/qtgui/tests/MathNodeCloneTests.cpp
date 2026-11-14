#include "FormationAcceptance.hpp"
#include "FactoryManager.hpp"
#include "MathNode.hpp"
#include "MathElement.hpp"
#include <memory>
using namespace formaccept;

class StoredStringElement : public MathElement
{
public:
   StoredStringElement(const std::string &name):MathElement("MathElement",name){}
   GmatBase *Clone() const override { return new StoredStringElement(*this); }
   void setText(const std::string &text) { stringValue=text; elementType=Gmat::STRING_TYPE; }
   std::string EvaluateString() override { return stringValue; }
};

static QJsonObject state(MathNode *object)
{
   QJsonObject p{{"name",QString::fromStdString(object->GetName())},{"type",QString::fromStdString(object->GetTypeName())},{"element_type",object->GetElementType()},{"number",object->IsNumber()},{"function",object->IsFunction()},{"function_input",object->IsFunctionInput()}};
   if (object->GetElementType()==Gmat::REAL_TYPE) p["value"]=object->GetRealValue();
   else if (object->GetElementType()==Gmat::STRING_TYPE) p["value"]=QString::fromStdString(object->EvaluateString());
   else if (object->GetElementType()==Gmat::RMATRIX_TYPE)
   {
      const auto &matrix=object->GetMatrixValue(); require(matrix.IsSized(),"Active native matrix is unsized");
      p["rows"]=matrix.GetNumRows(); p["columns"]=matrix.GetNumColumns(); QJsonArray values;
      for (int i=0;i<matrix.GetNumRows();++i) for (int j=0;j<matrix.GetNumColumns();++j) values.append(matrix(i,j)); p["value"]=values;
   }
   return p;
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("MathNodeClone");
   try
   {
      require(argc==3 && acceptanceWidgetPlatform(),"Supply math Clone startup and catalog"); Rig rig(QFileInfo(argv[1]).absoluteFilePath()); require(rig.window.buildScript(),"Math Clone baseline Build failed");
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object(); require(catalog["cases"].toArray().size()==44,"Math Clone candidate omitted");
      const auto text=rig.source->toPlainText(); const int history=rig.source->document()->availableUndoSteps(); const auto configured=Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT);
      const Rmatrix first(2,3,2.0,-3.0,5.5,7.0,11.0,13.0),second(1,2,-11.0,17.0); QJsonArray rows,failed;
      auto save=[&](){rig.save("clone-progress.json",QJsonDocument(rows).toJson());};
      for (const auto &entry:catalog["cases"].toArray())
      {
         const auto spec=entry.toObject(); const auto type=spec["type"].toString();
         for (const auto &kind:QStringList{"default-clone","scalar-clone","matrix-clone","scalar-base-assignment","matrix-base-assignment","matrix-reshape"})
         {
            auto make=[&](const char *name){return std::unique_ptr<MathNode>(dynamic_cast<MathNode*>(FactoryManager::Instance()->CreateObject(Gmat::MATH_NODE,type.toStdString(),name)));};
            auto source=make("CloneSource"),destination=make("AssignDestination"); require(source && destination && source->GetTypeName()==spec["native_type"].toString().toStdString(),"Math Clone constructor/type mismatch");
            QJsonObject row{{"factory_type",type},{"native_type",spec["native_type"]},{"case",kind}};
            try
            {
               if (kind.contains("scalar")) source->SetRealValue(17.25);
               if (kind.contains("matrix")) source->SetMatrixValue(first);
               if (kind=="matrix-reshape") source->SetMatrixValue(second);
               source->SetFunctionInputFlag(!source->IsFunction()); source->SetNumberFlag(kind.contains("scalar"));
               const auto before=state(source.get()); row["source"]=before;
               require(before["function"].toBool()==(type!="MathElement") && before["function_input"].toBool()!=before["function"].toBool() && before["number"].toBool()==kind.contains("scalar"),"Native independent flag seeds differ");
               if (kind.contains("scalar")) require(before["element_type"].toInt()==Gmat::REAL_TYPE && before["value"].toDouble()==17.25,"Native scalar differs from independent literal");
               if (kind.contains("matrix")) require(before["rows"].toInt()==(kind=="matrix-reshape"?1:2) && before["columns"].toInt()==(kind=="matrix-reshape"?2:3) && before["value"].toArray()==(kind=="matrix-reshape"?QJsonArray{-11,17}:QJsonArray{2,-3,5.5,7,11,13}),"Native matrix storage differs from independent literals");
               if (kind.contains("base-assignment"))
               {
                  destination->SetMatrixValue(second); *destination=*source;
                  auto copied=state(destination.get()),expected=before; expected["name"]="AssignDestination"; row["copied"]=copied;
                  require(copied==expected,"MathNode base assignment changed identity,active value or flags");
                  const auto self=state(destination.get()); *destination=*destination; require(state(destination.get())==self,"MathNode base self-assignment changed active state");
               }
               else
               {
                  std::unique_ptr<GmatBase> object(source->Clone()); auto *cloned=dynamic_cast<MathNode*>(object.get()); require(cloned,"Math Clone returned an incompatible object"); row["copied"]=state(cloned); require(state(cloned)==before,"Math Clone lost active value or flags");
                  if (cloned->GetElementType()==Gmat::RMATRIX_TYPE) { const auto &matrix=cloned->GetMatrixValue(); Rmatrix changed(matrix); changed(0,0)=-123; cloned->SetMatrixValue(changed); }
                  else cloned->SetRealValue(-123);
                  require(state(source.get())==before,"Math Clone shares active data with source");
               }
               require(state(source.get())==before,"Math native copy modified its source"); row["source_unchanged"]=true; row["passed"]=true;
            }
            catch (BaseException &e) { row["passed"]=false; row["error"]=QString::fromStdString(e.GetFullMessage()); failed.append(type+":"+kind); }
            catch (std::exception &e) { row["passed"]=false; row["error"]=e.what(); failed.append(type+":"+kind); }
            if (row.contains("source")) { row["source_after"]=state(source.get()); row["source_unchanged"]=row["source_after"]==row["source"]; }
            if (kind.contains("base-assignment")) row["destination_after"]=state(destination.get());
            rows.append(row); save();
         }
      }
      QJsonArray strings;
      for (bool assignment:{false,true})
      {
         QJsonObject row{{"base_assignment",assignment}};
         try
         {
            StoredStringElement source("StringSource"),destination("StringDestination"); source.setText("orbit Ω string"); source.SetFunctionInputFlag(true); const auto before=state(&source); row["source"]=before; require(before["element_type"].toInt()==Gmat::STRING_TYPE && before["value"].toString()=="orbit Ω string","Native stored string differs from independent literal");
            if (assignment) { static_cast<MathNode&>(destination)=source; auto expected=before; expected["name"]="StringDestination"; row["copied"]=state(&destination); require(state(&destination)==expected,"MathNode base assignment lost stored string state"); }
            else { std::unique_ptr<GmatBase> object(source.Clone()); auto *clone=dynamic_cast<MathNode*>(object.get()); require(clone,"Stored string Clone absent"); row["copied"]=state(clone); require(state(clone)==before,"MathNode copy constructor lost stored string state"); }
            require(state(&source)==before,"Stored string copy modified source"); row["passed"]=true;
         }
         catch (BaseException &e) { row["passed"]=false; row["error"]=QString::fromStdString(e.GetFullMessage()); failed.append(assignment?"string-base-assignment":"string-clone"); }
         catch (std::exception &e) { row["passed"]=false; row["error"]=e.what(); failed.append(assignment?"string-base-assignment":"string-clone"); }
         strings.append(row);
      }
      require(rows.size()==264 && strings.size()==2,"Math active-state control omitted");
      require(rig.source->toPlainText()==text && rig.source->document()->availableUndoSteps()==history && configured==Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT),"Unmanaged math controls changed source/history/configuration"); acceptanceView(rig.window);
      const QJsonObject proof{{"checkpoint","math-node-active-state-clone-base-assignment"},{"outcome",failed.isEmpty()?"passed":"failed"},{"factory_spellings",44},{"rows",rows},{"stored_string_controls",strings},{"failed_controls",failed},{"scope","All 44 advertised spellings:default/scalar/matrix Clone,MathNode-base active-state assignment and matrix reshape,with independent literals,flags,identity,self-assignment and source independence. Two protected stored-string controls use a concrete MathElement derivative. Unused scalar matrix-cache storage is not an active-state assertion. No derived polymorphic Copy,child graph ownership,expression/provider evaluation,mathematical formula,science or whole NASA qualification."}};
      rig.save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return failed.isEmpty()?0:1;
   }
   catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
   catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
