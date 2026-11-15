#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "PropagationGroupsDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Propagate.hpp"
#include "BaseException.hpp"
extern "C" {
#include "SpiceUsr.h"
}
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok,const char *message) {if(!ok)throw std::runtime_error(message);}
void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();}
void write(const QString &path,const QByteArray &bytes) {QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Exact policy GUI evidence write failed");}
QByteArray read(const QString &path) {QFile f(path);require(f.open(QIODevice::ReadOnly),"Exact policy GUI saved file missing");return f.readAll();}
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("ExactEphemerisPolicy");
   try {
      require(argc==2&&acceptanceWidgetPlatform(),"Supply exact policy GUI startup");const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());TestSettings settings;QTemporaryDir files;require(files.isValid(),"Exact policy GUI private files unavailable");
      const auto kernel=files.filePath("affine Δ.bsp");SpiceInt handle;spkopn_c(kernel.toUtf8().constData(),"Exact policy GUI fixture",0,&handle);require(!failed_c(),"Exact policy GUI SPK open failed");SpiceDouble epochs[]={-120,0,120},states[3][6];for(int i=0;i<3;++i){const double t=epochs[i];states[i][0]=7000+t;states[i][1]=-100+2*t;states[i][2]=200-.5*t;states[i][3]=1;states[i][4]=2;states[i][5]=-.5;}spkw09_c(handle,-199993,399,"J2000",-120,120,"AFFINE EARTH",1,3,states,epochs);spkcls_c(handle);require(!failed_c(),"Exact policy GUI SPK write failed");
      MainWindow window;require(window.initialize(startup),"Exact policy GUI runtime unavailable");window.show();events();acceptanceView(window);auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *area=window.findChild<QMdiArea *>("workspace");auto *tree=window.findChild<QTreeWidget *>("Mission");require(source&&area&&tree,"Exact policy GUI workspace missing");
      QString accepted="% Exact count policy Ω\nCreate Spacecraft E1 E2;\nE1.DateFormat = TDBModJulian;\nE1.Epoch = '21545';\nE1.NAIFId = -199993;\nE1.OrbitSpiceKernelName = {'"+kernel+"'};\nCreate Propagator N A B;\nA.Type = SPK;\nA.StepSize = 10;\nA.StartEpoch = FromSpacecraft;\nB.Type = SPK;\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nPropagate 'Policy' A(E1) {EphemerisPolicy = 'SingleGroup'};\nKeep = Keep + 1;\n% preserve tail Ω\n";
      source->setPlainText(accepted);require(window.buildScript(),"Exact policy GUI baseline failed to build");
      auto open=[&] {auto *tabs=window.findChild<QTabWidget *>();for(int i=0;tabs&&i<tabs->count();++i)if(tabs->tabText(i)=="Mission")tabs->setCurrentIndex(i);QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->text(0).endsWith("Policy")){require(!item,"Exact policy tree label ambiguous");item=*it;}require(item,"Exact policy actual tree item missing");tree->expandAll();tree->setCurrentItem(item);tree->scrollToItem(item);events();tree->itemDoubleClicked(item,0);events();auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Exact policy actual editor missing");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();acceptanceView(*child);return child;};
      QString previous="SingleGroup";QJsonArray observations;int phase=0;
      for(const auto &policy:QStringList{"ExactlyOneGroup","ExactlyOneSetup"}) {
         auto *child=open();auto editor=[&]{auto *p=dynamic_cast<CommandEditor *>(child->widget());require(p,"Exact policy current editor missing");return p;};auto *preview=child->findChild<QPlainTextEdit *>("commandSource");require(preview,"Exact policy source preview missing");const auto draft=preview->toPlainText();
         for(bool accept:{false,true}) {
            std::exception_ptr failure;bool visited=false;QTimer::singleShot(0,child,[&] {auto *dialog=dynamic_cast<PropagationGroupsDialog *>(QApplication::activeModalWidget());try {
               require(dialog,"Exact policy actual groups dialog missing");visited=true;auto *combo=dialog->findChild<QComboBox *>("propagationEphemerisPolicy");auto *table=dialog->findChild<QTableWidget *>("propagationGroupsTable");auto *box=dialog->findChild<QDialogButtonBox *>();auto *error=dialog->findChild<QLabel *>("propagationGroupsError");require(combo&&table&&box&&error&&table->rowCount()==1,"Exact policy dialog controls missing");
               for(const auto &value:QStringList{"Native","SingleSetup","SingleGroup","ExactlyOneSetup","ExactlyOneGroup"})require(combo->findData(value)>=0,"Native count policy missing from GUI choices");require(combo->currentData().toString()==previous,"Exact policy dialog lost current selection");combo->setCurrentIndex(combo->findData(policy));events();require(box->button(QDialogButtonBox::Ok)->isEnabled(),"One ephemeris group rejected by exact policy GUI");
               auto *setup=qobject_cast<QComboBox *>(table->cellWidget(0,0));require(setup&&setup->findText("N")>=0&&setup->findText("A")>=0,"Exact policy actual setup row missing");setup->setCurrentText("N");events();require(!box->button(QDialogButtonBox::Ok)->isEnabled()&&error->text().contains("exactly one"),"Exact policy GUI accepted zero ephemeris groups");setup->setCurrentText("A");events();require(box->button(QDialogButtonBox::Ok)->isEnabled(),"Exact policy GUI failed to recover its valid group");acceptanceView(*dialog);if(accept)box->button(QDialogButtonBox::Ok)->click();else dialog->reject();
            }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});auto *choose=child->findChild<QPushButton *>("editPropagationGroups");require(choose,"Exact policy groups action missing");choose->click();if(failure)std::rethrow_exception(failure);require(visited,"Exact policy groups dialog unvisited");events();if(!accept)require(preview->toPlainText()==draft&&source->toPlainText()==accepted&&!editor()->hasChanges(),"Exact policy Cancel changed source or draft");
         }
         const auto before=accepted;accepted.replace("EphemerisPolicy = '"+previous+"'","EphemerisPolicy = '"+policy+"'");require(source->toPlainText()==before&&editor()->hasChanges(),"Exact policy selection escaped pending state");auto apply=[&]{auto *box=child->findChild<QDialogButtonBox *>();auto *button=box?box->button(QDialogButtonBox::Apply):nullptr;require(button,"Exact policy Apply missing");button->click();events();};apply();require(source->toPlainText()==accepted&&!editor()->hasChanges(),"Exact policy Apply changed unrelated text or lost quotes");const int undo=source->document()->availableUndoSteps();apply();require(source->toPlainText()==accepted&&source->document()->availableUndoSteps()==undo,"Exact policy repeated Apply changed history");require(child->close(),"Exact policy clean editor would not close");events();
         source->undo();require(source->toPlainText()==before&&window.buildScript(),"Exact policy Undo lost exact source/model");source->redo();require(source->toPlainText()==accepted&&window.buildScript(),"Exact policy Redo lost exact source/model");const auto path=files.filePath(QString::number(phase)+" policy Δ.script"),copy=files.filePath(QString::number(phase)+" policy copy ü.script");require(window.saveScriptTo(path)&&read(path)==accepted.toUtf8()&&window.saveScriptTo(copy)&&read(copy)==accepted.toUtf8()&&window.loadScript(copy)&&window.buildScript()&&source->toPlainText()==accepted,"Exact policy Unicode save/reopen changed source");
         const int runUndo=source->document()->availableUndoSteps();require(window.runMission()==MainWindow::RunResult::Completed,"Exact policy GUI SPK mission failed");Propagate *native=nullptr;for(auto *c=Moderator::Instance()->GetFirstCommand();c;c=c->GetNext())if((native=dynamic_cast<Propagate *>(c)))break;require(native&&native->GetStringParameter(native->GetParameterID("EphemerisPolicy"))==policy.toStdString()&&Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Exact policy GUI mission lost native mode or continuation");require(source->toPlainText()==accepted&&source->document()->availableUndoSteps()==runUndo,"Exact policy GUI Run changed source/history");
         auto *reopened=open();require(reopened->findChild<QPlainTextEdit *>("commandSource")->toPlainText().contains(policy),"Exact policy reopened editor lost mode");require(reopened->close(),"Exact policy reopened editor would not close");events();observations.append(QJsonObject{{"policy",policy},{"zero_rejected_in_GUI",true},{"source_history_quotes_unicode",true},{"native_SPK_completed",true}});const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!evidence.isEmpty())write(QDir(evidence).filePath(QString::number(phase)+"-accepted.script"),accepted.toUtf8());previous=policy;++phase;
      }
      QJsonObject proof{{"checkpoint","exact-ephemeris-policy-GUI"},{"outcome","passed"},{"observations",observations},{"scope","Two exact policies through actual groups dialog validation/Cancel/Apply/history/Unicode/native SPK mission. Independent affine numerical checks are separate native cases; all provider/science/capacity/platform conjunctions remain open."}};const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!evidence.isEmpty())write(QDir(evidence).filePath("proof.json"),QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<e.what()<<std::endl;}return 1;
}
