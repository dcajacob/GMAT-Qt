#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "FileManager.hpp"
#include "Parameter.hpp"
#include <QAbstractItemView>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <memory>
using namespace formaccept;
static void reveal(QWidget *input)
{
   require(input,"Save control absent");
   for(auto *p=input;p;p=p->parentWidget()){
      for(auto *tabs:input->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(input);
   }
   events();require(input->isVisible()&&input->isEnabled(),"Save control inaccessible");acceptanceView(*input);
}
static GmatCommand *command()
{
   for(auto *p=Moderator::Instance()->GetFirstCommand();p;p=p->GetNext())if(p->GetTypeName()=="Save")return p;
   throw std::runtime_error("Native Save command absent");
}
static CommandEditor *open(Rig &rig)
{
   const auto snapshot=rig.window.missionSnapshot();int index=-1;
   for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].type=="Save"){require(index<0,"Save mission node ambiguous");index=i;}
   require(index>=0,"Save mission node absent");QTreeWidgetItem *entry=nullptr;
   for(QTreeWidgetItemIterator it(rig.mission);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index)entry=*it;
   require(entry,"Save mission tree item absent");reveal(rig.mission);rig.mission->expandAll();rig.mission->scrollToItem(entry);events();const auto point=rig.mission->visualItemRect(entry).center();
   QTest::mouseClick(rig.mission->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();QTest::mouseDClick(rig.mission->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();
   auto *child=rig.area->activeSubWindow();auto *panel=child?dynamic_cast<CommandEditor *>(child->widget()):nullptr;require(panel,"Actual Save command editor absent");return panel;
}
static void type(QLineEdit *input,const QString &text)
{
   reveal(input);input->setFocus();QTest::keyClick(input,Qt::Key_A,Qt::ControlModifier);QApplication::clipboard()->setText(text);QTest::keyClick(input,Qt::Key_V,Qt::ControlModifier);QTest::keyClick(input,Qt::Key_Tab);events();require(input->text()==text,"Typed Save field changed");
}
static void style(CommandEditor &panel,const QString &value)
{
   auto *box=panel.findChild<QComboBox *>("commandSaveFilenameStyle");reveal(box);const int index=box->findData(value);require(index>=0,"Save filename style absent");
   if(box->currentIndex()!=index){QTest::qWait(QApplication::doubleClickInterval()+20);QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,QPoint(box->width()-10,box->height()/2));events();require(box->view()->isVisible(),"Save style popup did not open");acceptanceView(*box->view()->window());QTest::keyClick(box->view(),Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(box->view(),Qt::Key_Down);QTest::keyClick(box->view(),Qt::Key_Return);events();}
   require(box->currentData().toString()==value,"Save style did not commit");
}
static void settings(GmatBase *save,const QString &mode,const QStringList &objects)
{
   require(save&&save->GetStringParameter("FilenameStyle")==mode.toStdString()&&save->GetStringParameter("Filename").empty(),"Native Save filename mode differs");
   const auto names=save->GetStringArrayParameter(save->GetParameterID("ObjectNames"));QStringList actual;for(const auto &name:names)actual.append(QString::fromStdString(name));require(actual==objects,"Native Save object order/membership differs");
}
static void discard(Rig &rig,CommandEditor &panel)
{
   QMdiSubWindow *child=nullptr;for(auto *candidate:rig.area->subWindowList())if(candidate->widget()==&panel)child=candidate;require(child,"Pending Save window absent");rig.area->setActiveSubWindow(child);reveal(child);QTimer timer;timer.setInterval(5);std::exception_ptr error;
   QObject::connect(&timer,&QTimer::timeout,&rig.window,[&]{if(auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())){timer.stop();try{auto *button=box->button(QMessageBox::Discard);require(button,"Save Discard button absent");acceptanceView(*box);QTest::mouseClick(button,Qt::LeftButton);}catch(...){error=std::current_exception();box->reject();}}});
   timer.start();require(child->close(),"Save pending close rejected");timer.stop();events();if(error)std::rethrow_exception(error);
}
static void selection(CommandEditor &panel,const QStringList &objects)
{
   type(panel.findChild<QLineEdit *>("commandField_Objects"),objects.join(' '));auto *choose=panel.findChild<QPushButton *>("commandChoose_Objects");reveal(choose);std::exception_ptr error;
   QTimer::singleShot(0,&panel,[&]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());try{
      require(dialog&&dialog->objectName()=="commandObjectDialog","Save object selector absent");auto *list=dialog->findChild<QListWidget *>("commandObjectList");require(list,"Save object choices absent");QStringList selected;
      for(int i=0;i<list->count();++i)if(list->item(i)->checkState()==Qt::Checked)selected.append(list->item(i)->text());require(selected==objects,"Save selector changed typed selection/order");
      const auto found=list->findItems("Unused",Qt::MatchExactly);require(found.size()==1&&found.first()->checkState()==Qt::Unchecked,"Unselected Save resource missing");list->setCurrentItem(found.first());reveal(list);QTest::keyClick(list,Qt::Key_Space);events();require(found.first()->checkState()==Qt::Checked,"Save selector checkbox did not change");QTest::keyClick(list,Qt::Key_Space);events();require(found.first()->checkState()==Qt::Unchecked,"Save selector checkbox did not restore");acceptanceView(*dialog);
      auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);reveal(ok);QTest::mouseClick(ok,Qt::LeftButton);
   }catch(...){error=std::current_exception();if(dialog)dialog->reject();}});
   QTest::mouseClick(choose,Qt::LeftButton);events();if(error)std::rethrow_exception(error);require(panel.findChild<QLineEdit *>("commandField_Objects")->text()==objects.join(' '),"Save selection did not persist");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("SaveFilenameAcceptance");
   try{
      require(argc==4,"Supply startup, filename style and selection size");Rig rig(QFileInfo(argv[1]).absoluteFilePath());const QString mode=argv[2],size=argv[3];require(QStringList{"Worksheet","LegacyData"}.contains(mode)&&QStringList{"Single","Multi"}.contains(size),"Unknown Save filename profile");
      const QStringList objects=size=="Single"?QStringList{"Saved"}:QStringList{"Saved","Grid","Craft"};
      const QString original="% Preserve filename fixture Ω\nCreate Variable Saved Unused Keep;\nSaved = 17;\nUnused = 99;\nKeep = 41;\nCreate Array Grid[2,2];\nGrid(1,1) = 1.25;\nGrid(1,2) = 2.5;\nGrid(2,1) = 3.75;\nGrid(2,2) = 5;\nCreate Spacecraft Craft;\nCraft.DisplayStateType = Cartesian;\nCraft.X = 7000;\nCraft.Y = 0;\nCraft.Z = 0;\nCraft.VX = 0;\nCraft.VY = 7.5;\nCraft.VZ = 0;\nBeginMissionSequence;\nSaved = Saved + 2;\nSave 'Snapshot' Unused;\nKeep = Keep + 1;\n% Preserve filename tail Δ\n";
      rig.source->setPlainText(original);require(rig.window.buildScript(),"Save filename fixture failed");auto *native=command();settings(native,"Worksheet",{"Unused"});const auto generated=native->GetGeneratingString(Gmat::SCRIPTING);const int history=rig.source->document()->availableUndoSteps();
      auto pristine=[&]{return rig.source->toPlainText()==original&&rig.source->document()->availableUndoSteps()==history&&command()==native&&native->GetGeneratingString(Gmat::SCRIPTING)==generated;};
      auto *panel=open(rig);auto *box=panel->findChild<QComboBox *>("commandSaveFilenameStyle");auto *filename=panel->findChild<QLineEdit *>("commandSaveFilename");require(box&&box->currentData().toString()=="Worksheet"&&filename&&filename->text().isEmpty(),"Save GUI default is not worksheet/blank filename");selection(*panel,objects);style(*panel,"LegacyData");
      type(filename,rig.files.filePath("Draft's Ω.txt"));require(!box->isEnabled()&&box->currentData().toString()=="LegacyData","Explicit filename dependency lost style draft");type(filename,"");require(box->isEnabled()&&box->currentData().toString()=="LegacyData","Clearing explicit filename lost style draft");
      require(panel->hasChanges()&&pristine(),"Save draft changed native/source/history");discard(rig,*panel);require(pristine(),"Save Discard changed applied source/native/history");
      panel=open(rig);selection(*panel,objects);style(*panel,"LegacyData");style(*panel,mode);require(pristine()&&panel->hasChanges(),"Save settings escaped pending isolation");auto *buttons=panel->findChild<QDialogButtonBox *>();auto *apply=buttons?buttons->button(QDialogButtonBox::Apply):nullptr;reveal(apply);QTest::mouseClick(apply,Qt::LeftButton);events();require(!panel->hasChanges(),"Save filename Apply failed");const auto applied=rig.source->toPlainText();settings(command(),mode,objects);rig.closePanels();
      rig.source->undo();require(rig.source->toPlainText()==original&&rig.window.buildScript(),"Save exact Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Save exact Redo failed");const auto saved=rig.files.filePath("Save filenames Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Save Unicode persistence failed");
      std::unique_ptr<GmatBase> clone(command()->Clone());settings(clone.get(),mode,objects);rig.save("initial.script",original.toUtf8());rig.save("applied.script",applied.toUtf8());
      const QString output=QString::fromStdString(FileManager::Instance()->GetAbsPathname(FileManager::OUTPUT_PATH));auto files=[&]{return QDir(output).entryList({"Saved*","Grid*","Craft*","Unused*"},QDir::Files,QDir::Name);};require(files().isEmpty(),"Save output directory was not isolated");
      const auto result=rig.window.runMission();events();rig.save("messages.txt",rig.messages->toPlainText().toUtf8());require(result==MainWindow::RunResult::Completed,"Save filename mission failed");
      QStringList expected=mode=="Worksheet"?(size=="Single"?QStringList{"Saved.Variable"}:QStringList{"Saved.Variable","Grid.Array","Craft.Spacecraft"}):QStringList{size=="Single"?"Saved.Variable.data":"Saved_Grid_Craft.data"};expected.sort();require(files()==expected,"Actual Save filenames differ from worksheet/legacy rule");QJsonArray outputs;QString combined;
      for(const auto &name:expected){const auto data=read(output+name);require(!data.isEmpty(),"Save output empty");rig.save(name,data);combined+=QString::fromUtf8(data);outputs.append(QJsonObject{{"filename",name},{"bytes",data.size()}});}
      require(combined.contains("Saved = 19;")&&!combined.contains("Unused ="),"Save selected scalar value/membership differs");
      if(size=="Multi")require(combined.contains("Create Array Grid")&&combined.contains("1.25")&&combined.contains("Create Spacecraft Craft")&&combined.contains("Craft.X = 7000"),"Save additional selected types/values differ");
      auto *keep=dynamic_cast<Parameter *>(Moderator::Instance()->GetInternalObject("Keep"));require(keep&&keep->EvaluateReal()==42&&rig.source->toPlainText()==applied,"Save changed source or skipped successor");settings(command(),mode,objects);
      rig.save("proof.json",QJsonDocument(QJsonObject{{"outcome","passed"},{"filename_style",mode},{"selection_size",size},{"selected_objects",QJsonArray::fromStringList(objects)},{"outputs",outputs},{"explicit_filename",""},{"worksheet_default",true},{"selector_and_typed_order",true},{"explicit_filename_disables_style_and_retains_draft",true},{"draft_Discard_Apply_Undo_Redo_Unicode",true},{"native_clone_settings",true},{"saved_scalar",19},{"successor_scalar",42},{"new_requirement_credit",0},{"scope","Exact default filename rule, explicit legacy compatibility and bounded selected values through the actual Save command editor. No universal any-Resource serialization/current-build/NASA/platform/native OS input approval."}}).toJson());return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
