#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ScriptCompatibility.hpp"
#include "Array.hpp"
#include "ScriptInterpreter.hpp"
#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTest>
#include <QTimer>
using namespace formaccept;
namespace {
int views=0;
void view(QWidget &widget){acceptanceView(widget);++views;}
GmatBase *configured(const QString &name){return Moderator::Instance()->GetConfiguredObject(name.toStdString());}
QJsonArray cells(GmatBase *object){
   auto *array=dynamic_cast<Array*>(object);require(array,"Clone Array missing");
   require(array->GetRowCount()==2&&array->GetColCount()==2,"Clone dimensions differ");
   const auto &value=array->GetRmatrix();return QJsonArray{value(0,0),value(0,1),value(1,0),value(1,1)};
}
void matrix(GmatBase *object,double first,double second){require(cells(object)==QJsonArray{first,second,3.75,4.},"Clone Array contents differ");}
void type(QLineEdit *line,const QString &value){
   require(line&&line->isVisible()&&line->isEnabled(),"Clone name control absent");view(*line);
   line->setFocus();QTest::keyClick(line,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(line,value);events();
}
void cloneDialog(Rig &rig,const std::function<void(QDialog&)> &check){
   const auto items=rig.resources->findItems("Cells",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Included resource tree item absent");
   auto *item=items.first();rig.resources->setCurrentItem(item);rig.resources->scrollToItem(item);events();
   std::exception_ptr failure;bool observed=false;
   QTimer::singleShot(0,&rig.window,[&]{
      auto *menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
      try {
         require(menu,"Actual Resource Tree menu absent");view(*menu);QAction *clone=nullptr;
         for(auto *action:menu->actions())if(action->objectName()=="cloneResource")clone=action;
         require(clone&&clone->isEnabled(),"Included Array Clone action unavailable");
         QTimer::singleShot(0,&rig.window,[&]{
            auto *dialog=rig.window.findChild<QDialog*>("cloneResourceDialog");
            try {
               if(!dialog)std::cerr<<rig.window.statusBar()->currentMessage().toStdString()<<'\n';
               require(dialog,"Actual included-resource Clone dialog absent");observed=true;view(*dialog);check(*dialog);
            }
            catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
         });
         QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(clone).center());
      }catch(...){failure=std::current_exception();if(menu)menu->close();}
   });
   rig.resources->customContextMenuRequested(rig.resources->visualItemRect(item).center());events();
   if(failure)std::rethrow_exception(failure);require(observed,"Included-resource dialog was not observed");
}
}
int main(int argc,char **argv){
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("CloneIncludes");
   try {
      require(argc==3,"Expected startup and included-source case");const QString mode=QString::fromLocal8Bit(argv[2]);
      require(QStringList{"Config","SplitMission","Mixed","BoundaryRecovery","NestedMixed","ArrayExpression","CapturedRevision"}.contains(mode),"Unknown clone include case");
      Rig rig(QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath());
      const auto report=rig.files.filePath("Clone values.csv");
      const QString prefix="% Included clone root Ω\nCreate Variable Keep Counter;\nKeep = 7;\nCounter = 0;\n";
      const QString config="% Included grouped Array configuration alpha\nCreate Array Cells[2,2] Other[1,1];\nCells(1,1) = 2.5000; % literal alpha\nCells(1,2) = 1.25;\nCells(2,1) = 3.75;\nCells(2,2) = 4;\n";
      const QString output="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\n";
      const QString commands="Keep = Keep + 2; % do not copy mission assignment alpha\nCounter = Counter + 1;\nCells(1,1) = Cells(1,1) + 1;\nReport Values Keep Counter Cells(1,1) Cells(1,2) Cells(2,1) Cells(2,2);\n";
      const QString tail="% Root tail sentinel β\n";
      QMap<QString,QByteArray> files;
      auto include=[&](const QString &name,const QString &text){const auto path=rig.files.filePath(name);write(path,text.toUtf8());files[path]=text.toUtf8();return "#Include '"+path+"'\n";};
      QString baseline;
      if(mode=="Config")baseline=prefix+include("Configuration Ω.script",config)+output+"BeginMissionSequence;\n"+commands+tail;
      else if(mode=="SplitMission")baseline=prefix+include("Configuration Ω.script",config)+output+include("Mission Ω.script","BeginMissionSequence;\n"+commands)+tail;
      else if(mode=="NestedMixed"){
         const auto nested=include("Nested config.script",config)+output+include("Nested mission.script","BeginMissionSequence;\n"+commands);
         baseline=prefix+include("Outer Ω.script",nested)+tail;
      }else if(mode=="ArrayExpression"){
         const auto managed=setArrayExpressions(config+"BeginMissionSequence;\n","Cells",R"([{"row":1,"column":2,"expression":"Keep*2"}])",2,2);
         const auto begin=managed.indexOf("% GMAT-Qt-Array-Expressions Cells begin");
         require(begin>=0,"Managed expression fixture absent");
         baseline=prefix+include("Configuration Ω.script",config)+output+"BeginMissionSequence;\n"+include("Array expression Ω.script",managed.mid(begin))+commands+tail;
      }else baseline=prefix+include("Mixed Ω.script",config+output+(mode=="BoundaryRecovery"?QString():QString("BeginMissionSequence;\n"))+commands)+tail;
      rig.save("baseline.script",baseline.toUtf8());
      int fileIndex=0;for(auto it=files.cbegin();it!=files.cend();++it)rig.save(QString("include-built-%1.script").arg(fileIndex++),it.value());
      rig.source->setPlainText(baseline);
      bool built=rig.window.buildScript();
      if(mode=="BoundaryRecovery"){
         require(!built,"Missing mission boundary was unexpectedly accepted");
         QString diagnostic;for(const auto &error:Moderator::GetScriptInterpreter()->GetErrorList())diagnostic+=QString::fromStdString(error)+'\n';
         rig.save("expected-boundary-error.txt",diagnostic.toUtf8());require(diagnostic.contains("before BeginMissionSequence"),"Build failed for a different reason than the missing boundary");
         const auto beforeUndo=rig.source->document()->availableUndoSteps();
         const auto failure=rig.window.cloneResource("Cells","BlockedClone",baseline,{},MainWindow::ResourceClonePolicy::Native,MainWindow::ResourceCloneSourcePolicy::CopyIncludedStatements);
         require(!failure.isEmpty()&&!configured("BlockedClone")&&rig.source->toPlainText()==baseline&&rig.source->document()->availableUndoSteps()==beforeUndo,"Clone bypassed the failed-Build guard");
         require(files.size()==1,"Boundary fixture has unexpected file count");auto it=files.begin();auto repaired=it.value();const auto position=repaired.indexOf("Keep = Keep + 2;");require(position>=0,"Boundary recovery insertion point absent");
         repaired.insert(position,"BeginMissionSequence;\n");write(it.key(),repaired);it.value()=repaired;rig.save("include-repaired-0.script",repaired);
         built=rig.window.buildScript();
      }
      if(!built){
         rig.save("messages-failed-build.txt",rig.messages->toPlainText().toUtf8());
         throw std::runtime_error(("Included clone fixture rejected: "+rig.window.statusBar()->currentMessage()+"\n"+rig.messages->toPlainText()).toStdString());
      }
      matrix(configured("Cells"),2.5,1.25);require(!configured("CellsTwin"),"Clone exists before action");
      const bool needsCopy=mode!="Config"&&mode!="SplitMission";
      if(needsCopy){const auto error=rig.window.cloneResource("Cells","NoConsent",baseline);require(!error.isEmpty()&&!configured("NoConsent")&&rig.source->toPlainText()==baseline,"Default API silently copied include statements");matrix(configured("Cells"),2.5,1.25);}
      if(mode=="CapturedRevision"){
         require(files.size()==1,"Revision fixture has unexpected file count");
         auto it=files.begin();auto changed=it.value();changed.replace("Cells(1,1) = 2.5000","Cells(1,1) = 22.0000");
         require(changed!=it.value(),"Revision fixture did not change");write(it.key(),changed);it.value()=changed;
      }
      const auto undo=rig.source->document()->availableUndoSteps();
      auto copyChoice=[&](QDialog &dialog,bool commit){
         auto *box=dialog.findChild<QCheckBox*>("cloneCopyIncludes");auto *buttons=dialog.findChild<QDialogButtonBox*>("resourceCloneButtons");
         require(buttons,"Clone buttons missing");auto *ok=buttons->button(QDialogButtonBox::Ok);
         require(bool(box)==needsCopy,"Clone source choice differs from required layout");
         if(box){require(!box->isChecked()&&!ok->isEnabled(),"Include copying was not explicit");auto *notice=dialog.findChild<QLabel*>("cloneIncludeNotice");require(notice&&notice->text().contains("last successful Build")&&notice->text().contains("files stay unchanged"),"Include copy consequences are not explained");view(*box);QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,QPoint(12,box->height()/2));events();require(box->isChecked()&&ok->isEnabled(),"Include choice did not enable Clone");}
         type(dialog.findChild<QLineEdit*>("resourceName"),"CellsTwin");
         require(rig.source->toPlainText()==baseline&&!configured("CellsTwin"),"Draft clone/source copy registered early");
         if(commit){view(*ok);QTest::mouseClick(ok,Qt::LeftButton);events();require(dialog.result()==QDialog::Accepted,"Included clone failed to commit");}
         else {auto *cancel=buttons->button(QDialogButtonBox::Cancel);view(*cancel);QTest::mouseClick(cancel,Qt::LeftButton);}
      };
      cloneDialog(rig,[&](QDialog &dialog){copyChoice(dialog,false);});
      require(rig.source->toPlainText()==baseline&&rig.source->document()->availableUndoSteps()==undo&&!configured("CellsTwin"),"Clone Cancel changed source/history/model");
      cloneDialog(rig,[&](QDialog &dialog){copyChoice(dialog,true);});rig.closePanels();
      const auto cloned=rig.source->toPlainText();rig.save("cloned.script",cloned.toUtf8());
      require(cloned!=baseline&&cloned.startsWith(prefix)&&cloned.endsWith(tail),"Clone changed unrelated root sentinels");
      require(cloned.count("Keep = Keep + 2;")== (needsCopy||mode=="Config"?1:0),"Clone copied/removed an unrelated mission assignment");
      if(needsCopy)require(!QRegularExpression("^\\s*#Include\\b",QRegularExpression::MultilineOption).match(cloned).hasMatch(),"Consented source still has a live include");
      else require(cloned.count("#Include '")==baseline.count("#Include '"),"Normal clone removed an include link");
      matrix(configured("Cells"),2.5,1.25);matrix(configured("CellsTwin"),2.5,1.25);
      if(mode=="ArrayExpression")require(arrayExpressions(cloned,"CellsTwin")==arrayExpressions(cloned,"Cells")&&arrayExpressions(cloned,"Cells")!="[]","Included Array expressions lost in source copy/clone");
      rig.source->undo();events();require(rig.source->toPlainText()==baseline&&rig.window.buildScript()&&!configured("CellsTwin"),"Clone/source copy Undo did not restore include source atomically");
      matrix(configured("Cells"),mode=="CapturedRevision"?22.:2.5,1.25);
      rig.source->redo();events();require(rig.source->toPlainText()==cloned&&rig.window.buildScript(),"Clone/source copy Redo is not exact");
      const auto saved=rig.files.filePath("Cloned source Δ.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==cloned,"Unicode clone source save/reopen differs");
      matrix(configured("Cells"),2.5,1.25);matrix(configured("CellsTwin"),2.5,1.25);
      const auto history=rig.source->document()->availableUndoSteps();
      require(rig.window.runMission()==MainWindow::RunResult::Completed,"Cloned included mission failed");events();
      rig.save("messages.txt",rig.messages->toPlainText().toUtf8());const auto csv=read(report);rig.save("values.csv",csv);
      const auto rows=QString::fromUtf8(csv).trimmed().split('\n');require(rows.size()==1,"Mission ran more than once");
      const auto columns=rows.first().split(',');const QJsonArray expected{9,1,3.5,mode=="ArrayExpression"?14.:1.25,3.75,4.};
      require(columns.size()==expected.size(),"Included mission output shape differs");QJsonArray values;
      for(int i=0;i<columns.size();++i){bool valid=false;const auto value=columns[i].toDouble(&valid);require(valid&&std::isfinite(value)&&value==expected[i].toDouble(),"Included mission/clone side effect differs");values.append(value);}
      auto *engine=Moderator::Instance();matrix(engine->GetInternalObject("Cells"),3.5,mode=="ArrayExpression"?14.:1.25);matrix(engine->GetInternalObject("CellsTwin"),2.5,mode=="ArrayExpression"?14.:1.25);
      require(rig.source->toPlainText()==cloned&&rig.source->document()->availableUndoSteps()==history,"Run changed cloned source/history");
      QJsonArray includeRecords;fileIndex=0;
      for(auto it=files.cbegin();it!=files.cend();++it){const auto after=read(it.key());require(after==it.value(),"Clone changed an included file");rig.save(QString("include-action-%1.script").arg(fileIndex),it.value());rig.save(QString("include-after-%1.script").arg(fileIndex++),after);includeRecords.append(QJsonObject{{"path",it.key()},{"bytes",after.size()}});}
      const QJsonObject proof{{"checkpoint","resource-clone-includes"},{"outcome","passed"},{"case",mode},{"explicit_include_copy",needsCopy},{"captured_revision",mode=="CapturedRevision"},{"boundary_rejection_recovered",mode=="BoundaryRecovery"},{"configured_original",QJsonArray{2.5,1.25,3.75,4.}},{"runtime_original",cells(engine->GetInternalObject("Cells"))},{"runtime_clone",cells(engine->GetInternalObject("CellsTwin"))},{"raw_values",values},{"expected_values",expected},{"included_files",includeRecords},{"views",views},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;
   }catch(BaseException &error){std::cerr<<error.GetFullMessage()<<'\n';}catch(std::exception &error){std::cerr<<error.what()<<'\n';}return 1;
}
