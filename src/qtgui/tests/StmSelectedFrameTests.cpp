#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "CoordinateSystem.hpp"
#include "GmatCommand.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
void write(const QString &path,const QByteArray &data){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"STM GUI evidence write failed");}
QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"STM GUI evidence missing");return f.readAll();}
void click(QPushButton *button){require(button&&button->isVisible()&&button->isEnabled(),"STM GUI button unavailable");QTest::mouseClick(button,Qt::LeftButton);events();}
void choose(QComboBox *combo,const QString &text){
   require(combo&&combo->isVisible()&&combo->isEnabled(),"STM GUI combo unavailable");
   const int row=combo->findText(text);require(row>=0,"STM GUI combo choice missing");
   QTest::mouseClick(combo,Qt::LeftButton,Qt::NoModifier,QPoint(combo->width()-10,combo->height()/2));events();
   auto *view=combo->view();require(view->isVisible(),"STM GUI choice popup did not open");acceptanceView(*view->window());
   QTest::qWait(QApplication::doubleClickInterval()+20);events();
   const auto index=combo->model()->index(row,combo->modelColumn(),combo->rootModelIndex());view->scrollTo(index,QAbstractItemView::PositionAtCenter);events();
   const auto hit=view->visualRect(index).intersected(view->viewport()->rect());require(!hit.isEmpty(),"STM GUI choice has no visible hit rectangle");
   QTest::mouseClick(view->viewport(),Qt::LeftButton,Qt::NoModifier,hit.center());events();
   require(combo->currentText()==text&&!QApplication::activePopupWidget(),"STM GUI choice did not commit and close");
}
}

