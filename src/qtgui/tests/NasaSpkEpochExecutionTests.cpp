#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "BaseException.hpp"
extern "C" {
#include "SpiceUsr.h"
}
#include <QAction>
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
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static void write(const QString &p,const QByteArray &b) {QFile f(p);require(f.open(QIODevice::WriteOnly)&&f.write(b)==b.size(),"Epoch execution evidence write failed");}
static QByteArray read(const QString &p) {QFile f(p);require(f.open(QIODevice::ReadOnly),"Epoch execution artifact missing");return f.readAll();}
static std::array<double,6> trajectory(double t) {return {7000+1.2*t+.003*t*t+1e-6*t*t*t,-2000+4.5*t-.002*t*t+5e-7*t*t*t,500-.7*t+.0004*t*t-2e-7*t*t*t,1.2+.006*t+3e-6*t*t,4.5-.004*t+1.5e-6*t*t,-.7+.0008*t-6e-7*t*t};}
static void spiceCheck() {if (failed_c()) {char message[1841];getmsg_c("LONG",sizeof(message),message);reset_c();throw std::runtime_error(message);}}
static void fixture(const QString &path,double center) {
   const double offsets[]={-1200,-800,-360,0,170,515,920,1400,1800};SpiceDouble epochs[9],states[9][6];
   for (int i=0;i<9;++i) {epochs[i]=center+offsets[i];const auto s=trajectory(offsets[i]);for (int j=0;j<6;++j) states[i][j]=s[j];}
   SpiceInt handle;spkopn_c(path.toUtf8().constData(),"Independent epoch-format polynomial",0,&handle);spiceCheck();spkw13_c(handle,-100055,399,"J2000",epochs[0],epochs[8],"NAIF EPOCH FORMAT EXECUTION",7,9,states,epochs);spiceCheck();spkcls_c(handle);spiceCheck();
}
static void reveal(QWidget *w) {require(w,"Epoch execution control absent");for (auto *p=w;p;p=p->parentWidget()) {for (auto *tabs:w->window()->findChildren<QTabWidget *>()) if (tabs->indexOf(p)>=0) tabs->setCurrentWidget(p);if (auto *scroll=qobject_cast<QScrollArea *>(p)) scroll->ensureWidgetVisible(w);}events();require(w->isVisible()&&w->isEnabled(),"Epoch execution control unreachable");}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSpkEpochExecution");
   try {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply startup,epoch matrix and scenario");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Epoch execution temporary directory missing");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();const auto matrix=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();const QString scenario=argv[3];QJsonObject spec;for (auto value:matrix["cases"].toArray()) if (value.toObject()["key"]==scenario) spec=value.toObject();require(!spec.isEmpty(),"Unknown epoch execution scenario");
      QDir::setCurrent(QFileInfo(startup).absolutePath());const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes) {if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes);};
      MainWindow window;require(window.initialize(startup),"Epoch execution initialization failed");window.show();events();acceptanceView(window);
      const auto kernel=files.filePath("Epoch polynomial Δ.bsp");const double center=spec["center_et"].toDouble();fixture(kernel,center);save("polynomial.bsp",read(kernel));save("scenario.json",QJsonDocument(spec).toJson());
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Epoch execution workspace absent");
      QJsonArray observations;double maxEpoch=0,maxPosition=0,maxVelocity=0;int applies=0,missions=0,reportRows=0;QJsonArray formats;
      for (auto value:spec["inputs"].toArray()) {
         const auto input=value.toObject();const QString key=input["key"].toString(),format=input["format"].toString(),epoch=input["start_epoch"].toString();const auto report=files.filePath(key+" epoch Δ.csv"),saved=files.filePath(key+" epoch Ω.script");
         QString initial="% Independent epoch input / cubic interpolation witness Δ\nCreate Spacecraft Vehicle;\nVehicle.NAIFId = -100055;\nVehicle.DateFormat = TDBModJulian;\nVehicle.Epoch = '"+spec["spacecraft_tdb_mjd"].toString()+"';\nVehicle.OrbitSpiceKernelName = {'"+kernel+"'};\nCreate Propagator Read;\nRead.Type = SPK;\nRead.StepSize = 37;\nRead.EpochFormat = TDBModJulian;\nRead.StartEpoch = 'FromSpacecraft';\nCreate Variable Keep;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 17;\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nBeginMissionSequence;\n";
         int previous=0;for (int stop:{97,301,557}) {initial+="Propagate Read(Vehicle) {Vehicle.ElapsedSecs = "+QString::number(stop-previous)+"};\nReport Values Vehicle.TDBModJulian Vehicle.ElapsedSecs Vehicle.EarthICRF.X Vehicle.EarthICRF.Y Vehicle.EarthICRF.Z Vehicle.EarthICRF.VX Vehicle.EarthICRF.VY Vehicle.EarthICRF.VZ;\n";previous=stop;}initial+="Keep = Keep + 1;\n";
         source->setPlainText(initial);save(key+"-initial.script",initial.toUtf8());if (!window.buildScript()) {save(key+"-build-messages.txt",messages->toPlainText().toUtf8());std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Epoch execution initial Build failed");}
         QPointer<QMdiSubWindow> child;ResourceEditor *panel=nullptr;
         auto view=[&] {child->showNormal();child->setGeometry(area->viewport()->rect().adjusted(4,4,-4,-4));area->setActiveSubWindow(child);child->raise();events();acceptanceView(*child);};
         auto open=[&] {for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->tabText(i)=="Resources") tabs->setCurrentIndex(i);auto items=tree->findItems("Read",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Epoch execution resource tree identity missing");tree->itemDoubleClicked(items.first(),0);events();child=area->activeSubWindow();panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(panel&&!panel->hasChanges(),"Actual epoch resource editor missing/dirty");view();};
         auto configured=[&] {auto *p=dynamic_cast<PropSetup *>(Moderator::Instance()->GetConfiguredObject("Read"));require(p&&p->GetPropagator(),"Epoch configured propagator missing");return p->GetPropagator();};
         open();auto *formatControl=panel->findChild<QComboBox *>("resource_EpochFormat"),*epochControl=panel->findChild<QComboBox *>("resource_StartEpoch");auto *step=panel->findChild<QLineEdit *>("resource_StepSize");require(formatControl&&epochControl&&step&&epochControl->isEditable(),"Actual epoch execution controls missing");
         const auto before=source->toPlainText();const auto undo=source->document()->availableUndoSteps();reveal(formatControl);require(formatControl->findText(format)>=0,"Epoch format absent from real selector");formatControl->setCurrentText(format);reveal(epochControl);epochControl->setCurrentText(epoch);reveal(step);step->setText("23");view();
         require(panel->hasChanges()&&source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&configured()->GetStringParameter("StartEpoch")=="FromSpacecraft"&&configured()->GetRealParameter("StepSize")==37,"Epoch/format draft escaped Apply");
         auto *apply=panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);reveal(apply);QTest::mouseClick(apply,Qt::LeftButton);events();panel=dynamic_cast<ResourceEditor *>(child->widget());if (panel&&panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("resourceStatus")->text().toStdString();require(panel&&!panel->hasChanges(),"Epoch tuple Apply failed");view();
         require(QString::fromStdString(configured()->GetStringParameter("EpochFormat"))==format&&QString::fromStdString(configured()->GetStringParameter("StartEpoch"))==epoch&&configured()->GetRealParameter("StepSize")==23,"Epoch tuple configured readback differs");
         const auto wanted=source->toPlainText();require(wanted!=before&&child->close(),"Epoch Apply unchanged or clean close failed");events();source->undo();require(source->toPlainText()==before&&window.buildScript(),"Epoch tuple exact Undo failed");source->redo();require(source->toPlainText()==wanted&&window.buildScript(),"Epoch tuple exact Redo failed");++applies;
         require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted,"Epoch Unicode save/reopen failed");save(key+"-mission.script",wanted.toUtf8());open();formatControl=panel->findChild<QComboBox *>("resource_EpochFormat");epochControl=panel->findChild<QComboBox *>("resource_StartEpoch");require(formatControl&&epochControl&&formatControl->currentText()==format&&epochControl->currentText()==epoch,"Epoch reopened GUI tuple differs");require(child->close(),"Epoch reopened clean close failed");events();
         const int history=source->document()->availableUndoSteps();messages->clear();const auto outcome=window.runMission();events();save(key+"-messages.txt",messages->toPlainText().toUtf8());if (outcome!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(outcome==MainWindow::RunResult::Completed,"Epoch format execution mission failed");++missions;
         const auto raw=read(report);save(key+"-values.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==3,"Epoch execution report row count differs");QJsonArray rows;int index=0;const double expectedStart=input["expected_start_et"].toDouble(),epochTolerance=input["epoch_tolerance_seconds"].toDouble();
         for (const auto &line:lines) {
            const auto words=line.split(',');require(words.size()==8,"Epoch execution report columns differ");std::array<double,8> numbers{};QJsonArray actual;
            for (int i=0;i<8;++i) {bool ok=false;numbers[i]=words[i].trimmed().toDouble(&ok);require(ok&&std::isfinite(numbers[i]),"Epoch execution nonnumeric/nonfinite report");actual.append(numbers[i]);}
            const double reportedEt=(numbers[0]-21545)*86400,local=reportedEt-center,requested=std::array<int,3>{97,301,557}[index++],epochError=std::abs(reportedEt-expectedStart-requested),elapsedError=std::abs(numbers[1]-(expectedStart-(center-240)+requested));
            if (epochError>epochTolerance||elapsedError>epochTolerance) std::cerr<<"EPOCH_DIFF "<<scenario.toStdString()<<"/"<<key.toStdString()<<" epoch_error="<<epochError<<" elapsed_error="<<elapsedError<<" bound="<<epochTolerance<<std::endl;
            require(epochError<=epochTolerance&&elapsedError<=epochTolerance,"SPK epoch execution differs from independent NAIF input time");maxEpoch=std::max(maxEpoch,epochError);
            const auto expected=trajectory(local);QJsonArray expectedValues,errors;
            for (int i=0;i<6;++i) {const double error=std::abs(numbers[i+2]-expected[i]);expectedValues.append(expected[i]);errors.append(error);if (i<3) maxPosition=std::max(maxPosition,error);else maxVelocity=std::max(maxVelocity,error);if (error>(i<3?2e-5:2e-8)) std::cerr<<"EPOCH_STATE_DIFF "<<scenario.toStdString()<<"/"<<key.toStdString()<<" component="<<i<<" error="<<error<<std::endl;require(error<=(i<3?2e-5:2e-8),"Epoch execution state differs from independent cubic/derivative truth");}
            rows.append(QJsonObject{{"actual",actual},{"expected_state",expectedValues},{"state_errors",errors},{"reported_et",reportedEt},{"epoch_error_seconds",epochError},{"elapsed_error_seconds",elapsedError},{"requested_offset_seconds",requested}});++reportRows;
         }
         require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Epoch execution continuation sentinel differs");require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==history&&!window.isRunning()&&window.findChild<QAction *>("runMission")->isEnabled(),"Epoch execution changed source/history or retained run lock");
         auto *output=window.findChild<QTreeWidget *>("Output");auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Epoch execution output tree report missing");output->itemDoubleClicked(reports.first(),0);events();child=area->activeSubWindow();require(child&&dynamic_cast<ReportViewer *>(child->widget()),"Actual epoch report viewer missing");view();auto *shown=child->findChild<QPlainTextEdit *>();require(shown&&shown->isReadOnly()&&shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Epoch viewer differs from checked raw report");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*child);require(shown->horizontalScrollBar()->value()==shown->horizontalScrollBar()->maximum(),"Epoch result columns did not scroll into view");require(child->close(),"Epoch viewer clean close failed");events();
         observations.append(QJsonObject{{"input",input},{"rows",rows},{"continued_sentinel",42}});save("observations.json",QJsonDocument(observations).toJson());if (input["kind"]=="explicit") formats.append(format);
      }
      require(missions==spec["inputs"].toArray().size()&&applies==missions&&reportRows==3*missions,"Epoch execution context cardinality differs");
      const QJsonObject proof{{"checkpoint","spk-epoch-execution-"+scenario},{"outcome","passed"},{"scenario",scenario},{"missions",missions},{"resource_applies",applies},{"report_rows",reportRows},{"explicit_formats",formats},{"observations",observations},{"held_inputs",spec["held_inputs"]},{"max_epoch_error_seconds",maxEpoch},{"max_position_error_km",maxPosition},{"max_velocity_error_km_s",maxVelocity},{"facets",QJsonArray{"actual-SPK-format-epoch-step-controls","pending-tuple-exact-undo-redo-unicode-reopen","independent-NAIF-five-scale-inputs","explicit-inherited-ephemeris-start-execution","nonuniform-type13-cubic-state-and-derivative","checked-raw-report-viewer-and-continuation","leap-second-UTC-MJD-ambiguity-held"}},{"scope","Declared NAIF input matrix at four2000/2016/2017epochs,including Gregorian leap-second input. Explicit start epochs in all ten formats away from the leap second,nine at the leap second,plus inherited and EphemStart. Gregorian epoch bound1.1ms,numeric100microseconds;state truth2e-5km/2e-8km/s evaluated at reported TDB epoch. Shared LSK/NAIF/provider conventions are explicit;no measured clock/independent leap-history,ambiguous UTC MJD inside leap second,origin/general kernel/frame/domain/native OS input/GPU/hardware/platform/stress/current-private NASA/full-current-build qualification."}};
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
