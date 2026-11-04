#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "Debugger.hpp"
#include "ReportViewer.hpp"
#include "Array.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
using namespace formaccept;

static void reveal(QWidget *widget)
{
   require(widget,"Loop-report control absent");
   for (QWidget *p=widget;p;p=p->parentWidget())
      if (auto *scroll=qobject_cast<QScrollArea *>(p)) scroll->ensureWidgetVisible(widget);
   events();require(widget->isVisible()&&widget->isEnabled(),"Loop-report control unreachable");
}
static QJsonObject settings(GmatBase *object)
{
   require(object,"Loop-report object absent");QJsonObject result;
   for (const char *name:{"Precision","ColumnWidth"}) result[name]=object->GetIntegerParameter(name);
   for (const char *name:{"FixedWidth","WriteHeaders","WriteReport"}) result[name]=object->GetBooleanParameter(name);
   for (const char *name:{"LeftJustify","ZeroFill"}) result[name]=QString::fromStdString(object->GetOnOffParameter(name));
   for (const char *name:{"Filename","Delimiter"}) result[name]=QString::fromStdString(object->GetStringParameter(name));
   return result;
}
static QJsonObject native()
{
   auto *engine=Moderator::Instance();QJsonObject result;
   for (const char *name:{"I","V","W","Keep","Count"}) {
      auto *object=engine->GetInternalObject(name);require(object,"Loop-report scalar absent");
      result[name]=object->GetRealParameter("Value");
   }
   auto *array=dynamic_cast<Array *>(engine->GetInternalObject("A"));
   auto *sat=engine->GetInternalObject("Sat"),*text=engine->GetInternalObject("S");
   require(array&&sat&&text,"Loop-report typed object absent");
   result["element"]=array->GetRmatrixParameter("RmatValue")(1,0);
   result["mass"]=sat->GetRealParameter("DryMass");
   result["S"]=QString::fromStdString(text->GetStringParameter("Value"));return result;
}
static void compareNative(const QJsonObject &actual,const QJsonObject &expected)
{
   require(actual.keys()==expected.keys(),"Loop-report native field set differs");
   for(auto i=expected.begin();i!=expected.end();++i) {
      if(i.value().isString()) require(actual[i.key()]==i.value(),"Loop-report String value differs from literal oracle");
      else {const auto value=actual[i.key()].toDouble();require(std::isfinite(value)&&std::abs(value-i.value().toDouble())<=1e-12,"Loop-report native number differs from literal oracle");}
   }
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaReportLoop");
   try {
      require(argc==4&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen","Supply startup,cases,key on a native widget platform");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();const auto document=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();QJsonObject spec;
      const QString key=argv[3];for(auto value:document["cases"].toArray())if(value.toObject()["key"]==key)spec=value.toObject();require(!spec.isEmpty(),"Loop-report case absent");
      TestSettings environment;QTemporaryDir files;require(files.isValid(),"Loop-report fixture absent");
      QStringList paths;for(int i=0;i<3;++i)paths<<files.filePath("Loop report "+QString::number(i)+" Δ.txt");
      const auto other=files.filePath("Unselected report Ω.txt"),saved=files.filePath("Report in loop Ω.script");
      auto resolve=[&](QString text){for(int i=0;i<3;++i)text.replace("@FILE"+QString::number(i)+"@",paths[i]);text.replace("@OTHER@",other);require(!text.contains('@'),"Loop-report unresolved fixture token");return text;};
      auto expectedSettings=[&](QJsonObject s){s["Filename"]=resolve(s["Filename"].toString());return s;};
      const auto baseline=expectedSettings(document["base"].toObject());const auto expected=spec["expected"].toArray();const auto parameters=spec["parameters"].toArray();QStringList selected;for(auto p:parameters)selected<<p.toString();
      QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;window.resize(1200,850);window.show();require(window.initialize(startup),"Loop-report initialization failed");events();acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission"),*outputs=window.findChild<QTreeWidget *>("Output");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&outputs&&area,"Loop-report workspace absent");
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(evidence+"/"+name,bytes);};save("case.json",QJsonDocument(spec).toJson());
      QString initial="% Runtime Report settings Ω\nCreate Variable I V W Keep Count;\nI = 0;\nV = 12.5;\nW = -0.003125;\nKeep = 41;\nCount = 0;\nCreate Array A[2,2];\nA(2,1) = 0.25;\nCreate String S;\nS = 'initial';\nCreate Spacecraft Sat;\nSat.DryMass = 850;\nCreate ReportFile Output Other;\nOther.Filename = '"+other+"';\n";
      for(auto i=baseline.begin();i!=baseline.end();++i) {
         QString value=i.value().isBool()?(i.value().toBool()?"true":"false"):i.value().isDouble()?QString::number(i.value().toInt()):i.value().toString();
         if(i.key()=="Filename"||i.key()=="Delimiter")value="'"+value+"'";
         initial+="Output."+i.key()+" = "+value+";\n";
      }
      const QString oldReport="Report 'Runtime report' Other Keep; % retained Report comment\n",newReport="Report 'Runtime report' Output "+selected.join(" ")+"; % retained Report comment\n";
      initial+="BeginMissionSequence;\nFor 'Runtime loop' I = 1:1:"+QString::number(spec["iterations"].toInt())+";\n"+resolve(spec["body"].toString())+oldReport+"'After report' Count = Count + 0;\nEndFor;\n% Preserve tail Δ\n";
      write(saved,initial.toUtf8());save("initial.script",initial.toUtf8());if(!(window.loadScript(saved)&&window.buildScript())){save("build-diagnostic.txt",messages->toPlainText().toUtf8());std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Loop-report initial build failed");}QString wanted=initial;QJsonArray edits;
      auto open=[&](const QString &label){
         for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)=="Mission")tabs->setCurrentIndex(i);
         const auto snapshot=window.missionSnapshot();int index=-1;for(int i=0;i<snapshot.nodes.size();++i)if(missionCommandLabel(snapshot.nodes[i].statement).name==label){require(index<0,"Loop-report label ambiguous");index=i;}require(index>=0,"Loop-report command absent");
         QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator i(tree);*i;++i)if((*i)->data(0,Qt::UserRole).isValid()&&(*i)->data(0,Qt::UserRole).toInt()==index)item=*i;require(item,"Loop-report tree item absent");
         tree->expandAll();tree->scrollToItem(item);tree->itemDoubleClicked(item,0);events();auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Loop-report actual command editor absent");
         if(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible"){child->showNormal();child->move(16,12);child->resize(std::min(1000,area->viewport()->width()-32),std::min(710,area->viewport()->height()-24));}events();acceptanceView(*child);return child;
      };
      auto click=[&](QMdiSubWindow *child,QDialogButtonBox::StandardButton action){auto *box=child->findChild<QDialogButtonBox *>();require(box&&box->button(action),"Loop-report editor action absent");reveal(box->button(action));acceptanceView(*child);QTest::mouseClick(box->button(action),Qt::LeftButton);events();};
      for(auto value:spec["edits"].toArray()) {
         const auto edit=value.toObject();const auto label=edit["label"].toString(),destination=edit["destination"].toString(),before=resolve(edit["before"].toString()),expression=resolve(edit["expression"].toString());
         const auto old="'"+label+"' "+destination+" = "+before+";",replacement="'"+label+"' "+destination+" = "+expression+";";
         require(wanted.count(old)==1,"Loop-report exact assignment anchor absent");const auto pristine=wanted;const int undo=source->document()->availableUndoSteps();
         QPointer<QMdiSubWindow> child=open(label);auto set=[&]{auto *panel=dynamic_cast<CommandEditor *>(child->widget());auto *dest=panel->findChild<QLineEdit *>("commandField_Destination"),*expr=panel->findChild<QLineEdit *>("commandField_Expression");require(dest&&expr&&dest->text()==destination&&expr->text()==before,"Loop-report typed assignment differs");reveal(expr);expr->setText(expression);events();require(panel->hasChanges()&&source->toPlainText()==pristine&&source->document()->availableUndoSteps()==undo&&settings(Moderator::Instance()->GetConfiguredObject("Output"))==baseline,"Loop-report draft escaped configured/source isolation");};
         set();click(child,QDialogButtonBox::Cancel);require(!child&&source->toPlainText()==pristine&&source->document()->availableUndoSteps()==undo,"Loop-report Cancel changed source/history");child=open(label);set();click(child,QDialogButtonBox::Apply);
         wanted.replace(old,replacement);require(source->toPlainText()==wanted,"Loop-report assignment Apply changed unrelated source");const int appliedUndo=source->document()->availableUndoSteps();
         // Apply may retire the original owner after rebuilding the mission.
         if(child){click(child,QDialogButtonBox::Apply);require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==appliedUndo,"Loop-report clean Apply changed history");require(child->close(),"Loop-report clean owner did not close");events();}
         source->undo();require(source->toPlainText()==pristine&&window.buildScript(),"Loop-report assignment Undo failed");source->redo();require(source->toPlainText()==wanted&&window.buildScript()&&settings(Moderator::Instance()->GetConfiguredObject("Output"))==baseline,"Loop-report assignment Redo changed configured settings");
         edits.append(QJsonObject{{"label",label},{"destination",destination},{"expression",expression},{"cancelled",true},{"history_exact",true}});
      }
      QPointer<QMdiSubWindow> child=open("Runtime report");auto *panel=dynamic_cast<CommandEditor *>(child->widget());auto *file=panel->findChild<QLineEdit *>("commandField_Report file"),*param=panel->findChild<QLineEdit *>("commandField_Parameters");auto *choose=panel->findChild<QPushButton *>("commandChoose_Parameters");require(file&&param&&choose,"Loop-report typed Report controls absent");reveal(file);file->setText("Output");reveal(choose);acceptanceView(*child);
      std::exception_ptr pickerFailure;QTimer::singleShot(0,panel,[&]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());try{
         require(dialog,"Loop-report actual parameter picker absent");auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry");auto *list=dialog->findChild<QListWidget *>("reportSelectedParameters");require(entry&&list,"Loop-report parameter controls absent");dialog->findChild<QPushButton *>("reportParameterRemoveAll")->click();
         for(const auto &name:selected){entry->setEditText(name=="A(2,1)"?"A":name);if(name=="A(2,1)"){dialog->findChild<QSpinBox *>("reportArrayRow")->setValue(2);dialog->findChild<QSpinBox *>("reportArrayColumn")->setValue(1);dialog->findChild<QPushButton *>("reportAddElement")->click();}else dialog->findChild<QPushButton *>("reportAddParameter")->click();}
         QStringList names;for(int i=0;i<list->count();++i)names<<list->item(i)->text();require(names==selected,"Loop-report ordered parameter selection differs");acceptanceView(*dialog);dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      }catch(...){pickerFailure=std::current_exception();if(dialog)dialog->reject();}});choose->click();events();if(pickerFailure)std::rethrow_exception(pickerFailure);require(param->text()==selected.join(" ")&&source->toPlainText()==wanted,"Loop-report picker escaped pending state");const auto beforeReport=wanted;click(child,QDialogButtonBox::Apply);wanted.replace(oldReport,newReport);require(source->toPlainText()==wanted,"Loop-report Report Apply changed unrelated source");if(child){require(child->close(),"Loop-report Report owner did not close");events();}
      source->undo();require(source->toPlainText()==beforeReport&&window.buildScript(),"Loop-report Report Undo failed");source->redo();require(source->toPlainText()==wanted&&window.buildScript(),"Loop-report Report Redo failed");require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted&&settings(Moderator::Instance()->GetConfiguredObject("Output"))==baseline,"Loop-report Unicode save/reopen changed mission/settings");save("mission.script",wanted.toUtf8());save("edits.json",QJsonDocument(edits).toJson());
      const auto snapshot=window.missionSnapshot();QMap<int,bool> observers;for(int i=0;i<snapshot.nodes.size();++i){const auto label=missionCommandLabel(snapshot.nodes[i].statement).name;if(label=="Runtime report"||label=="After report"){require(snapshot.nodes[i].parent>=0&&snapshot.nodes[snapshot.nodes[i].parent].type=="For","Loop-report command escaped For");observers[i]=label=="After report";}}
      require(observers.size()==2,"Loop-report observation point count differs");for(auto i=observers.begin();i!=observers.end();++i)require(window.setBreakpoint(i.key(),true),"Loop-report breakpoint rejected");
      Debugger *debug=nullptr;for(auto *object:window.children())if(auto *candidate=dynamic_cast<Debugger *>(object))debug=candidate;require(debug,"Loop-report native debugger absent");const auto prior=debug->paused;QJsonArray samples;QStringList fileBytes{"","",""};std::exception_ptr failure;
      for(const auto &path:paths)QFile::remove(path);QFile::remove(other);
      debug->paused=[&](const QString &type){if(prior)prior(type);try{
         auto *dialog=window.findChild<QDialog *>("debuggerDialog");require(dialog&&dialog->isVisible(),"Loop-report actual debugger dialog absent");acceptanceView(*dialog);const int index=dialog->property("commandIndex").toInt();require(observers.contains(index),"Loop-report unselected pause");const bool after=observers[index];const int iteration=samples.size()/2;require(iteration<expected.size()&&after==(samples.size()%2==1),"Loop-report execution order differs");const auto want=expected[iteration].toObject();
         auto *engine=Moderator::Instance();const auto actualSettings=settings(engine->GetInternalObject("Output")),actualNative=native();compareNative(actualNative,want["native"].toObject());require(actualSettings==expectedSettings(want["settings"].toObject()),"Loop-report runtime settings differ from prescribed per-iteration values");
         if(after)fileBytes[want["output_index"].toInt()]+=want["row"].toString();QJsonArray artifacts;
         for(int i=0;i<3;++i){const bool exists=QFileInfo::exists(paths[i]);const auto bytes=exists?read(paths[i]):QByteArray();if(bytes!=fileBytes[i].toUtf8())std::cerr<<"REPORT_DIFF "<<key.toStdString()<<" iteration="<<iteration<<" after="<<after<<" actual=["<<bytes.constData()<<"] expected=["<<fileBytes[i].toStdString()<<"]"<<std::endl;require(bytes==fileBytes[i].toUtf8(),"Loop-report bytes differ from independent literal oracle");if(!fileBytes[i].isEmpty())require(exists,"Loop-report expected file missing");if(exists)save(QString::number(iteration)+(after?"-after-":"-before-")+QString::number(i)+".txt",bytes);artifacts.append(QJsonObject{{"index",i},{"exists",exists},{"bytes",QString::fromUtf8(bytes)}});}
         require(!QFileInfo::exists(other)&&settings(engine->GetConfiguredObject("Output"))==baseline,"Loop-report runtime mutated unselected output or configured object");
         samples.append(QJsonObject{{"iteration",iteration+1},{"after",after},{"native",actualNative},{"settings",actualSettings},{"files",artifacts}});save("samples.json",QJsonDocument(samples).toJson());
      }catch(...){failure=std::current_exception();QTimer::singleShot(0,&window,[&]{window.stopMission();});}QTimer::singleShot(0,&window,[&]{debug->resume();});};
      QTimer watchdog;watchdog.setSingleShot(true);QObject::connect(&watchdog,&QTimer::timeout,&window,[&]{failure=std::make_exception_ptr(std::runtime_error("Loop-report observation watchdog expired"));window.stopMission();});const int undo=source->document()->availableUndoSteps();messages->clear();acceptanceView(window);watchdog.start(30000);const auto result=window.runMission();watchdog.stop();debug->paused=prior;events();save("messages.txt",messages->toPlainText().toUtf8());if(failure)std::rethrow_exception(failure);
      require(result==MainWindow::RunResult::Completed&&samples.size()==2*expected.size(),"Loop-report mission failed or missing execution boundary");require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==undo&&read(saved)==wanted.toUtf8()&&settings(Moderator::Instance()->GetConfiguredObject("Output"))==baseline&&!window.isRunning(),"Loop-report Run changed source/history/file/configured settings");
      const auto last=expected.last().toObject()["output_index"].toInt();auto found=outputs->findItems("Output",Qt::MatchExactly|Qt::MatchRecursive);require(found.size()==1,"Loop-report completed Output item absent");outputs->itemDoubleClicked(found.front(),0);events();auto *viewer=area->activeSubWindow();require(viewer&&dynamic_cast<ReportViewer *>(viewer->widget()),"Loop-report actual viewer absent");auto *text=viewer->findChild<QPlainTextEdit *>();require(text&&text->isReadOnly()&&text->toPlainText().trimmed()==fileBytes[last].trimmed(),"Loop-report viewer differs from final runtime filename bytes");acceptanceView(*viewer);
      QJsonArray artifacts;for(int i=0;i<3;++i){if(QFileInfo::exists(paths[i]))save("final-"+QString::number(i)+".txt",read(paths[i]));artifacts.append(fileBytes[i]);}
      const QJsonObject proof{{"checkpoint","report-loop-"+key},{"outcome","passed"},{"case",key},{"iterations",expected.size()},{"native_boundaries",samples.size()},{"assignment_edits",edits.size()},{"parameters",parameters},{"final_files",artifacts},{"facets",QJsonArray{"actual-typed-assignment-and-report-parameter-picker","pending-cancel-exact-apply-undo-redo-unicode-reopen","native-before-after-settings-and-typed-data","independent-exact-bytes-each-iteration-and-file","configured-object-source-history-and-unselected-output-isolation","actual-final-runtime-report-viewer"}},{"scope",document["scope"]}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;source->document()->setModified(false);return 0;
   }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