int main(int argc,char **argv){
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("StmSelectedFrame");
   try{
      require(argc==2&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen","Supply STM GUI startup on native widgets");
      TestSettings settings;QTemporaryDir files;require(files.isValid(),"STM GUI temporary files missing");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &data){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),data);};
      MainWindow window;window.resize(1200,850);window.show();require(window.initialize(startup),"STM GUI runtime failed");
      auto *source=window.findChild<QPlainTextEdit*>("scriptEditor");auto *messages=window.findChild<QPlainTextEdit*>("messageWindow");
      auto *tree=window.findChild<QTreeWidget*>("Mission");auto *area=window.findChild<QMdiArea*>("workspace");require(source&&messages&&tree&&area,"STM GUI workspace missing");
      struct Clean{QPlainTextEdit *source;~Clean(){source->document()->setModified(false);}}clean{source};
      const QStringList types{"OrbitSTM","OrbitSTMA","OrbitSTMB","OrbitSTMC","OrbitSTMD"};
      QString initial="% Selected STM frame GUI Ω\nCreate Spacecraft Sat;\nCreate CoordinateSystem ReviewFrame;\nReviewFrame.Origin = Earth;\nReviewFrame.Axes = ICRF;\nCreate Variable Keep;\nKeep = 41;\n";
      QStringList reports;for(int k=0;k<5;++k){reports<<files.filePath(types[k]+".txt");initial+="Create ReportFile R"+QString::number(k)+";\nR"+QString::number(k)+".Filename = '"+reports[k]+"';\nR"+QString::number(k)+".WriteHeaders = false;\nR"+QString::number(k)+".Precision = 25;\n";}
      initial+="BeginMissionSequence;\n";for(int k=0;k<5;++k)initial+="Report 'Select"+QString::number(k)+"' R"+QString::number(k)+" Sat."+types[k]+";\n";
      initial+="Keep = Keep + 1;\n% Preserve selected-frame tail Δ\n";source->setPlainText(initial);require(window.buildScript(),"STM GUI fixture did not build");save("initial.script",initial.toUtf8());
      auto open=[&](const QString &label){
         const auto snapshot=window.missionSnapshot();int index=-1;
         for(int i=0;i<snapshot.nodes.size();++i)if(missionCommandLabel(snapshot.nodes[i].statement).name==label)index=i;
         QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index)item=*it;
         require(index>=0&&item,"STM GUI report command missing");
         tree->expandAll();tree->scrollToItem(item);tree->itemDoubleClicked(item,0);events();
         QMdiSubWindow *owner=nullptr;for(auto *w:area->subWindowList())if(w->property("commandIndex").isValid()&&w->property("commandIndex").toInt()==index&&dynamic_cast<CommandEditor*>(w->widget()))owner=w;
         require(owner,"STM GUI command editor missing");owner->showNormal();owner->move(0,0);owner->resize(area->viewport()->size());area->setActiveSubWindow(owner);events();acceptanceView(*owner);return owner;
      };
      auto select=[&](QMdiSubWindow *owner,const QString &type,bool accept){
         auto *panel=dynamic_cast<CommandEditor*>(owner->widget());std::exception_ptr failure;
         QTimer::singleShot(0,panel,[&]{auto *dialog=panel->findChild<QDialog*>("reportParameterDialog");try{
            require(dialog,"STM GUI report picker missing");acceptanceView(*dialog);
            choose(dialog->findChild<QComboBox*>("reportPropertyObject"),"Sat");
            choose(dialog->findChild<QComboBox*>("reportPropertyType"),type);
            choose(dialog->findChild<QComboBox*>("reportPropertyDependency"),"ReviewFrame");
            click(dialog->findChild<QPushButton*>("reportUseReference"));
            auto *entry=dialog->findChild<QComboBox*>("reportParameterEntry");require(entry&&entry->currentText()=="Sat.ReviewFrame."+type,"STM GUI browser omitted coordinate qualification");
            click(dialog->findChild<QPushButton*>("reportParameterRemoveAll"));click(dialog->findChild<QPushButton*>("reportAddParameter"));
            auto *list=dialog->findChild<QListWidget*>("reportSelectedParameters");require(list&&list->count()==1&&list->item(0)->text()==entry->currentText(),"STM GUI selected list differs");
            auto *buttons=dialog->findChild<QDialogButtonBox*>();require(buttons,"STM GUI picker buttons missing");
            click(buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel));
         }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});
         click(panel->findChild<QPushButton*>("commandChoose_Parameters"));if(failure)std::rethrow_exception(failure);
      };
      QJsonArray transactions;QString current=initial;
      for(int k=0;k<5;++k){
         auto *owner=open("Select"+QString::number(k));auto *panel=dynamic_cast<CommandEditor*>(owner->widget());
         const int undo=source->document()->availableUndoSteps();select(owner,types[k],false);
         require(!panel->hasChanges()&&source->toPlainText()==current&&source->document()->availableUndoSteps()==undo,"STM GUI Cancel changed source/history/draft");
         select(owner,types[k],true);require(panel->hasChanges()&&source->toPlainText()==current&&source->document()->availableUndoSteps()==undo,"STM GUI selection escaped pending draft");
         auto *buttons=panel->findChild<QDialogButtonBox*>();require(buttons,"STM GUI command buttons missing");click(buttons->button(QDialogButtonBox::Apply));
         const auto prior=current;current.replace(" R"+QString::number(k)+" Sat."+types[k]+";"," R"+QString::number(k)+" Sat.ReviewFrame."+types[k]+";");
         panel=dynamic_cast<CommandEditor*>(owner->widget());require(panel&&!panel->hasChanges()&&source->toPlainText()==current,"STM GUI Apply changed unrelated source");
         require(owner->close(),"STM GUI clean editor did not close");events();
         source->undo();require(source->toPlainText()==prior&&window.buildScript(),"STM GUI exact Undo failed");
         source->redo();require(source->toPlainText()==current&&window.buildScript(),"STM GUI exact Redo failed");
         transactions.append(QJsonObject{{"type",types[k]},{"reference","Sat.ReviewFrame."+types[k]},{"cancel",true},{"pending",true},{"apply",true},{"undo",true},{"redo",true}});
      }
      const auto path=files.filePath("Selected STM Ω.script");require(window.saveScriptTo(path)&&window.loadScript(path)&&window.buildScript()&&source->toPlainText()==current,"STM GUI Unicode save/reopen changed source");
      save("mission.script",current.toUtf8());const int undo=source->document()->availableUndoSteps();const auto outcome=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());
      require(outcome==MainWindow::RunResult::Completed&&source->toPlainText()==current&&source->document()->availableUndoSteps()==undo,"STM GUI report-only mission failed or changed source/history");
      auto *sat=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("Sat"));auto *frame=dynamic_cast<CoordinateSystem*>(Moderator::Instance()->GetInternalObject("ReviewFrame"));
      require(sat&&frame&&frame->AreAxesOfType("ICRFAxes")&&Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"STM GUI runtime owner/frame/continuation differs");
      const auto &native=sat->GetRmatrixParameter("OrbitSTM");for(int i=0;i<6;++i)for(int j=0;j<6;++j)require(native(i,j)==(i==j?1.:0.),"STM GUI read mutated owner STM");
      auto *output=window.findChild<QTreeWidget*>("Output");require(output,"STM GUI Output tree missing");QJsonArray matrices;
      for(int k=0;k<5;++k){
         const auto bytes=read(reports[k]);save(types[k]+".txt",bytes);const auto words=QString::fromUtf8(bytes).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
         const int size=k?3:6,row=k?3*((k-1)/2):0,col=k?3*((k-1)%2):0;require(words.size()==size*size,"STM GUI report dimensions differ");QJsonArray values;
         for(int i=0;i<size;++i)for(int j=0;j<size;++j){bool valid=false;double v=words[size*i+j].toDouble(&valid);require(valid&&std::isfinite(v)&&std::abs(v-(i+row==j+col?1.:0.))<=1e-12,"STM GUI identity/partition independent expectation failed");values.append(v);}
         const auto found=output->findItems("R"+QString::number(k),Qt::MatchExactly|Qt::MatchRecursive);require(found.size()==1,"STM GUI output resource missing");output->itemDoubleClicked(found[0],0);events();
         QMdiSubWindow *viewer=nullptr;for(auto *w:area->subWindowList())if(auto *v=dynamic_cast<ReportViewer*>(w->widget())){auto *text=v->findChild<QPlainTextEdit*>();if(text&&text->toPlainText().trimmed()==QString::fromUtf8(bytes).trimmed()&&text->isReadOnly())viewer=w;}
         require(viewer,"STM GUI readonly viewer missing/wrong bytes");area->setActiveSubWindow(viewer);acceptanceView(*viewer);
         matrices.append(QJsonObject{{"type",types[k]},{"values",values},{"readonly_viewer",true}});
      }
      QJsonObject proof{{"checkpoint","stm-selected-frame-gui"},{"outcome","passed"},{"transactions",transactions},{"matrices",matrices},{"save_reopen",true},{"native_missions",1},{"native_propagations",0},{"owner_unchanged",true},{"runtime_frame","ICRF"},{"runtime_keep",42},{"scope","Five actual frame-qualified readonly STM selectors; Cancel/pending/Apply/Undo/Redo/Unicode persistence; fresh identity/partition reports and readonly viewers. No new propagation or whole-row qualification."}};
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;
   }catch(BaseException &e){std::cerr<<e.GetFullMessage();}catch(std::exception &e){std::cerr<<e.what();}return 1;
}
