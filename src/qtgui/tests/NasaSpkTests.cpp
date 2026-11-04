#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "TimeSystemConverter.hpp"
#include "GmatTime.hpp"
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
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *text) {if (!ok) throw std::runtime_error(text);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static std::array<double,6> trajectory(double t)
{
   // Cubic position and its exact derivative. Nonuniform knots below force
   // genuine interpolation, independently of GMAT's exporter and integrator.
   return {7000+1.2*t+.003*t*t+1e-6*t*t*t,-2000+4.5*t-.002*t*t+5e-7*t*t*t,500-.7*t+.0004*t*t-2e-7*t*t*t,
           1.2+.006*t+3e-6*t*t,4.5-.004*t+1.5e-6*t*t,-.7+.0008*t-6e-7*t*t};
}
static void spiceCheck()
{
   if (!failed_c()) return;
   char message[1841];getmsg_c("LONG",sizeof(message),message);reset_c();throw std::runtime_error(message);
}
static void fixture(const QString &path,int type)
{
   SpiceDouble epochs[]={-1200,-800,-360,0,170,515,920,1400,1800},states[9][6];
   for (int i=0;i<9;++i) {const auto state=trajectory(epochs[i]);for (int j=0;j<6;++j) states[i][j]=state[j];}
   SpiceInt handle;spkopn_c(path.toUtf8().constData(),"GMAT acceptance independent polynomial",0,&handle);spiceCheck();
   if (type==9) spkw09_c(handle,-100055,399,"J2000",epochs[0],epochs[8],"NONUNIFORM CUBIC TYPE9",3,9,states,epochs);
   else spkw13_c(handle,-100055,399,"J2000",epochs[0],epochs[8],"NONUNIFORM CUBIC TYPE13",7,9,states,epochs);
   spiceCheck();spkcls_c(handle);spiceCheck();
}
static QString converted(const QString &epoch,const QString &from,const QString &to)
{
   GmatTime mjd;std::string output;
   TimeSystemConverter::Instance()->Convert(from.toStdString(),GmatTime(-999.999),epoch.toStdString(),to.toStdString(),mjd,output);
   return QString::fromStdString(to.endsWith("ModJulian") ? mjd.ToString() : output);
}
static double physical(const QString &epoch,const QString &format)
{
   return converted(epoch,format,"TDBModJulian").toDouble();
}
static Propagator *configured()
{
   auto *setup=dynamic_cast<PropSetup *>(Moderator::Instance()->GetConfiguredObject("Read"));require(setup && setup->GetPropagator(),"SPK configured propagator missing");return setup->GetPropagator();
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSpk");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and SPK case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"SPK fixture directory unavailable");
      const QString mode=QString::fromLocal8Bit(argv[2]);const bool controls=mode=="EpochControls",backward=mode.endsWith("Backward"),ephemStart=mode=="EphemStart",explicitEpoch=mode.startsWith("Explicit");const int type=mode.startsWith("Type9") ? 9 : 13;
      require(controls || mode=="Type9Forward" || mode=="Type9Backward" || mode=="Type13Forward" || mode=="Type13Backward" || ephemStart || mode=="ExplicitNumeric" || mode=="ExplicitGregorian","Unknown SPK test case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;require(window.initialize(startup),"SPK runtime initialization failed");window.show();
      const auto kernel=files.filePath("polynomial.bsp"),report=files.filePath("values.txt");fixture(kernel,type);
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");auto *area=window.findChild<QMdiArea *>("workspace");auto *tree=window.findChild<QTreeWidget *>("Resources");require(editor && area && tree,"SPK workspace missing");
      const auto start=ephemStart ? QString("EphemStart") : QString("FromSpacecraft");
      QString source="% Independent nonuniform polynomial SPK\nCreate Spacecraft Vehicle;\nVehicle.NAIFId = -100055;\nVehicle.DateFormat = TDBModJulian;\nVehicle.Epoch = '21545.0013888888888889';\nVehicle.OrbitSpiceKernelName = {'"+kernel+"'};\nCreate Propagator Read;\nRead.Type = SPK;\nRead.StepSize = 37;\nRead.EpochFormat = TDBModJulian;\nRead.StartEpoch = '"+start+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 17;\nValues.WriteHeaders = false;\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\n";
      // Propagate's elapsed stop is relative to each command's initial epoch;
      // the report parameter retains the mission's cumulative elapsed time.
      if (!controls) {int previous=0;for (int stop:{97,301,557}) {const int interval=stop-previous;source+="Propagate "+QString(backward ? "BackProp " : "")+"Read(Vehicle) {Vehicle.ElapsedSecs = "+QString::number(backward ? -interval : interval)+"};\nReport Values Vehicle.TDBModJulian Vehicle.ElapsedSecs Vehicle.EarthICRF.X Vehicle.EarthICRF.Y Vehicle.EarthICRF.Z Vehicle.EarthICRF.VX Vehicle.EarthICRF.VY Vehicle.EarthICRF.VZ;\n";previous=stop;}}
      source+="Keep = Keep + 1;\n";editor->setPlainText(source);require(window.buildScript(),"Polynomial SPK mission did not build");
      ResourceEditor *panel=nullptr;QMdiSubWindow *child=nullptr;
      auto open=[&](bool trigger) {
         if (trigger) {const auto items=tree->findItems("Read",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"SPK resource tree entry missing");tree->itemDoubleClicked(items.front(),0);events();}
         QList<QMdiSubWindow *> matches;for (auto *candidate:area->subWindowList()) if (candidate->property("resourceName").toString()=="Read" && dynamic_cast<ResourceEditor *>(candidate->widget())) matches.append(candidate);
         require(matches.size()==1,"Actual SPK resource editor missing or duplicated");child=matches.front();panel=dynamic_cast<ResourceEditor *>(child->widget());child->showNormal();area->setActiveSubWindow(child);events();acceptanceView(*child);require(!panel->hasChanges(),"Fresh SPK editor is dirty");
      };open(true);
      auto apply=[&] {panel->requestApply();events();open(false);};
      QJsonArray formats,rows;int invalid=0;double positionError=0,velocityError=0;
      if (controls) {
         auto format=[&] {return panel->findChild<QComboBox *>("resource_EpochFormat");};auto epoch=[&] {return panel->findChild<QComboBox *>("resource_StartEpoch");};auto step=[&] {return panel->findChild<QLineEdit *>("resource_StepSize");};
         require(format() && epoch() && step(),"SPK epoch format and editable start-epoch selectors missing");require(epoch()->isEditable() && epoch()->findText("FromSpacecraft")>=0 && epoch()->findText("EphemStart")>=0,"SPK start source choices missing");
         const auto before=editor->toPlainText();const auto undo=editor->document()->availableUndoSteps();epoch()->setCurrentText("21545.0013888888888889");require(panel->hasChanges() && editor->toPlainText()==before && configured()->GetStringParameter("StartEpoch")=="FromSpacecraft" && editor->document()->availableUndoSteps()==undo,"Explicit SPK epoch bypassed pending Apply");apply();
         const auto reference=physical("21545.0013888888888889","TDBModJulian");
         for (const auto &name:TimeSystemConverter::Instance()->GetValidTimeRepresentations()) {
            const auto next=QString::fromStdString(name);require(format()->findText(next)>=0,"Supported SPK epoch format absent from selector");format()->setCurrentText(next);events();
            require(std::abs(physical(epoch()->currentText(),next)-reference)*86400<=.0011,"GUI epoch format change moved the physical start epoch");acceptanceView(*child);apply();
            require(QString::fromStdString(configured()->GetStringParameter("EpochFormat"))==next && std::abs(physical(QString::fromStdString(configured()->GetStringParameter("StartEpoch")),next)-reference)*86400<=.0011,"Applied SPK format/epoch pair changed the physical instant");formats.append(next);
         }
         for (const auto &sentinel:QStringList{"FromSpacecraft","EphemStart"}) {epoch()->setCurrentText(sentinel);format()->setCurrentText("A1ModJulian");require(epoch()->currentText()==sentinel,"SPK format conversion changed a start-source keyword");apply();require(QString::fromStdString(configured()->GetStringParameter("StartEpoch"))==sentinel,"SPK source keyword not applied");}
         for (const auto &bad:QStringList{"0","-1","nan","inf","text"}) {
            const auto appliedSource=editor->toPlainText();const auto oldStep=configured()->GetRealParameter("StepSize");const auto history=editor->document()->availableUndoSteps();step()->setText(bad);panel->requestApply();events();
            require(panel==child->widget() && panel->hasChanges() && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && editor->toPlainText()==appliedSource && configured()->GetRealParameter("StepSize")==oldStep && editor->document()->availableUndoSteps()==history,"Invalid SPK step committed or lacks atomic rejection");++invalid;
         }
         step()->setText("19");epoch()->setCurrentText("not a date");const auto appliedSource=editor->toPlainText();panel->requestApply();events();require(panel==child->widget() && panel->hasChanges() && editor->toPlainText()==appliedSource && configured()->GetRealParameter("StepSize")==37,"Invalid paired SPK epoch partially changed the step or source");++invalid;
         const auto oldFormat=format()->currentText();format()->setCurrentText("UTCGregorian");events();require(format()->currentText()==oldFormat && epoch()->currentText()=="not a date" && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty(),"Invalid pending epoch conversion did not restore the visible format and preserve input");++invalid;
         epoch()->setCurrentText("21545.0013888888888889");apply();require(configured()->GetRealParameter("StepSize")==19,"Corrected SPK settings did not apply");
         // Public GUI API must preserve a date on a format-only transaction too.
         require(window.applyResourceChanges("Read",{{"EpochFormat","UTCGregorian"}},editor->toPlainText()).isEmpty(),"SPK format-only transaction failed");events();require(std::abs(physical(QString::fromStdString(configured()->GetStringParameter("StartEpoch")),"UTCGregorian")-physical("21545.0013888888888889","A1ModJulian"))*86400<=.0011,"Format-only transaction reinterpreted the epoch instead of converting it");
      } else {
         auto *step=panel->findChild<QLineEdit *>("resource_StepSize");require(step && step->isVisible() && step->isEnabled(),"SPK step magnitude control unavailable");step->setText("23");acceptanceView(*child);apply();require(configured()->GetRealParameter("StepSize")==23,"SPK GUI step magnitude did not commit");
         if (explicitEpoch) {
            const auto before=editor->toPlainText();auto *format=panel->findChild<QComboBox *>("resource_EpochFormat");auto *epoch=panel->findChild<QComboBox *>("resource_StartEpoch");require(format && epoch && epoch->isEditable(),"Explicit SPK epoch controls unavailable");
            const auto selected=mode=="ExplicitGregorian" ? QString("TDBGregorian") : QString("TDBModJulian");format->setCurrentText(selected);epoch->setCurrentText(converted("21545.0027777777777778","TDBModJulian",selected));
            require(editor->toPlainText()==before && configured()->GetStringParameter("StartEpoch")=="FromSpacecraft","Explicit SPK epoch committed before Apply");acceptanceView(*child);apply();require(QString::fromStdString(configured()->GetStringParameter("EpochFormat"))==selected,"Explicit SPK format was not applied");formats.append(selected);
         }
         require(child->close(),"Clean SPK panel did not close");events();
         if (window.runMission()!=MainWindow::RunResult::Completed) {std::cerr<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString();throw std::runtime_error("Independent polynomial SPK mission failed");}
         const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if (!evidence.isEmpty()) {QFile::copy(kernel,QDir(evidence).filePath("polynomial.bsp"));QFile::copy(report,QDir(evidence).filePath("values.txt"));QFile saved(QDir(evidence).filePath("source.script"));require(saved.open(QIODevice::WriteOnly),"Cannot retain SPK source");saved.write(editor->toPlainText().toUtf8());}
         QFile data(report);require(data.open(QIODevice::ReadOnly),"SPK numeric report missing");const auto lines=QString::fromUtf8(data.readAll()).trimmed().split('\n');require(lines.size()==3,"SPK report must contain every requested interior stop");int index=0;
         for (const auto &line:lines) {const auto words=line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==8,"SPK report has wrong field count");std::array<double,8> values{};QJsonArray row;
            for (int i=0;i<8;++i) {bool valid=false;values[i]=words[i].toDouble(&valid);require(valid && std::isfinite(values[i]),"SPK report nonnumeric/nonfinite");row.append(values[i]);}
            const double time=(values[0]-21545)*86400,requested=(backward ? -1 : 1)*std::array<int,3>{97,301,557}[index++],startTime=ephemStart ? -1200 : explicitEpoch ? 240 : 120;
            // Report's elapsed clock stays relative to the spacecraft's
            // configured epoch (t=120), even when EphemStart moves the arc.
            const double expectedElapsed=startTime-120+requested;
            if (std::abs(values[1]-expectedElapsed)>1e-5 || std::abs(time-startTime-requested)>1e-5) {std::cerr.precision(17);std::cerr<<"SPK time="<<time<<" elapsed="<<values[1]<<" expected start="<<startTime<<" stop="<<requested<<std::endl;}
            require(std::abs(values[1]-expectedElapsed)<=1e-5 && std::abs(time-startTime-requested)<=1e-5,"SPK start source/direction/interior stop differs from requested epoch");const auto expected=trajectory(time);
            for (int i=0;i<6;++i) {const double error=std::abs(values[i+2]-expected[i]);if (i<3) positionError=std::max(positionError,error);else velocityError=std::max(velocityError,error);require(error<=(i<3 ? 2e-5 : 2e-8),"SPK interpolation differs from independent cubic/derivative oracle");}rows.append(row);
         }
         require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"SPK propagation did not continue to following command");
         auto *outputTree=window.findChild<QTreeWidget *>("Output");require(outputTree,"SPK output tree missing");const auto items=outputTree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"SPK numeric output entry missing");outputTree->itemDoubleClicked(items.front(),0);events();
         QMdiSubWindow *reportWindow=nullptr;for (auto *candidate:area->subWindowList()) if (dynamic_cast<ReportViewer *>(candidate->widget())) {require(!reportWindow,"Duplicate SPK report viewer");reportWindow=candidate;}
         require(reportWindow,"Actual SPK report viewer did not open");reportWindow->showNormal();reportWindow->move(0,0);reportWindow->resize(area->viewport()->size());area->setActiveSubWindow(reportWindow);events();acceptanceView(*reportWindow);auto *shown=reportWindow->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==lines.join('\n').trimmed(),"Visible SPK report does not match the independently checked raw numeric rows");
         require(area->viewport()->rect().contains(reportWindow->geometry()),"SPK report window exceeds the available workspace");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*reportWindow);require(shown->horizontalScrollBar()->value()==shown->horizontalScrollBar()->maximum(),"SPK result columns did not scroll into view");
      }
      const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if (!output.isEmpty() && controls) {require(QFile::copy(kernel,QDir(output).filePath("polynomial.bsp")),"Cannot retain SPK fixture");QFile saved(QDir(output).filePath("source.script"));require(saved.open(QIODevice::WriteOnly),"Cannot retain SPK source");saved.write(editor->toPlainText().toUtf8());}
      const QJsonObject proof{{"checkpoint","spk."+mode},{"outcome","passed"},{"facets",controls ? QJsonArray{"ten-format-conversion","pending-apply","source-keywords","invalid-step-epoch-atomicity","visible-format-rollback","correction","format-only-transaction"} : QJsonArray{"actual-spk-step-control","independent-polynomial-spk","nonuniform-knots","interior-epochs","signed-propagation","reported-clock","continued-command"}},
         {"spk_type",type},{"epoch_formats",formats},{"invalid_rejections",invalid},{"rows",rows},{"max_position_error_km",positionError},{"max_velocity_error_km_per_s",velocityError},{"scope","Synthetic Earth-relative ICRF cubic; no general SPK kernel, frame/time standards, origin, multi-spacecraft, native input or platform qualification"}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
