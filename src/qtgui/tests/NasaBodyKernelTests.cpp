#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CelestialBodyPanel.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "CelestialBody.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static CelestialBody *body(const QString &name="Probe") {
   auto *p=dynamic_cast<CelestialBody *>(Moderator::Instance()->GetConfiguredObject(name.toStdString()));
   require(p,"Configured celestial body missing");return p;
}
static QStringList kernels() {
   QStringList values;for (const auto &s:body()->GetStringArrayParameter("OrbitSpiceKernelName")) values.append(QString::fromStdString(s));return values;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaBodyKernel");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and body kernel case");
      TestSettings settings;QTemporaryDir files;require(files.isValid(),"Body kernel fixture directory missing");
      const QString mode=argv[2];require(mode=="Defaults" || mode=="ListActions" || mode=="Validation","Unknown body kernel case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto shipped=QFileInfo("../data/planetary_ephem/spk/ceres_1900_2100.bsp").canonicalFilePath();require(!shipped.isEmpty(),"Shipped Ceres kernel missing");
      const auto a=files.filePath("A Ceres Δ.bsp"),b=files.filePath("B Ceres Δ.bsp"),c=files.filePath("C Ceres Δ.bsp"),stale=files.filePath("Stale Δ.bsp"),bad=files.filePath("Not SPK Δ.bsp");
      for (const auto &p:QStringList{a,b,c,stale}) require(QFile::copy(shipped,p),"Cannot create body kernel path fixture");
      {QFile f(bad);require(f.open(QIODevice::WriteOnly),"Cannot create invalid body kernel");f.write("Not a SPICE kernel\n");}
      const QString initial="% Body-kernel acceptance: preserve unrelated source Δ\nCreate Asteroid Probe;\nProbe.NAIFId = 2000001;\nProbe.OrbitSpiceKernelName = {'"+a+"'};\nCreate Planet UserPlanet;\nCreate Comet UserComet;\nCreate Moon UserMoon;\nCreate Variable Keep;\nKeep = 41; % preserve initial value\nBeginMissionSequence;\nKeep = Keep + 1; % preserve mission\n";
      MainWindow window;require(window.initialize(startup),"Body kernel runtime initialization failed");window.show();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source && tree && area,"Body kernel workspace missing");
      source->setPlainText(initial);require(window.buildScript(),"Body kernel baseline failed to build");
      CelestialBodyPanel *panel=nullptr;QMdiSubWindow *child=nullptr;QListWidget *list=nullptr;QJsonArray checks;int cancels=0,rejections=0;
      auto view=[&] {
         if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);child->showNormal();events();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();require(area->activeSubWindow()==child && area->subWindowList(QMdiArea::StackingOrder).back()==child,"Body kernel panel is obscured");}
         acceptanceView(*child);
      };
      auto open=[&](const QString &name="Probe") {
         const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Body resource tree entry missing");tree->itemDoubleClicked(items.front(),0);events();
         child=nullptr;for (auto *w:area->subWindowList()) if (w->property("resourceName").toString()==name && dynamic_cast<CelestialBodyPanel *>(w->widget())) child=w;
         require(child,"Actual celestial-body panel missing");panel=dynamic_cast<CelestialBodyPanel *>(child->widget());auto *tabs=panel->findChild<QTabWidget *>("bodyPages");require(tabs && tabs->count()==4,"Body settings pages missing");tabs->setCurrentIndex(1);events();list=panel->findChild<QListWidget *>("body_OrbitSpiceKernelName");require(!panel->hasChanges(),"Fresh celestial-body panel is dirty");view();
      };
      auto close=[&] {require(child->close(),"Clean body panel did not close");events();panel=nullptr;child=nullptr;list=nullptr;};
      auto pending=[&] {QStringList v;for (int i=0;i<list->count();++i) v.append(list->item(i)->text());return v;};
      auto action=[&](const QString &name) {auto *button=panel->findChild<QPushButton *>("body"+name+"_OrbitSpiceKernelName");require(button && button->isVisible() && button->isEnabled(),"Body kernel list action unavailable");button->click();events();};
      auto choose=[&](const QString &operation,const QString &path,bool accept=true) {
         std::exception_ptr error;bool visited=false;
         QTimer::singleShot(0,panel,[&] {
            auto *picker=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
            try {
               require(picker,"Actual body kernel file picker missing");visited=true;picker->setDirectory(QFileInfo(path).absolutePath());picker->selectFile(path);events();acceptanceView(*picker);
               auto *buttons=picker->findChild<QDialogButtonBox *>();auto *ok=buttons ? buttons->button(QDialogButtonBox::Open) : nullptr;
               QElapsedTimer wait;wait.start();while (ok && !ok->isEnabled() && wait.elapsed()<3000) QApplication::processEvents(QEventLoop::AllEvents,10);
               require(ok && ok->isEnabled() && picker->selectedFiles()==QStringList{path},"Body picker did not select exact Unicode path");buttons->button(accept ? QDialogButtonBox::Open : QDialogButtonBox::Cancel)->click();
            }catch (...) {error=std::current_exception();if (picker) picker->reject();}
         });action(operation);if (error) std::rethrow_exception(error);require(visited,"Body picker callback did not run");if (!accept) ++cancels;
      };
      auto apply=[&](const QStringList &wanted) {
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();
         require(kernels()==wanted,"Body Apply did not retain exact ordered kernel list");
         panel=dynamic_cast<CelestialBodyPanel *>(child->widget());require(panel && !panel->hasChanges(),"Body Apply did not refresh clean panel");
         panel->findChild<QTabWidget *>("bodyPages")->setCurrentIndex(1);list=panel->findChild<QListWidget *>("body_OrbitSpiceKernelName");view();
      };
      if (mode=="Defaults") {
         const QMap<QString,QString> defaults{{"Probe","Sun"},{"UserPlanet","Sun"},{"UserComet","Sun"},{"UserMoon","Earth"}};
         const auto undo=source->document()->availableUndoSteps();
         for (auto i=defaults.cbegin();i!=defaults.cend();++i) {
            open(i.key());auto *central=panel->findChild<QLineEdit *>("body_CentralBody");auto *src=panel->findChild<QComboBox *>("body_PosVelSource");auto *id=panel->findChild<QLineEdit *>("body_NAIFId");
            require(central && central->isVisible() && central->isReadOnly() && central->text()==i.value() && body(i.key())->GetStringParameter(body(i.key())->GetParameterID("CentralBody"))==i.value().toStdString(),"User-body central default/read-only state differs from current NASA GUI contract");
            require(src && src->isEnabled() && src->count()==1 && src->currentText()=="SPICE" && id && !id->isReadOnly() && list && list->isEnabled(),"User-body SPICE-only source/dependencies incorrect");
            const auto error=window.applyResourceChanges(i.key(),{{"CentralBody","Mars"}},initial);
            require(error.contains("read-only") && source->toPlainText()==initial && source->document()->availableUndoSteps()==undo,"GUI apply API bypassed inactive central-body field or changed history");
            checks.append(QJsonObject{{"body",i.key()},{"central",i.value()},{"source","SPICE"},{"central_read_only",true}});view();close();
         }
         open("Earth");auto *src=panel->findChild<QComboBox *>("body_PosVelSource");auto *central=panel->findChild<QLineEdit *>("body_CentralBody");
         require(src && !src->isEnabled() && central && central->isReadOnly(),"Built-in body incorrectly allows local ephemeris/origin editing");checks.append(QJsonObject{{"body","Earth"},{"local_source_disabled",true}});close();
         require(source->toPlainText()==initial && source->document()->availableUndoSteps()==undo,"Read-only/default exploration changed source/history");
      } else if (mode=="ListActions") {
         open();require(pending()==QStringList{a},"Initial body kernel list differs");const auto undo=source->document()->availableUndoSteps();
         choose("Add",b,false);require(pending()==QStringList{a} && !panel->hasChanges(),"Body Add Cancel changed pending list");
         choose("Add",b);choose("Add",b);require(pending()==QStringList{a,b},"Body Add failed ordered append or exact-path duplicate suppression");
         list->setCurrentRow(0);action("Up");require(pending()==QStringList{a,b},"Body Up at first row changed order");
         list->setCurrentRow(1);action("Down");require(pending()==QStringList{a,b},"Body Down at last row changed order");
         action("Up");require(pending()==QStringList{b,a} && list->currentRow()==0,"Body Up lost selection or changed wrong row");action("Down");require(pending()==QStringList{a,b} && list->currentRow()==1,"Body Down lost selection or changed wrong row");
         choose("Replace",c,false);require(pending()==QStringList{a,b},"Body Replace Cancel changed pending list");
         list->setCurrentRow(0);choose("Replace",c);require(pending()==QStringList{c,b},"Body Replace changed wrong row/order");
         list->setCurrentRow(1);action("Remove");require(pending()==QStringList{c} && panel->hasChanges() && source->toPlainText()==initial && kernels()==QStringList{a} && source->document()->availableUndoSteps()==undo,"Body list draft bypassed Apply or changed history");view();
         panel->findChild<QPushButton *>("cancelPanel")->click();events();require(source->toPlainText()==initial && kernels()==QStringList{a} && source->document()->availableUndoSteps()==undo,"Body panel Cancel committed draft list");++cancels;
         open();choose("Add",b);list->setCurrentRow(1);action("Up");apply({b,a});const auto applied=source->toPlainText();close();
         source->undo();require(source->toPlainText()==initial && window.buildScript() && kernels()==QStringList{a},"Body kernel Undo did not restore exact source/list");source->redo();require(source->toPlainText()==applied && window.buildScript() && kernels()==QStringList{b,a},"Body kernel Redo did not restore exact source/list");
         open();list->selectAll();action("Remove");require(pending().isEmpty(),"Body Remove selected failed to clear list");apply({});close();const auto empty=source->toPlainText();
         source->undo();require(source->toPlainText()==applied && window.buildScript() && kernels()==QStringList{b,a},"Empty body list Undo lost kernels");source->redo();require(source->toPlainText()==empty && window.buildScript() && kernels().isEmpty(),"Empty body list Redo did not retain explicit clear");
         const auto saved=files.filePath("Body kernels Δ.script");require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==empty && kernels().isEmpty(),"Body kernel Unicode save/reopen lost explicit empty list");
         checks=QJsonArray{"Add","Replace","Remove","Up","Down","edge-noop","duplicate-suppression","inner-Cancel","panel-Cancel","pending-isolation","ordered-Apply","empty-list","exact-Undo-Redo","Unicode-save-reopen"};
      } else {
         open();const auto undo=source->document()->availableUndoSteps();
         auto reject=[&](const QString &diagnostic) {
            panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();
            std::cout<<"BODY_REJECTION expected="<<diagnostic.toStdString()<<" same_panel="<<(panel==child->widget())<<" dirty="<<panel->hasChanges()<<" source_unchanged="<<(source->toPlainText()==initial)<<" history_unchanged="<<(source->document()->availableUndoSteps()==undo)<<" actual="<<panel->findChild<QLabel *>("bodyStatus")->text().toStdString()<<std::endl;
            require(panel==child->widget() && panel->hasChanges() && panel->findChild<QLabel *>("bodyStatus")->text().contains(diagnostic,Qt::CaseInsensitive) && source->toPlainText()==initial && kernels()==QStringList{a} && source->document()->availableUndoSteps()==undo,"Invalid body kernel Apply partially committed or lacks precise diagnostic");++rejections;view();
         };
         choose("Add",stale);require(QFile::remove(stale),"Cannot simulate selected file disappearing before Apply");reject("readable kernel");list->setCurrentRow(1);action("Remove");require(!panel->hasChanges(),"Removing stale pending file failed to restore clean state");
         list->setCurrentRow(0);choose("Replace",bad);reject("SPK");choose("Replace",a);require(!panel->hasChanges(),"Restoring valid body kernel failed to clear dirty state");
         auto *id=panel->findChild<QLineEdit *>("body_NAIFId");require(id && !id->isReadOnly(),"User-body NAIF input unavailable");id->setText("not-an-integer");reject("integer");id->setText("2000001");require(!panel->hasChanges(),"Correcting body NAIF failed to restore clean state");
         choose("Add",b);apply({a,b});require(source->toPlainText().contains("Keep = 41; % preserve initial value") && source->toPlainText().contains("Keep = Keep + 1; % preserve mission"),"Corrected body Apply changed unrelated source");close();checks=QJsonArray{"selected-file-disappears","invalid-SPK-type","invalid-NAIF","atomic-source-model-history","correction-and-Apply"};
      }
      const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if (!output.isEmpty()) {QFile f(QDir(output).filePath("source.script"));require(f.open(QIODevice::WriteOnly),"Cannot retain body kernel source");f.write(source->toPlainText().toUtf8());}
      const QJsonArray facets=mode=="Defaults" ? QJsonArray{"four-user-body-source-defaults","actual-body-panels","spice-only-choice","built-in-local-source-lock","four-user-body-central-defaults","visible-read-only-origin","api-read-only-rejection","source-history-isolation"} : mode=="ListActions" ? QJsonArray{"actual-file-pickers","five-list-actions","cancel","ordered-and-empty-apply","exact-undo-redo","unicode-save-reopen"} : QJsonArray{"actual-file-pickers","invalid-atomicity"};
      const QJsonObject proof{{"checkpoint","body-kernel-"+mode},{"facets",facets},{"outcome","passed"},{"checks",checks},{"cancels",cancels},{"invalid_rejections",rejections},{"scope","Configured celestial-body GUI source/origin/list validation and persistence only. No SPK propagation/state, native picker input, other platform, stress or current private NASA corpus qualification."}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
