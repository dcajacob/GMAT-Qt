#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "PropagationGroupsDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatCommand.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
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
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <iostream>
#include <functional>
#include <stdexcept>
static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaPropagationModifier");
   try {
      require(argc==2 && acceptanceWidgetPlatform(),"Expected startup for modifier-order case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Modifier fixture directory missing");const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;require(window.initialize(startup),"Modifier test runtime initialization failed");window.show();
      const QString command="Propagate 'Modifier parsing' Synchronized BackProp Numeric(Num) Read(Eph); % retain modifier spelling";
      const QString initial="% Incomplete SPK setup is intentional: configured-editor validation only\nCreate Spacecraft Num Eph;\nCreate Propagator Numeric Read;\nRead.Type = SPK;\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\n"+command+"\nKeep = Keep + 1;\n";
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source && tree && area,"Modifier workspace missing");source->setPlainText(initial);require(window.buildScript(),"Core parser did not accept the documented Synchronized BackProp order");
      int index=-1;const auto snapshot=window.missionSnapshot();for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="Propagate") index=i;require(index>=0,"Modifier mission node missing");QTreeWidgetItemIterator item(tree);while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item;require(*item,"Modifier mission tree entry missing");tree->itemDoubleClicked(*item,0);events();QMdiSubWindow *child=nullptr;for (auto *c:area->subWindowList()) if (c->property("commandIndex").isValid() && c->property("commandIndex").toInt()==index && dynamic_cast<CommandEditor *>(c->widget())) child=c;require(child,"Actual modifier command editor missing");auto *panel=dynamic_cast<CommandEditor *>(child->widget());int viewingChecks=0;auto view=[&] {
         if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);child->showNormal();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();child->move(0,0);child->resize(area->viewport()->size());events();std::cout<<"MODIFIER_VIEW active="<<(area->activeSubWindow()==child)<<" top="<<(area->subWindowList(QMdiArea::StackingOrder).back()==child)<<" child="<<child->x()<<","<<child->y()<<","<<child->width()<<","<<child->height()<<" viewport="<<area->viewport()->width()<<","<<area->viewport()->height()<<std::endl;require(area->activeSubWindow()==child && area->subWindowList(QMdiArea::StackingOrder).back()==child && area->viewport()->rect().contains(child->geometry()),"Modifier command panel is obscured or exceeds the workspace");++viewingChecks;}
         acceptanceView(*child);
      };view();
      auto *groups=panel->findChild<QPushButton *>("editPropagationGroups");require(groups && groups->isVisible() && groups->isEnabled(),"Documented alternate modifier order hides assignment controls");
      auto dialog=[&](const std::function<void(PropagationGroupsDialog *)> &check) {std::exception_ptr error;QTimer::singleShot(0,panel,[&] {auto *dialog=dynamic_cast<PropagationGroupsDialog *>(panel->findChild<QDialog *>("propagationGroupsDialog"));try {require(dialog,"Actual alternate-order assignment dialog missing");check(dialog);}catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});groups->click();if (error) std::rethrow_exception(error);};
      const auto undo=source->document()->availableUndoSteps();dialog([&](PropagationGroupsDialog *d) {require(d->findChild<QCheckBox *>("propagationGroupBackwards")->isChecked() && d->findChild<QComboBox *>("propagationGroupMode")->currentData().toString()=="Synchronized" && d->statement()==command,"Alternate modifier parse or unchanged statement preservation failed");acceptanceView(*d);d->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();});require(!panel->hasChanges() && source->toPlainText()==initial && source->document()->availableUndoSteps()==undo,"Unchanged alternate-order OK modified pending/source/history");
      QJsonArray trials;auto *pending=panel->findChild<QPlainTextEdit *>("commandSource");auto *status=panel->findChild<QLabel *>("commandStatus");require(pending && status,"Modifier raw command/status fields missing");
      for (const auto &order:QStringList{"Synchronized BackProp","BackProp Synchronized"}) for (const auto &flag:QStringList{"STM","AMatrix","Covariance"}) {
         const auto bad=QString("Propagate 'Modifier parsing' ")+order+" Numeric(Num, '"+flag+"') Read(Eph); % retain modifier spelling";pending->setPlainText(bad);events();require(groups->isVisible() && groups->isEnabled(),"Legal modifier order lost group controls after raw flag edit");
         dialog([&](PropagationGroupsDialog *d) {require(!d->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled() && d->findChild<QLabel *>("propagationGroupsError")->text().contains("SPK"),"Alternate-order group view bypassed configured SPK flag validation");acceptanceView(*d);d->reject();});
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();require(panel==child->widget() && panel->hasChanges() && source->toPlainText()==initial && source->document()->availableUndoSteps()==undo && status->text().contains("SPK"),"Alternate-order raw Apply bypassed configured capability validation or changed source/history");trials.append(QJsonObject{{"order",order},{"flag",flag},{"outcome","rejected"}});view();
      }
      pending->setPlainText(command);require(!panel->hasChanges(),"Restoring exact alternate command did not clear dirty state");dialog([&](PropagationGroupsDialog *d) {d->findChild<QCheckBox *>("propagationGroupBackwards")->setChecked(false);require(d->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Corrected alternate-order settings remain invalid");d->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();});
      const QString corrected="Propagate 'Modifier parsing' Synchronized Numeric(Num) Read(Eph) ; % retain modifier spelling";require(pending->toPlainText()==corrected && panel->hasChanges() && source->toPlainText()==initial,"Actual modifier edit did not produce the expected pending statement");panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();require(source->toPlainText()==QString(initial).replace(command,corrected),"Corrected alternate-order Apply changed unrelated source");auto *fresh=dynamic_cast<CommandEditor *>(child->widget());require(fresh && !fresh->hasChanges(),"Corrected modifier Apply did not refresh clean panel");view();require(child->close(),"Clean modifier panel failed to close");events();const auto applied=source->toPlainText();source->undo();require(source->toPlainText()==initial && window.buildScript(),"Applied modifier Undo did not restore exact alternate spelling/model");source->redo();require(source->toPlainText()==applied && window.buildScript(),"Applied modifier Redo did not restore exact corrected source/model");const auto saved=files.filePath("Modifiers Δ.script");require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==applied,"Modifier Unicode save/reopen changed accepted source");
      const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if (!output.isEmpty()) {QFile file(QDir(output).filePath("source.script"));require(file.open(QIODevice::WriteOnly),"Cannot retain modifier source");file.write(source->toPlainText().toUtf8());}
      const QJsonObject proof{{"checkpoint","spk-modifier-order"},{"outcome","passed"},{"invalid_rejections",6},{"explicit_frontmost_views",viewingChecks},{"trials",trials},{"facets",QJsonArray{"documented-alternate-modifier-order","actual-group-dialog","unchanged-exact-source-preservation","both-order-configured-flag-rejection","raw-apply-source-history-isolation","corrected-apply","exact-undo-redo","unicode-save-reopen"}},{"scope","Configured-editor parser/preservation/capability case only. No mission runs, SPK interpolation, time/frame science, native OS input or platform/stress qualification."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
