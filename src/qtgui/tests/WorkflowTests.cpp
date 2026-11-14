#include "ReportParameterDialog.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "FindReplaceDialog.hpp"
#include <QCheckBox>
#include <QHeaderView>
#include <QLabel>
#include "QtPlotReceiver.hpp"
#include "OrbitCamera.hpp"
#include "CoordinateConverter.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "ResourceEditor.hpp"
#include "ResourceForm.hpp"
#include <QStatusBar>
#include <QMdiArea>
#include <QMdiSubWindow>
#include "VisualModelDialog.hpp"
#include "AttitudeDialog.hpp"
#include "SpacecraftOrbit.hpp"
#include "ODEModel.hpp"
#include "PropSetup.hpp"
#include "FileManager.hpp"
#include "GmatCommand.hpp"
#include "AttitudeConversionUtility.hpp"
#include "GmatConstants.hpp"
#include <QSlider>
#include <QColorDialog>
#include "PropagationForm.hpp"
#include "PropagationStopsDialog.hpp"
#include "PropagationGroupsDialog.hpp"
#include "CommandEditor.hpp"
#include "ResourceProperties.hpp"
#include "ScriptCompatibility.hpp"
#include "Rvector.hpp"
#include <QApplication>
#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QInputDialog>
#include <QFileDialog>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTextDocument>
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QClipboard>
#include <QTabWidget>
#include <QTabBar>
#include <QMenu>
#include <QMessageBox>
#include <QScreen>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <utility>
#include <QKeyEvent>
#include <QEventLoop>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>

static void require(bool condition, const char *message)
{
   if (!condition) { std::cerr<<"CHECK FAILED: "<<message<<'\n'; throw std::runtime_error(message); }
}

// Visible replay waits only at test-owned, safe boundaries. In particular a
// zero-delay modal driver must not dismiss its window before native exposure.
// Ordinary unattended execution keeps the original timer scheduling.
static QJsonArray workflowViewingStages;
static void workflowView(QWidget &widget)
{
   if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")!="visible") return;
   auto *window=widget.window();
   if (window!=&widget && !window->layout() && !qobject_cast<QMdiSubWindow *>(window)) {
      const auto available=window->screen()->availableGeometry().size()-QSize(40,60);
      window->resize(widget.sizeHint().expandedTo(QSize(1100,700)).boundedTo(available));
      widget.setGeometry(window->rect());
   }
   acceptanceView(widget);
   require(widget.isVisible() && window->windowHandle() && window->windowHandle()->isExposed(),"Workflow viewing boundary was not exposed");
   workflowViewingStages.append(QJsonObject{{"stage",workflowViewingStages.size()+1},
      {"class",QString::fromLatin1(widget.metaObject()->className())},{"name",widget.objectName()},
      {"visible",widget.isVisible()},{"exposed",window->windowHandle()->isExposed()},
      {"native_window_id",static_cast<qint64>(window->winId())}});
}
static void workflowViewModal()
{
   if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")!="visible") return;
   auto *target=QApplication::activePopupWidget();
   if (!target) target=QApplication::activeModalWidget();
   require(target,"Workflow modal driver has no active window to expose");
   workflowView(*target);
   if (auto *picker=qobject_cast<QFileDialog *>(target)) {
      // Qt 6.10 intentionally leaves a focused filename edit untouched by
      // selectFile(). Exposure gives that edit focus, unlike the old hidden
      // callback. Release it before the unchanged selection/accept driver.
      auto *filename=picker->findChild<QLineEdit *>("fileNameEdit");
      require(filename,"Workflow widget file chooser has no filename edit");
      filename->clearFocus();
      require(!filename->hasFocus(),"Workflow file chooser selection remains focus-blocked");
   }
}
template <typename Callback> static void workflowAfterExposure(int delay,Callback callback)
{
   QTimer::singleShot(delay,[callback=std::move(callback)]() mutable {workflowViewModal();callback();});
}
template <typename Context,typename Callback> static void workflowAfterExposure(int delay,Context *context,Callback callback)
{
   QTimer::singleShot(delay,context,[callback=std::move(callback)]() mutable {workflowViewModal();callback();});
}

