#include "FormationAcceptance.hpp"
#include "ScriptCompatibility.hpp"
#include "Array.hpp"
#include <QJsonArray>
#include <QJsonDocument>
using namespace formaccept;

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("CloneSelfReference");
   try {
      require(argc==3,"Supply startup and original array name");
      const QString mode=argv[2]; const bool included=mode=="XIncluded";
      const QString original=included?QString("X"):mode,copy=original+"Copy",other=original+"More";
      require(original=="Cells" || original=="X","Unknown self-reference case");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString comment="% Literal text "+original+" remains unchanged\n";
      QString initial="Create Spacecraft Anchor;\nAnchor.X = 7000;\nCreate Array "+original+"[1,2] "+other+"[1,1] External[1,1];\n";
      initial+=other+"(1,1) = 11;\nExternal(1,1) = 7;\nCreate Variable Keep;\nKeep = 41;\n";
      initial+="Create ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nKeep = Keep + 1;\n";
      initial+="Report Values "+original+"(1,1) "+original+"(1,2) External(1,1) "+other+"(1,1) Anchor.X Keep;\n";
      const QString formula=original+"(1,2)+External(1,1)+"+other+"(1,1)+Anchor.X/7000";
      const QJsonArray expressions{
         QJsonObject{{"row",1},{"column",1},{"expression",formula}},
         QJsonObject{{"row",1},{"column",2},{"expression","2"}}
      };
      initial=setArrayExpressions(initial,original,QString::fromUtf8(QJsonDocument(expressions).toJson(QJsonDocument::Compact)),1,2);
      initial.prepend(comment);
      QString includePath; QByteArray includeBody;
      if(included) {
         const auto begin=initial.indexOf("% GMAT-Qt-Array-Expressions "+original+" begin\n");
         const auto endMarker="% GMAT-Qt-Array-Expressions "+original+" end\n";
         const auto end=initial.indexOf(endMarker,begin)+endMarker.size();
         require(begin>=0&&end>begin,"Included initializer block absent");
         includePath=rig.files.filePath("Initial grid Ω.script"); includeBody=initial.mid(begin,end-begin).toUtf8();
         write(includePath,includeBody);
         initial.replace(begin,end-begin,"#Include '"+includePath+"'\n");
      }
      rig.source->setPlainText(initial);
      require(rig.window.buildScript(),"Self-reference fixture Build failed");
      rig.save("initial.script",initial.toUtf8());
      const auto error=included ? rig.window.cloneResource(original,copy,initial,{},MainWindow::ResourceClonePolicy::Native,MainWindow::ResourceCloneSourcePolicy::CopyIncludedStatements) :
         rig.window.cloneResource(original,copy,initial);
      require(error.isEmpty(),qPrintable(error));
      rig.closePanels();
      const auto cloned=rig.source->toPlainText(); rig.save("cloned.script",cloned.toUtf8());
      require(cloned.count(comment)==1,"Clone changed or duplicated unrelated literal comment text");
      require(arrayExpressions(cloned,original)==QString::fromUtf8(QJsonDocument(expressions).toJson(QJsonDocument::Compact)),"Clone changed original formulas");
      rig.source->undo(); require(rig.source->toPlainText()==initial&&rig.window.buildScript(),"Clone exact Undo failed");
      require(!Moderator::Instance()->GetConfiguredObject(copy.toStdString()),"Undo retained the clone");
      rig.source->redo(); require(rig.source->toPlainText()==cloned&&rig.window.buildScript(),"Clone exact Redo failed");
      const QString candidate=cloned+"Report Values "+copy+"(1,1) "+copy+"(1,2) Keep;\n";
      rig.source->setPlainText(candidate); require(rig.window.buildScript(),"Cloned caller Build failed");
      const auto saved=rig.files.filePath("Self reference Ω.script"); require(rig.window.saveScriptTo(saved),"Self-reference Save failed");
      QJsonArray phases;
      for(const auto &phase:QStringList{"initial","no-build-repeat","reopened"}) {
         if(phase=="reopened") require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Self-reference Unicode reopen failed");
         const auto undo=rig.source->document()->availableUndoSteps();
         const auto status=rig.window.runMission(); events();
         rig.save(phase+"-messages.txt",rig.messages->toPlainText().toUtf8());
         require(status==MainWindow::RunResult::Completed,"Self-reference mission failed");
         const auto values=[&](const QString &name) {
            auto *array=dynamic_cast<Array*>(Moderator::Instance()->GetInternalObject(name.toStdString()));
            require(array&&array->GetRowCount()==1&&array->GetColCount()==2,"Self-reference array shape/type differs");
            const auto &matrix=array->GetRmatrixParameter("RmatValue"); return QJsonArray{matrix(0,0),matrix(0,1)};
         };
         const auto actualOriginal=values(original),actualCopy=values(copy);
         const auto report=read(rig.report);rig.save(phase+"-report.csv",report);
         const QJsonObject state{{"phase",phase},{"original",actualOriginal},{"copy",actualCopy},{"report",QString::fromUtf8(report)}};
         phases.append(state);rig.save("phase-results.json",QJsonDocument(phases).toJson());
         require(actualOriginal==QJsonArray{19,2},"Original ordered formula differs from independent result");
         require(actualCopy==QJsonArray{19,2},"Clone self-reference used the original array's completed state");
         require(report=="19,2,7,11,7000,42\n19,2,42\n","Clone changed external/prefix/property references or report results");
         require(rig.source->toPlainText()==candidate&&rig.source->document()->availableUndoSteps()==undo&&read(saved)==candidate.toUtf8(),"Self-reference Run changed source/history/file");
         if(included) require(read(includePath)==includeBody,"Clone changed the included initializer file");
      }
      const QJsonObject proof{{"checkpoint","clone-self-reference-"+mode},{"case",mode},{"include_expanded",included},{"outcome","passed"},{"phases",phases},{"expected_original",QJsonArray{19,2}},{"expected_copy",QJsonArray{19,2}},{"external_reference_values",QJsonArray{7,11,7000}},{"comment_preserved",true},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return 0;
   } catch(BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
     catch(std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
