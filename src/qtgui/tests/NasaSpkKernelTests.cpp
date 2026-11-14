#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
extern "C" {
#include "SpiceUsr.h"
}
#include <QApplication>
#include <QAbstractItemModel>
#include <QDialog>
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
#include <QRegularExpression>
#include <QScrollBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static std::array<double,6> trajectory(int which,double t)
{
   return which==0 ? std::array<double,6>{7000+t,-100+2*t,200-.5*t,1,2,-.5} : std::array<double,6>{-1500-3*t,6000+.25*t,-300+1.5*t,-3,.25,1.5};
}
static void spiceCheck() {if (failed_c()) {char message[1841];getmsg_c("LONG",sizeof(message),message);reset_c();throw std::runtime_error(message);}}
static void fixture(const QString &path,int which)
{
   SpiceDouble epochs[]={0,170,900},states[3][6];for (int i=0;i<3;++i) {const auto value=trajectory(which,epochs[i]);for (int j=0;j<6;++j) states[i][j]=value[j];}
   SpiceInt handle;spkopn_c(path.toUtf8().constData(),"GMAT file selection fixture",0,&handle);spiceCheck();spkw09_c(handle,-100078,399,"J2000",0,900,which==0 ? "AFFINE A" : "AFFINE B",1,3,states,epochs);spiceCheck();spkcls_c(handle);spiceCheck();
}
static GmatBase *vehicle() {auto *p=Moderator::Instance()->GetConfiguredObject("Vehicle");require(p,"Kernel test spacecraft missing");return p;}
static QStringList kernels() {QStringList result;for (const auto &name:vehicle()->GetStringArrayParameter("OrbitSpiceKernelName")) result.append(QString::fromStdString(name));return result;}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSpkKernel");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and kernel selection case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Kernel fixture directory missing");const QString mode=QString::fromLocal8Bit(argv[2]);require(mode=="Swap" || mode=="Priority" || mode=="Invalid" || mode=="PickerCancel","Unknown SPK kernel case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;require(window.initialize(startup),"Kernel test runtime initialization failed");window.show();
      const auto a=files.filePath("Kernel A Δ.bsp"),b=files.filePath("Kernel B Δ.bsp"),bad=files.filePath("Not SPK Δ.bsp"),report=files.filePath("states.txt");fixture(a,0);fixture(b,1);{QFile text(bad);require(text.open(QIODevice::WriteOnly),"Invalid fixture creation failed");text.write("This is deliberately not a SPICE kernel.\n");}
      const auto destination=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if (!destination.isEmpty()) for (const auto &path:QStringList{a,b,bad}) require(QFile::copy(path,QDir(destination).filePath(QFileInfo(path).fileName())),"Cannot retain kernel fixture");
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *area=window.findChild<QMdiArea *>("workspace");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");require(source && area && tree && messages,"Kernel test workspace missing");
      const QString baseline="% File selection must choose the actual trajectory\nCreate Spacecraft Vehicle;\nVehicle.NAIFId = -100078;\nVehicle.DateFormat = TDBModJulian;\nVehicle.Epoch = '21545.0027777777777778';\nVehicle.OrbitSpiceKernelName = {'"+a+"'};\nCreate Propagator Read;\nRead.Type = SPK;\nRead.StepSize = 17;\nRead.StartEpoch = 'FromSpacecraft';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 17;\nValues.WriteHeaders = false;\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nPropagate Read(Vehicle) {Vehicle.ElapsedSecs = 73};\nReport Values Vehicle.TDBModJulian Vehicle.ElapsedSecs Vehicle.EarthICRF.X Vehicle.EarthICRF.Y Vehicle.EarthICRF.Z Vehicle.EarthICRF.VX Vehicle.EarthICRF.VY Vehicle.EarthICRF.VZ;\nKeep = Keep + 1;\n";
      source->setPlainText(baseline);require(window.buildScript(),"Kernel selection baseline failed to build");ResourceEditor *panel=nullptr;QMdiSubWindow *child=nullptr;QTableWidget *table=nullptr;int phase=0,invalid=0,chooserCancels=0;QJsonArray samples;QByteArray lastReport;double positionError=0,velocityError=0;
      auto save=[&](const QString &name,const QByteArray &data) {if (destination.isEmpty()) return;QFile file(QDir(destination).filePath(name));require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Cannot retain kernel test phase");};
      auto open=[&](bool trigger) {
         if (trigger) {const auto items=tree->findItems("Vehicle",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Kernel spacecraft resource entry missing");tree->itemDoubleClicked(items.front(),0);events();}
         child=nullptr;for (auto *c:area->subWindowList()) if (c->property("resourceName").toString()=="Vehicle" && dynamic_cast<ResourceEditor *>(c->widget())) child=c;require(child,"Actual spacecraft resource editor missing");panel=dynamic_cast<ResourceEditor *>(child->widget());child->showNormal();area->setActiveSubWindow(child);
         auto *tabs=panel->findChild<QTabWidget *>("resourceEditorTabs");require(tabs,"Spacecraft settings tabs missing");int advanced=-1;for (int i=0;i<tabs->count();++i) if (tabs->tabText(i)=="Advanced") advanced=i;require(advanced>=0,"Spacecraft SPICE settings inaccessible");tabs->setCurrentIndex(advanced);auto *sections=panel->findChild<QTabBar *>("propertySections");require(sections,"Spacecraft property sections missing");int spice=-1;for (int i=0;i<sections->count();++i) if (sections->tabText(i)=="SPICE") spice=i;require(spice>=0,"Spacecraft SPICE section missing");sections->setCurrentIndex(spice);events();table=panel->findChild<QTableWidget *>("resourceProperties");require(table && !panel->hasChanges(),"Fresh spacecraft kernel panel is dirty/missing");acceptanceView(*child);
      };
      auto listEdit=[&](const QStringList &wanted,bool accept,bool duplicate,bool reorderOnly=false,bool cancelOnly=false) {
         auto *button=panel->findChild<QPushButton *>("chooseProperty_OrbitSpiceKernelName");require(button && button->isVisible() && button->isEnabled(),"Actual spacecraft kernel selector unavailable");std::exception_ptr error;
         QTimer::singleShot(0,panel,[&] {auto *dialog=panel->findChild<QDialog *>("kernelFileDialog");try {
            require(dialog,"Kernel list dialog missing");auto *list=dialog->findChild<QListWidget *>("kernelFileList");auto *remove=dialog->findChild<QPushButton *>("kernelFileRemove");auto *add=dialog->findChild<QPushButton *>("kernelFileAdd");require(list && remove && add,"Kernel list actions missing");acceptanceView(*dialog);
            if (reorderOnly) {require(list->count()==2 && wanted.size()==2,"Kernel precedence reorder requires two files");auto *last=list->takeItem(1);list->insertItem(0,last);}
            else {
               if (!cancelOnly) {list->selectAll();remove->click();require(list->count()==0,"Kernel Remove selected did not clear pending list");}
               auto choose=[&](const QString &path,bool acceptFile=true) {std::exception_ptr pickerError;QTimer::singleShot(0,dialog,[&] {auto *picker=dialog->findChild<QFileDialog *>("kernelFilePicker");try {require(picker && picker->fileMode()==QFileDialog::ExistingFiles,"Kernel file picker is not an existing-file selector");picker->setDirectory(QFileInfo(path).absolutePath());picker->selectFile(path);events();acceptanceView(*picker);auto *buttons=picker->findChild<QDialogButtonBox *>();auto *ok=buttons ? buttons->button(QDialogButtonBox::Open) : nullptr;QElapsedTimer wait;wait.start();while (ok && !ok->isEnabled() && wait.elapsed()<3000) QApplication::processEvents(QEventLoop::AllEvents,10);require(ok && ok->isEnabled() && picker->selectedFiles()==QStringList{path},"Kernel picker did not select the requested Unicode existing path");buttons->button(acceptFile ? QDialogButtonBox::Open : QDialogButtonBox::Cancel)->click();}catch (...) {pickerError=std::current_exception();if (picker) picker->reject();}});add->click();if (pickerError) std::rethrow_exception(pickerError);};
               if (cancelOnly) {choose(b,false);choose(bad,false);chooserCancels+=2;}
               else {for (const auto &path:wanted) choose(path);if (duplicate && !wanted.isEmpty()) choose(wanted.back());}
            }
            QStringList actual;for (int i=0;i<list->count();++i) actual.append(list->item(i)->text());require(actual==wanted,"Kernel picker/order/deduplication changed the requested list");acceptanceView(*dialog);dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
         }catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});button->click();if (error) std::rethrow_exception(error);
      };
      auto apply=[&](const QStringList &wanted) {panel->requestApply();events();open(false);require(kernels()==wanted,"Kernel Apply did not retain the exact ordered configured list");};
      auto close=[&] {require(child->close(),"Clean spacecraft kernel editor did not close");events();panel=nullptr;child=nullptr;};
      auto run=[&](int expected,const QString &label,const QString &failure=QString()) {
         messages->clear();const auto result=window.runMission();events();save(QString::number(phase)+"-"+label+".script",source->toPlainText().toUtf8());save(QString::number(phase)+"-"+label+"-messages.txt",messages->toPlainText().toUtf8());
         if (!failure.isEmpty()) {require(result!=MainWindow::RunResult::Completed && messages->toPlainText().contains(failure),"Invalid kernel/NAIF runtime did not report the required failure");++invalid;++phase;return;}
         if (result!=MainWindow::RunResult::Completed) {std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Kernel selection mission failed");}
         QFile data(report);require(data.open(QIODevice::ReadOnly),"Selected-kernel report missing");const auto raw=data.readAll();lastReport=raw;save(QString::number(phase)+"-"+label+"-states.txt",raw);const auto words=QString::fromUtf8(raw).trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==8,"Selected-kernel report has wrong field count");std::array<double,8> values{};QJsonArray actual;
         for (int i=0;i<8;++i) {bool valid=false;values[i]=words[i].toDouble(&valid);require(valid && std::isfinite(values[i]),"Kernel state report nonnumeric/nonfinite");actual.append(values[i]);}
         const auto time=(values[0]-21545)*86400;require(std::abs(time-313)<=1e-5 && std::abs(values[1]-73)<=1e-5,"Selected-kernel arc clock differs from the requested start/stop");const auto truth=trajectory(expected,time);
         for (int i=0;i<6;++i) {const double error=std::abs(values[i+2]-truth[i]);if (i<3) positionError=std::max(positionError,error);else velocityError=std::max(velocityError,error);require(error<=(i<3 ? 1e-5 : 1e-8),"Selected kernel/order returned a stale or incorrect independent affine trajectory");}
         require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Selected-kernel run did not continue to the next command");samples.append(QJsonObject{{"phase",phase},{"label",label},{"kernel_index",expected},{"values",actual},{"configured_kernels",QJsonArray::fromStringList(kernels())}});++phase;
      };
      if (mode=="PickerCancel") {
         open(true);const auto undo=source->document()->availableUndoSteps();listEdit({a},true,false,false,true);require(!panel->hasChanges() && kernels()==QStringList{a} && source->toPlainText()==baseline && source->document()->availableUndoSteps()==undo,"Inner file picker Cancel added a valid/invalid file or changed source/model/history");acceptanceView(*child);close();
      } else if (mode=="Swap") {
         run(0,"initial-A");open(true);const auto undo=source->document()->availableUndoSteps();listEdit({b},false,false);++chooserCancels;require(!panel->hasChanges() && kernels()==QStringList{a} && source->toPlainText()==baseline && source->document()->availableUndoSteps()==undo,"Kernel list Cancel changed pending/model/source/history");listEdit({b},true,true);require(panel->hasChanges() && kernels()==QStringList{a} && source->toPlainText()==baseline,"Kernel dialog OK bypassed resource Apply");panel->findChild<QPushButton *>("cancelPanel")->click();events();require(kernels()==QStringList{a} && source->toPlainText()==baseline && source->document()->availableUndoSteps()==undo,"Spacecraft panel Cancel committed the selected kernel");
         open(true);listEdit({b},true,false);apply({b});const auto changed=source->toPlainText();close();run(1,"selected-B");open(true);listEdit({a},true,false);apply({a});const auto restored=source->toPlainText();close();run(0,"restored-A");source->undo();require(source->toPlainText()==changed && window.buildScript() && kernels()==QStringList{b},"Kernel-list Undo did not restore exact source/list");source->redo();require(source->toPlainText()==restored && window.buildScript() && kernels()==QStringList{a},"Kernel-list Redo did not restore exact source/list");const auto saved=files.filePath("Kernel selection Δ.script");require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==restored && kernels()==QStringList{a},"Kernel selection did not survive exact Unicode save/reopen");
      } else if (mode=="Priority") {
         open(true);listEdit({a,b},true,true);apply({a,b});close();run(1,"A-then-B");open(true);listEdit({b,a},true,false,true);apply({b,a});close();run(0,"B-then-A");open(true);listEdit({a,b},true,false,true);apply({a,b});close();run(1,"A-then-B-again");
      } else {
         open(true);const auto applied=source->toPlainText();const auto undo=source->document()->availableUndoSteps();listEdit({bad},true,false);panel->requestApply();events();require(panel==child->widget() && panel->hasChanges() && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && source->toPlainText()==applied && kernels()==QStringList{a} && source->document()->availableUndoSteps()==undo,"Invalid SPK file type partially committed or lacked a diagnostic");++invalid;
         listEdit({},true,false);apply({});close();run(0,"empty-list","at least one orbit SPICE kernel");open(true);listEdit({a},true,false);apply({a});int id=-1;for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="NAIFId") id=row;require(id>=0 && !table->isRowHidden(id),"Spacecraft NAIF ID control missing/hidden");table->item(id,1)->setText("-100079");panel->requestApply();events();open(false);require(vehicle()->GetIntegerParameter("NAIFId")==-100079,"Typed NAIF ID did not apply");close();run(0,"missing-naif-id","NAIF ID -100079");open(true);id=-1;for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="NAIFId") id=row;require(id>=0,"Recovery NAIF field missing");table->item(id,1)->setText("-100078");panel->requestApply();events();open(false);close();run(0,"recovered-A");
      }
      // Show the final checked data through the application's actual report route.
      if (mode!="PickerCancel") {
      auto *output=window.findChild<QTreeWidget *>("Output");const auto items=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Selected kernel output entry missing");output->itemDoubleClicked(items.front(),0);events();QMdiSubWindow *reportWindow=nullptr;for (auto *c:area->subWindowList()) if (dynamic_cast<ReportViewer *>(c->widget())) reportWindow=c;require(reportWindow,"Actual selected-kernel report did not open");reportWindow->showNormal();reportWindow->move(0,0);reportWindow->resize(area->viewport()->size());area->setActiveSubWindow(reportWindow);events();acceptanceView(*reportWindow);auto *shown=reportWindow->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(lastReport).trimmed(),"Kernel report does not show the exact last independently checked state");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*reportWindow);
      }
      save("final.script",source->toPlainText().toUtf8());
      const QJsonObject proof{{"checkpoint","spk-kernel."+mode},{"outcome","passed"},{"samples",samples},{"invalid_rejections",invalid},{"chooser_cancels",chooserCancels},{"max_position_error_km",positionError},{"max_velocity_error_km_per_s",velocityError},
         {"facets",mode=="Swap" ? QJsonArray{"actual-spacecraft-kernel-file-pickers","unicode-paths","duplicate-suppression","two-level-cancel-isolation","pending-apply","same-id-file-swap-back","independent-affine-six-state","exact-undo-redo","unicode-save-reopen"} : mode=="Priority" ? QJsonArray{"actual-spacecraft-kernel-file-pickers","duplicate-suppression","pending-list-order","last-loaded-kernel-precedence","same-id-cache-replacement","independent-affine-six-state"} : mode=="PickerCancel" ? QJsonArray{"actual-spacecraft-kernel-file-pickers","valid-selection-inner-cancel","invalid-type-selection-inner-cancel","kernel-list-ok-without-change","source-model-history-isolation"} : QJsonArray{"actual-spacecraft-kernel-file-pickers","invalid-file-type-atomicity","empty-list-runtime-diagnostic","missing-naif-runtime-diagnostic","corrected-id-recovery","independent-affine-six-state"}},
         {"scope","Two synthetic same-ID Earth-relative ICRF affine Type9 kernels. In-process list model reorder is not native drag/drop qualification; general SPK/frame/time, multi-spacecraft, Origin, platform and stress remain open."}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