int main(int argc, char **argv)
{
   QApplication app(argc, argv);
   app.setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication::setOrganizationName("GMATTests");
   QApplication::setApplicationName("QtWorkflow");
   if (argc < 3 || argc > 5 || (argc==5 && QString(argv[3])!="--attitude-capture" && QString(argv[3])!="--orbit-capture")) return 2;
   const auto startup = QFileInfo(argv[1]).absoluteFilePath();
   const auto script = QFileInfo(argv[2]).absoluteFilePath();
   const auto screenshot = argc == 4 ? QFileInfo(argv[3]).absoluteFilePath() : QString();
   const auto attitudeCapture = argc == 5 && QString(argv[3])=="--attitude-capture" ? QFileInfo(argv[4]).absoluteFilePath() : QString();
   const auto orbitCapture = argc == 5 && QString(argv[3])=="--orbit-capture" ? QFileInfo(argv[4]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings isolatedSettings;
      {
         const QString grouped="Propagate 'Groups' BackProp Synchronized Prop(Sat, OtherSat) {Sat.ElapsedSecs = -60, StopTolerance = 1e-8}; % keep groups";
         PropagationGroupsDialog groups(grouped,{"Prop","Other"},{"Sat","OtherSat","Fleet"}); workflowView(groups);
         auto *groupTable=groups.findChild<QTableWidget *>("propagationGroupsTable");
         require(groupTable->rowCount()==1 && groupTable->item(0,1)->text()=="Sat, OtherSat","Existing group objects lost");
         groups.findChild<QComboBox *>("propagationGroupMode")->setCurrentIndex(0); groups.findChild<QCheckBox *>("propagationGroupBackwards")->setChecked(false);
         groupTable->item(0,1)->setText("Fleet");
         require(groups.statement()=="Propagate 'Groups' Prop(Fleet) {Sat.ElapsedSecs = -60, StopTolerance = 1e-8}; % keep groups","Group editing changed stopping conditions or comment");
         PropagationGroupsDialog variational("Propagate Prop(Sat, 'STM', 'AMatrix') {Sat.ElapsedSecs = 60};",{"Prop"},{"Sat"}); workflowView(variational);
         require(variational.findChild<QCheckBox *>("propagationGroupSTM")->isChecked() && variational.findChild<QCheckBox *>("propagationGroupAMatrix")->isChecked(),"Variational flags not loaded");
         variational.findChild<QCheckBox *>("propagationGroupSTM")->setChecked(false);
         require(variational.statement()=="Propagate Prop(Sat, 'AMatrix') {Sat.ElapsedSecs = 60};","Variational flags not independently editable");
         require(PropagationGroupsDialog::supports("Propagate Prop(Sat, 'Covariance') {Sat.ElapsedSecs = 60};"),"Covariance flag does not support group editing");
         const QString stopSource="Propagate 'Keep' BackProp Prop(Sat) {Sat.ElapsedSecs = 60, StopTolerance = 1e-8, OrbitColor = Green}; % comment";
         PropagationStopsDialog stops(stopSource); workflowView(stops);
         stops.findChild<QTableWidget *>("propagationStopsTable")->item(0,1)->setText("120");
         auto stopExpected=stopSource; stopExpected.replace("= 60,","= 120,");
         require(stops.statement()==stopExpected,"Stop table changed options, modifier, label or comment");
         QString changed; PropagationForm form({"Prop"},{"Sat"},[&](const QString &value) { changed=value; }); workflowView(form);
         const QString source="  Propagate 'Keep label' BackProp Prop(Sat) {Sat.A1ModJulian = 20000}; % keep comment\n";
         form.setStatement(source); require(!form.isHidden(),"Labeled custom propagation hidden");
         form.findChild<QLineEdit *>("propagationDuration")->setText("20001");
         auto expected=source; expected.replace("20000","20001"); require(changed==expected,"Propagation edit lost label, BackProp or comment");
         form.findChild<QLineEdit *>("propagationStopParameter")->setText("Sat.ElapsedSecs");
         expected.replace("Sat.A1ModJulian","Sat.ElapsedSecs"); require(changed==expected,"Stop parameter edit changed unrelated spans");
         form.findChild<QCheckBox *>("propagationBackwards")->setChecked(false); expected.replace("BackProp ","");
         require(changed==expected,"Backward toggle lost source formatting");
         auto *tolerance=form.findChild<QLineEdit *>("propagationTolerance"); tolerance->setText("1e-9");
         require(changed.contains("Sat.ElapsedSecs = 20001, StopTolerance = 1e-9") && changed.endsWith("% keep comment\n"),"Tolerance insertion lost stop or comment");
         tolerance->clear(); require(changed==expected,"Clearing tolerance did not restore omitted default");
         form.setStatement("Propagate Prop(Sat) {Sat.ElapsedSecs = 60,StopTolerance = 1e-5}; % keep");
         tolerance->setText("2e-6");
         require(changed=="Propagate Prop(Sat) {Sat.ElapsedSecs = 60,StopTolerance = 2e-6}; % keep","Existing tolerance formatting changed");
         const QString tabbed="Propagate Prop(Sat) {Sat.ElapsedSecs = 60,\tStopTolerance\t=\t1e-5  , OrbitColor = Green}; % keep";
         form.setStatement(tabbed); tolerance->setText("2e-6");
         auto tabbedExpected=tabbed; tabbedExpected.replace("1e-5","2e-6");
         require(changed==tabbedExpected,"Tolerance edit normalized unrelated whitespace or options");
         const QString multiline="Propagate Prop(Sat) {Sat.ElapsedSecs = 60,\n  StopTolerance = 1e-5\n}; % keep";
         form.setStatement(multiline); tolerance->setText("2e-6");
         auto multilineExpected=multiline; multilineExpected.replace("1e-5","2e-6");
         require(changed==multilineExpected,"Tolerance edit normalized authored line breaks");


         form.setStatement("Propagate Prop(Sat) {Sat.Earth.Periapsis};"); require(!form.isHidden() && !form.findChild<QLineEdit *>("propagationDuration")->isEnabled(),"Apsis propagation lacks event controls");
         form.findChild<QLineEdit *>("propagationStopParameter")->setText("Sat.Earth.Apoapsis");
         require(changed=="Propagate Prop(Sat) {Sat.Earth.Apoapsis};","Apsis stop acquired a spurious goal");
      }
      {
         const QString dynamics="BeginMissionSequence;\nGMAT total = 2 + 3; % scientific code stays exact\n";
         const QString input="% Create OpenFramesInterface ignored comment;\nCreate OpenFramesInterface Display;\n"
            "Display.Add = {Sat, Earth};\nDisplay.View = {Camera};\nDisplay.DrawLabel = [true false];\n"
            "Display.ShowVR = false;\nCreate OpenFramesView Camera;\nCamera.ViewFrame = CoordinateSystem;\n"
            "Camera.SetDefaultLocation = On;\nCamera.DefaultEye = [100 200 300];\nCamera.DefaultUp = [1 0 1];\n"+dynamics;
         const auto converted=convertOpenFramesViews(input);
         require(converted.error.isEmpty() && converted.plots==1 && converted.script.endsWith(dynamics),"View conversion changed mission calculations");
         require(converted.script.contains("Create OrbitView Display;") && converted.script.contains("Display.ViewPointVector = [100 200 300]") &&
            converted.script.contains("retained as a comment") && converted.script.contains("exact camera up vector"),"View conversion lost settings or omitted limitations");
         require(convertOpenFramesViews(dynamics).script==dynamics,"Converter changed a script without OFI");
         auto multiple=input; multiple.replace("Display.View = {Camera}","Display.View = {Camera, Close}");
         multiple.replace("BeginMissionSequence;","Create OpenFramesView Close;\nClose.SetCurrentLocation = On;\nClose.CurrentEye = [0 -9000 500];\nClose.CurrentCenter = [1 2 3];\nClose.CurrentUp = [1 1 0];\nClose.FOVy = 22.5;\nBeginMissionSequence;");
         const auto multi=convertOpenFramesViews(multiple);
         require(multi.error.isEmpty() && multi.script.endsWith(dynamics),"Multiple-view conversion changed mission commands");
         const auto multiSetting=qtCameraSettings(multi.script).value("Display");
         require(multiSetting.primaryName=="Camera" && multiSetting.views.size()==1 && multiSetting.views[0].name=="Close" &&
            multiSetting.views[0].eye==std::array<double,3>{0,-9000,500} && multiSetting.views[0].center==std::array<double,3>{1,2,3} &&
            multiSetting.views[0].up==std::array<double,3>{1,1,0} && multiSetting.views[0].fieldOfView==22.5,"Additional current camera pose/FOV lost");
         for (const auto &invalid:{QString(multiple).replace("Close.FOVy = 22.5","Close.FOVy = 180"),
               QString(multiple).replace("{Camera, Close}","{Camera, Missing}"),
               QString(multiple).replace("Close.CurrentUp = [1 1 0]","Close.CurrentUp = [0 0 0]")})
            require(!convertOpenFramesViews(invalid).error.isEmpty(),"Invalid secondary view silently accepted");

         auto trajectoryInput=input; trajectoryInput.replace("Camera.ViewFrame = CoordinateSystem","Camera.ViewFrame = Sat;\nCamera.ViewTrajectory = On");
         const auto trajectory=convertOpenFramesViews(trajectoryInput);
         require(trajectory.error.isEmpty() && trajectory.script.contains("Display.ViewPointReference = [0 0 0]") &&
            !qtCameraSettings(trajectory.script).value("Display").bodyRelative && trajectory.script.endsWith(dynamics),"Stored trajectory camera still follows object");
         auto secondary=multiple; secondary.replace("Close.SetCurrentLocation", "Close.ViewFrame = Sat;\nClose.ViewTrajectory = On;\nClose.SetCurrentLocation");
         const auto secondaryTrajectory=convertOpenFramesViews(secondary);
         require(secondaryTrajectory.error.isEmpty() && qtCameraSettings(secondaryTrajectory.script).value("Display").views[0].reference.isEmpty(),"Secondary trajectory camera still follows object");
         require(!convertOpenFramesViews(QString(trajectoryInput).replace("Camera.ViewFrame = Sat","Camera.ViewFrame = Missing")).error.isEmpty(),"Unknown trajectory object silently accepted");
         const auto automatic=convertOpenFramesViews(QString(trajectoryInput).replace("Camera.SetDefaultLocation = On","Camera.SetDefaultLocation = Off"));
         require(automatic.error.isEmpty() && qtCameraSettings(automatic.script).value("Display").automaticTrajectory=="Sat" && automatic.script.endsWith(dynamics),"Automatic trajectory metadata or dynamics lost");
         const auto automaticSecondary=convertOpenFramesViews(QString(secondary).replace("Close.SetCurrentLocation = On","Close.SetCurrentLocation = Off"));
         require(automaticSecondary.error.isEmpty() && qtCameraSettings(automaticSecondary.script).value("Display").views[0].automaticTrajectory=="Sat","Automatic named trajectory lost");
         const auto automaticLookAt=convertOpenFramesViews(QString(trajectoryInput).replace("Camera.SetDefaultLocation = On","Camera.SetDefaultLocation = Off;\nCamera.LookAtFrame = Sat"));
         require(automaticLookAt.error.isEmpty() && qtCameraSettings(automaticLookAt.script).value("Display").lookAtRotation,"Automatic LookAt mode lost in conversion");
         const auto automaticOrigin=convertOpenFramesViews(QString(input).replace("Camera.SetDefaultLocation = On","Camera.SetDefaultLocation = Off"));
         require(automaticOrigin.error.isEmpty() && qtCameraSettings(automaticOrigin.script).value("Display").automaticTrajectory=="CoordinateSystem" &&
            automaticOrigin.script.endsWith(dynamics),"Automatic CoordinateSystem conversion changed dynamics or lost framing");
         const auto segment=convertOpenFramesViews(QString(trajectoryInput).replace("Camera.ViewFrame = Sat","Camera.ViewFrame = Sat.Prop"));
         require(segment.error.isEmpty() && qtCameraSettings(segment.script).value("Display").segmentFrame=="Sat.Prop","Segment camera metadata lost");
         const auto cameras=qtCameraSettings(converted.script);
         require(cameras.contains("Display") && cameras["Display"].perspective && cameras["Display"].fieldOfView==45,
            "Conversion lost default OF perspective/FOV");
         require(cameras["Display"].up==std::optional<std::array<double,3>>({1,0,1}),"Conversion changed non-axis up vector");
         for (const auto &badUp:{QString("[0,0,0]"),QString("[1,2]"),QString("[1,2,\"z\"]"),QString("null")}) {
            bool invalid=false;
            try { qtCameraSettings("% GMAT-Qt-Camera {\"plot\":\"Display\",\"perspective\":true,\"fieldOfView\":45,\"up\":"+badUp+"}"); }
            catch (const std::exception &) { invalid=true; }
            require(invalid,"Invalid up vector accepted");
         }
         require(!convertOpenFramesViews(QString(input).replace("[1 0 1]","[0 0 0]")).error.isEmpty(),"Zero OF up vector accepted");
         const auto absolute=convertOpenFramesViews(QString(input).replace("Camera.ViewFrame = CoordinateSystem","Camera.InertialFrame = On;\nCamera.ViewFrame = Earth"));
         require(absolute.error.isEmpty() && !qtCameraSettings(absolute.script).value("Display").bodyRelative,"InertialFrame On incorrectly follows body rotation");
         require(!convertOpenFramesViews(QString(input).replace("[100 200 300]","[invalid]")).error.isEmpty(),"Invalid primary eye silently replaced");
         require(!convertOpenFramesViews(QString(input).replace("Camera.ViewFrame","Camera.ShortestAngle = Invalid;\nCamera.ViewFrame")).error.isEmpty(),
            "Invalid camera alignment mode silently changed");
         const auto custom=convertOpenFramesViews(QString(input).replace("Camera.ViewFrame", "Camera.FOVy = 37.5;\nCamera.ViewFrame"));
         require(custom.error.isEmpty() && qtCameraSettings(custom.script)["Display"].fieldOfView==37.5,"Conversion rounded the OF field of view");
         require(!convertOpenFramesViews(QString(input).replace("Camera.ViewFrame", "Camera.FOVy = 180;\nCamera.ViewFrame")).error.isEmpty(),
            "Out-of-range OF FOV was silently changed");
         bool rejected=false;
         try { qtCameraSettings(qtCameraDirective("Display",{true,45})+qtCameraDirective("Display",{true,60})); }
         catch (const std::exception &) { rejected=true; }
         require(rejected,"Conflicting camera directives were accepted");
         require(!convertOpenFramesViews(input+"GMAT total = Camera.FOVy;\n").error.isEmpty(),"Converter removed a view used by calculations");
         require(!convertOpenFramesViews("Create OpenFramesVector Vec;\nVec.VectorType = 'Thrust Vector';\n").error.isEmpty(),"Unsupported OFI declaration was silently removed");
         require(!convertOpenFramesViews(input+"Display.Axes = Off;\n").error.isEmpty(),"Dynamic viewer setting was silently rewritten");
      }
      // Saved scripts and referenced assets must outlive the open window.
      // Keep the owners before window so normal cleanup follows its teardown.
      std::vector<std::unique_ptr<QTemporaryDir>> fixtureDirectories;
      auto fixtureDirectory=[&]() -> QTemporaryDir & {
         fixtureDirectories.emplace_back(new QTemporaryDir);
         return *fixtureDirectories.back();
      };
      MainWindow window;
      auto checkedLoadScript=[&](const QString &path) {
         require(QFileInfo(path).isFile(),qPrintable("Script fixture missing before Open: "+path));
         return window.loadScript(path);
      };
      window.show();
      require(window.initialize(startup), "Runtime initialization failed");
      require(checkedLoadScript(script), "Script read failed");
      require(window.buildScript(), "Script interpretation failed"); workflowView(window);
      auto *sat = dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
      require(sat != nullptr, "Configured spacecraft missing");
      const auto epoch = sat->GetEpoch();
      require(window.runMission() == MainWindow::RunResult::Completed, "Propagation did not complete");
      auto *propagated = dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat"));
      require(propagated != nullptr, "Propagated spacecraft missing");
      require(std::abs((propagated->GetEpoch() - epoch) * 86400.0 - 600.0) < 0.01,
         "Mission did not propagate the requested 600 seconds");
      require(window.runMission() == MainWindow::RunResult::Completed, "Repeat execution failed");
      auto *editor = window.findChild<QPlainTextEdit *>("scriptEditor");
      require(editor != nullptr, "Script editor missing");
      {
         const auto before=editor->toPlainText(); QString error="Apply not invoked";
         const double initial=Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("A1Epoch");
         {
            QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("QtSat"),[&](const QMap<QString,QString> &changes) {
               error=window.applyResourceChanges("QtSat",changes,before); return error;
            },&owner,before); workflowView(panel);
            auto *format=panel.findChild<QComboBox *>("spacecraftEpochFormat"); auto *grid=panel.findChild<QTableWidget *>(); QTableWidgetItem *date=nullptr;
            for (int row=0;row<grid->rowCount();++row) if (grid->item(row,0)->text()=="Epoch") date=grid->item(row,1);
            require(format && date,"Spacecraft epoch conversion controls absent");
            for (const auto &name:{"DateFormat","DisplayStateType","AnomalyType"})
               require(!panel.findChild<QPushButton *>(QString("chooseProperty_")+name),"Enumeration incorrectly offers a color picker");
            require(panel.findChild<QPushButton *>("chooseProperty_OrbitColor") && panel.findChild<QPushButton *>("chooseProperty_TargetColor"),
               "Actual spacecraft colors lost their pickers");
            for (const auto &field:resourceProperties(*Moderator::Instance()->GetConfiguredObject("QtSat"))) {
               if (field.name=="DateFormat" || field.name=="DisplayStateType" || field.name=="AnomalyType") require(!field.color,"Enumeration metadata still identifies a color");
            }
            format->setCurrentText("UTCGregorian"); const auto converted=date->text();
            date->setText("bad date"); format->setCurrentText("TAIModJulian");
            require(format->currentText()=="UTCGregorian" && date->text()=="bad date","Invalid spacecraft date changed format");
            date->setText(converted); format->setCurrentText("TAIModJulian");
            format->setCurrentText("UTCGregorian");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         }
         require(error.isEmpty(),qPrintable(error));
         require(std::abs(Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("A1Epoch")-initial)*86400.<.001,"Format change moved spacecraft epoch");
         auto &files=fixtureDirectory(); const auto path=files.filePath("epoch-converted.script");
         require(window.saveScriptTo(path) && checkedLoadScript(path) && window.buildScript(),"Converted spacecraft epoch did not reopen");
         require(window.runMission()==MainWindow::RunResult::Completed &&
            std::abs((dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat"))->GetEpoch()-initial)*86400.-600)<.01,
            "Converted epoch changed reopened propagation");
         editor->setPlainText(before); require(window.buildScript(),"Epoch fixture restoration failed");
         sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
      }

      {
         const auto before=editor->toPlainText(); QString error="Apply not invoked";
         const double initialSma=Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("SMA");
         {
            QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("QtSat"),[&](const QMap<QString,QString> &changes) {
               error=window.applyResourceChanges("QtSat",changes,before); return error;
            },&owner,before); workflowView(panel);
            auto *choice=panel.findChild<QComboBox *>("spacecraftStateRepresentation"); auto *grid=panel.findChild<QTableWidget *>();
            require(choice,"State representation conversion control absent");
            const auto field=[&](const QString &name)->QTableWidgetItem * {
               for (int row=0;row<grid->rowCount();++row) if (grid->item(row,0)->text()==name) return grid->item(row,1);
               return nullptr;
            };
            choice->setCurrentText("Keplerian");
            if (!field("SMA") || !field("ECC") || field("X")) {
               std::cerr<<"Representation: "<<choice->currentText().toStdString()<<'\n';
               for (auto *label:panel.findChildren<QLabel *>()) std::cerr<<label->text().toStdString()<<'\n';
            }
            require(field("SMA") && field("ECC") && !field("X"),"State conversion did not update element labels");
            require(std::abs(field("SMA")->text().toDouble()-initialSma)<1e-6,"State conversion changed the initial orbit");
            field("SMA")->setText("invalid"); choice->setCurrentText("Cartesian");
            require(choice->currentText()=="Keplerian" && field("SMA") && field("SMA")->text()=="invalid","Failed state conversion changed pending fields");
            field("SMA")->setText(QString::number(initialSma+100,'g',17)); choice->setCurrentText("Cartesian");
            require(field("X") && field("VX") && !field("SMA"),"Cartesian conversion did not refresh fields");
            require(std::abs(Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("SMA")-initialSma)<1e-6,
               "State preview changed configured spacecraft before Apply");
            choice->setCurrentText("Keplerian");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         }
         require(error.isEmpty(),qPrintable(error));
         require(std::abs(Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("SMA")-initialSma-100)<1e-6,
            "Converted pending state edit was lost during Apply");
         editor->undo(); require(editor->toPlainText()==before && window.buildScript(),"Converted state edit Undo failed");
         sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
      }
      {
         const auto originalMission=editor->toPlainText(); auto &files=fixtureDirectory();
         const auto reportPath=files.filePath("orbit-frames.txt");
         auto fixture=originalMission;
         // Use explicit input time in both runs. The engine's default
         // constructor epoch is rounded through a double before precision-time
         // initialization; parsing its printed time is a distinct operation.
         fixture.replace("Create Spacecraft QtSat;","Create Spacecraft QtSat;\nQtSat.DateFormat = TAIModJulian;\nQtSat.Epoch = '21545';");
         // Request a tight stopping epoch, then compare the actual epochs too.
         fixture.replace(QRegularExpression("(QtSat\\.ElapsedSecs\\s*=\\s*600(?:\\.0*)?)(\\s*\\})"),"\\1, StopTolerance = 1e-10\\2");
         require(fixture.contains("StopTolerance = 1e-10"),"Orbit-frame fixture stopping tolerance not set");
         fixture.replace("BeginMissionSequence;", "Create CoordinateSystem QtEarthFixed QtMoonEq QtBaryFrame QtCircular;\n"
            "QtEarthFixed.Origin = Earth;\nQtEarthFixed.Axes = BodyFixed;\nQtMoonEq.Origin = Luna;\nQtMoonEq.Axes = MJ2000Eq;\n"
            "Create Barycenter QtBary;\nQtBary.BodyNames = {Earth, Luna};\nQtBaryFrame.Origin = QtBary;\nQtBaryFrame.Axes = MJ2000Eq;\n"
            "QtCircular.Origin = Earth;\nQtCircular.Axes = ObjectReferenced;\nQtCircular.Primary = Earth;\nQtCircular.Secondary = QtSat;\nQtCircular.XAxis = R;\nQtCircular.ZAxis = N;\n"
            "Create Variable OrbitUnused;\nCreate ReportFile FrameReport;\nFrameReport.Filename = '"+reportPath+"';\nFrameReport.WriteHeaders = false;\nFrameReport.Precision = 16;\n"
            "FrameReport.Add = {QtSat.A1ModJulian, QtSat.EarthMJ2000Eq.X, QtSat.EarthMJ2000Eq.Y, QtSat.EarthMJ2000Eq.Z, QtSat.EarthMJ2000Eq.VX, QtSat.EarthMJ2000Eq.VY, QtSat.EarthMJ2000Eq.VZ};\n"
            "QtProp.Accuracy = 1e-13;\nQtProp.InitialStepSize = 10;\nBeginMissionSequence;");
         editor->setPlainText(fixture); require(window.buildScript(),"Orbit-frame fixture build failed");
         auto readReport=[&] {
            QFile file(reportPath); require(file.open(QIODevice::ReadOnly),"Orbit-frame report unavailable");
            const auto lines=QString::fromUtf8(file.readAll()).trimmed().split('\n'); require(lines.size()>1,"Orbit-frame report lacks samples");
            QVector<QVector<double>> samples;
            for (const auto &line:{lines.front(),lines.back()}) {
               const auto values=line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(values.size()==7,"Orbit-frame report columns missing");
               QVector<double> sample; for (const auto &value:values) sample.append(value.toDouble()); samples.append(sample);
            }
            return samples;
         };
         require(window.runMission()==MainWindow::RunResult::Completed,"Orbit-frame reference run failed");
         const auto reference=readReport(); const auto before=editor->toPlainText();
         const auto referenceStop=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat"))->GetEpochGT();
         const auto referenceEpoch=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"))->GetEpochGT();
         auto *referenceForces=dynamic_cast<ODEModel *>(Moderator::Instance()->GetInternalObject("QtForces"));
         require(referenceForces,"Orbit-frame reference force model missing");
         const auto derivatives=referenceForces->GetDerivativesForSpacecraft(dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat")));
         auto integrationTime=[&] {
            for (auto *command=Moderator::Instance()->GetFirstCommand();command;command=command->GetNext()) {
               if (command->GetTypeName()!="Propagate") continue;
               auto *setup=dynamic_cast<PropSetup *>(command->GetRefObject(Gmat::PROP_SETUP,"QtProp",0));
               require(setup && setup->GetODEModel(),"Orbit-frame integration clock missing");
               return setup->GetODEModel()->GetTime();
            }
            throw std::runtime_error("Orbit-frame propagation command missing");
         };
         const auto referenceIntegrationTime=integrationTime();

         auto compareOrbit=[&](const QVector<QVector<double>> &samples,const QString &description) {
            const auto initialOffset=(dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"))->GetEpochGT()-referenceEpoch).GetTimeInSec();
            require(std::abs(initialOffset)<1e-9,qPrintable(QString("Orbit round trip changed initial epoch by %1 sec").arg(initialOffset,0,'g',17)));
            const auto stop=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat"))->GetEpochGT();
            const auto epochOffset=(stop-referenceStop).GetTimeInSec();
            require(std::abs(epochOffset)<1e-6,"Round-trip propagation stopping epochs differ by more than one microsecond");
            // The absolute spacecraft epoch can be rounded through a double.
            // Align physical states with the executing propagator's elapsed
            // seconds, not that rounded absolute-time difference. Retain both
            // independent sub-microsecond bounds and the original state limits.
            const auto dt=integrationTime()-referenceIntegrationTime;
            require(std::abs(dt)<1e-6,"Round-trip integration durations differ by more than one microsecond");
            for (int sample=0;sample<2;++sample) {
               require(std::abs(samples[sample][0]-reference[sample][0])<1e-10,"Orbit round trip changed epoch");
               for (int i=0;i<6;++i) {
                  // The engine can finish at slightly different epochs despite
                  // the requested stop tolerance. Align only that bounded
                  // sub-microsecond offset using reference velocity/acceleration;
                  // the second-order remainder is far below these tolerances.
                  const auto expected=reference[sample][i+1]+(sample ? derivatives[i]*dt : 0);
                  require(std::abs(samples[sample][i+1]-expected)<(i<3 ? 1e-6 : 1e-9),qPrintable(QString("%1 changed sample %2 element %3: got %4 expected %5; epoch offset %6 sec").arg(description).arg(sample).arg(i).arg(samples[sample][i+1],0,'g',17).arg(expected,0,'g',17).arg(dt,0,'g',17)));
               }
            }
         };
         const auto initial=Moderator::Instance()->GetConfiguredObject("QtSat")->GetStringParameter("CoordinateSystem");
         QString applied="Apply not invoked"; Rvector6 expectedFixed;
         {
            QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("QtSat"),[&](const QMap<QString,QString> &changes) {
               applied=window.applyResourceChanges("QtSat",changes,before); return applied;
            },&owner,before); workflowView(panel);
            auto *frame=panel.findChild<QComboBox *>("spacecraftOrbitFrame"),*representation=panel.findChild<QComboBox *>("spacecraftStateRepresentation"),*anomaly=panel.findChild<QComboBox *>("spacecraftAnomalyType");
            auto *grid=panel.findChild<QTableWidget *>();
            auto field=[&](const QString &name)->QTableWidgetItem * { for (int row=0;row<grid->rowCount();++row) if (grid->item(row,0)->text()==name) return grid->item(row,1); return nullptr; };
            require(frame && representation && anomaly,"Typed spacecraft orbit selectors missing");
            require(!field("StateType") && grid->isColumnHidden(3),"Orbit tab exposes deprecated input-state alias or redundant action column");
            const auto x=field("X")->text(); field("X")->setText("nan"); frame->setCurrentText("QtEarthFixed");
            require(frame->currentText()==QString::fromStdString(initial) && field("X")->text()=="nan","Invalid frame conversion lost pending correction input");
            field("X")->setText(x);
            const auto epoch=field("Epoch")->text(); field("Epoch")->setText("bad epoch"); frame->setCurrentText("QtEarthFixed");
            require(frame->currentText()==QString::fromStdString(initial) && field("Epoch")->text()=="bad epoch","Invalid epoch allowed orbit-frame conversion");
            field("Epoch")->setText(epoch); frame->setCurrentText("QtEarthFixed");
            require(frame->currentText()=="QtEarthFixed" && representation->findText("Planetodetic")>=0,"Body-fixed frame representations not refreshed");
            Rvector6 oldState; for (int i=0;i<6;++i) oldState[i]=reference[0][i+1];
            CoordinateConverter converter;
            auto *clock=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
            if (clock->HasPrecisionTime()) converter.Convert(clock->GetEpochGT(),oldState,spacecraftOrbitFrame(QString::fromStdString(initial)),expectedFixed,spacecraftOrbitFrame("QtEarthFixed"));
            else converter.Convert(clock->GetEpoch(),oldState,spacecraftOrbitFrame(QString::fromStdString(initial)),expectedFixed,spacecraftOrbitFrame("QtEarthFixed"));
            for (int i=0;i<6;++i) {
               const QStringList names={"X","Y","Z","VX","VY","VZ"};
               require(std::abs(field(names[i])->text().toDouble()-expectedFixed[i])<1e-7,qPrintable(QString("Frame preview differs from engine coordinate conversion: %1 got %2 expected %3 epoch %4").arg(names[i],field(names[i])->text(),QString::number(expectedFixed[i],'g',17),field("Epoch")->text())));
            }
            representation->setCurrentText("Planetodetic"); require(field("PlanetodeticLAT") && field("PlanetodeticLON") && !field("X"),"Planetodetic labels missing");
            frame->setCurrentText(QString::fromStdString(initial));
            require(representation->currentText()=="Cartesian" && representation->findText("Planetodetic")<0 && field("X"),"Inertial frame did not fall back from fixed-only representation");
            require(std::abs(field("X")->text().toDouble()-reference[0][1])<1e-7,qPrintable("Fixed/planetodetic round trip changed X: "+field("X")->text()));
            representation->setCurrentText("Keplerian");
            require(anomaly->isEnabled() && field("TA"),"Keplerian anomaly selector unavailable");
            anomaly->setCurrentText("MA"); require(field("MA") && !field("TA"),"Anomaly selection reinterprets old element instead of converting");
            anomaly->setCurrentText("EA"); require(field("EA"),"Eccentric anomaly labels missing"); anomaly->setCurrentText("TA");
            require(anomaly->findText("HA")<0,"Elliptic orbit offers hyperbolic anomaly");
            anomaly->addItem("HA"); anomaly->setCurrentText("HA"); require(anomaly->currentText()=="TA" && field("TA"),"Elliptic orbit accepted hyperbolic anomaly");
            representation->setCurrentText("Cartesian");
            require(std::abs(field("X")->text().toDouble()-reference[0][1])<1e-7,qPrintable("Anomaly round trip changed X: "+field("X")->text()));
            for (const auto &type:QStringList{"ModifiedKeplerian","SphericalAZFPA","SphericalRADEC","Equinoctial","ModifiedEquinoctial","AlternateEquinoctial","Delaunay","BrouwerMeanShort","BrouwerMeanLong"}) {
               representation->setCurrentText(type); require(representation->currentText()==type,qPrintable("Failed to display "+type));
               representation->setCurrentText("Cartesian");
               const QStringList names={"X","Y","Z","VX","VY","VZ"};
               for (int i=0;i<6;++i) require(std::abs(field(names[i])->text().toDouble()-reference[0][i+1])<1e-7,qPrintable(type+" display round trip changed the orbit"));
            }
            representation->setCurrentText("Keplerian");
            frame->setCurrentText("QtCircular"); require(frame->currentText()==QString::fromStdString(initial) && field("SMA"),"Circular-reference frame accepted");
            frame->addItem("MissingFrame"); frame->setCurrentText("MissingFrame"); require(frame->currentText()==QString::fromStdString(initial),"Unknown frame accepted");
            frame->setCurrentText("QtBaryFrame");
            require(frame->currentText()=="QtBaryFrame" && representation->currentText()=="Cartesian" && representation->findText("Keplerian")<0,"Non-body-origin frame representation restrictions missing");
            frame->setCurrentText("QtMoonEq");
            require(representation->findText("Keplerian")>=0 && field("X"),"Moon-origin frame selection unavailable");
            const auto shifted=field("X")->text().toDouble(); require(std::abs(shifted-reference[0][1])>1000,"Origin-changing conversion omitted translation");
            frame->setCurrentText(QString::fromStdString(initial));
            for (int i=0;i<6;++i) {
               const QStringList names={"X","Y","Z","VX","VY","VZ"}; require(std::abs(field(names[i])->text().toDouble()-reference[0][i+1])<1e-7,qPrintable(QString("Frame/origin representation round trip changed physical state: %1 got %2 expected %3").arg(names[i],field(names[i])->text(),QString::number(reference[0][i+1],'g',17))));
            }
            require(Moderator::Instance()->GetConfiguredObject("QtSat")->GetStringParameter("CoordinateSystem")==initial && editor->toPlainText()==before,"Frame preview mutated configured mission");
            frame->setCurrentText("QtEarthFixed"); representation->setCurrentText("Planetodetic");
            if (!orbitCapture.isEmpty()) {
               owner.setWindowTitle("Spacecraft orbit"); owner.resize(1000,720); panel.setGeometry(owner.rect()); owner.show(); panel.show();
               QEventLoop exposed; QTimer::singleShot(200,&exposed,&QEventLoop::quit); exposed.exec();
               require(panel.grab().save(orbitCapture),"Orbit panel capture failed");
            }
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         }
         require(applied.isEmpty(),qPrintable(applied)); const auto after=editor->toPlainText();
         require(after.mid(after.indexOf("BeginMissionSequence;"))==before.mid(before.indexOf("BeginMissionSequence;")),"Orbit-frame Apply rewrote mission commands");
         editor->undo(); require(editor->toPlainText()==before && window.buildScript(),"Orbit-frame paired Apply not one exact Undo");
         editor->redo(); require(editor->toPlainText()==after && window.buildScript(),"Orbit-frame paired Apply not one exact Redo");
         const auto path=files.filePath("orbit-frames.script"); require(window.saveScriptTo(path) && checkedLoadScript(path) && window.buildScript(),"Orbit-frame save/reopen failed");
         auto *configured=Moderator::Instance()->GetConfiguredObject("QtSat");
         require(configured->GetStringParameter("CoordinateSystem")=="QtEarthFixed" && configured->GetStringParameter("DisplayStateType")=="Planetodetic","Orbit-frame settings lost on reopen");
         require(window.runMission()==MainWindow::RunResult::Completed,"GUI-converted orbit failed propagation"); const auto converted=readReport();
         compareOrbit(converted,"Frame save/reopen");
         require(editor->toPlainText().contains("QtSat.X =") && !editor->toPlainText().contains("QtSat.PlanetodeticLAT ="),"Planetodetic display did not retain Cartesian script input");
         // Reconstructing an unrelated resource must preserve this spacecraft too.
         const auto planetodeticScript=editor->toPlainText();
         require(window.applyResourceChanges("QtProp",{{"InitialStepSize","45"}},planetodeticScript).isEmpty(),"Unrelated propagator Apply failed");
         const auto unchanged=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"))->GetState("Cartesian");
         for (int i=0;i<6;++i) require(std::abs(unchanged[i]-expectedFixed[i])<1e-7,"Unrelated Apply changed Planetodetic spacecraft state");
         editor->undo(); require(editor->toPlainText()==planetodeticScript && window.buildScript(),"Unrelated orbit-preserving Apply Undo failed");
         require(window.deleteResource("OrbitUnused",planetodeticScript).isEmpty(),"Unrelated resource deletion failed");
         const auto afterDeletion=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"))->GetState("Cartesian");
         for (int i=0;i<6;++i) require(std::abs(afterDeletion[i]-expectedFixed[i])<1e-7,"Unrelated resource deletion changed Planetodetic orbit");
         editor->undo(); require(editor->toPlainText()==planetodeticScript && window.buildScript(),"Orbit-preserving deletion Undo failed");
         // Exercise non-true anomaly through Apply, serialization and reopened
         // table reads, then restore the body-fixed fixture for epoch edits.
         for (const auto &selectedAnomaly:QStringList{"MA","EA"}) {
            editor->setPlainText(planetodeticScript); require(window.buildScript(),"Anomaly fixture restore failed");
            const auto source=editor->toPlainText(); QString result="Apply not invoked";
            {
               QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("QtSat"),[&](const auto &changes) {
                  result=window.applyResourceChanges("QtSat",changes,source); return result;
               },&owner,source); workflowView(panel);
               panel.findChild<QComboBox *>("spacecraftOrbitFrame")->setCurrentText(QString::fromStdString(initial));
               panel.findChild<QComboBox *>("spacecraftStateRepresentation")->setCurrentText("Keplerian");
               panel.findChild<QComboBox *>("spacecraftAnomalyType")->setCurrentText(selectedAnomaly);
               panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
               require(result.isEmpty(),qPrintable(result));
            }
            require(window.saveScriptTo(path) && checkedLoadScript(path) && window.buildScript(),"Anomaly save/reopen failed");
            auto *anomalySat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
            require(QString::fromStdString(anomalySat->GetAnomalyType())==selectedAnomaly,"Anomaly selection lost on reopen");
            const auto anomalyState=anomalySat->GetState("Keplerian"); bool displayed=false;
            for (const auto &property:resourceProperties(*anomalySat)) if (property.name==selectedAnomaly) displayed=std::abs(property.value.toDouble()-anomalyState[5])<1e-10;
            require(displayed,"Reopened anomaly table read a scalar value instead of the complete state");
            require(window.runMission()==MainWindow::RunResult::Completed,"GUI anomaly orbit failed propagation");
            compareOrbit(readReport(),"Anomaly save/reopen");
         }
         editor->setPlainText(planetodeticScript); require(window.buildScript(),"Planetodetic fixture restore after anomaly test failed");
         // Pending state and epoch edits are converted at the new epoch, rather
         // than requiring an intermediate Apply of either setting.
         const auto currentScript=editor->toPlainText(); applied="Apply not invoked"; Rvector6 expectedPending;
         require(!window.applyResourceChanges("QtSat",{{"CoordinateSystem",QString::fromStdString(initial)},{"DisplayStateType","Cartesian"},{"X","nan"}},currentScript).isEmpty() && editor->toPlainText()==currentScript,
            "Invalid grouped orbit Apply changed the mission");
         {
            QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("QtSat"),[&](const QMap<QString,QString> &changes) {
               applied=window.applyResourceChanges("QtSat",changes,currentScript); return applied;
            },&owner,currentScript); workflowView(panel);
            auto *representation=panel.findChild<QComboBox *>("spacecraftStateRepresentation"); representation->setCurrentText("Cartesian");
            auto *grid=panel.findChild<QTableWidget *>(); auto field=[&](const QString &name)->QTableWidgetItem * { for (int row=0;row<grid->rowCount();++row) if (grid->item(row,0)->text()==name) return grid->item(row,1); return nullptr; };
            const double changedEpoch=field("Epoch")->text().toDouble()+1;
            field("Epoch")->setText(QString::number(changedEpoch,'g',17)); field("X")->setText(QString::number(field("X")->text().toDouble()+10,'g',17));
            Rvector6 editedFixed,expected; const QStringList names={"X","Y","Z","VX","VY","VZ"}; for (int i=0;i<6;++i) editedFixed[i]=field(names[i])->text().toDouble();
            CoordinateConverter converter;
            std::unique_ptr<Spacecraft> clock(dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat")->Clone()));
            clock->SetStringParameter("Epoch",QString::number(changedEpoch,'g',17).toStdString());
            if (clock->HasPrecisionTime()) converter.Convert(clock->GetEpochGT(),editedFixed,spacecraftOrbitFrame("QtEarthFixed"),expected,spacecraftOrbitFrame(QString::fromStdString(initial)));
            else converter.Convert(clock->GetEpoch(),editedFixed,spacecraftOrbitFrame("QtEarthFixed"),expected,spacecraftOrbitFrame(QString::fromStdString(initial)));
            expectedPending=expected;
            panel.findChild<QComboBox *>("spacecraftOrbitFrame")->setCurrentText(QString::fromStdString(initial));
            for (int i=0;i<6;++i) require(std::abs(field(names[i])->text().toDouble()-expected[i])<1e-7,"Pending epoch/state frame conversion used stale settings");
            require(std::abs(Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("A1Epoch")-changedEpoch)>0.5,"Pending epoch conversion mutated configured spacecraft");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         }
         require(applied.isEmpty(),qPrintable(applied));
         require(std::abs(Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("A1Epoch")-reference[0][0]-1)<1e-8,"Pending epoch lost on paired Apply");
         require(window.saveScriptTo(path) && checkedLoadScript(path) && window.runMission()==MainWindow::RunResult::Completed,"Pending epoch/state save/reopen propagation failed");
         const auto pendingReport=readReport();
         for (int i=0;i<6;++i) require(std::abs(pendingReport[0][i+1]-expectedPending[i])<(i<3 ? 1e-6 : 1e-9),qPrintable(QString("Pending frame/epoch/state Apply changed element %1: got %2 expected %3").arg(i).arg(pendingReport[0][i+1],0,'g',17).arg(expectedPending[i],0,'g',17)));
         auto hyperbolic=fixture;
         hyperbolic.replace("Create Spacecraft QtSat;","Create Spacecraft QtSat;\nQtSat.DisplayStateType = Keplerian;\nQtSat.SMA = -18000;\nQtSat.ECC = 1.5;\nQtSat.INC = 35;\nQtSat.RAAN = 20;\nQtSat.AOP = 30;\nQtSat.TA = 20;");
         editor->setPlainText(hyperbolic); require(window.runMission()==MainWindow::RunResult::Completed,"Hyperbolic orbit reference run failed");
         const auto hyperReference=readReport(); const auto hyperStop=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat"))->GetEpochGT();
         const auto hyperDerivatives=dynamic_cast<ODEModel *>(Moderator::Instance()->GetInternalObject("QtForces"))->GetDerivativesForSpacecraft(dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat")));
         QString hyperError="Apply not invoked";
         {
            QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("QtSat"),[&](const auto &changes) {
               hyperError=window.applyResourceChanges("QtSat",changes,hyperbolic); return hyperError;
            },&owner,hyperbolic); workflowView(panel);
            auto *representation=panel.findChild<QComboBox *>("spacecraftStateRepresentation"),*anomaly=panel.findChild<QComboBox *>("spacecraftAnomalyType");
            require(anomaly->findText("HA")>=0 && anomaly->findText("EA")<0,"Hyperbolic orbit anomaly choices incorrect");
            for (const auto &type:QStringList{"IncomingAsymptote","OutgoingAsymptote"}) {
               representation->setCurrentText(type); require(representation->currentText()==type,"Hyperbolic asymptote conversion rejected");
               representation->setCurrentText("Cartesian");
               auto *grid=panel.findChild<QTableWidget *>(); int element=0;
               for (int row=0;row<grid->rowCount();++row) if (QStringList{"X","Y","Z","VX","VY","VZ"}.contains(grid->item(row,0)->text())) {
                  const auto index=QStringList{"X","Y","Z","VX","VY","VZ"}.indexOf(grid->item(row,0)->text());
                  require(std::abs(grid->item(row,1)->text().toDouble()-hyperReference[0][index+1])<1e-7,"Asymptote display round trip changed orbit"); ++element;
               }
               require(element==6,"Hyperbolic Cartesian state fields missing");
            }
            representation->setCurrentText("Keplerian"); anomaly->setCurrentText("HA");
            require(anomaly->currentText()=="HA","Hyperbolic anomaly conversion failed");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         }
         require(hyperError.isEmpty(),qPrintable(hyperError));
         require(window.saveScriptTo(path) && checkedLoadScript(path) && window.buildScript(),"Hyperbolic anomaly save/reopen failed");
         auto *hyperSat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
         require(hyperSat->GetAnomalyType()=="HA","Hyperbolic anomaly lost on reopen");
         const auto hyperState=hyperSat->GetState("Keplerian"); bool hyperDisplayed=false;
         for (const auto &property:resourceProperties(*hyperSat)) if (property.name=="HA") hyperDisplayed=std::abs(property.value.toDouble()-hyperState[5])<1e-10;
         require(hyperDisplayed,"Hyperbolic anomaly table value or label incorrect");
         require(window.runMission()==MainWindow::RunResult::Completed,"Reopened hyperbolic orbit propagation failed"); const auto hyperReport=readReport();
         const auto hyperDt=(dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat"))->GetEpochGT()-hyperStop).GetTimeInSec();
         require(std::abs(hyperDt)<1e-6,"Hyperbolic stopping epoch changed");
         for (int sample=0;sample<2;++sample) for (int i=0;i<6;++i)
            require(std::abs(hyperReport[sample][i+1]-hyperReference[sample][i+1]-(sample ? hyperDerivatives[i]*hyperDt : 0))<(i<3 ? 1e-6 : 1e-9),"Hyperbolic anomaly save/reopen changed propagated state");
         editor->setPlainText(originalMission); require(window.buildScript(),"Orbit-frame fixture restoration failed");
         sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
         std::cout<<"PASS: spacecraft orbit frame/origin, representation/anomaly conversion, pending epoch/state Apply, Undo/Redo, save/reopen and report execution\n";
         if (!orbitCapture.isEmpty()) return 0;
      }
      {
         const auto before=editor->toPlainText(); auto &files=fixtureDirectory();
         const auto reportPath=files.filePath("ballistics.txt");
         const auto spadPath=QFileInfo("../data/vehicle/spad/SphericalModel.spo").absoluteFilePath();
         require(QFileInfo::exists(spadPath),"SPAD qualification fixture missing");
         editor->setPlainText("Create Spacecraft MassSat;\nCreate ForceModel MassFM;\nMassFM.SRP = On;\nMassFM.SRP.SRPModel = SPADFile;\n"
            "Create Propagator MassProp;\nMassProp.FM = MassFM;\nCreate ReportFile MassReport;\nMassReport.Filename = '"+reportPath+"';\n"
            "MassReport.WriteHeaders = false;\nMassReport.FixedWidth = false;\nMassReport.Delimiter = ',';\nMassReport.Precision = 16;\n"
            "BeginMissionSequence;\nPropagate MassProp(MassSat) {MassSat.ElapsedSecs = 60}; % retain mass mission\n"
            "Report MassReport MassSat.DryMass MassSat.Cd MassSat.Cr MassSat.DragArea MassSat.SRPArea;\n");
         require(window.buildScript(),"Ballistics GUI fixture failed to build");
         const auto source=editor->toPlainText(); QString error="Apply not called";
         {
            QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("MassSat"),[&](const QMap<QString,QString> &changes) {
               error=window.applyResourceChanges("MassSat",changes,source); return error;
            },&owner,source); workflowView(panel);
            auto *button=panel.findChild<QPushButton *>("spacecraftBallisticsMass"); require(button,"Ballistics editor missing");
            auto *grid=panel.findChild<QTableWidget *>();
            const auto value=[&](const QString &name) {
               for (int row=0;row<grid->rowCount();++row) if (grid->item(row,0)->text()==name) {
                  if (auto *combo=qobject_cast<QComboBox *>(grid->cellWidget(row,1))) return combo->currentText();
                  return grid->item(row,1)->text();
               }
               return QString();
            };
            const auto original=value("DryMass");
            workflowAfterExposure(0,&panel,[&] {
               auto *dialog=panel.findChild<QDialog *>("ballisticsMassDialog"); require(dialog,"Ballistics dialog missing");
               dialog->findChild<QLineEdit *>("ballistics_DryMass")->setText("999"); dialog->reject();
            }); button->click();
            require(value("DryMass")==original && !panel.hasChanges(),"Cancelled ballistics edits leaked into properties");
            workflowAfterExposure(0,&panel,[&] {
               auto *dialog=panel.findChild<QDialog *>("ballisticsMassDialog");
               require(dialog->findChildren<QGroupBox *>().size()==2,"wx ballistics groups missing");
               auto *mass=dialog->findChild<QLineEdit *>("ballistics_DryMass"); auto *buttons=dialog->findChild<QDialogButtonBox *>();
               mass->setText("-1"); buttons->button(QDialogButtonBox::Ok)->click();
               require(!dialog->findChild<QLabel *>("ballisticsError")->text().isEmpty() && value("DryMass")==original,
                  "Invalid ballistics mass accepted or partially applied");
               const QMap<QString,QString> numeric={{"DryMass","975.5"},{"Cd","2.4"},{"Cr","1.7"},{"DragArea","18"},{"SRPArea","14"},
                  {"SPADSRPScaleFactor","1.25"},{"SPADDragScaleFactor","1.5"}};
               for (auto it=numeric.cbegin();it!=numeric.cend();++it) dialog->findChild<QLineEdit *>("ballistics_"+it.key())->setText(it.value());
               for (const auto &name:{QString("SPADSRPInterpolationMethod"),QString("SPADDragInterpolationMethod")}) {
                  auto *combo=dialog->findChild<QComboBox *>("ballistics_"+name);
                  require(combo && combo->findText("Bicubic")>=0,"SPAD interpolation choices missing"); combo->setCurrentText("Bicubic");
               }
               auto *file=dialog->findChild<QLineEdit *>("ballistics_SPADSRPFile");
               const auto previous=file->text();
               workflowAfterExposure(0,dialog,[&] { dialog->findChild<QFileDialog *>("ballisticsFileDialog")->reject(); });
               dialog->findChild<QPushButton *>("ballisticsBrowse_SPADSRPFile")->click();
               require(file->text()==previous,"Cancelled SPAD chooser changed the path");
               workflowAfterExposure(0,dialog,[&] {
                  auto *picker=dialog->findChild<QFileDialog *>("ballisticsFileDialog");
                  require(picker->fileMode()==QFileDialog::ExistingFile,"SPAD chooser is not an input picker");
                  picker->selectFile(spadPath); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection);
               }); dialog->findChild<QPushButton *>("ballisticsBrowse_SPADSRPFile")->click();
               require(file->text()==spadPath,"SPAD file selection lost its path");
               dialog->findChild<QLineEdit *>("ballistics_SPADDragFile")->setText(spadPath);
               buttons->button(QDialogButtonBox::Ok)->click();
            }); button->click();
            require(panel.hasChanges() && value("DryMass")=="975.5" && value("SPADSRPInterpolationMethod")=="Bicubic" &&
               editor->toPlainText()==source && Moderator::Instance()->GetConfiguredObject("MassSat")->GetRealParameter("DryMass")!=975.5,
               "Ballistics OK failed to preserve pending edits or mutated mission before Apply");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         }
         require(error.isEmpty(),qPrintable(error));
         require(editor->toPlainText().contains("% retain mass mission"),"Ballistics Apply lost mission comments");
         const auto appliedMassSource=editor->toPlainText();
         editor->undo(); require(editor->toPlainText()==source && window.buildScript(),"Ballistics Apply was not one reversible source edit");
         require(Moderator::Instance()->GetConfiguredObject("MassSat")->GetRealParameter("DryMass")!=975.5,"Ballistics Undo retained the edited mass");
         editor->redo(); require(editor->toPlainText()==appliedMassSource && window.buildScript(),"Ballistics Redo lost applied settings");
         const auto path=files.filePath("mass.script");
         require(window.saveScriptTo(path) && checkedLoadScript(path) && window.buildScript(),"Ballistics save/reopen failed");
         require(window.runMission()==MainWindow::RunResult::Completed,"GUI-configured SPAD mission failed");
         QFile report(reportPath); require(report.open(QIODevice::ReadOnly),"Ballistics report missing");
         const auto numbers=QString::fromUtf8(report.readAll()).trimmed().split(',');
         require(numbers.size()==5,"Ballistics report fields missing");
         const double expected[]={975.5,2.4,1.7,18,14};
         for (int i=0;i<5;++i) require(std::abs(numbers[i].toDouble()-expected[i])<1e-10,"Reopened ballistics settings changed report results");
         auto *configured=Moderator::Instance()->GetConfiguredObject("MassSat");
         require(configured->GetStringParameter("SPADSRPFile")==spadPath.toStdString() &&
            configured->GetStringParameter("SPADDragInterpolationMethod")=="Bicubic" &&
            configured->GetRealParameter("SPADSRPScaleFactor")==1.25 && configured->GetRealParameter("SPADDragScaleFactor")==1.5,
            "Reopened SPAD configuration lost settings");
         editor->setPlainText(before); require(window.buildScript(),"Ballistics fixture restoration failed");
         sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
      }
      {
         const auto source=editor->toPlainText(); auto &assets=fixtureDirectory();
         QFile mesh(assets.filePath("preview model.obj")); require(mesh.open(QIODevice::WriteOnly),"Preview mesh fixture failed");
         mesh.write("v -1 -1 0\nv 1 -1 0\nv 0 1 0\nf 1 2 3\n"); mesh.close();
         QFile invalidMesh(assets.filePath("broken.obj")); require(invalidMesh.open(QIODevice::WriteOnly),"Bad mesh fixture failed");
         invalidMesh.write("not a model\n"); invalidMesh.close();
         auto *configured=Moderator::Instance()->GetConfiguredObject("QtSat");
         const double initialEpoch=configured->GetRealParameter("A1Epoch");
         double state[6]; for (int i=0;i<6;++i) state[i]=configured->GetRealParameter("Element"+std::to_string(i+1));
         QString error="Visual model Apply not called";
         {
            QWidget owner; ResourceEditor panel(*configured,[&](const auto &changes) {
               error=window.applyResourceChanges("QtSat",changes,source); return error;
            },&owner,source); workflowView(panel);
            auto *button=panel.findChild<QPushButton *>("spacecraftVisualModel"); require(button,"Visual model editor absent");
            workflowAfterExposure(0,&panel,[&] {
               auto *dialog=panel.findChild<QDialog *>("visualModelDialog");
               require(dialog,"Visual model dialog missing"); dialog->findChild<QLineEdit *>("visual_ModelOffsetX")->setText("2"); dialog->reject();
            }); button->click(); require(!panel.hasChanges(),"Visual model Cancel changed pending properties");
            workflowAfterExposure(0,&panel,[&] {
               auto *base=panel.findChild<QDialog *>("visualModelDialog"); auto *dialog=dynamic_cast<VisualModelDialog *>(base);
               auto field=[&](const QString &name) { return dialog->findChild<QLineEdit *>("visual_"+name); };
               auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
               field("ModelScale")->setText("nan"); ok->click();
               require(dialog->isVisible() && !dialog->findChild<QLabel *>("visualModelError")->text().isEmpty(),"Nonfinite visual transform accepted");
               dialog->findChild<QPushButton *>("visualAutoscale")->click(); require(field("ModelScale")->text()=="1","Autoscale did not reset wx scale");
               field("ModelOffsetX")->setText("1"); field("ModelOffsetY")->setText("2"); field("ModelOffsetZ")->setText("3");
               dialog->findChild<QPushButton *>("visualRecenter")->click();
               for (const auto &axis:QStringList{"X","Y","Z"}) require(field("ModelOffset"+axis)->text()=="0","Recenter retained an offset");
               dialog->findChild<QSlider *>("visualSlider_ModelRotationX")->setValue(90);
               dialog->findChild<QSlider *>("visualSlider_ModelRotationY")->setValue(-30);
               dialog->findChild<QSlider *>("visualSlider_ModelRotationZ")->setValue(45);
               dialog->findChild<QSlider *>("visualSlider_ModelOffsetX")->setValue(125);
               dialog->findChild<QSlider *>("visualSlider_ModelOffsetY")->setValue(-50);
               dialog->findChild<QSlider *>("visualSlider_ModelOffsetZ")->setValue(75); field("ModelScale")->setText("2.5");
               require(field("ModelRotationX")->text()=="90" && field("ModelOffsetX")->text()=="1.25","Transform sliders did not update numeric fields");
               field("ModelFile")->setText(invalidMesh.fileName()); ok->click();
               require(dialog->isVisible() && !dialog->findChild<QLabel *>("visualModelError")->text().isEmpty(),"Unreadable model accepted");
               const auto previous=field("ModelFile")->text();
               workflowAfterExposure(0,dialog,[&] { dialog->findChild<QFileDialog *>("visualModelFileDialog")->reject(); });
               dialog->findChild<QPushButton *>("visualBrowseModel")->click(); require(field("ModelFile")->text()==previous,"Cancelled model chooser changed path");
               workflowAfterExposure(0,dialog,[&] {
                  auto *picker=dialog->findChild<QFileDialog *>("visualModelFileDialog"); require(picker->fileMode()==QFileDialog::ExistingFile,"Visual model chooser accepted nonexistent files");
                  picker->selectFile(mesh.fileName()); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection);
               }); dialog->findChild<QPushButton *>("visualBrowseModel")->click();
               for (const auto &axis:QStringList{"X","Y","Z"})
                  require(field("ModelRotation"+axis)->text()=="0" && field("ModelOffset"+axis)->text()=="0","New model selection did not reset wx transforms");
               require(field("ModelScale")->text()=="1","New model selection did not reset wx scale");
               field("ModelRotationX")->setText("90"); field("ModelRotationY")->setText("-30");
               field("ModelOffsetY")->setText("-.5"); field("ModelOffsetZ")->setText(".75");
               field("ModelOffsetX")->setText("7"); field("ModelRotationZ")->setText("225"); field("ModelScale")->setText("2000");
               QMetaObject::invokeMethod(field("ModelScale"),"editingFinished",Qt::DirectConnection);
               require(field("ModelOffsetX")->text()=="3.5" && field("ModelRotationZ")->text()=="-135" && field("ModelScale")->text()=="1000",
                  "Visual editor did not show the engine's normalized bounds");
               field("ModelOffsetX")->setText("1.25"); field("ModelRotationZ")->setText("45"); field("ModelScale")->setText("2.5");
               workflowAfterExposure(0,dialog,[&] {
                  auto *picker=dialog->findChild<QColorDialog *>("visualModelColorDialog"); picker->setCurrentColor(QColor(10,100,220)); picker->accept();
               }); dialog->findChild<QPushButton *>("visualChoose_OrbitColor")->click();
               require(field("OrbitColor")->text()=="[10 100 220]","Visual color picker lost selected RGB values");
               field("TargetColor")->setText("Yellow");
               workflowAfterExposure(0,dialog,[&] { dialog->findChild<QColorDialog *>("visualModelColorDialog")->reject(); });
               dialog->findChild<QPushButton *>("visualChoose_TargetColor")->click();
               require(field("TargetColor")->text()=="Yellow","Cancelled visual color picker changed pending color");
               dialog->findChild<QPushButton *>("visualShowEarth")->click();
               require(dialog->previewModel()->curves[1].visible && dialog->previewModel()->curves[1].wireframeObject,"Earth size reference is not a wireframe");
               dialog->findChild<QPushButton *>("visualShowEarth")->click(); ok->click();
               const auto &curve=dialog->previewModel()->curves[0];
               const auto expectedModelPath=QFileInfo(mesh.fileName()).canonicalFilePath();
               const auto previewModelPath=QFileInfo(curve.modelPath).canonicalFilePath();
               if (curve.modelPath!=mesh.fileName())
                  std::cerr<<"Preview resolved path: "<<curve.modelPath.toStdString()<<"; authored path: "<<mesh.fileName().toStdString()<<'\n';
               // Preview uses the engine's resolved native path; separators may
               // differ from QFile's authored spelling on Windows.
               require(!expectedModelPath.isEmpty() && previewModelPath==expectedModelPath && curve.modelScale==2.5 && curve.modelOffset[0]==1.25 && curve.modelRotation[0]==90,
                  qPrintable(QString("Recovered visual preview differs: path %1 expected %2 scale %3 offset %4 rotation %5").arg(curve.modelPath,mesh.fileName()).arg(curve.modelScale).arg(curve.modelOffset[0]).arg(curve.modelRotation[0])));
            }); button->click();
            require(panel.hasChanges() && editor->toPlainText()==source && configured->GetRealParameter("ModelRotationX")!=90,
               "Visual model preview altered configured spacecraft before Apply");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         }
         require(error.isEmpty(),qPrintable(error)); const auto changed=editor->toPlainText();
         require(changed.mid(changed.indexOf("BeginMissionSequence"))==source.mid(source.indexOf("BeginMissionSequence")),"Visual Apply changed mission commands");
         editor->undo(); require(editor->toPlainText()==source && window.buildScript(),"Visual Apply Undo lost source");
         editor->redo(); require(editor->toPlainText()==changed && window.buildScript(),"Visual Apply Redo lost transforms");
         const auto path=assets.filePath("model.script"); require(window.saveScriptTo(path) && checkedLoadScript(path) && window.buildScript(),"Visual model save/reopen failed");
         configured=Moderator::Instance()->GetConfiguredObject("QtSat");
         require(configured->GetStringParameter("ModelFile")==mesh.fileName().toStdString() && configured->GetRealParameter("ModelScale")==2.5 &&
            configured->GetRealParameter("ModelOffsetY")==-.5 && configured->GetRealParameter("ModelOffsetZ")==.75 &&
            configured->GetRealParameter("ModelRotationY")==-30 && configured->GetRealParameter("ModelRotationZ")==45,"Reopened model lost transform settings");
         for (int i=0;i<6;++i) require(std::abs(configured->GetRealParameter("Element"+std::to_string(i+1))-state[i])<1e-9,"Visual settings changed configured orbit");
         require(window.runMission()==MainWindow::RunResult::Completed &&
            std::abs((Moderator::Instance()->GetInternalObject("QtSat")->GetRealParameter("A1Epoch")-initialEpoch)*86400-600)<.01,
            "Visual model edit changed reopened propagation");
         editor->setPlainText(source); require(window.buildScript(),"Visual fixture restoration failed");
         sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
      }
      {
         window.findChild<QAction *>("scriptFind")->trigger();
         auto *integrated=window.findChild<QDialog *>("findReplaceDialog");
         require(integrated && integrated->isVisible(),"Find menu did not open script search"); workflowView(*integrated); integrated->hide();
         QPlainTextEdit text; const QString original="Sat Satellite sat Sat\nSat"; text.setPlainText(original); workflowView(text);
         FindReplaceDialog search(&text,&window); workflowView(search);
         auto *find=search.findChild<QComboBox *>("findText"),*replace=search.findChild<QComboBox *>("replaceText");
         find->setEditText("Sat"); replace->setEditText("SatSat");
         search.findChild<QCheckBox *>("findMatchCase")->setChecked(true);
         search.findChild<QCheckBox *>("findWholeWords")->setChecked(true);
         require(search.findNext() && text.textCursor().selectionStart()==0,"Find skipped first match");
         require(search.findNext(true) && text.textCursor().selectionEnd()==original.size(),"Previous did not wrap to last match");
         search.findChild<QPushButton *>("replaceAll")->click();
         require(text.toPlainText()=="SatSat Satellite sat SatSat\nSatSat","Replace all ignored case/word boundaries or reprocessed inserted text");
         text.undo(); require(text.toPlainText()==original,"Replace all was not a single undo operation");
         text.setTextCursor(QTextCursor(text.document())); search.findNext(); replace->setEditText("Probe");
         search.findChild<QPushButton *>("replaceCurrent")->click();
         require(text.toPlainText()=="Probe Satellite sat Sat\nSat","Replace current changed the wrong occurrence");
         text.undo(); require(text.toPlainText()==original,"Single replacement could not undo");
         text.setReadOnly(true); search.findChild<QPushButton *>("replaceAll")->click();
         require(text.toPlainText()==original,"Search modified read-only script"); text.setReadOnly(false);
         find->setEditText("Missing"); require(!search.findNext() && search.findChild<QLabel *>("findStatus")->text()=="No matches found.","Missing search did not report status");
         require(find->findText("Sat")>=0,"Search history was lost");
      }

      require(window.createResource("CoordinateSystem","UserFrame",editor->toPlainText()).isEmpty(),"Coordinate-system creation failed");
      const auto beforeAxes=editor->toPlainText();
      bool cancelledAxes=false;
      {
         QWidget parent;
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("UserFrame"),[&](const QMap<QString,QString> &changes) {
            return window.applyResourceChanges("UserFrame",changes,beforeAxes);
         },&parent); workflowView(panel);
         auto *button=panel.findChild<QPushButton *>("editCoordinateAxes"); require(button,"Dedicated coordinate axes editor missing");
         workflowAfterExposure(0,&panel,[&] {
            if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
               auto *type=dialog->findChild<QComboBox *>("coordinateAxisType");
               if (type) { type->setCurrentText("ObjectReferenced"); cancelledAxes=true; }
               dialog->reject();
            }
         });
         button->click();
      }
      require(cancelledAxes && editor->toPlainText()==beforeAxes &&
         Moderator::Instance()->GetConfiguredObject("UserFrame")->GetStringParameter("Axes")=="MJ2000Eq","Axes Cancel changed the configured mission");
      bool appliedAxes=false,axisPickers=false;
      {
         QWidget parent;
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("UserFrame"),[&](const QMap<QString,QString> &changes) {
            const auto error=window.applyResourceChanges("UserFrame",changes,beforeAxes); appliedAxes=error.isEmpty();
            if (!error.isEmpty()) std::cerr<<error.toStdString()<<'\n'; return error;
         },&parent); workflowView(panel);
         workflowAfterExposure(0,&panel,[&] {
            if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
               dialog->findChild<QComboBox *>("coordinateAxisType")->setCurrentText("ObjectReferenced");
               axisPickers=dialog->findChild<QPushButton *>("chooseProperty_Primary") && dialog->findChild<QPushButton *>("chooseProperty_Secondary");
               auto *table=dialog->findChild<QTableWidget *>();
               for (int row=0;row<table->rowCount();++row) {
                  const auto name=table->item(row,0)->text();
                  const QMap<QString,QString> values={{"Primary","Earth"},{"Secondary","QtSat"},{"XAxis","R"},{"ZAxis","N"}};
                  if (values.contains(name)) {
                     if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1))) combo->setCurrentText(values[name]);
                     else table->item(row,1)->setText(values[name]);
                  }
               }
               auto *buttons=dialog->findChild<QDialogButtonBox *>();
               buttons->button(QDialogButtonBox::Apply)->click();
               if (dialog->result()!=QDialog::Accepted) dialog->reject();
            }
         });
         panel.findChild<QPushButton *>("editCoordinateAxes")->click();
         require(!appliedAxes && editor->toPlainText()==beforeAxes && panel.hasChanges(),"Nested axes OK committed before parent Apply");
         panel.requestApply();
      }
      require(appliedAxes && axisPickers,"Dependent coordinate-axis settings or reference pickers are missing");
      auto *userFrame=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("UserFrame"));
      require(userFrame && userFrame->GetStringParameter("Axes")=="ObjectReferenced" &&
         userFrame->GetOwnedObject(0)->GetStringParameter("Secondary")=="QtSat","Axis type or secondary reference was not replaced");
      bool secondaryExposed=false;
      for (const auto &field:resourceProperties(*userFrame)) secondaryExposed=secondaryExposed || field.name=="Secondary";
      require(secondaryExposed,"Owned coordinate-axis properties missing after reconstruction");
      require(window.runMission()==MainWindow::RunResult::Completed,"Mission with edited coordinate system failed");
      auto checkRadialFrame=[&] {
         auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat"));
         auto *frame=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetInternalObject("UserFrame"));
         auto *inertial=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetInternalObject("EarthMJ2000Eq"));
         const auto state=sat->GetMJ2000State(sat->GetEpoch());
         Rvector6 radial; CoordinateConverter converter;
         converter.Convert(sat->GetEpoch(),state,inertial,radial,frame);
         const double radius=std::hypot(state[0],state[1],state[2]);
         require(std::abs(radial[0]-radius)<1e-8 && std::abs(radial[1])<1e-8 && std::abs(radial[2])<1e-8,
            "Edited radial frame changed the expected coordinate transform");
      };
      checkRadialFrame();
      auto &axesFiles=fixtureDirectory();
      require(window.saveScriptTo(axesFiles.filePath("axes.script")) && checkedLoadScript(axesFiles.filePath("axes.script")) && window.runMission()==MainWindow::RunResult::Completed,
         "Coordinate axes save/reopen failed");
      checkRadialFrame();
      const auto configuredAxes=editor->toPlainText();
      require(!window.applyResourceChanges("UserFrame",{{"XAxis","R"},{"YAxis","R"},{"ZAxis",""}},configuredAxes).isEmpty(),
         "Invalid duplicate coordinate axes accepted");
      require(editor->toPlainText()==configuredAxes,"Invalid axes changed the source");
      require(!window.applyResourceChanges("EarthMJ2000Eq",{{"Axes","MJ2000Ec"}},configuredAxes).isEmpty(),"Built-in axes were modified");
      editor->setPlainText(beforeAxes); require(window.buildScript(),"Restoring initial coordinate system failed");
      bool appliedType=false;
      {
         QWidget parent;
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("UserFrame"),[&](const QMap<QString,QString> &changes) {
            const auto error=window.applyResourceChanges("UserFrame",changes,beforeAxes); appliedType=error.isEmpty(); return error;
         },&parent); workflowView(panel);
         workflowAfterExposure(0,&panel,[&] {
            if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
               dialog->findChild<QComboBox *>("coordinateAxisType")->setCurrentText("MJ2000Ec");
               dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
               if (dialog->result()!=QDialog::Accepted) dialog->reject();
            }
         });
         panel.findChild<QPushButton *>("editCoordinateAxes")->click();
         require(!appliedType && editor->toPlainText()==beforeAxes && panel.hasChanges(),"Nested type-only OK committed before parent Apply");
         panel.requestApply();
      }
      require(appliedType && Moderator::Instance()->GetConfiguredObject("UserFrame")->GetStringParameter("Axes")=="MJ2000Ec",
         "Axes type-only dialog edit did not replace axes");
      editor->undo(); require(editor->toPlainText()==beforeAxes && window.buildScript(),"Axes edit Undo failed");
      {
         const auto base=editor->toPlainText();
         require(window.applyResourceChanges("UserFrame",{{"Axes","MOEEq"},{"Epoch","30000"}},base).isEmpty(),"Epoch-based axes edit failed");
         auto *frame=Moderator::Instance()->GetConfiguredObject("UserFrame");
         bool epochField=false;
         for (const auto &field:resourceProperties(*frame)) epochField=epochField || (field.name=="Epoch" && field.unit=="A1ModJulian" && field.value=="30000");
         require(epochField && frame->GetOwnedObject(0)->GetRealParameter("Epoch")==30000,"Epoch field is missing or was silently ignored");
         auto epochTransform=[&](double time) {
            auto *inertial=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetInternalObject("EarthMJ2000Eq"));
            auto *epochFrame=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetInternalObject("UserFrame"));
            Rvector6 transformed; CoordinateConverter converter;
            converter.Convert(time,Rvector6(7000,1000,200,0,0,0),epochFrame,transformed,inertial);
            return transformed;
         };
         require(window.runMission()==MainWindow::RunResult::Completed,"Epoch frame mission failed");
         const auto first=epochTransform(31000),later=epochTransform(32000);
         for (int axis=0;axis<3;++axis) require(std::abs(first[axis]-later[axis])<1e-10,"Mean-of-epoch axes changed with evaluation time");
         require(std::abs(std::hypot(first[0],first[1],first[2])-std::hypot(7000.,1000.,200.))<1e-8,"Epoch rotation changed vector length");
         require(window.applyResourceChanges("UserFrame",{{"Epoch","40000"}},editor->toPlainText()).isEmpty() &&
            window.runMission()==MainWindow::RunResult::Completed,"Editing an existing owned epoch failed");
         const auto changed=epochTransform(31000);
         require(std::hypot(first[0]-changed[0],first[1]-changed[1],first[2]-changed[2])>1,"Epoch edit did not change the coordinate transform");
         const auto path=axesFiles.filePath("epoch.script");
         require(window.saveScriptTo(path) && checkedLoadScript(path) && window.runMission()==MainWindow::RunResult::Completed,"Epoch axes save/reopen failed");
         const auto reopened=epochTransform(31000);
         for (int axis=0;axis<3;++axis) require(std::abs(changed[axis]-reopened[axis])<1e-10,"Epoch save/reopen changed calculations");
         require(window.applyResourceChanges("UserFrame",{{"Axes","LocalAlignedConstrained"},{"ReferenceObject","Sun"},
            {"ConstraintCoordinateSystem","EarthMJ2000Eq"}},editor->toPlainText()).isEmpty(),"Constrained axes configuration failed");
         bool referencePicker=false,constraintPicker=false;
         for (const auto &field:resourceProperties(*Moderator::Instance()->GetConfiguredObject("UserFrame"))) {
            if (field.name=="ReferenceObject") referencePicker=field.references.contains("Sun");
            if (field.name=="ConstraintCoordinateSystem") constraintPicker=field.references.contains("EarthMJ2000Eq");
         }
         require(referencePicker && constraintPicker,"Constrained axes reference choices missing");
         auto checkConstraint=[&] {
            require(window.runMission()==MainWindow::RunResult::Completed,"Constrained axes mission failed");
            auto *inertial=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetInternalObject("EarthMJ2000Eq"));
            auto *local=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetInternalObject("UserFrame"));
            auto *solar=Moderator::Instance()->GetSolarSystemInUse();
            const double time=30000;
            const auto direction=solar->GetBody("Sun")->GetMJ2000State(time)-solar->GetBody("Earth")->GetMJ2000State(time);
            const double length=std::hypot(direction[0],direction[1],direction[2]);
            Rvector6 x,z; CoordinateConverter converter;
            converter.Convert(time,Rvector6(1,0,0,0,0,0),local,x,inertial);
            converter.Convert(time,Rvector6(0,0,1,0,0,0),local,z,inertial);
            const double ux=direction[0]/length,uy=direction[1]/length,uz=direction[2]/length;
            const double vertical=std::sqrt(1-uz*uz);
            const double expectedZ[]={-uz*ux/vertical,-uz*uy/vertical,(1-uz*uz)/vertical};
            for (int axis=0;axis<3;++axis) require(std::abs(x[axis]-direction[axis]/length)<1e-10 && std::abs(z[axis]-expectedZ[axis])<1e-10,
               "Constrained axes differ from independently constructed Sun alignment and projected Z constraint");
         };
         checkConstraint();
         const auto valid=editor->toPlainText();
         for (const auto &invalid:QVector<QMap<QString,QString>>{
            {{"AlignmentVectorX","0"}},{{"ConstraintVectorX","1"},{"ConstraintVectorZ","0"}},
            {{"ConstraintReferenceVectorZ","0"}},{{"ReferenceObject","Earth"}},{{"ConstraintCoordinateSystem","UserFrame"}}}) {
            require(!window.applyResourceChanges("UserFrame",invalid,valid).isEmpty() && editor->toPlainText()==valid,
               "Invalid constrained geometry was accepted or changed the script");
         }
         require(window.saveScriptTo(axesFiles.filePath("constrained.script")) && checkedLoadScript(axesFiles.filePath("constrained.script")),
            "Constrained axes save/reopen failed");
         checkConstraint();
         editor->setPlainText(base); require(window.buildScript(),"Coordinate fixture restoration failed");
      }
      const QString beforeEdit = editor->toPlainText();
      auto dryMass = [] { return Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("DryMass"); };
      const double originalMass = dryMass();
      require(!window.applyResourceChanges("QtSat", {{"DryMass", "not-a-number"}}, beforeEdit).isEmpty(),
         "Invalid numeric input was accepted");
      require(dryMass() == originalMass && editor->toPlainText() == beforeEdit,
         "Rejected edit changed the mission");
      const auto editError = window.applyResourceChanges("QtSat", {{"DryMass", "975.5"}}, beforeEdit);
      if (!editError.isEmpty()) std::cerr << editError.toStdString() << '\n';
      require(editError.isEmpty(), "Resource edit failed");
      require(dryMass() == 975.5 && editor->document()->isModified(), "Resource edit not synchronized to script");
      const auto afterEdit = editor->toPlainText();
      require(!window.applyResourceChanges("QtSat", {{"DryMass", "1000"}}, beforeEdit).isEmpty(),
         "Stale resource panel overwrote current mission");
      require(dryMass() == 975.5, "Stale resource edit mutated model");
      require(!window.applyResourceChanges("QtSat", {{"CoordinateSystem", "NoSuchFrame"}}, afterEdit).isEmpty(),
         "Invalid reference was accepted");
      require(dryMass() == 975.5 && editor->toPlainText() == afterEdit, "Failed model validation was not rolled back");
      editor->undo();
      require(editor->toPlainText() == beforeEdit, "Resource edit destroyed script undo history");
      require(window.buildScript() && dryMass() == originalMass, "Undo did not restore mission settings");
      const auto forceScript=editor->toPlainText();
      auto forceValue=[](const QString &name) {
         for (const auto &field:resourceProperties(*Moderator::Instance()->GetConfiguredObject("QtForces")))
            if (field.name==name) return field.value;
         return QString();
      };
      require(!forceValue("GravityField.Earth.Degree").isEmpty(),"Owned gravity settings missing from force editor");
      require(!window.applyResourceChanges("QtForces",{{"GravityField.Earth.Degree","-1"}},forceScript).isEmpty(),
         "Invalid gravity degree accepted");
      require(editor->toPlainText()==forceScript,"Rejected owned-force edit changed script");
      const auto gravityError=window.applyResourceChanges("QtForces",{{"GravityField.Earth.Degree","6"},{"GravityField.Earth.Order","6"}},forceScript);
      if (!gravityError.isEmpty()) std::cerr<<gravityError.toStdString()<<'\n';
      require(gravityError.isEmpty() && forceValue("GravityField.Earth.Degree")=="6" && forceValue("GravityField.Earth.Order")=="6",
         "Owned force edits did not survive model reconstruction");
      require(window.runMission()==MainWindow::RunResult::Completed,"Edited gravity model failed actual propagation");
      editor->undo(); require(editor->toPlainText()==forceScript && window.buildScript(),"Owned force edit was not undoable");
      require(window.applyResourceChanges("QtSat",{{"Attitude","Spinner"},{"AttitudeDisplayStateType","EulerAngles"}},forceScript).isEmpty(),
         "Attitude representation change failed");
      const auto attitudeScript=editor->toPlainText();
      bool attitudeField=false;
      for (const auto &field:resourceProperties(*Moderator::Instance()->GetConfiguredObject("QtSat")))
         if (field.name=="EulerAngle1") attitudeField=true;
      require(attitudeField,"Attitude state fields missing from spacecraft editor");
      const auto attitudeError=window.applyResourceChanges("QtSat",{{"EulerAngle1","12.5"}},attitudeScript);
      if (!attitudeError.isEmpty()) std::cerr<<attitudeError.toStdString()<<'\n';
      require(attitudeError.isEmpty(),"Attitude state edit failed");
      require(std::abs(Moderator::Instance()->GetConfiguredObject("QtSat")->GetRealParameter("EulerAngle1")-12.5)<1e-9,
         "Attitude edit did not survive reconstruction");
      require(window.runMission()==MainWindow::RunResult::Completed,"Edited attitude failed propagation");
      editor->undo(); require(editor->toPlainText()==attitudeScript && window.buildScript(),"Attitude edit was not undoable");
      editor->undo(); require(editor->toPlainText()==forceScript && window.buildScript(),"Attitude representation was not undoable");
      {
         const auto originalMission=editor->toPlainText();
         auto &files=fixtureDirectory(); const auto report=files.filePath("attitude.txt");
         auto fixture=originalMission;
         fixture.replace("BeginMissionSequence;","Create ReportFile AttitudeReport;\nAttitudeReport.Filename = '"+report+"';\nAttitudeReport.WriteHeaders = false;\n"
            "AttitudeReport.Add = {QtSat.Q1, QtSat.Q2, QtSat.Q3, QtSat.Q4};\nBeginMissionSequence;");
         editor->setPlainText(fixture); if (!window.buildScript()) { app.processEvents(); for (auto *text:window.findChildren<QPlainTextEdit *>()) if (text!=editor) std::cerr<<text->toPlainText().right(5000).toStdString(); throw std::runtime_error("Attitude dialog fixture failed"); }
         const auto before=editor->toPlainText(); QString applied="Apply not invoked";
         const auto expected=AttitudeConversionUtility::ToQuaternion(AttitudeConversionUtility::ToCosineMatrix(
            Rvector3(15*GmatMathConstants::RAD_PER_DEG,25*GmatMathConstants::RAD_PER_DEG,35*GmatMathConstants::RAD_PER_DEG),3,2,1));
         {
            QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("QtSat"),[&](const QMap<QString,QString> &values) {
               applied=window.applyResourceChanges("QtSat",values,before); return applied;
            },&owner,before); workflowView(panel);
            auto *open=panel.findChild<QPushButton *>("spacecraftAttitude"); require(open,"Focused attitude editor absent");
            workflowAfterExposure(0,[&] {
               auto *dialog=dynamic_cast<AttitudeDialog *>(QApplication::activeModalWidget()); require(dialog,"Attitude dialog not open");
               dialog->findChild<QComboBox *>("attitudeModel")->setCurrentText("Spinner");
               dialog->findChild<QLineEdit *>("attitude_Q1")->setText("0.3"); dialog->reject();
            }); open->click();
            require(!panel.hasChanges() && editor->toPlainText()==before,"Attitude Cancel changed spacecraft");
            workflowAfterExposure(0,[&] {
               auto *dialog=dynamic_cast<AttitudeDialog *>(QApplication::activeModalWidget()); require(dialog,"Attitude dialog not open");
               auto combo=[&](const QString &name) { auto *widget=dialog->findChild<QComboBox *>("attitude_"+name); require(widget,qPrintable("Attitude selector missing: "+name)); return widget; };
               auto field=[&](const QString &name) { auto *widget=dialog->findChild<QLineEdit *>("attitude_"+name); require(widget,qPrintable("Attitude numeric field missing: "+name)); return widget; };
               auto *model=dialog->findChild<QComboBox *>("attitudeModel"); model->setCurrentText("Spinner");
               combo("AttitudeDisplayStateType")->setCurrentText("EulerAngles");
               combo("EulerAngleSequence")->setCurrentText("321");
               for (int i=1;i<=3;++i) field(QString("EulerAngle%1").arg(i))->setText(QString::number(i*10+5));
               combo("AttitudeRateDisplayStateType")->setCurrentText("EulerAngleRates");
               for (int i=1;i<=3;++i) field(QString("EulerAngleRate%1").arg(i))->setText(QString::number(i*.1));
               field("EulerAngle2")->setText("nan"); combo("AttitudeDisplayStateType")->setCurrentText("Quaternion");
               require(combo("AttitudeDisplayStateType")->currentText()=="EulerAngles" && field("EulerAngle2")->text()=="nan" &&
                  !dialog->findChild<QLabel *>("attitudeError")->text().isEmpty(),"Invalid attitude conversion did not retain inputs");
               field("EulerAngle2")->setText("25"); combo("AttitudeDisplayStateType")->setCurrentText("Quaternion");
               require(!dialog->findChild<QLineEdit *>("attitude_EulerAngle1") && field("Q4"),"Quaternion fields did not replace Euler fields");
               for (int i=1;i<=4;++i) require(std::abs(field(QString("Q%1").arg(i))->text().toDouble()-expected[i-1])<1e-12,"Pending Euler to quaternion conversion changed orientation");
               // A zero quaternion must fail as a group, preserving correction inputs.
               for (int i=1;i<=4;++i) field(QString("Q%1").arg(i))->setText("0");
               dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
               require(dialog->isVisible() && !dialog->findChild<QLabel *>("attitudeError")->text().isEmpty(),"Zero quaternion accepted");
               for (int i=1;i<=4;++i) field(QString("Q%1").arg(i))->setText(QString::number(expected[i-1],'g',17));
               combo("AttitudeDisplayStateType")->setCurrentText("MRPs"); require(field("MRP1") && !dialog->findChild<QLineEdit *>("attitude_Q1"),"MRP fields not refreshed");
               combo("AttitudeDisplayStateType")->setCurrentText("DirectionCosineMatrix");
               require(field("DCM33") && !dialog->findChild<QLineEdit *>("attitude_MRP1"),"DCM fields not refreshed");
               const auto diagonal=field("DCM11")->text(); field("DCM11")->setText("2");
               combo("AttitudeDisplayStateType")->setCurrentText("EulerAngles");
               require(combo("AttitudeDisplayStateType")->currentText()=="DirectionCosineMatrix","Invalid DCM conversion accepted");
               field("DCM11")->setText(diagonal); combo("AttitudeDisplayStateType")->setCurrentText("EulerAngles");
               for (int i=1;i<=3;++i) require(std::abs(field(QString("EulerAngle%1").arg(i))->text().toDouble()-(i*10+5))<1e-10,"Orientation representation round trip changed angles");
               combo("AttitudeRateDisplayStateType")->setCurrentText("AngularVelocity");
               require(field("AngularVelocityZ") && !dialog->findChild<QLineEdit *>("attitude_EulerAngleRate1"),"Rate fields not refreshed");
               require(dialog->findChild<QLabel *>("attitudeUnit_AngularVelocityZ")->text()=="deg/s","Attitude rate units missing");
               combo("AttitudeRateDisplayStateType")->setCurrentText("EulerAngleRates");
               for (int i=1;i<=3;++i) require(std::abs(field(QString("EulerAngleRate%1").arg(i))->text().toDouble()-i*.1)<1e-10,"Rate conversion round trip changed values");
               combo("AttitudeRateDisplayStateType")->setCurrentText("AngularVelocity");
               for (const auto &axis:QStringList{"X","Y","Z"}) field("AngularVelocity"+axis)->setText("0");
               require(!dialog->findChild<QComboBox *>("attitude_AttitudeCoordinateSystem"),"Locked spinner frame is editable");
               model->setCurrentText("CoordinateSystemFixed");
               auto *frame=combo("AttitudeCoordinateSystem"); const auto originalFrame=frame->currentText(); frame->addItem("MissingFrame"); frame->setCurrentText("MissingFrame");
               require(combo("AttitudeCoordinateSystem")->currentText()==originalFrame,"Missing attitude frame accepted");
               model->setCurrentText("PrecessingSpinner");
               require(field("SpinRate") && !dialog->findChild<QLineEdit *>("attitude_Q1") && !dialog->findChild<QLineEdit *>("attitude_EulerAngle1"),"Precessing model fields not refreshed");
               field("SpinRate")->setText("0.75"); model->setCurrentText("NadirPointing");
               require(field("BodyAlignmentVectorX") && combo("AttitudeReferenceBody") && combo("AttitudeConstraintType"),"Nadir controls absent");
               require(!dialog->findChild<QComboBox *>("attitude_AttitudeCoordinateSystem"),"Locked nadir reference frame remains editable");
               model->setCurrentText("CCSDS-AEM"); require(field("AttitudeFileName") && dialog->findChild<QPushButton *>("attitudeBrowse_AttitudeFileName"),"Attitude ephemeris chooser absent");
               workflowAfterExposure(0,[&] { auto *chooser=qobject_cast<QFileDialog *>(QApplication::activeModalWidget()); require(chooser && chooser->fileMode()==QFileDialog::ExistingFile,"AEM chooser permits new input files"); chooser->reject(); });
               dialog->findChild<QPushButton *>("attitudeBrowse_AttitudeFileName")->click();
               model->setCurrentText("PrecessingSpinner"); require(std::abs(field("SpinRate")->text().toDouble()-.75)<1e-12,qPrintable("Switching models lost pending spinner data: "+field("SpinRate")->text()));
               model->setCurrentText("Spinner");
               for (int i=1;i<=3;++i) require(std::abs(field(QString("EulerAngle%1").arg(i))->text().toDouble()-(i*10+5))<1e-10,"Switching models lost pending orientation");
               combo("AttitudeDisplayStateType")->setCurrentText("Quaternion");
               if (!attitudeCapture.isEmpty()) {
                  QEventLoop exposed; QTimer::singleShot(200,&exposed,&QEventLoop::quit); exposed.exec();
                  require(dialog->grab().save(attitudeCapture),"Attitude dialog capture failed");
               }
               dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            }); open->click();
            require(panel.hasChanges() && editor->toPlainText()==before,"Attitude OK skipped deferred Apply");
            require(Moderator::Instance()->GetConfiguredObject("QtSat")->GetStringParameter("Attitude")!="Spinner","Dialog mutated configured attitude");
            auto *grid=panel.findChild<QTableWidget *>();
            for (int row=0;row<grid->rowCount();++row) if (grid->item(row,0)->text()=="DryMass") grid->item(row,1)->setText("850");
            workflowAfterExposure(0,[&] { auto *dialog=dynamic_cast<AttitudeDialog *>(QApplication::activeModalWidget()); require(dialog && dialog->findChild<QLineEdit *>("attitude_Q4"),"Reopened pending attitude model absent"); dialog->reject(); }); open->click();
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         }
         require(applied.isEmpty(),qPrintable(applied)); const auto after=editor->toPlainText();
         require(after.mid(after.indexOf("BeginMissionSequence;"))==before.mid(before.indexOf("BeginMissionSequence;")),"Attitude Apply rewrote mission commands");
         editor->undo(); require(editor->toPlainText()==before && window.buildScript(),"Attitude paired Apply not one exact Undo");
         editor->redo(); require(editor->toPlainText()==after && window.buildScript(),"Attitude paired Apply not one exact Redo");
         const auto path=files.filePath("attitude.script"); require(window.saveScriptTo(path) && checkedLoadScript(path) && window.buildScript(),"Attitude save/reopen failed");
         auto *edited=Moderator::Instance()->GetConfiguredObject("QtSat");
         require(edited->GetStringParameter("Attitude")=="Spinner" && edited->GetStringParameter("AttitudeDisplayStateType")=="Quaternion" && edited->GetRealParameter("DryMass")==850,"Attitude Apply lost paired spacecraft edits");
         for (int i=1;i<=4;++i) require(std::abs(edited->GetRealParameter("Q"+std::to_string(i))-expected[i-1])<1e-12,"Attitude quaternion changed after reopen");
         require(window.runMission()==MainWindow::RunResult::Completed,"GUI-configured attitude mission failed");
         QFile output(report); require(output.open(QIODevice::ReadOnly),"Attitude execution report missing");
         const auto lines=QString::fromUtf8(output.readAll()).trimmed().split('\n'); require(lines.size()>1,"Attitude report samples missing");
         for (const auto &line:{lines.front(),lines.back()}) {
            const auto values=line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(values.size()==4,"Attitude report columns changed");
            for (int i=0;i<4;++i) require(std::abs(values[i].toDouble()-expected[i])<1e-12,"GUI attitude report changed orientation during propagation");
         }
         editor->setPlainText(originalMission); require(window.buildScript(),"Attitude fixture restoration failed");
         if (!attitudeCapture.isEmpty()) { std::cout<<"PASS: attitude dialog conversions, model switching, deferred Apply, save/reopen and report execution\n"; return 0; }
      }
      editor->setPlainText("Create Array QtMatrix[2,3];\nGMAT QtMatrix(1,1) = 4;\nGMAT QtMatrix(2,3) = 9;\nBeginMissionSequence;\n");
      require(window.buildScript(), "Array fixture did not build");
      const auto arrayScript=editor->toPlainText();
      const auto fields=resourceProperties(*Moderator::Instance()->GetConfiguredObject("QtMatrix"));
      bool foundMatrix=false;
      for (const auto &field:fields) if (field.name=="RmatValue")
         foundMatrix=field.rows==2 && field.columns==3 && field.value=="4 0 0; 0 0 9";
      require(foundMatrix,"Array grid missing or transposed");
      {
         QWidget owner;
         ResourceEditor gridPanel(*Moderator::Instance()->GetConfiguredObject("QtMatrix"),
            [](const QMap<QString,QString>&) { return QString(); },&owner); workflowView(gridPanel);
         auto *editCells=gridPanel.findChild<QPushButton *>("editCells_RmatValue");
         require(editCells,"Array has no cell editor button");
         bool gridOpened=false,invalidRejected=false;
         workflowAfterExposure(0,&gridPanel,[&] {
            auto *dialog=gridPanel.findChild<QDialog *>("numericGridDialog");
            if (!dialog) return;
            auto *grid=dialog->findChild<QTableWidget *>("numericGrid");
            gridOpened=grid && grid->rowCount()==2 && grid->columnCount()==3 && grid->item(1,2)->text()=="9";
            if (!gridOpened) { dialog->reject(); return; }
            auto *buttons=dialog->findChild<QDialogButtonBox *>();
            grid->item(1,2)->setText("nan"); buttons->button(QDialogButtonBox::Ok)->click();
            invalidRejected=dialog->isVisible();
            grid->item(1,2)->setText("-12.5"); buttons->button(QDialogButtonBox::Ok)->click();
         });
         editCells->click();
         require(gridOpened && invalidRejected && gridPanel.hasChanges(),"Array grid validation/edit failed");
         require(Moderator::Instance()->GetConfiguredObject("QtMatrix")->GetRealParameter("SingleValue",1,2)==9,
            "Grid mutated configured array before Apply");
         const auto value=gridPanel.findChild<QTableWidget *>()->findItems("4 0 0; 0 0 -12.5",Qt::MatchExactly);
         require(value.size()==1,"Grid did not preserve cell order");
         workflowAfterExposure(0,&gridPanel,[&] {
            auto *dialog=gridPanel.findChild<QDialog *>("numericGridDialog");
            dialog->findChild<QTableWidget *>("numericGrid")->item(0,0)->setText("777");
            dialog->reject();
         });
         editCells->click();
         require(value.first()->text()=="4 0 0; 0 0 -12.5","Cancel changed matrix values");
         bool resized=false;
         workflowAfterExposure(0,&gridPanel,[&] {
            auto *dialog=gridPanel.findChild<QDialog *>("numericGridDialog");
            auto *rows=dialog->findChild<QSpinBox *>("arrayRows");
            auto *columns=dialog->findChild<QSpinBox *>("arrayColumns");
            if (!rows || !columns) { dialog->reject(); return; }
            rows->setValue(3); columns->setValue(2);
            auto *grid=dialog->findChild<QTableWidget *>("numericGrid");
            resized=grid->rowCount()==3 && grid->columnCount()==2 && grid->item(0,0)->text()=="4" && grid->item(2,1)->text()=="0";
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
         });
         editCells->click();
         require(resized && value.first()->text()=="4 0; 0 0; 0 0","Array resize controls lost retained cells or failed to initialize new cells");
         gridPanel.discardChanges();
      }
      require(!window.applyResourceChanges("QtMatrix",{{"RmatValue","1 2; 3"}},arrayScript).isEmpty(),
         "Ragged array dimensions were accepted");
      require(!window.applyResourceChanges("QtMatrix",{{"RmatValue","1 2 3; 4 nan 6"}},arrayScript).isEmpty(),
         "Nonfinite array value was accepted");
      require(editor->toPlainText()==arrayScript,"Rejected array edit changed script");
      const auto arrayError=window.applyResourceChanges("QtMatrix",{{"RmatValue","0 -2 3.125; 4 5 6"}},arrayScript);
      if (!arrayError.isEmpty()) std::cerr<<arrayError.toStdString()<<'\n';
      require(arrayError.isEmpty(),"Array edit failed");
      auto *array=Moderator::Instance()->GetConfiguredObject("QtMatrix");
      require(array->GetRealParameter("SingleValue",0,0)==0 && array->GetRealParameter("SingleValue",0,2)==3.125 &&
         array->GetRealParameter("SingleValue",1,2)==6,"Array values did not survive serialization");
      editor->undo();
      require(editor->toPlainText()==arrayScript && window.buildScript(),"Array edit was not one undoable change");
      require(Moderator::Instance()->GetConfiguredObject("QtMatrix")->GetRealParameter("SingleValue",1,2)==9,
         "Array undo failed to restore initial values");
      require(window.applyResourceChanges("QtMatrix",{{"RmatValue","1 2; 3 4; 5 6"}},arrayScript).isEmpty(),
         "Array resize failed");
      array=Moderator::Instance()->GetConfiguredObject("QtMatrix");
      require(array->GetIntegerParameter("NumRows")==3 && array->GetIntegerParameter("NumCols")==2 &&
         array->GetRealParameter("SingleValue",2,1)==6,"Array resize did not survive engine reconstruction");
      editor->undo(); require(editor->toPlainText()==arrayScript && window.buildScript(),"Array resize undo failed");
      require(!window.createResource("Array","InvalidDimensions",editor->toPlainText(),0,3).isEmpty(),
         "Zero-sized array accepted");
      require(window.createResource("Array","CreatedArray",editor->toPlainText(),3,2).isEmpty(),"Array creation failed");
      auto *createdArray=Moderator::Instance()->GetConfiguredObject("CreatedArray");
      require(createdArray && createdArray->GetIntegerParameter("NumRows")==3 && createdArray->GetIntegerParameter("NumCols")==2,
         "Created array dimensions wrong");
      editor->setPlainText("Create Array Formula[1,2];\nCreate Variable result;\nCreate Spacecraft ExprSat;\n"
         "BeginMissionSequence;\n% Keep this calculation exactly\nresult = Formula(1,1) + Formula(1,2);\n");
      require(window.buildScript(),"Expression fixture failed to build");
      const auto beforeExpressions=editor->toPlainText();
      QString expressionError="Apply was not invoked";
      {
         QWidget owner;
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Formula"),[&](const QMap<QString,QString> &changes) {
            expressionError=window.applyResourceChanges("Formula",changes,beforeExpressions); return expressionError;
         },&owner,beforeExpressions); workflowView(panel);
         auto *button=panel.findChild<QPushButton *>("arrayExpressions"); require(button,"Expression grid missing");
         auto fill=[&](bool accept) {
            workflowAfterExposure(0,&panel,[&,accept] {
               auto *dialog=panel.findChild<QDialog *>("arrayExpressionDialog");
               if (!dialog) return;
               auto *grid=dialog->findChild<QTableWidget *>("arrayExpressionGrid");
               grid->item(0,0)->setText("sqrt(16) + 1"); grid->item(0,1)->setText("Formula(1,1) * 3");
               if (accept) dialog->accept(); else dialog->reject();
            }); button->click();
         };
         fill(false); require(!panel.hasChanges(),"Cancel changed expression cells");
         fill(true); require(panel.hasChanges() && editor->toPlainText()==beforeExpressions,"Expression grid applied before Apply");
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      }
      require(expressionError.isEmpty(),qPrintable("Expression Apply failed: "+expressionError));
      const auto formulaScript=editor->toPlainText();
      require(formulaScript.endsWith("% Keep this calculation exactly\nresult = Formula(1,1) + Formula(1,2);\n"),"Expression editor changed existing commands");
      auto formulaResult=[&] {
         require(window.runMission()==MainWindow::RunResult::Completed,"Array formula execution failed");
         return Moderator::Instance()->GetInternalObject("result")->GetRealParameter("Value");
      };
      require(formulaResult()==20,"Array formulas did not execute in row order");
      editor->undo(); require(editor->toPlainText()==beforeExpressions && formulaResult()==0,"Array expression Undo failed");
      editor->redo(); require(editor->toPlainText()==formulaScript && formulaResult()==20,"Array expression Redo failed");
      require(window.applyResourceChanges("ExprSat",{{"DryMass","850"}},formulaScript).isEmpty() &&
         arrayExpressions(editor->toPlainText(),"Formula")==arrayExpressions(formulaScript,"Formula") && formulaResult()==20,
         "Unrelated resource edit lost array expressions");
      auto &expressionFiles=fixtureDirectory();
      const auto expressionPath=expressionFiles.filePath("expressions.script");
      require(window.saveScriptTo(expressionPath) && checkedLoadScript(expressionPath) && formulaResult()==20,"Expression save/reopen changed results");
      {
         QWidget owner;
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Formula"),[](const QMap<QString,QString>&) { return QString(); },&owner,editor->toPlainText()); workflowView(panel);
         bool restored=false;
         workflowAfterExposure(0,&panel,[&] {
            auto *dialog=panel.findChild<QDialog *>("arrayExpressionDialog");
            if (!dialog) return;
            auto *grid=dialog->findChild<QTableWidget *>("arrayExpressionGrid");
            restored=grid->item(0,0)->text()=="sqrt(16) + 1" && grid->item(0,1)->text()=="Formula(1,1) * 3";
            dialog->reject();
         });
         panel.findChild<QPushButton *>("arrayExpressions")->click();
         require(restored && !panel.hasChanges(),"Reopened expression grid lost formula text"); panel.discardChanges();
      }
      const auto withExpressions=editor->toPlainText();
      require(window.applyResourceChanges("Formula",{{"@ArrayExpressions","[]"}},withExpressions).isEmpty() && formulaResult()==0,
         "Clearing expression cells did not restore numeric initialization");
      editor->undo(); require(editor->toPlainText()==withExpressions && formulaResult()==20,"Cleared expression Undo failed");
      const auto expressionSource=editor->toPlainText();
      require(!window.applyResourceChanges("Formula",{{"@ArrayExpressions",R"([{"row":1,"column":1,"expression":"1; Stop"}])"}},expressionSource).isEmpty(),
         "Expression grid accepted an extra mission command");
      require(!window.applyResourceChanges("Formula",{{"@ArrayExpressions",R"([{"row":1,"column":1,"expression":"MissingVariable + 2"}])"}},expressionSource).isEmpty(),
         "Unknown expression variable was accepted");
      require(editor->toPlainText()==expressionSource && formulaResult()==20,"Rejected formula changed source or engine state");
      {
         QWidget owner; QString combinedError="Apply was not invoked";
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Formula"),[&](const QMap<QString,QString> &changes) {
            require(changes.contains("RmatValue") && changes.contains("@ArrayExpressions"),"Panel did not submit both pending array edits");
            combinedError=window.applyResourceChanges("Formula",changes,expressionSource); return combinedError;
         },&owner,expressionSource); workflowView(panel);
         auto *properties=panel.findChild<QTableWidget *>();
         for (int row=0;row<properties->rowCount();++row)
            if (properties->item(row,0)->text()=="RmatValue") properties->item(row,1)->setText("3 0 0; 0 0 0");
         workflowAfterExposure(0,&panel,[&] {
            auto *dialog=panel.findChild<QDialog *>("arrayExpressionDialog");
            auto *grid=dialog->findChild<QTableWidget *>("arrayExpressionGrid");
            require(grid->rowCount()==2 && grid->columnCount()==3,"Expression grid ignored pending resize");
            grid->item(0,0)->setText(""); grid->item(0,1)->setText("Formula(1,1) * 4");
            grid->item(1,2)->setText("Formula(1,2) + 9"); dialog->accept();
         }); panel.findChild<QPushButton *>("arrayExpressions")->click();
         QTableWidgetItem *numeric=nullptr;
         for (int row=0;row<properties->rowCount();++row)
            if (properties->item(row,0)->text()=="RmatValue") numeric=properties->item(row,1);
         require(numeric,"Array numeric field missing"); numeric->setText("3");
         workflowAfterExposure(0,&panel,[&] {
            auto *dialog=panel.findChild<QDialog *>("arrayExpressionDialog");
            auto *grid=dialog->findChild<QTableWidget *>("arrayExpressionGrid");
            auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
            require(grid->rowCount()==2 && grid->columnCount()==3 && grid->item(1,2)->text()=="Formula(1,2) + 9" && !ok->isEnabled(),
               "Pending shrink hid formulas or accepted out-of-range cells");
            grid->item(0,1)->setText(""); grid->item(1,2)->setText("");
            require(ok->isEnabled(),"Clearing removed cells did not allow shrink"); dialog->reject();
         }); panel.findChild<QPushButton *>("arrayExpressions")->click();
         numeric->setText("3 0 0; 0 0 0");
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         require(combinedError.isEmpty(),qPrintable(combinedError));
      }
      require(formulaResult()==15,"Combined array edit did not use the new numeric initial value");
      require(Moderator::Instance()->GetInternalObject("Formula")->GetRealParameter("SingleValue",1,2)==21,
         "Formula in newly added cell did not execute");
      editor->undo(); require(editor->toPlainText()==expressionSource && formulaResult()==20,"Combined array edit was not atomic under Undo");
      require(!window.applyResourceChanges("Formula",{{"RmatValue","7 0"},{"@ArrayExpressions",R"([{"row":1,"column":2,"expression":"UnknownCellInput + 1"}])"}},expressionSource).isEmpty(),
         "Invalid combined array expression accepted");
      require(editor->toPlainText()==expressionSource && formulaResult()==20,"Failed combined Apply changed numeric values or formulas");
      require(!window.applyResourceChanges("Formula",{{"RmatValue","7"}},expressionSource).isEmpty(),
         "Array shrink silently removed an expression cell");
      require(editor->toPlainText()==expressionSource && formulaResult()==20,"Rejected shrink changed array state");
      editor->setPlainText("Create SolarPowerSystem SolarPower;\nCreate NuclearPowerSystem NuclearPower;\nBeginMissionSequence;\n");
      require(window.buildScript(),"Power-system panel fixture failed");
      for (const auto &name:{QString("SolarPower"),QString("NuclearPower")}) {
         QWidget owner;
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject(name.toStdString()),[](const QMap<QString,QString>&) { return QString(); },&owner,editor->toPlainText()); workflowView(panel);
         auto *sections=panel.findChild<QTabBar *>("propertySections"); auto *grid=panel.findChild<QTableWidget *>();
         require(sections && grid,"Power-system sections missing");
         const auto checkSection=[&](const QString &section,const QStringList &fields) {
            int tab=-1; for (int i=0;i<sections->count();++i) if (sections->tabText(i)==section) tab=i;
            require(tab>=0,qPrintable("Missing power section: "+section)); sections->setCurrentIndex(tab);
            for (const auto &field:fields) {
               int row=-1; for (int i=0;i<grid->rowCount();++i) if (grid->item(i,0)->text()==field) row=i;
               require(row>=0 && grid->isRowHidden(row) && panel.findChild<QWidget *>("resource_"+field),qPrintable("Missing primary power control: "+field));
            }
            for (int row=0;row<grid->rowCount();++row)
               require(grid->isRowHidden(row)==(grid->item(row,0)->data(Qt::UserRole+1).toBool() || grid->item(row,0)->data(Qt::UserRole).toString()!=section),"Power section displayed unrelated settings");
         };
         checkSection("General",{"EpochFormat","InitialEpoch","InitialMaxPower","AnnualDecayRate","Margin"});
         checkSection("Bus coefficients",{"BusCoeff1","BusCoeff2","BusCoeff3"});
         auto *format=panel.findChild<QComboBox *>("powerEpochFormat"); QTableWidgetItem *epoch=nullptr;
         for (int row=0;row<grid->rowCount();++row) if (grid->item(row,0)->text()=="InitialEpoch") epoch=grid->item(row,1);
         require(format && epoch,"Power epoch conversion controls missing");
         const auto initialFormat=format->currentText(); const auto initialEpoch=epoch->text();
         epoch->setText("01 Jan 2000 12:00:00.000"); format->setCurrentText("TAIModJulian");
         require(std::abs(epoch->text().toDouble()-(21545.0+32.0/86400.0))<1e-9,"Power epoch conversion changed the represented instant");
         const auto convertedEpoch=epoch->text();
         const auto epochSource=editor->toPlainText();
         require(window.applyResourceChanges(name,{{"EpochFormat",format->currentText()},{"InitialEpoch",convertedEpoch}},epochSource).isEmpty(),"Converted power epoch did not apply");
         auto *convertedPower=Moderator::Instance()->GetConfiguredObject(name.toStdString());
         require(convertedPower->GetStringParameter("EpochFormat")=="TAIModJulian" &&
            std::abs(QString::fromStdString(convertedPower->GetStringParameter("InitialEpoch")).remove("'").toDouble()-convertedEpoch.toDouble())<1e-9,
            "Power epoch and format were not stored together");
         editor->undo(); require(editor->toPlainText()==epochSource && window.buildScript(),"Power epoch Undo failed");
         epoch->setText("invalid epoch"); format->setCurrentText("UTCGregorian");
         require(format->currentText()=="TAIModJulian" && epoch->text()=="invalid epoch","Failed epoch conversion changed pending fields");
         epoch->setText(convertedEpoch); format->setCurrentText("UTCGregorian");
         require(epoch->text().contains("01 Jan 2000 12:00:00.000"),"Power epoch conversion did not round trip");
         format->setCurrentText(initialFormat); epoch->setText(initialEpoch);

         if (name=="SolarPower") {
            checkSection("Solar coefficients",{"SolarCoeff1","SolarCoeff2","SolarCoeff3","SolarCoeff4","SolarCoeff5"});
            checkSection("Shadow",{"ShadowModel","ShadowBodies"});
            workflowAfterExposure(0,&panel,[&] {
               auto *dialog=panel.findChild<QDialog *>("resourceSelectionDialog"); auto *list=dialog->findChild<QListWidget *>("resourceSelectionList");
               bool earth=false,luna=false;
               for (int i=0;i<list->count();++i) {
                  earth=earth || list->item(i)->text()=="Earth"; luna=luna || list->item(i)->text()=="Luna";
                  require(list->item(i)->text()!="SolarPower","Shadow picker included non-body resource");
                  list->item(i)->setCheckState(list->item(i)->text()=="Luna" ? Qt::Checked : Qt::Unchecked);
               }
               require(earth && luna,"Shadow picker omitted celestial bodies"); dialog->accept();
            }); panel.findChild<QPushButton *>("chooseProperty_ShadowBodies")->click();
            bool selected=false; for (int row=0;row<grid->rowCount();++row)
               if (grid->item(row,0)->text()=="ShadowBodies") selected=grid->item(row,1)->text()=="Luna";
            require(selected && panel.hasChanges(),"Shadow picker did not update pending list"); panel.discardChanges();
         } else for (int i=0;i<sections->count();++i)
            require(sections->tabText(i)!="Solar coefficients" && sections->tabText(i)!="Shadow","Nuclear panel displayed solar-only sections");
      }
      const auto powerSource=editor->toPlainText();
      require(window.applyResourceChanges("SolarPower",{{"ShadowBodies","Earth, Luna"}},powerSource).isEmpty(),"Shadow-body list Apply failed");
      const auto powerEdited=editor->toPlainText();
      const auto &shadowBodies=Moderator::Instance()->GetConfiguredObject("SolarPower")->GetStringArrayParameter("ShadowBodies");
      require(shadowBodies.size()==2 && shadowBodies[0]=="Earth" && shadowBodies[1]=="Luna","Shadow-body list did not survive reconstruction");
      require(!window.applyResourceChanges("SolarPower",{{"ShadowBodies","NuclearPower"}},powerEdited).isEmpty() && editor->toPlainText()==powerEdited,
         "Invalid shadow-body edit was not rejected atomically");
      editor->undo(); require(editor->toPlainText()==powerSource && window.buildScript(),"Shadow-body list Undo failed");
      editor->setPlainText("Create ChemicalTank FuelA FuelB;\nCreate ChemicalThruster Engine;\n"
         "GMAT Engine.Tank = {FuelA, FuelB};\nGMAT Engine.MixRatio = [2 3];\n"
         "Create ReportFile PickerReport;\nCreate Spacecraft Vehicle;\nGMAT Vehicle.Tanks = {FuelA, FuelB};\nGMAT Vehicle.Thrusters = {Engine};\nBeginMissionSequence;\n");
      require(window.buildScript(),"Hardware fixture failed");
      {
         QWidget owner;
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Vehicle"),
            [](const QMap<QString,QString>&) { return QString(); },&owner); workflowView(panel);
         auto *choice=panel.findChild<QComboBox *>("spacecraftOrbitFrame");
         require(choice && choice->findText("EarthFixed")>=0,"Coordinate-system selector missing choices");
         choice->setCurrentText("EarthFixed");
         require(choice->currentText()=="EarthFixed" && panel.hasChanges() && Moderator::Instance()->GetConfiguredObject("Vehicle")->GetStringParameter("CoordinateSystem")!="EarthFixed",
            "Coordinate selector failed conversion or applied prematurely");
         panel.discardChanges();
      }
      {
         QWidget owner; auto &destination=fixtureDirectory();
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("PickerReport"),
            [](const QMap<QString,QString>&) { return QString(); },&owner); workflowView(panel);
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_Filename");
         require(choose,"Report filename picker missing");
         const auto path=destination.filePath("new report.txt");
         bool opened=false;
         workflowAfterExposure(0,&panel,[&] {
            auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
            if (!dialog) return;
            opened=dialog->acceptMode()==QFileDialog::AcceptSave && dialog->fileMode()==QFileDialog::AnyFile; dialog->selectFile(path); QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);
         });
         choose->click();
         require(opened && panel.hasChanges() && !QFileInfo::exists(path) &&
            panel.findChild<QTableWidget *>()->findItems(path,Qt::MatchExactly).size()==1,
            "Filename picker rejected a new output path or created the file prematurely");
         panel.discardChanges();
      }
      {
         QWidget owner; auto &source=fixtureDirectory();
         const auto path=source.filePath("spacecraft model.obj"); QFile model(path);
         require(model.open(QIODevice::WriteOnly),"Input picker fixture failed"); model.write("v 0 0 0\n"); model.close();
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Vehicle"),
            [](const QMap<QString,QString>&) { return QString(); },&owner); workflowView(panel);
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_ModelFile");
         require(choose,"Spacecraft model file picker missing");
         bool inputMode=false;
         workflowAfterExposure(0,&panel,[&] {
            auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
            inputMode=dialog->fileMode()==QFileDialog::ExistingFile && dialog->acceptMode()==QFileDialog::AcceptOpen;
            dialog->selectFile(path); dialog->reject();
         });
         choose->click(); require(inputMode && !panel.hasChanges(),"Input picker mode or Cancel is incorrect");
         workflowAfterExposure(0,&panel,[&] {
            auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
            dialog->selectFile(path); QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);
         });
         choose->click();
         require(panel.hasChanges() && panel.findChild<QTableWidget *>()->findItems(path,Qt::MatchExactly).size()==1,
            "Input picker did not preserve the selected path with spaces");
         require(Moderator::Instance()->GetConfiguredObject("Vehicle")->GetStringParameter("ModelFile")!=path.toStdString(),
            "Browsing input applied the resource prematurely"); panel.discardChanges();
      }
      {
         auto &kernels=fixtureDirectory(); const auto kernel=QFileInfo(startup).dir().absoluteFilePath("../data/vehicle/ephem/spk/GEOSat.bsp");
         const auto first=kernels.filePath("first kernel.bsp"),second=kernels.filePath("second,kernel.bsp");
         require(kernels.isValid() && QFile::copy(kernel,first) && QFile::copy(kernel,second),"Kernel fixtures unavailable");
         const auto before=editor->toPlainText(); QString error="No Apply";
         QWidget owner;
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Vehicle"),
            [&](const QMap<QString,QString> &changes) { error=window.applyResourceChanges("Vehicle",changes,before); return error; },&owner); workflowView(panel);
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_OrbitSpiceKernelName");
         require(choose && panel.findChild<QPushButton *>("chooseProperty_FrameSpiceKernelName") &&
            panel.findChild<QPushButton *>("chooseProperty_AttitudeSpiceKernelName") && panel.findChild<QPushButton *>("chooseProperty_SCClockSpiceKernelName"),"Kernel list controls missing");
         bool ordered=false;
         workflowAfterExposure(0,&panel,[&] {
            auto *dialog=panel.findChild<QDialog *>("kernelFileDialog"); if (!dialog) return;
            auto *list=dialog->findChild<QListWidget *>("kernelFileList");
            for (const auto &path:{first,second,first}) {
               workflowAfterExposure(0,dialog,[path] {
                  auto *picker=qobject_cast<QFileDialog *>(QApplication::activeModalWidget()); if (!picker) return;
                  picker->selectFile(path); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection);
               }); dialog->findChild<QPushButton *>("kernelFileAdd")->click();
            }
            ordered=list->count()==2; if (ordered) list->insertItem(0,list->takeItem(1)); dialog->accept();
         }); choose->click(); require(ordered && panel.hasChanges(),"Kernel Add did not preserve unique ordered files");
         workflowAfterExposure(0,&panel,[&] {
            auto *dialog=panel.findChild<QDialog *>("kernelFileDialog"); if (!dialog) return;
            auto *list=dialog->findChild<QListWidget *>("kernelFileList"); list->selectAll();
            dialog->findChild<QPushButton *>("kernelFileRemove")->click(); dialog->reject();
         }); choose->click();
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         require(error.isEmpty(),qPrintable(error));
         auto actual=Moderator::Instance()->GetConfiguredObject("Vehicle")->GetStringArrayParameter("OrbitSpiceKernelName");
         require(actual.size()==2 && actual[0]==second.toStdString() && actual[1]==first.toStdString(),"Kernel Apply lost order, comma-containing path or cancelled selection");
         const auto selected=editor->toPlainText(); editor->undo();
         require(editor->toPlainText()==before && window.buildScript() && Moderator::Instance()->GetConfiguredObject("Vehicle")->GetStringArrayParameter("OrbitSpiceKernelName").empty(),"Kernel Undo failed");
         editor->redo(); require(window.buildScript(),"Kernel Redo failed");
         require(window.saveScriptTo(kernels.filePath("selected.script")) && checkedLoadScript(kernels.filePath("selected.script")) && window.buildScript(),"Kernel save/reopen failed");
         require(editor->toPlainText()==selected,"Kernel save/reopen changed source");
         require(!window.applyResourceChanges("Vehicle",{{"OrbitSpiceKernelName",kernels.filePath("missing.bsp")}},selected).isEmpty() && editor->toPlainText()==selected,"Missing kernel did not roll back");
         require(window.applyResourceChanges("Vehicle",{{"OrbitSpiceKernelName",""}},selected).isEmpty() && Moderator::Instance()->GetConfiguredObject("Vehicle")->GetStringArrayParameter("OrbitSpiceKernelName").empty(),"Clearing kernel files failed");
      }
      const auto hardwareScript=editor->toPlainText();
      require(!window.applyResourceChanges("Vehicle",{{"Tanks","MissingTank"}},hardwareScript).isEmpty(),
         "Missing hardware reference accepted");
      require(editor->toPlainText()==hardwareScript,"Rejected hardware link changed script");
      QString tankError="Picker Apply did not run";
      {
         QWidget owner;
         ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Engine"),
            [&](const QMap<QString,QString> &changes) { tankError=window.applyResourceChanges("Engine",changes,hardwareScript); return tankError; },&owner); workflowView(panel);
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_Tank");
         require(choose,"Tank reference picker missing");
         bool candidatesCorrect=false;
         workflowAfterExposure(0,&panel,[&] {
            auto *dialog=panel.findChild<QDialog *>("resourceSelectionDialog");
            if (!dialog) return;
            auto *list=dialog->findChild<QListWidget *>("resourceSelectionList");
            candidatesCorrect=list && list->count()==2 && list->item(0)->text()=="FuelA" && list->item(1)->text()=="FuelB";
            require(candidatesCorrect,"Tank picker candidates or order incorrect");
            dialog->findChild<QPushButton *>("resourceClearSelection")->click();
            for (int i=0;i<list->count();++i) require(list->item(i)->checkState()==Qt::Unchecked,"Clear selection retained a checked resource");
            dialog->findChild<QPushButton *>("resourceSelectAll")->click();
            for (int i=0;i<list->count();++i) require(list->item(i)->checkState()==Qt::Checked,"Select all omitted a resource");
            dialog->reject();
         });
         choose->click();
         require(candidatesCorrect && !panel.hasChanges(),"Picker lost order, included wrong resource types, or Cancel changed values");
         workflowAfterExposure(0,&panel,[&] {
            auto *dialog=panel.findChild<QDialog *>("resourceSelectionDialog");
            auto *list=dialog->findChild<QListWidget *>("resourceSelectionList");
            list->insertItem(0,list->takeItem(1));
            dialog->accept();
         });
         choose->click();
         require(panel.hasChanges() && Moderator::Instance()->GetConfiguredObject("Engine")->GetStringArrayParameter("Tank").front()=="FuelA",
            "Picker changed engine before Apply");
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      }
      if (!tankError.isEmpty()) std::cerr<<tankError.toStdString()<<'\n';
      require(tankError.isEmpty(),"Tank reorder failed");
      auto *engine=Moderator::Instance()->GetConfiguredObject("Engine");
      require(engine->GetStringArrayParameter("Tank").front()=="FuelB" && engine->GetRvectorParameter("MixRatio")[0]==3 &&
         engine->GetRvectorParameter("MixRatio")[1]==2,"Tank reorder detached mixture ratios from tanks");
      require(window.applyResourceChanges("Engine",{{"MixRatio","4 5"}},editor->toPlainText()).isEmpty(),
         "Mixture vector edit failed");
      require(Moderator::Instance()->GetConfiguredObject("Engine")->GetRvectorParameter("MixRatio")[1]==5,
         "Mixture vector did not survive script rebuild");
      require(window.applyResourceChanges("Engine",{{"Tank","FuelA, FuelB"},{"MixRatio","7 11"}},editor->toPlainText()).isEmpty(),
         "Combined tank and mixture edit failed");
      engine=Moderator::Instance()->GetConfiguredObject("Engine");
      require(engine->GetStringArrayParameter("Tank").front()=="FuelA" && engine->GetRvectorParameter("MixRatio")[0]==7 &&
         engine->GetRvectorParameter("MixRatio")[1]==11,"Combined edit reassigned ratios by old tank order");
      const auto pairedSource=editor->toPlainText();
      for (const auto &invalid:{QString("7"),QString("0 2"),QString("1 -2"),QString("1 1e999")})
         require(!window.applyResourceChanges("Engine",{{"Tank","FuelB, FuelA"},{"MixRatio",invalid}},pairedSource).isEmpty() &&
            editor->toPlainText()==pairedSource,"Invalid combined tank/mixture edit changed source");
      require(window.applyResourceChanges("Engine",{{"Tank","FuelB"},{"MixRatio","13"}},editor->toPlainText()).isEmpty(),
         "Combined tank removal and ratio resize failed");
      require(window.applyResourceChanges("Engine",{{"Tank","FuelB, FuelA"},{"MixRatio","17 19"}},editor->toPlainText()).isEmpty(),
         "Combined tank addition and ratio resize failed");
      auto &mixtureFiles=fixtureDirectory(); const auto mixturePath=mixtureFiles.filePath("tank-mixtures.script");
      require(window.saveScriptTo(mixturePath) && checkedLoadScript(mixturePath) && window.buildScript(),"Combined mixture save/reopen failed");
      engine=Moderator::Instance()->GetConfiguredObject("Engine");
      require(engine->GetStringArrayParameter("Tank").front()=="FuelB" && engine->GetRvectorParameter("MixRatio").GetSize()==2 &&
         engine->GetRvectorParameter("MixRatio")[0]==17 && engine->GetRvectorParameter("MixRatio")[1]==19,"Combined mixture did not survive round trip");
      {
         QWidget owner; QString error="Not applied"; const auto before=editor->toPlainText();
         ResourceEditor panel(*engine,[&](const auto &changes) { error=window.applyResourceChanges("Engine",changes,before); return error; },&owner); workflowView(panel);
         auto *button=panel.findChild<QPushButton *>("thrusterTankMixtures"); require(button,"Combined mixture editor missing");
         workflowAfterExposure(0,[&] {
            auto *dialog=panel.findChild<QDialog *>("tankMixtureDialog"); require(dialog,"Combined mixture dialog absent");
            dialog->findChild<QPushButton *>("tankMixtureRemove")->click(); dialog->reject();
         }); button->click(); require(!panel.hasChanges(),"Mixture Cancel changed pending values");
         workflowAfterExposure(0,[&] {
            auto *dialog=panel.findChild<QDialog *>("tankMixtureDialog");
            auto *grid=dialog->findChild<QTableWidget *>("tankMixtureTable");
            auto *available=dialog->findChild<QComboBox *>("tankMixtureAvailable");
            require(grid->rowCount()==2 && grid->item(0,0)->text()=="FuelB" && available->count()==2,"Mixture editor tank inventory incorrect");
            require(grid->horizontalHeader()->sectionResizeMode(1)==QHeaderView::Interactive,"Mixture widths locked");
            grid->setColumnWidth(1,240); require(grid->columnWidth(1)==240,"Mixture column cannot be resized");
            available->setCurrentText("FuelA"); require(!dialog->findChild<QPushButton *>("tankMixtureAdd")->isEnabled(),"Duplicate tank can be added");
            grid->selectRow(1); dialog->findChild<QPushButton *>("tankMixtureRemove")->click();
            require(dialog->findChild<QPushButton *>("tankMixtureAdd")->isEnabled(),"Removed tank cannot be readded");
            dialog->findChild<QPushButton *>("tankMixtureAdd")->click();
            require(grid->rowCount()==2 && grid->item(1,1)->text()=="1","New tank did not receive default ratio");
            grid->item(1,1)->setText("19"); dialog->findChild<QPushButton *>("tankMixtureUp")->click();
            require(grid->item(0,0)->text()=="FuelA" && grid->item(0,1)->text()=="19","Reordering detached ratio from tank");
            auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
            grid->item(0,1)->setText("0"); ok->click();
            require(dialog->isVisible() && !dialog->findChild<QLabel *>("tankMixtureError")->text().isEmpty(),"Invalid mixture accepted");
            // Same numeric vector as before, but explicitly assigned to new order.
            grid->item(0,1)->setText("17"); grid->item(1,1)->setText("19"); ok->click();
         }); button->click();
         require(panel.hasChanges() && Moderator::Instance()->GetConfiguredObject("Engine")->GetStringArrayParameter("Tank").front()=="FuelB", "Combined editor changed engine before Apply");
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         require(error.isEmpty(),qPrintable(error));
      }
      require(window.saveScriptTo(mixturePath) && checkedLoadScript(mixturePath) && window.buildScript(),"Mixture dialog round trip failed");
      engine=Moderator::Instance()->GetConfiguredObject("Engine");
      require(engine->GetStringArrayParameter("Tank").front()=="FuelA" && engine->GetRvectorParameter("MixRatio")[0]==17 &&
         engine->GetRvectorParameter("MixRatio")[1]==19,"Mixture dialog lost explicit ratios unchanged from original vector");
      editor->setPlainText("Create Variable count;\nBeginMissionSequence;\nWhile count < 1e12;\ncount = count + 1;\nEndWhile;\n");
      bool paused = false, resumed = false, protectedEdits = false;
      double countAtPause = 0;
      auto counter = [] { return Moderator::Instance()->GetInternalObject("count")->GetRealParameter("Value"); };
      QTimer::singleShot(40, &window, [&] {
         protectedEdits = window.isRunning() && editor->isReadOnly() && !window.buildScript()
            && window.runMission() == MainWindow::RunResult::Busy && !checkedLoadScript(script);
         window.pauseMission();
         paused = Moderator::Instance()->GetRunState() == Gmat::PAUSED;
         countAtPause = counter();
      });
      QTimer::singleShot(110, &window, [&] {
         paused = paused && counter() == countAtPause;
         window.resumeMission();
         resumed = Moderator::Instance()->GetRunState() == Gmat::RUNNING;
      });
      QTimer::singleShot(200, &window, [&] { window.stopMission(); });
      require(window.runMission() == MainWindow::RunResult::Stopped, "Stop was not reported as interruption");
      require(paused && resumed && protectedEdits, "Pause/resume or reentrancy protection failed");
      require(!window.isRunning() && !editor->isReadOnly(), "Run controls did not reset");
      bool closeBlocked = false;
      QTimer::singleShot(40, &window, [&] { closeBlocked = !window.close() && window.isVisible(); });
      require(window.runMission() == MainWindow::RunResult::Stopped && closeBlocked,
         "Window closed before stopping the active engine");
      editor->setPlainText("Not a valid GMAT command;\n");
      require(window.runMission() == MainWindow::RunResult::Failed, "Invalid script ran the previous mission");
      require(checkedLoadScript(script), "Could not recover after invalid script");
      require(window.runMission() == MainWindow::RunResult::Completed, "Recovery execution failed");
      auto *tree = window.findChild<QTreeWidget *>("Resources");
      require(tree != nullptr, "Resource tree missing");
      const auto items = tree->findItems("QtSat", Qt::MatchExactly | Qt::MatchRecursive);
      require(items.size() == 1, "Spacecraft resource not displayed");
      tree->itemDoubleClicked(items.first(), 0);
      ResourceEditor *panel = nullptr;
      for (auto *widget : window.findChildren<QWidget *>())
         if (auto *candidate = dynamic_cast<ResourceEditor *>(widget)) panel = candidate;
      require(panel != nullptr, "Resource double click did not open editable panel");
      auto *table = panel->findChild<QTableWidget *>();
      require(table != nullptr, "Resource property table missing");
      auto *sections=panel->findChild<QTabBar *>("propertySections");
      require(sections && sections->tabText(sections->currentIndex())=="Orbit","Spacecraft did not open on Orbit");
      int massSection=-1;
      for (int i=0;i<sections->count();++i) if (sections->tabText(i)=="Ballistic/Mass") massSection=i;
      require(massSection>=0,"Spacecraft mass section missing");
      for (int row=0;row<table->rowCount();++row) {
         if (table->item(row,0)->text()=="DryMass") require(table->isRowHidden(row),"Orbit includes mass properties");
         if (table->item(row,0)->text()=="X") require(table->isRowHidden(row) && panel->findChild<QLineEdit *>("resource_X")->isVisible(),"Primary orbit state missing or duplicated in Advanced");
      }
      sections->setCurrentIndex(massSection);
      panel->findChild<QTabWidget *>("resourceEditorTabs")->setCurrentIndex(0);
      panel->findChild<QTabWidget *>("resourceFormTabs")->setCurrentIndex(0);
      bool massFound = false;
      for (int row = 0; row < table->rowCount(); ++row) {
         if (table->item(row, 0)->text() == "DryMass") {
            require(table->isRowHidden(row),"Mass duplicated in Advanced");
            const auto unchangedScript=editor->toPlainText();
            auto *field=panel->findChild<QLineEdit *>("ballistics_DryMass");
            require(field && field->isVisible(),"Primary mass editor not visible");
            const auto originalText=field->text(); field->setFocus(); QApplication::processEvents();
            field->selectAll(); QApplication::clipboard()->setText("888.25");
            window.findChild<QAction *>("edit_paste")->trigger();
            require(field->text()=="888.25" && editor->toPlainText()==unchangedScript,"Property Paste modified the script");
            window.findChild<QAction *>("edit_undo")->trigger();
            require(field->text()==originalText && editor->toPlainText()==unchangedScript,"Property Undo modified the script");
            window.findChild<QAction *>("edit_redo")->trigger();
            panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->setFocus();
            QApplication::processEvents();
            require(field->text()=="888.25","Primary mass edit lost on focus change");
            massFound = true; break;
         }
      }
      require(massFound && panel->hasChanges(), "Mass is not editable in the panel");
      sections->setCurrentIndex(0);
      require(panel->hasChanges(),"Switching sections discarded pending edits");
      sections->setCurrentIndex(sections->count()-1);
      auto *propertyFilter=panel->findChild<QLineEdit *>("propertyFilter");
      propertyFilter->setText("drymass");
      for (int row=0;row<table->rowCount();++row)
         require(table->isRowHidden(row)==(table->item(row,0)->data(Qt::UserRole+1).toBool() || table->item(row,0)->text()!="DryMass"),"Property filter did not combine with sections");
      propertyFilter->clear(); sections->setCurrentIndex(massSection);
      tree->itemDoubleClicked(items.first(),0);
      int resourcePanelCount=0;
      for (auto *widget:window.findChildren<QWidget *>())
         if (dynamic_cast<ResourceEditor *>(widget)) ++resourcePanelCount;
      require(resourcePanelCount==1 && panel->hasChanges(),"Reopening a resource duplicated its panel or lost pending edits");
      require(!window.buildScript() && window.runMission() == MainWindow::RunResult::Failed,
         "Unapplied resource settings were silently ignored during run");
      require(!checkedLoadScript(script), "Loading discarded pending resource settings");
      require(!window.createResource("Spacecraft","PendingSat",editor->toPlainText()).isEmpty(),"Creation ignored pending panel changes");
      if (!screenshot.isEmpty()) {
         QApplication::processEvents();
         require(window.grab().save(screenshot), "Resource screenshot failed");
      }
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      require(dryMass() == 888.25, "Apply button did not commit the entered value");
      const auto beforeCreate=editor->toPlainText();
      for (const auto &name:{"QtSat","Earth","bad name","Bad; Stop;"})
         require(!window.createResource("Spacecraft",name,beforeCreate).isEmpty(),"Invalid or duplicate name accepted");
      require(!window.createResource("NoSuchType","CreatedSat",beforeCreate).isEmpty(),"Unknown type accepted");
      require(window.createResource("Spacecraft","CreatedSat",beforeCreate).isEmpty(),"Spacecraft creation failed");
      require(Moderator::Instance()->GetConfiguredObject("CreatedSat") && editor->document()->isModified(),"Created spacecraft not synchronized");
      require(!window.createResource("Spacecraft","StaleSat",beforeCreate).isEmpty(),"Stale creation snapshot accepted");
      editor->undo(); require(editor->toPlainText()==beforeCreate && window.buildScript(),"Creation could not be undone");
      require(!Moderator::Instance()->GetConfiguredObject("CreatedSat"),"Undo retained created resource");
      bool createdThroughDialog=false;
      bool capturedCreateDialog=screenshot.isEmpty();
      workflowAfterExposure(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog");
         if (!dialog) return;
         dialog->findChild<QComboBox *>("resourceType")->setCurrentText("Spacecraft");
         dialog->findChild<QLineEdit *>("resourceName")->setText("CreatedSat");
         if (!screenshot.isEmpty()) capturedCreateDialog=dialog->grab().save(screenshot+".create.png");
         dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
         createdThroughDialog=dialog->result()==QDialog::Accepted;
         if (!createdThroughDialog) dialog->reject();
      });
      window.findChild<QAction *>("createResource")->trigger();
      require(createdThroughDialog && tree->findItems("CreatedSat",Qt::MatchExactly|Qt::MatchRecursive).size()==1,"New resource dialog did not populate tree");
      require(capturedCreateDialog,"Could not capture resource creation dialog");
      const auto createdEpoch=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("CreatedSat"))->GetEpoch();
      require(window.applyMissionChange(window.missionSnapshot(),-1,MissionEdit::Append,
         "Propagate QtProp(CreatedSat) {CreatedSat.ElapsedSecs = 60};").isEmpty(),"Created spacecraft could not be used by mission");
      const auto createdRun=window.runMission();
      if (createdRun!=MainWindow::RunResult::Completed) {
         std::cerr<<"Created-resource run: "<<window.statusBar()->currentMessage().toStdString()<<'\n';
         if (auto *messages=window.findChild<QPlainTextEdit *>("messageWindow")) std::cerr<<messages->toPlainText().right(1500).toStdString()<<'\n';
         for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *view=dynamic_cast<ResourceEditor *>(child->widget())) {
            std::cerr<<"Panel "<<child->property("resourceName").toString().toStdString()<<" pending="<<view->hasChanges()<<'\n';
            if (auto *form=dynamic_cast<ResourceForm *>(view->findChild<QWidget *>("resourceForm"))) { const auto pending=form->changes(); for (auto it=pending.cbegin();it!=pending.cend();++it) std::cerr<<"  "<<it.key().toStdString()<<"="<<it.value().toStdString()<<'\n'; }
         }
      }
      require(createdRun==MainWindow::RunResult::Completed,"Mission with created spacecraft failed");
      auto *createdResult=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("CreatedSat"));
      require(createdResult && std::abs((createdResult->GetEpoch()-createdEpoch)*86400-60)<.01,"Created spacecraft did not propagate");
      require(window.applyResourceChanges("CreatedSat",{{"DryMass","932"}},editor->toPlainText()).isEmpty(),"Could not update resource behind clean panel");
      const auto currentCreated=tree->findItems("CreatedSat",Qt::MatchExactly|Qt::MatchRecursive);
      tree->itemDoubleClicked(currentCreated.first(),0);
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      bool refreshedMass=false;
      for (auto *widget:window.findChildren<QWidget *>()) if (auto *resource=dynamic_cast<ResourceEditor *>(widget)) {
         if (resource->parentWidget()->property("resourceName").toString()!="CreatedSat") continue;
         auto *properties=resource->findChild<QTableWidget *>();
         for (int row=0;row<properties->rowCount();++row)
            if (properties->item(row,0)->text()=="DryMass") refreshedMass=properties->item(row,1)->text()=="932";
      }
      if (!refreshedMass) {
         std::cerr<<"Configured created mass="<<Moderator::Instance()->GetConfiguredObject("CreatedSat")->GetRealParameter("DryMass")<<'\n';
         for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *view=dynamic_cast<ResourceEditor *>(child->widget())) {
            std::cerr<<"Refresh panel "<<child->property("resourceName").toString().toStdString()<<" pending="<<view->hasChanges()<<'\n';
            auto *properties=view->findChild<QTableWidget *>("resourceProperties");
            for (int row=0;properties && row<properties->rowCount();++row) if (properties->item(row,0)->text()=="DryMass") std::cerr<<"  stored mass="<<properties->item(row,1)->text().toStdString()<<'\n';
            if (auto *field=view->findChild<QLineEdit *>("ballistics_DryMass")) std::cerr<<"  inline mass="<<field->text().toStdString()<<'\n';
         }
      }
      require(refreshedMass,"Reopening a clean stale resource panel did not refresh its values");
      QApplication::processEvents();
      auto *missionTree=window.findChild<QTreeWidget *>("Mission");
      QTreeWidgetItem *propagationItem=nullptr;
      for (QTreeWidgetItemIterator it(missionTree);*it;++it) {
         if (!(*it)->data(0,Qt::UserRole).isValid()) continue;
         const auto &node=window.missionSnapshot().nodes[(*it)->data(0,Qt::UserRole).toInt()];
         if (node.type=="Propagate" && node.statement.contains("QtSat")) propagationItem=*it;
      }
      require(propagationItem!=nullptr,"Propagation command missing from tree");
      window.findChild<QTabWidget *>()->setCurrentIndex(1);
      missionTree->expandAll(); missionTree->itemDoubleClicked(propagationItem,0);
      CommandEditor *commandPanel=nullptr;
      for (auto *widget:window.findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<CommandEditor *>(widget)) commandPanel=candidate;
      require(commandPanel!=nullptr,"Propagation command panel missing");
      auto *form=commandPanel->findChild<QWidget *>("propagationForm");
      auto *commandText=commandPanel->findChild<QPlainTextEdit *>("commandSource");
      require(form && !form->isHidden(),"Simple propagation form unavailable");
      const QString originalCommand=commandText->toPlainText();
      const QString advanced="Propagate QtProp(QtSat) {QtSat.ElapsedSecs = 600, QtSat.Earth.Periapsis};";
      commandText->setPlainText(advanced);
      require(form->isHidden() && commandText->toPlainText()==advanced,"Form simplified an advanced command");
      auto *editStops=commandPanel->findChild<QPushButton *>("editPropagationStops"); require(editStops && !editStops->isHidden(),"Multiple stopping conditions lack GUI editor");
      workflowAfterExposure(0,[&] {
         auto *dialog=commandPanel->findChild<QDialog *>("propagationStopsDialog"); auto *table=dialog->findChild<QTableWidget *>("propagationStopsTable");
         require(table->rowCount()==2 && table->item(1,1)->text().isEmpty() && !(table->item(1,1)->flags() & Qt::ItemIsEditable),"Apsis row has editable goal");
         table->item(0,1)->setText("120"); dialog->reject();
      }); editStops->click(); require(commandText->toPlainText()==advanced,"Stopping conditions Cancel changed command");
      workflowAfterExposure(0,[&] {
         auto *dialog=commandPanel->findChild<QDialog *>("propagationStopsDialog"); auto *table=dialog->findChild<QTableWidget *>("propagationStopsTable");
         require(table->horizontalHeader()->sectionResizeMode(0)==QHeaderView::Interactive,"Stop columns are locked");
         dialog->findChild<QPushButton *>("propagationStopAdd")->click();
         require(!dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Blank stop row accepted");
         table->item(2,0)->setText("QtSat.ElapsedDays"); table->item(2,1)->setText("0.001");
         dialog->findChild<QPushButton *>("propagationStopUp")->click();
         require(table->item(1,0)->text()=="QtSat.ElapsedDays" && table->item(1,1)->text()=="0.001","Stop reorder detached goal");
         table->selectRow(2); dialog->findChild<QPushButton *>("propagationStopRemove")->click();
         dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      }); editStops->click();
      require(commandText->toPlainText().contains("QtSat.ElapsedSecs = 600, QtSat.ElapsedDays = 0.001"),"Multiple stop GUI source update failed");
      commandText->undo(); require(commandText->toPlainText()==advanced,"Stopping conditions edit was not one undo step");
      commandText->setPlainText(originalCommand);
      auto *duration=commandPanel->findChild<QLineEdit *>("propagationDuration");
      auto *units=commandPanel->findChild<QComboBox *>("propagationUnits");
      require(!form->isHidden() && duration->text().toDouble()==600,"Form did not load command duration");
      auto *selectedSpacecraft=commandPanel->findChild<QComboBox *>("propagationSpacecraft");
      selectedSpacecraft->setCurrentText("CreatedSat");
      require(commandText->toPlainText().contains("QtProp(CreatedSat) {CreatedSat.ElapsedSecs"),"Spacecraft selection did not update its stop condition");
      selectedSpacecraft->setCurrentText("QtSat");
      const auto beforeForm=editor->toPlainText();
      duration->setText("not-a-number");
      auto *applyCommand=commandPanel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);
      applyCommand->click();
      require(commandPanel->hasChanges() && editor->toPlainText()==beforeForm,"Invalid form input altered the mission");
      units->setCurrentIndex(units->findData("ElapsedDays")); duration->setText("0.01");
      require(commandText->toPlainText().contains("QtSat.ElapsedDays = 0.01"),"Form did not synchronize duration and units");
      commandText->setPlainText("Propagate 'GUI stop' QtProp(QtSat) {QtSat.ElapsedDays = 0.01}; % retain stop comment");
      workflowAfterExposure(0,[&] {
         auto *dialog=commandPanel->findChild<QDialog *>("propagationParameterDialog");
         require(dialog,"Stop parameter browser missing");
         dialog->findChild<QComboBox *>("reportPropertyObject")->setCurrentText("QtSat");
         auto *property=dialog->findChild<QComboBox *>("reportPropertyType");
         require(property->findText("A1ModJulian")>=0,"Epoch stop parameter absent from browser"); property->setCurrentText("A1ModJulian");
         dialog->findChild<QPushButton *>("reportUseReference")->click();
         dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      }); commandPanel->findChild<QPushButton *>("propagationChooseStop")->click();
      const auto epochGoal=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"))->GetEpoch()+.01;
      duration->setText(QString::number(epochGoal,'g',17));
      require(units->currentData().toString().isEmpty() && commandText->toPlainText().contains("QtSat.A1ModJulian") &&
         commandText->toPlainText().contains("'GUI stop'") && commandText->toPlainText().contains("% retain stop comment"),"Selected epoch stop lost source or type");
      const auto selectedStop=commandText->toPlainText();
      workflowAfterExposure(0,[&] { commandPanel->findChild<QDialog *>("propagationParameterDialog")->reject(); });
      commandPanel->findChild<QPushButton *>("propagationChooseGoal")->click(); require(commandText->toPlainText()==selectedStop,"Stop goal picker Cancel changed source");
      if (!screenshot.isEmpty()) { duration->setFocus(); QApplication::processEvents(); require(window.grab().save(screenshot+".propagation.png"),"Propagation form capture failed"); }
      applyCommand->click();
      auto &propagationFiles=fixtureDirectory(); const auto propagationPath=propagationFiles.filePath("selected-stop.script");
      require(window.saveScriptTo(propagationPath) && checkedLoadScript(propagationPath) && window.buildScript(),"Selected stop condition save/reopen failed");
      const auto formEpoch=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"))->GetEpoch();
      require(window.runMission()==MainWindow::RunResult::Completed,"Form-edited propagation failed");
      const auto finalEpoch=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("QtSat"))->GetEpoch();
      require(std::abs((finalEpoch-formEpoch)*86400-864)<.01,"Form-selected duration did not propagate for 0.01 days");
      const auto beforeDeletion=editor->toPlainText();
      require(!window.deleteResource("QtSat",beforeDeletion).isEmpty(),"Deletion removed a spacecraft used by a command");
      require(!window.deleteResource("QtForces",beforeDeletion).isEmpty(),"Deletion removed a force model used by a propagator");
      require(!window.deleteResource("EarthMJ2000Eq",beforeDeletion).isEmpty(),"Deletion removed a built-in coordinate system");
      require(editor->toPlainText()==beforeDeletion,"Rejected deletion changed the script");
      require(window.createResource("Spacecraft","UnusedSat",beforeDeletion).isEmpty(),"Deletion fixture creation failed");
      const auto beforeUnusedDelete=editor->toPlainText();
      const auto unusedItems=tree->findItems("UnusedSat",Qt::MatchExactly|Qt::MatchRecursive);
      require(unusedItems.size()==1,"Unused resource missing from tree");
      tree->itemDoubleClicked(unusedItems.first(),0);
      ResourceEditor *unusedPanel=nullptr;
      for (auto *widget:window.findChildren<QWidget *>()) if (auto *resource=dynamic_cast<ResourceEditor *>(widget))
         if (resource->parentWidget()->property("resourceName").toString()=="UnusedSat") unusedPanel=resource;
      require(unusedPanel!=nullptr,"Unused resource panel missing");
      auto *unusedTable=unusedPanel->findChild<QTableWidget *>(); QTableWidgetItem *unusedMass=nullptr;
      for (int row=0;row<unusedTable->rowCount();++row) if (unusedTable->item(row,0)->text()=="DryMass") unusedMass=unusedTable->item(row,1);
      require(unusedMass!=nullptr,"Unused spacecraft mass missing");
      const auto oldMass=unusedMass->text(); unusedMass->setText("999");
      require(!window.deleteResource("UnusedSat",beforeUnusedDelete).isEmpty(),"Deletion discarded its panel changes");
      unusedMass->setText(oldMass);
      // NASA panels retain their dirty flag after reverting a draft. Clear it
      // through the existing no-op Apply path before testing clean deletion.
      require(!unusedPanel->hasChanges() && unusedPanel->hasUnappliedInput(),"Restored deletion draft did not retain NASA's dirty flag");
      auto *unusedButtons=unusedPanel->findChild<QDialogButtonBox *>("resourceButtons");
      require(unusedButtons && unusedButtons->button(QDialogButtonBox::Apply)->isEnabled(),"Reverted deletion draft has no enabled Apply");
      unusedButtons->button(QDialogButtonBox::Apply)->click();
      require(!unusedPanel->hasChanges() && !unusedPanel->hasUnappliedInput() && editor->toPlainText()==beforeUnusedDelete,"No-op deletion draft Apply changed source or retained dirty input");
      require(window.deleteResource("UnusedSat",beforeUnusedDelete).isEmpty(),"Unrelated open resource panel blocked deletion");
      require(!Moderator::Instance()->GetConfiguredObject("UnusedSat") && tree->findItems("UnusedSat",Qt::MatchExactly|Qt::MatchRecursive).isEmpty(),"Deleted resource remained in model or tree");
      require(!window.deleteResource("CreatedSat",beforeUnusedDelete).isEmpty(),"Stale deletion snapshot accepted");
      editor->undo(); require(editor->toPlainText()==beforeUnusedDelete && window.buildScript(),"Deletion undo failed");
      require(Moderator::Instance()->GetConfiguredObject("UnusedSat")!=nullptr,"Deletion undo did not restore resource");
      require(window.deleteResource("UnusedSat",editor->toPlainText()).isEmpty(),"Repeated deletion failed");
      require(window.createResource("Variable","ScratchOne",editor->toPlainText()).isEmpty() &&
         window.createResource("Variable","ScratchTwo",editor->toPlainText()).isEmpty(),"Variable deletion fixtures failed");
      require(window.deleteResource("ScratchOne",editor->toPlainText()).isEmpty() &&
         !Moderator::Instance()->GetConfiguredObject("ScratchOne") && Moderator::Instance()->GetConfiguredObject("ScratchTwo"),
         "Variable deletion affected a different declaration");
      require(window.runMission()==MainWindow::RunResult::Completed,"Mission failed after unused-resource deletion");
      window.findChild<QTabWidget *>()->setCurrentIndex(0); tree->expandAll();
      const auto scratchItems=tree->findItems("ScratchTwo",Qt::MatchExactly|Qt::MatchRecursive);
      require(scratchItems.size()==1,"Deletion menu fixture missing"); tree->scrollToItem(scratchItems.first());
      bool confirmedDeletion=false;
      workflowAfterExposure(0,&window,[&] {
         auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
         if (!menu) return;
         for (auto *action:menu->actions()) if (action->text()=="Delete resource…") {
            menu->setActiveAction(action);
            workflowAfterExposure(0,&window,[&] {
               if (auto *confirmation=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                  confirmedDeletion=true; confirmation->button(QMessageBox::Yes)->click();
               }
            });
            QKeyEvent activate(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
            QApplication::sendEvent(menu,&activate); return;
         }
         menu->close();
      });
      tree->customContextMenuRequested(tree->visualItemRect(scratchItems.first()).center());
      require(confirmedDeletion && !Moderator::Instance()->GetConfiguredObject("ScratchTwo"),"Context-menu deletion failed");
      const auto sample=QFileInfo(startup).dir().filePath("../samples/Ex_HohmannTransfer.script");
      require(checkedLoadScript(sample),"Could not load shipped OFI sample");
      const auto starFolder=QString::fromStdString(FileManager::Instance()->GetFullPathname("STAR_PATH"));
      require(QFileInfo(QDir(starFolder).filePath("inp_StarsHYGv3.txt")).isFile(),
         "Workflow requires the current inp_StarsHYGv3.txt catalogue through STAR_PATH");
      const auto originalSample=editor->toPlainText();
      const auto dynamics=originalSample.mid(originalSample.indexOf("BeginMissionSequence;"));
      int conversionPrompts=0;
      auto answerConversion=[&](QMessageBox::StandardButton answer) {
         workflowAfterExposure(0,&window,[&,answer] {
            auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (prompt && prompt->objectName()=="openFramesConversionPrompt") {
               ++conversionPrompts; prompt->button(answer)->click();
            } else if (prompt) prompt->reject();
         });
      };
      answerConversion(QMessageBox::No);
      require(!window.buildScript() && conversionPrompts==1,"OFI build did not offer conversion or ignored decline");
      require(editor->toPlainText()==originalSample && !editor->document()->isModified(),"Declined conversion changed the script");
      answerConversion(QMessageBox::Yes);
      require(window.buildScript() && conversionPrompts==2,"Accepted automatic OFI conversion failed");
      require(editor->toPlainText().endsWith(dynamics) && editor->document()->isModified(),"Conversion changed mission or failed to mark unsaved changes");
      require(window.runMission()==MainWindow::RunResult::Completed,"Converted Hohmann mission did not run");
      auto convertedView=window.plotReceiver()->model("OFI_EarthView");
      require(convertedView && convertedView->perspective && convertedView->fieldOfView==45,
         "Converted OF camera settings did not reach the renderer model");
      editor->undo(); require(editor->toPlainText()==originalSample,"Conversion was not one undoable edit");
      answerConversion(QMessageBox::Yes);
      require(window.runMission()==MainWindow::RunResult::Completed && conversionPrompts==3,
         "Run did not offer conversion after undo");
      auto rolledCamera=qtCameraSettings(editor->toPlainText()).value("OFI_EarthView");
      require(rolledCamera.views.size()==2 && rolledCamera.views[0].name=="Earth_View" && rolledCamera.views[1].name=="DefaultSC_View",
         "Shipped example lost additional named views");
      rolledCamera.up=std::array<double,3>{1,2,3};
      editor->setPlainText(setQtCameraSetting(editor->toPlainText(),"OFI_EarthView",rolledCamera));
      require(window.runMission()==MainWindow::RunResult::Completed,"Non-axis up mission failed");
      auto checkCameraUp=[&] {
         const auto model=window.plotReceiver()->model("OFI_EarthView");
         require(model && !model->cameras.empty(),"Camera history missing for non-axis up vector");
         for (const auto *camera:{&model->cameras.front(),&model->cameras.back()})
            for (int i=0;i<3;++i) require(std::abs(camera->up[i]-(i+1))<1e-10,"Non-axis camera up changed during propagation");
      };
      checkCameraUp();
      auto checkViewHistories=[&] {
         const auto model=window.plotReceiver()->model("OFI_EarthView");
         require(model && model->cameraViews.size()==3,"Named camera histories missing");
         for (int index=1;index<3;++index) {
            const auto &view=model->cameraViews[index];
            require(view.cameras.size()==model->cameras.size(),"Secondary camera history is incomplete");
            const QString reference=index==1 ? "Earth" : "DefaultSC";
            for (const auto *camera:{&view.cameras.front(),&view.cameras.back()}) {
               const PlotPoint *point=nullptr;
               for (const auto &curve:model->curves) if (curve.name==reference)
                  for (const auto &sample:curve.points) if (sample.frame==camera->frame) point=&sample;
               require(point,"Named camera reference frame is absent");
               require(std::abs(camera->target[0]-point->x)<1e-8 && std::abs(camera->target[1]-point->y)<1e-8 &&
                  std::abs(camera->target[2]-point->z)<1e-8,
                  "Additional camera failed to track its reference object");
               const double origin[]={point->x,point->y,point->z};
               for (int axis=0;axis<3;++axis) {
                  require(std::abs(camera->eye[axis]-origin[axis]+30000*point->bodyToView[axis*3+1])<1e-8,
                     "Body-relative eye does not follow object orientation");
                  require(std::abs(camera->up[axis]-point->bodyToView[axis*3+2])<1e-10,"Body-relative up does not follow object orientation");
               }
            }
         }
      };
      checkViewHistories();
      const auto convertedSource=editor->toPlainText();
      require(window.applyResourceChanges("OFI_EarthView",{{"StarCount","321"}},convertedSource).isEmpty(),
         "Converted plot resource edit failed");
      require(qtCameraSettings(editor->toPlainText())["OFI_EarthView"].fieldOfView==45,
         "Resource editing lost the Qt camera directive");
      editor->undo(); require(editor->toPlainText()==convertedSource && window.buildScript(),"Camera-preserving resource edit lost Undo");
      auto &cameraFiles=fixtureDirectory();
      const auto savedCamera=cameraFiles.filePath("converted.script");
      require(window.saveScriptTo(savedCamera) && checkedLoadScript(savedCamera) && window.runMission()==MainWindow::RunResult::Completed,
         "Converted camera save/reopen failed");
      checkCameraUp();
      convertedView=window.plotReceiver()->model("OFI_EarthView");
      require(convertedView && convertedView->perspective && convertedView->fieldOfView==45,"Save/reopen changed imported camera projection");
      {
         const auto before=editor->toPlainText();
         auto *keep=window.findChild<QPushButton *>("keepOrbitProjection");
         auto *projection=window.findChild<QComboBox *>("orbitProjection");
         auto *fov=window.findChild<QDoubleSpinBox *>("orbitFieldOfView");
         require(keep && keep->isEnabled() && projection && fov,"Projection persistence controls missing");
         auto *configured=Moderator::Instance()->GetConfiguredObject("OFI_EarthView");
         projection->setCurrentIndex(0); fov->setValue(37.5); keep->click();
         const auto after=editor->toPlainText(); const auto cameras=qtCameraSettings(after);
         require(cameras.size()==1 && !cameras["OFI_EarthView"].perspective && cameras["OFI_EarthView"].fieldOfView==37.5 &&
            after.endsWith(dynamics) && editor->document()->isModified(),"Keep projection changed calculations or failed to save selected settings");
         require(cameras["OFI_EarthView"].up==rolledCamera.up,"Keep projection discarded camera roll");
         require(Moderator::Instance()->GetConfiguredObject("OFI_EarthView")==configured,
            "Saving a camera comment reconstructed the mission during the viewer callback");
         keep->click(); require(editor->toPlainText()==after,"Repeated Keep projection duplicated metadata");
         editor->undo(); require(editor->toPlainText()==before,"Keep projection was not one undoable edit");
         editor->redo(); require(editor->toPlainText()==after,"Projection Redo failed");
         editor->moveCursor(QTextCursor::End); editor->insertPlainText("% unbuilt edit\n");
         const auto dirty=editor->toPlainText();
         bool refused=false;
         workflowAfterExposure(0,&window,[&] {
            if (auto *warning=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
               refused=warning->text().contains("Build the current script"); warning->accept();
            }
         });
         keep->click(); require(refused && editor->toPlainText()==dirty,"Keep projection overwrote an unbuilt script");
         editor->undo();
         require(window.saveScriptTo(savedCamera) && checkedLoadScript(savedCamera) && window.runMission()==MainWindow::RunResult::Completed,
            "Interactive projection save/reopen failed");
         checkCameraUp();
         const auto reopened=window.plotReceiver()->model("OFI_EarthView");
         require(reopened && !reopened->perspective && reopened->fieldOfView==37.5,"Saved interactive projection was not restored");
      }
      const auto beforeAxis=editor->toPlainText();
      require(window.applyResourceChanges("OFI_EarthView",{{"ViewUpAxis","Y"}},beforeAxis).isEmpty(),"Explicit camera up-axis edit failed");
      require(!qtCameraSettings(editor->toPlainText())["OFI_EarthView"].up,"Imported roll overrode an explicit up-axis edit");
      require(window.runMission()==MainWindow::RunResult::Completed,"Edited camera up axis failed to run");
      require(window.plotReceiver()->model("OFI_EarthView")->cameras.back().up==std::array<double,3>{0,1,0},"Edited up axis did not reach camera history");
      editor->undo(); require(editor->toPlainText()==beforeAxis && window.runMission()==MainWindow::RunResult::Completed,"Up-axis Undo failed");
      checkCameraUp();
      checkViewHistories();
      {
         auto *selector=window.findChild<QComboBox *>("orbitCameraView");
         auto *projection=window.findChild<QComboBox *>("orbitProjection");
         auto *fov=window.findChild<QDoubleSpinBox *>("orbitFieldOfView");
         auto *keep=window.findChild<QPushButton *>("keepOrbitProjection");
         require(selector && selector->count()==3,"Named camera selector missing");
         const auto model=window.plotReceiver()->model("OFI_EarthView");
         const auto frameCount=model->frame;
         auto *spacecraft=Moderator::Instance()->GetInternalObject("DefaultSC");
         selector->setCurrentIndex(2);
         require(model->selectedCamera==2 && model->perspective && model->fieldOfView==45,"Selecting camera failed to restore its projection");
         const auto first=orbitCamera(*model,model->cameraViews[2].cameras.front().frame,0,0,10000);
         const auto last=orbitCamera(*model,model->cameraViews[2].cameras.back().frame,0,0,10000);
         require((first.target-last.target).length()>1,"Selected camera does not follow replay history");
         fov->setValue(28.5); projection->setCurrentIndex(0); keep->click();
         const auto saved=qtCameraSettings(editor->toPlainText()).value("OFI_EarthView");
         require(!saved.views[1].perspective && saved.views[1].fieldOfView==28.5 && saved.fieldOfView==37.5,
            "Keep projection changed the wrong camera");
         selector->setCurrentIndex(0); require(model->fieldOfView==37.5,"Primary camera projection was lost during switching");
         selector->setCurrentIndex(2); require(model->fieldOfView==28.5 && !model->perspective,"Secondary camera projection was lost during switching");
         require(model->frame==frameCount && Moderator::Instance()->GetInternalObject("DefaultSC")==spacecraft,
            "Switching cameras reran or reconstructed the mission");
         require(window.saveScriptTo(savedCamera) && checkedLoadScript(savedCamera) && window.runMission()==MainWindow::RunResult::Completed,
            "Multiple camera save/reopen failed");
         checkViewHistories();
         selector=window.findChild<QComboBox *>("orbitCameraView"); selector->setCurrentIndex(2);
         const auto reopened=window.plotReceiver()->model("OFI_EarthView");
         require(!reopened->perspective && reopened->fieldOfView==28.5,"Saved secondary projection did not reopen");
         const auto good=editor->toPlainText(); auto broken=qtCameraSettings(good).value("OFI_EarthView");
         broken.views[0].reference="NoSuchObject";
         if (!broken.views[0].automaticBody.isEmpty()) broken.views[0].automaticBody="NoSuchObject";
         editor->setPlainText(setQtCameraSetting(good,"OFI_EarthView",broken));
         require(!window.buildScript(),"Unknown camera reference silently built");
         editor->setPlainText(good); require(window.runMission()==MainWindow::RunResult::Completed,"Invalid camera reference recovery failed");
         checkViewHistories();
      }
      QFile unchangedSample(sample);
      require(unchangedSample.open(QIODevice::ReadOnly) && QString::fromUtf8(unchangedSample.readAll())==originalSample,
         "Automatic conversion overwrote the example file");
      {
         auto trajectoryScript=originalSample;
         trajectoryScript.replace(QRegularExpression("TheView\\.ViewFrame\\s*=\\s*CoordinateSystem"),"TheView.ViewFrame = DefaultSC;\nTheView.ViewTrajectory = On");
         const auto converted=convertOpenFramesViews(trajectoryScript); require(converted.error.isEmpty(),"Trajectory runtime conversion failed");
         editor->setPlainText(converted.script);
         auto &files=fixtureDirectory(); const auto path=files.filePath("trajectory-camera.script");
         require(window.saveScriptTo(path) && checkedLoadScript(path) && window.runMission()==MainWindow::RunResult::Completed,"Trajectory camera save/reopen/run failed");
         const auto model=window.plotReceiver()->model("OFI_EarthView"); require(model && model->cameras.size()>2,"Trajectory camera history missing");
         bool moved=false;
         for (const auto &curve:model->curves) if (curve.name=="DefaultSC" && curve.points.size()>1) {
            const auto &a=curve.points.front(),&b=curve.points.back(); moved=std::hypot(b.x-a.x,b.y-a.y,b.z-a.z)>1;
         }
         require(moved,"Trajectory fixture spacecraft did not move");
         const auto first=model->cameras.front();
         for (const auto &camera:model->cameras) for (int axis=0;axis<3;++axis)
            require(std::abs(camera.eye[axis]-first.eye[axis])<1e-8 && std::abs(camera.target[axis]-first.target[axis])<1e-8,
               "Stored trajectory camera moves with spacecraft");
         require(std::abs(first.eye[0]-150000)<1e-8 && std::abs(first.target[0])<1e-8,"Stored trajectory camera pose was not retained");
         trajectoryScript.replace(QRegularExpression("TheView\\.SetDefaultLocation\\s*=\\s*On"),"TheView.SetDefaultLocation = Off");
         const auto automatic=convertOpenFramesViews(trajectoryScript); require(automatic.error.isEmpty(),"Automatic trajectory runtime conversion failed");
         editor->setPlainText(automatic.script);
         require(window.saveScriptTo(path) && checkedLoadScript(path) && window.runMission()==MainWindow::RunResult::Completed,"Automatic trajectory save/reopen/run failed");
         const auto autoModel=window.plotReceiver()->model("OFI_EarthView");
         require(autoModel && autoModel->automaticTrajectory=="DefaultSC","Automatic trajectory did not reach viewer");
         const auto autoCamera=orbitCamera(*autoModel,autoModel->frame,0,0,1,1);
         require(autoCamera.distance>1 && autoCamera.outward.y()<-.99,"Automatic trajectory camera direction missing");
         const auto good=editor->toPlainText(); auto invalid=qtCameraSettings(good).value("OFI_EarthView"); invalid.automaticTrajectory="Missing";
         editor->setPlainText(setQtCameraSetting(good,"OFI_EarthView",invalid)); require(!window.buildScript(),"Unknown automatic trajectory accepted");
         editor->setPlainText(good); require(window.runMission()==MainWindow::RunResult::Completed,"Automatic trajectory error recovery failed");
         osg::Vec3d azimuthUp;
         for (bool shortest:{false,true}) {
            auto aligned=trajectoryScript;
            aligned.replace("BeginMissionSequence;",QString("TheView.LookAtFrame = DefaultSC;\nTheView.ShortestAngle = %1;\n"
               "Earth_View.ViewFrame = DefaultSC;\nEarth_View.ViewTrajectory = On;\nEarth_View.SetDefaultLocation = Off;\nEarth_View.SetCurrentLocation = Off;\n"
               "Earth_View.LookAtFrame = DefaultSC;\nEarth_View.ShortestAngle = %1;\nBeginMissionSequence;").arg(shortest ? "On" : "Off"));
            const auto converted=convertOpenFramesViews(aligned); require(converted.error.isEmpty(),"Automatic trajectory LookAt conversion failed");
            editor->setPlainText(converted.script);
            require(window.saveScriptTo(path) && checkedLoadScript(path) && window.runMission()==MainWindow::RunResult::Completed,"Automatic LookAt save/reopen failed");
            const auto model=window.plotReceiver()->model("OFI_EarthView");
            require(model && model->cameraViews[1].automaticTrajectory=="DefaultSC","Named automatic LookAt did not reach viewer");
            auto *selector=window.findChild<QComboBox *>("orbitCameraView"); require(selector,"Automatic named camera selector missing");
            for (int selected:{0,1}) {
               selector->setCurrentIndex(selected);
               const auto camera=orbitCamera(*model,model->frame,0,0,1,1);
               osg::Vec3d direction;
               for (const auto &curve:model->curves) if (curve.name=="DefaultSC") {
                  const auto &point=curve.points.back(); direction.set(point.x,point.y,point.z); direction.normalize();
               }
               require((camera.outward+direction).length()<1e-8 && std::abs(camera.up*direction)<1e-8,"Automatic LookAt ignored target direction");
               if (selected==0) {
                  if (!shortest) azimuthUp=camera.up;
                  else require((camera.up-azimuthUp).length()>1e-4,"Automatic ShortestAngle did not change roll");
               }
            }
         }
         const auto beforePose=editor->toPlainText();
         const auto secondaryBefore=qtCameraSettings(beforePose).value("OFI_EarthView").views;
         require(window.applyResourceChanges("OFI_EarthView",{{"ViewPointVector","[0 -12345 0]"},{"ViewDirection","[0 0 0]"}},beforePose).isEmpty(),"Explicit automatic-camera override failed");
         const auto overridden=qtCameraSettings(editor->toPlainText()).value("OFI_EarthView");
         require(overridden.automaticTrajectory.isEmpty() && !overridden.lookAtRotation &&
            overridden.views[0].automaticTrajectory==secondaryBefore[0].automaticTrajectory && overridden.views[0].lookAtRotation,
            "Explicit pose was still automatic or changed named views");
         require(window.saveScriptTo(path) && checkedLoadScript(path) && window.runMission()==MainWindow::RunResult::Completed,"Manual camera override save/reopen failed");
         const auto overriddenModel=window.plotReceiver()->model("OFI_EarthView");
         const auto pose=orbitCamera(*overriddenModel,overriddenModel->frame,0,0,1,1);
         require(pose.target.length()<1e-8 && std::abs(pose.distance-12345)<1e-8,"Explicit camera distance/target ignored after reopen");
         editor->setPlainText(beforePose); require(window.buildScript(),"Automatic camera fixture restoration failed");
         for (const auto &change:QList<QMap<QString,QString>>{
               {{"ViewPointReference","Earth"}},{{"ViewPointVector","[0 -12345 0]"}},{{"ViewScaleFactor","2"}},
               {{"ViewDirection","[0 0 0]"}},{{"ViewUpAxis","X"}},{{"ViewUpCoordinateSystem","EarthFixed"}}}) {
            require(window.applyResourceChanges("OFI_EarthView",change,beforePose).isEmpty(),"Individual automatic-camera override failed");
            require(qtCameraSettings(editor->toPlainText()).value("OFI_EarthView").automaticTrajectory.isEmpty(),"Individual pose field retained automatic framing");
            editor->undo(); require(editor->toPlainText()==beforePose && window.buildScript(),"Automatic camera override Undo failed");
         }
         require(window.runMission()==MainWindow::RunResult::Completed && window.plotReceiver()->model("OFI_EarthView")->automaticTrajectory=="DefaultSC",
            "Undo did not restore automatic trajectory mode");
      }
      for (const bool aligned:{false,true}) {
         auto originScript=originalSample;
         originScript.replace(QRegularExpression("TheView\\.SetDefaultLocation\\s*=\\s*On"),"TheView.SetDefaultLocation = Off");
         QString extra="Earth_View.ViewFrame = CoordinateSystem;\nEarth_View.SetCurrentLocation = Off;\nEarth_View.SetDefaultLocation = Off;\n";
         if (aligned) extra+="TheView.LookAtFrame = DefaultSC;\nEarth_View.LookAtFrame = DefaultSC;\n";
         originScript.replace("BeginMissionSequence;",extra+"BeginMissionSequence;");
         const auto converted=convertOpenFramesViews(originScript); require(converted.error.isEmpty(),"Automatic origin conversion failed");
         editor->setPlainText(converted.script); auto &files=fixtureDirectory(); const auto path=files.filePath("origin-camera.script");
         require(window.saveScriptTo(path) && checkedLoadScript(path) && window.runMission()==MainWindow::RunResult::Completed,"Automatic origin save/reopen failed");
         const auto model=window.plotReceiver()->model("OFI_EarthView"); auto *selector=window.findChild<QComboBox *>("orbitCameraView");
         require(model && selector && model->automaticTrajectory=="CoordinateSystem" && model->cameraViews[1].automaticTrajectory=="CoordinateSystem", "Origin framing not delivered to primary/named cameras");
         const double radius=aligned ? 1 : static_cast<double>(static_cast<float>(12*6378.1363));
         for (int index:{0,1}) {
            selector->setCurrentIndex(index); const auto camera=orbitCamera(*model,model->frame,0,0,1,.4);
            const double halfVertical=model->fieldOfView*3.14159265358979323846/360;
            const double expected=radius/std::sin(std::atan(.4*std::tan(halfVertical)));
            require(camera.target.length()<1e-8 && std::abs(camera.distance-expected)<1e-6,"Automatic origin camera radius or center wrong");
         }
      }
      {
         auto bodyScript=originalSample;
         bodyScript.replace(QRegularExpression("TheView\\.ViewFrame\\s*=\\s*CoordinateSystem"),"TheView.ViewFrame = Earth");
         bodyScript.replace(QRegularExpression("TheView\\.SetDefaultLocation\\s*=\\s*On"),"TheView.SetDefaultLocation = Off");
         const auto converted=convertOpenFramesViews(bodyScript); require(converted.error.isEmpty(),"Automatic body conversion failed");
         const auto settings=qtCameraSettings(converted.script).value("OFI_EarthView");
         require(settings.automaticBody=="Earth" && settings.views[1].automaticBody=="DefaultSC","Automatic body metadata lost");
         editor->setPlainText(converted.script); auto &files=fixtureDirectory(); const auto path=files.filePath("body-camera.script");
         require(window.saveScriptTo(path) && checkedLoadScript(path) && window.runMission()==MainWindow::RunResult::Completed,"Automatic body save/reopen failed");
         const auto model=window.plotReceiver()->model("OFI_EarthView"); require(model && model->automaticBody=="Earth","Automatic body not delivered to viewer");
         double radius=0; osg::Vec3d center;
         for (const auto &curve:model->curves) if (curve.name=="Earth") {
            radius=curve.radius; const auto &point=curve.points.back(); center.set(point.x,point.y,point.z);
         }
         const auto camera=orbitCamera(*model,model->frame,0,0,1,1);
         require(radius>6000 && (camera.target-center).length()<1e-8 &&
            std::abs(camera.distance-radius/std::sin(model->fieldOfView*3.14159265358979323846/360))<1e-7,"Automatic planet radius/center incorrect");
         const auto before=editor->toPlainText();
         require(window.applyResourceChanges("OFI_EarthView",{{"ViewScaleFactor","2"}},before).isEmpty() &&
            qtCameraSettings(editor->toPlainText()).value("OFI_EarthView").automaticBody.isEmpty(),"Explicit pose did not override automatic body framing");
         editor->undo(); require(editor->toPlainText()==before && window.runMission()==MainWindow::RunResult::Completed,"Automatic body override Undo failed");
         require(window.applyResourceChanges("OFI_EarthView",{{"ViewPointReference","[0 0 0]"}},before).isEmpty() &&
            !qtCameraSettings(editor->toPlainText()).value("OFI_EarthView").bodyRelative,"Manual vector reference retained imported body-relative rotation");
         editor->undo(); require(editor->toPlainText()==before && window.buildScript(),"Body-reference override Undo failed");
      }
      {
         auto bodyScript=originalSample;
         bodyScript.replace(QRegularExpression("TheView\\.ViewFrame\\s*=\\s*CoordinateSystem"),"TheView.ViewFrame = Earth");
         bodyScript.replace(QRegularExpression("TheView\\.DefaultCenter\\s*=\\s*\\[[^\\]]*\\]"),"TheView.DefaultCenter = [100 200 300]");
         bodyScript.replace("BeginMissionSequence;","DefaultSC.Attitude = Spinner;\nDefaultSC.SpinRate = 0.2;\nBeginMissionSequence;");
         const auto converted=convertOpenFramesViews(bodyScript);
         require(converted.error.isEmpty(),"Body-relative fixture conversion failed");
         const auto setting=qtCameraSettings(converted.script).value("OFI_EarthView");
         require(setting.bodyRelative && setting.centerOffset==std::optional<std::array<double,3>>({100,200,300}) && setting.views[1].bodyRelative,
            "Conversion lost relative-camera mode or center offset");
         editor->setPlainText(converted.script);
         require(window.runMission()==MainWindow::RunResult::Completed,"Body-relative camera mission failed");
         auto checkBodyCamera=[&] {
            const auto model=window.plotReceiver()->model("OFI_EarthView");
            auto *fixed=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("EarthFixed"));
            auto *plotFrame=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject(model->coordinates.toStdString()));
            require(fixed && plotFrame && !model->cameras.empty(),"Independent camera coordinate frames missing");
            CoordinateConverter converter;
            for (const auto *camera:{&model->cameras.front(),&model->cameras.back()}) {
               double epoch=0;
               for (const auto &curve:model->curves) if (curve.name=="Earth")
                  for (const auto &point:curve.points) if (point.frame==camera->frame) epoch=point.epoch;
               require(epoch!=0,"Body camera sample epoch missing");
               Rvector6 eye,center,up;
               converter.Convert(epoch,Rvector6(150000,0,0,0,0,0),fixed,eye,plotFrame);
               converter.Convert(epoch,Rvector6(100,200,300,0,0,0),fixed,center,plotFrame);
               converter.Convert(epoch,Rvector6(1,0,1,0,0,0),fixed,up,plotFrame);
               for (int axis=0;axis<3;++axis) require(std::abs(camera->eye[axis]-eye[axis])<1e-7 &&
                  std::abs(camera->target[axis]-center[axis])<1e-8 && std::abs(camera->up[axis]-up[axis])<1e-10,
                  "Body camera differs from independent EarthFixed coordinate conversion");
            }
            if (model->coordinates!="EarthFixed")
               require(std::abs(model->cameras.front().eye[0]-model->cameras.back().eye[0])>100,"Body camera did not rotate with Earth");
         };
         checkBodyCamera(); checkViewHistories();
         require(window.applyResourceChanges("OFI_EarthView",{{"CoordinateSystem","EarthFixed"}},editor->toPlainText()).isEmpty() &&
            window.runMission()==MainWindow::RunResult::Completed,"Body camera in rotating plot frame failed");
         checkBodyCamera(); checkViewHistories();
         require(window.saveScriptTo(savedCamera) && checkedLoadScript(savedCamera) && window.runMission()==MainWindow::RunResult::Completed,
            "Body-relative camera save/reopen failed");
         checkBodyCamera(); checkViewHistories();
         const auto good=editor->toPlainText();
         require(!window.applyResourceChanges("OFI_EarthView",{{"ViewPointReference","[0 0 0]"}},good).isEmpty(),
            "Body-relative mode accepted a reference with no attitude");
         require(editor->toPlainText()==good && window.runMission()==MainWindow::RunResult::Completed,"Invalid body-camera edit lost the mission");
         checkBodyCamera();
      }
      {
         std::array<double,3> azimuthUp{};
         auto checkLookAt=[&] {
            const auto model=window.plotReceiver()->model("OFI_EarthView");
            require(model && !model->cameras.empty(),"Aligned camera history missing");
            for (int view=0;view<3;++view) {
               const auto &history=view==0 ? model->cameras : model->cameraViews[view].cameras;
               require(history.size()==model->cameras.size(),"Aligned secondary history incomplete");
               for (const auto *camera:{&history.front(),&history.back()}) {
                  std::array<double,3> earth{},sat{};
                  for (const auto &curve:model->curves) for (const auto &point:curve.points) if (point.frame==camera->frame) {
                     if (curve.name=="Earth") earth={point.x,point.y,point.z};
                     if (curve.name=="DefaultSC") sat={point.x,point.y,point.z};
                  }
                  const auto origin=view==2 ? sat : earth, target=view==2 ? earth : sat;
                  const double length=std::hypot(target[0]-origin[0],target[1]-origin[1],target[2]-origin[2]);
                  const double distance=view==0 ? 50000 : 30000;
                  double upDot=0;
                  for (int axis=0;axis<3;++axis) {
                     const double direction=(target[axis]-origin[axis])/length;
                     require(std::abs(camera->eye[axis]-origin[axis]+distance*direction)<1e-7 &&
                        std::abs(camera->target[axis]-origin[axis])<1e-8,"Two-frame camera did not rotate the entire stored pose");
                     upDot+=camera->up[axis]*direction;
                  }
                  require(std::abs(upDot)<1e-10,"Aligned camera up is not perpendicular to its direction");
               }
            }
         };
         for (bool shortest:{false,true}) {
            auto lookScript=originalSample;
            lookScript.replace(QRegularExpression("TheView\\.ViewFrame\\s*=\\s*CoordinateSystem"),"TheView.ViewFrame = Earth");
            lookScript.replace(QRegularExpression("TheView\\.DefaultEye\\s*=\\s*\\[[^\\]]*\\]"),"TheView.DefaultEye = [0 -50000 0]");
            lookScript.replace(QRegularExpression("TheView\\.DefaultUp\\s*=\\s*\\[[^\\]]*\\]"),"TheView.DefaultUp = [0 0 1]");
            const auto mode=shortest ? QString("On") : QString("Off");
            lookScript.replace("BeginMissionSequence;",QString("TheView.LookAtFrame = DefaultSC;\nTheView.ShortestAngle = %1;\n"
               "Earth_View.LookAtFrame = DefaultSC;\nEarth_View.ShortestAngle = %1;\n"
               "DefaultSC_View.LookAtFrame = Earth;\nDefaultSC_View.ShortestAngle = %1;\nBeginMissionSequence;").arg(mode));
            const auto converted=convertOpenFramesViews(lookScript);
            require(converted.error.isEmpty(),"Look-at conversion failed");
            const auto setting=qtCameraSettings(converted.script).value("OFI_EarthView");
            require(setting.lookAtRotation && setting.shortestAngle==shortest && setting.views[1].lookAtRotation && setting.views[1].shortestAngle==shortest,
               "Conversion lost two-frame alignment mode");
            editor->setPlainText(converted.script);
            require(window.runMission()==MainWindow::RunResult::Completed,"Two-frame camera mission failed");
            checkLookAt();
            const auto up=window.plotReceiver()->model("OFI_EarthView")->cameras.front().up;
            if (!shortest) azimuthUp=up;
            else require(std::hypot(up[0]-azimuthUp[0],up[1]-azimuthUp[1],up[2]-azimuthUp[2])>1e-4,"ShortestAngle did not change camera roll");
         }
         require(window.saveScriptTo(savedCamera) && checkedLoadScript(savedCamera) && window.runMission()==MainWindow::RunResult::Completed,
            "Two-frame camera save/reopen failed");
         checkLookAt();
         const auto beforeDirection=editor->toPlainText();
         require(window.applyResourceChanges("OFI_EarthView",{{"ViewDirection","Earth"}},beforeDirection).isEmpty(),"Explicit direction edit failed");
         require(!qtCameraSettings(editor->toPlainText()).value("OFI_EarthView").lookAtRotation,"Explicit direction edit retained hidden alignment");
         editor->undo(); require(editor->toPlainText()==beforeDirection && window.runMission()==MainWindow::RunResult::Completed,"Look-at override Undo failed");
         checkLookAt();
      }
      editor->setPlainText("Create OpenFramesVector Vec;\nVec.VectorType = 'Thrust Vector';\nBeginMissionSequence;\n");
      bool manualExplanation=false;
      workflowAfterExposure(0,&window,[&] {
         if (auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
            manualExplanation=prompt->windowTitle()=="OpenFrames views need manual conversion";
            prompt->accept();
         }
      });
      require(!window.buildScript() && manualExplanation,"Unsupported OpenFrames script lacked a specific explanation");
      require(editor->toPlainText().startsWith("Create OpenFramesVector"),"Unsupported conversion changed source");
      require(checkedLoadScript(script) && window.buildScript(),"Normal script no longer builds without conversion");
      std::cout<<"PASS: expression grid Cancel/Apply, formula order, source preservation, Undo/Redo, save/reopen, clear and failed-formula recovery\n";
      std::cout<<"PASS: automatic OpenFrames conversion offer, decline, accept, Run, undo, original-file preservation and unsupported-feature explanation\n";
      const auto capabilities=window.availableEngineTypes();
      require(capabilities.contains("OrbitView") && capabilities.contains("GmatFunction") && !capabilities.contains("OpenFramesInterface"),
         "Runtime engine capability inventory incorrect");
      std::cout<<"PASS: resource deletion, command/resource dependencies, target-panel protection, unrelated panels, undo and variable declarations\n";
      std::cout<<"PASS: propagation form, advanced-command preservation, invalid-input rollback and actual elapsed-days execution\n";
      std::cout<<"PASS: resource creation, name/type validation, pending/stale protection, undo, dialog/tree integration and actual propagation\n";
      std::cout << "PASS: propagation, repeated runs, pause/resume/stop, edit protection, close protection, invalid-script recovery, resource apply/rollback/stale-panel protection/undo\n";
      const QJsonObject workflowProof{{"checkpoint","workflow-visible-boundaries"},{"outcome","passed"},
         {"mode",qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")},{"stages",workflowViewingStages},
         {"stage_count",workflowViewingStages.size()},{"modal_driver_waits_for_exposure",true},
         {"all_original_assertions_preserved",true},{"scope","Existing Workflow only; explicit fixture and modal-driver viewing boundaries. Original runtime, scientific, source/history and result assertions retained. Automatic pre-map Show observations remain raw evidence, not a claim that every change or low-level input is paced."}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(workflowProof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return 0;
   } catch (const std::exception &error) {
      std::cerr << "FAIL: " << error.what() << '\n';
      return 1;
   }
}
