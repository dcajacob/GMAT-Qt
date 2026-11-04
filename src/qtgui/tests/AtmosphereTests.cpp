#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "AtmosphereDialog.hpp"
#include "Moderator.hpp"
#include "ODEModel.hpp"
#include "PropSetup.hpp"
#include "DragForce.hpp"
#include "Spacecraft.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <array>
#include <exception>
#include <functional>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) { std::cerr<<"CHECK FAILED: "<<message<<'\n'; throw std::runtime_error(message); } }
static DragForce *drag(ODEModel &fm) { for (int i=0;i<fm.GetNumForces();++i) if (auto *result=dynamic_cast<DragForce *>(fm.GetForce(i))) return result; return nullptr; }
// Propagate failed assertions after leaving the Qt event handler.
static void checkLater(QObject *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] {
      try { action(); }
      catch (...) {
         if (!failure) failure=std::current_exception();
         for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject();
         if (auto *dialog=qobject_cast<QDialog *>(owner)) dialog->reject();
      }
   });
}
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read atmosphere report/script"); return QString::fromUtf8(file.readAll()); }
static void set(QDialog &dialog,const QString &name,const QString &value)
{
   if (auto *combo=dialog.findChild<QComboBox *>("atmosphere_"+name)) {
      require(combo->findText(value)>=0,qPrintable("Atmosphere choice missing: "+name+" = "+value)); combo->setCurrentText(value);
   } else { auto *edit=dialog.findChild<QLineEdit *>("atmosphere_"+name); require(edit,"Atmosphere input missing"); edit->setText(value); }
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtAtmosphere");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath();
   const auto capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Atmosphere output directory unavailable");
      const auto cssi=files.filePath("CSSI weather.txt"),schatten=files.filePath("Schatten prediction.txt"),bad=files.filePath("invalid weather.txt");
      require(QFile::copy("../data/atmosphere/earth/SpaceWeather-v1.2.txt",cssi) && QFile::copy("../data/atmosphere/earth/SchattenPredict.txt",schatten),"Weather fixture copy failed");
      { QFile file(bad); require(file.open(QIODevice::WriteOnly),"Invalid weather fixture failed"); file.write("not a space-weather file\n"); }
      const auto report=files.filePath("density.txt"),saved=files.filePath("atmosphere.script");
      MainWindow window; window.show(); require(window.initialize(startup),"Atmosphere runtime initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto fixture=[&](const QString &epoch,const QMap<QString,QString> &properties) {
         QString script="Create Spacecraft DragSat;\nDragSat.DateFormat = UTCGregorian;\nDragSat.Epoch = '"+epoch+"';\n"
            "DragSat.X = 6778.1363;\nDragSat.Y = 0;\nDragSat.Z = 0;\nDragSat.VX = 0;\nDragSat.VY = 7.668;\nDragSat.VZ = 0;\n"
            "Create ForceModel DragFM;\nDragFM.PrimaryBodies = {Earth};\nDragFM.GravityField.Earth.Degree = 0;\nDragFM.GravityField.Earth.Order = 0;\n";
         if (!properties.isEmpty()) {
            script+="DragFM.ErrorControl = LargestStep;\nDragFM.Drag.AtmosphereModel = "+properties.value("AtmosphereModel")+";\n";
            for (auto it=properties.cbegin();it!=properties.cend();++it) if (it.key()!="AtmosphereModel") {
               const bool quoted=it.key().endsWith("File") || it.key()=="CSSISpaceWeatherFile";
               script+="DragFM.Drag."+it.key()+" = "+(quoted ? "'"+it.value()+"'" : it.value())+";\n";
            }
         }
         return script+"Create Propagator DragProp;\nDragProp.FM = DragFM;\nDragProp.Accuracy = 1e-13;\nDragProp.InitialStepSize = 10;\n"
            "Create ReportFile DragReport;\nDragReport.Filename = '"+report+"';\nDragReport.WriteHeaders = false;\nDragReport.FixedWidth = false;\nDragReport.Delimiter = ',';\nDragReport.Precision = 16;\n"
            "BeginMissionSequence;\nReport DragReport DragSat.DragFM.AtmosDensity DragSat.EarthMJ2000Eq.X DragSat.EarthMJ2000Eq.Y DragSat.EarthMJ2000Eq.Z DragSat.EarthMJ2000Eq.VX DragSat.EarthMJ2000Eq.VY DragSat.EarthMJ2000Eq.VZ;\n"
            "Propagate DragProp(DragSat) {DragSat.ElapsedSecs = 600, StopTolerance = 1e-10}; % retain atmosphere mission\n"
            "Report DragReport DragSat.DragFM.AtmosDensity DragSat.EarthMJ2000Eq.X DragSat.EarthMJ2000Eq.Y DragSat.EarthMJ2000Eq.Z DragSat.EarthMJ2000Eq.VX DragSat.EarthMJ2000Eq.VY DragSat.EarthMJ2000Eq.VZ;\n";
      };
      auto results=[&] {
         const auto rows=read(report).trimmed().split('\n'); require(rows.size()==2,"Atmosphere report row count incorrect");
         std::array<std::array<double,7>,2> data;
         for (int row=0;row<2;++row) {
            const auto values=rows[row].split(','); require(values.size()==7,"Atmosphere report fields missing");
            for (int i=0;i<7;++i) { bool valid=false; data[row][i]=values[i].toDouble(&valid); require(valid && std::isfinite(data[row][i]),"Atmosphere report not finite"); }
            // AtmosDensity reports kg/km^3; the drag model itself uses kg/m^3.
            require(data[row][0]>0 && data[row][0]<1,"Atmosphere density outside plausible range");
         }
         return data;
      };
      auto configure=[&](const QMap<QString,QString> &properties,bool exercise) {
         const auto source=editor->toPlainText(); QString error="Apply not invoked"; std::exception_ptr callbackFailure;
         QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("DragFM"),[&](const auto &changes) {
            error=window.applyResourceChanges("DragFM",changes,source); return error;
         },&owner,source);
         auto *button=panel.findChild<QPushButton *>("forceAtmosphere"); require(button,"Force-model atmosphere editor missing");
         if (exercise) {
            checkLater(&panel,callbackFailure,[&] { auto *dialog=panel.findChild<QDialog *>("atmosphereDialog"); set(*dialog,"AtmosphereModel","NRLMSISE00"); set(*dialog,"F107","999"); dialog->reject(); }); button->click(); if (callbackFailure) std::rethrow_exception(callbackFailure);
            require(!panel.hasChanges() && editor->toPlainText()==source,"Cancelled atmosphere edit changed mission");
         }
         checkLater(&panel,callbackFailure,[&] {
            auto *dialog=panel.findChild<QDialog *>("atmosphereDialog"); require(dialog,"Atmosphere dialog missing");
            if (exercise) {
               const auto size=dialog->size(); dialog->resize(600,440); app.processEvents();
               auto *scroll=dialog->findChild<QScrollArea *>("atmosphereScroll"); require(scroll && scroll->verticalScrollBar()->maximum()>0,"Narrow atmosphere dialog does not scroll");
               auto *buttons=dialog->findChild<QDialogButtonBox *>(); require(buttons->geometry().bottom()<dialog->height(),"Narrow atmosphere dialog hides acceptance buttons");
               dialog->resize(size);
            }
            set(*dialog,"AtmosphereModel",properties.value("AtmosphereModel"));
            auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
            if (exercise) {
               set(*dialog,"MagneticIndex","10"); ok->click();
               require(dialog->isVisible() && !dialog->findChild<QLabel *>("atmosphereError")->text().isEmpty(),"Out-of-range geomagnetic index accepted");
               set(*dialog,"MagneticIndex","3"); set(*dialog,"F107","nan"); ok->click(); require(dialog->isVisible(),"Nonfinite solar flux accepted"); set(*dialog,"F107","160");
               set(*dialog,"HistoricWeatherSource","CSSISpaceWeatherFile"); set(*dialog,"CSSISpaceWeatherFile",bad); ok->click(); require(dialog->isVisible(),"Invalid CSSI file accepted");
               set(*dialog,"CSSISpaceWeatherFile",files.filePath("missing.txt")); ok->click(); require(dialog->isVisible(),"Missing CSSI file accepted");
               set(*dialog,"CSSISpaceWeatherFile",files.path()); ok->click(); require(dialog->isVisible(),"Directory accepted as a weather file");
               set(*dialog,"CSSISpaceWeatherFile",files.filePath("missing.txt"));
               checkLater(dialog,callbackFailure,[&] { dialog->findChild<QFileDialog *>("atmosphereFileDialog")->reject(); }); dialog->findChild<QPushButton *>("atmosphereBrowse_CSSISpaceWeatherFile")->click();
               require(dialog->findChild<QLineEdit *>("atmosphere_CSSISpaceWeatherFile")->text().endsWith("missing.txt"),"Cancelled weather picker changed input");
               checkLater(dialog,callbackFailure,[&] { auto *picker=dialog->findChild<QFileDialog *>("atmosphereFileDialog"); require(picker->fileMode()==QFileDialog::ExistingFile,"Weather picker permits nonexistent inputs"); picker->selectFile(cssi); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); });
               dialog->findChild<QPushButton *>("atmosphereBrowse_CSSISpaceWeatherFile")->click();
               require(dialog->findChild<QLineEdit *>("atmosphere_CSSISpaceWeatherFile")->text()==cssi,"Weather picker did not retain selected path");
               set(*dialog,"HistoricWeatherSource","ConstantFluxAndGeoMag"); require(dialog->findChild<QLineEdit *>("atmosphere_CSSISpaceWeatherFile")->isEnabled(),"Required weather input unavailable in constant mode");
               set(*dialog,"PredictedWeatherSource","SchattenFile"); require(dialog->findChild<QComboBox *>("atmosphere_SchattenTimingModel")->isEnabled(),"Schatten controls not enabled");
               set(*dialog,"SchattenFile",cssi); ok->click(); require(dialog->isVisible(),"CSSI file accepted for Schatten prediction");
               set(*dialog,"SchattenFile",schatten);
               set(*dialog,"PredictedWeatherSource","CSSISpaceWeatherFile"); set(*dialog,"CSSISpaceWeatherFile",schatten); ok->click(); require(dialog->isVisible(),"Schatten file accepted for CSSI prediction");
               set(*dialog,"PredictedWeatherSource","ConstantFluxAndGeoMag");
               ok->click(); require(dialog->isVisible(),"Invalid retained weather file accepted in constant mode");
               set(*dialog,"CSSISpaceWeatherFile",cssi);
               set(*dialog,"AtmosphereModel","Exponential"); require(!dialog->findChild<QLineEdit *>("atmosphere_F107")->isEnabled() && dialog->findChild<QLineEdit *>("atmosphere_InputFile")->isEnabled(),"Exponential dependent controls incorrect");
               set(*dialog,"InputFile",files.filePath("missing atmosphere.txt")); ok->click(); require(dialog->isVisible(),"Missing exponential input accepted");
               set(*dialog,"InputFile",QFileInfo("../data/atmosphere/earth/EarthExponentialAtmosphereData.txt").absoluteFilePath());
               set(*dialog,"AtmosphereModel","None"); require(!dialog->findChild<QComboBox *>("atmosphere_DragModel")->isEnabled(),"None leaves drag controls active");
               set(*dialog,"AtmosphereModel","NRLMSISE00"); require(dialog->findChild<QLineEdit *>("atmosphere_F107")->text()=="160","Switching models lost pending flux");
            }
            for (auto it=properties.cbegin();it!=properties.cend();++it) set(*dialog,it.key(),it.value());
            if (!capture.isEmpty() && exercise) { app.processEvents(); require(dialog->grab().save(capture),"Atmosphere screenshot save failed"); }
            ok->click(); require(dialog->result()==QDialog::Accepted,"Atmosphere settings did not validate");
         }); button->click(); if (callbackFailure) std::rethrow_exception(callbackFailure);
         require(panel.hasChanges() && editor->toPlainText()==source,"Atmosphere OK bypassed pending Apply");
         checkLater(&panel,callbackFailure,[&] {
            auto *dialog=panel.findChild<QDialog *>("atmosphereDialog"); require(dialog->findChild<QComboBox *>("atmosphere_AtmosphereModel")->currentText()==properties.value("AtmosphereModel"),"Pending atmosphere selection lost on reopen"); dialog->reject();
         }); button->click(); if (callbackFailure) std::rethrow_exception(callbackFailure);
         // Check that unrelated pending force settings participate in the same Apply.
         auto *table=panel.findChild<QTableWidget *>();
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ErrorControl") {
            if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1))) combo->setCurrentText("LargestStep");
            else table->item(row,1)->setText("LargestStep");
         }
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         const auto applied=editor->toPlainText();
         require(Moderator::Instance()->GetConfiguredObject("DragFM")->GetStringParameter("ErrorControl")=="LargestStep","Paired force-model field did not apply");
         require(applied.mid(applied.indexOf("BeginMissionSequence;"))==source.mid(source.indexOf("BeginMissionSequence;")),"Atmosphere Apply changed mission source");
         editor->undo(); require(editor->toPlainText()==source && window.buildScript(),"Atmosphere Undo not atomic");
         editor->redo(); require(editor->toPlainText()==applied && window.buildScript(),"Atmosphere Redo changed settings");
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && read(saved)==applied,"Atmosphere save/reopen changed source");
      };
      double lowDensity=0;
      for (const auto &model:QStringList{"MSISE90","JacchiaRoberts","NRLMSISE00"}) {
         QMap<QString,QString> parameters={{"AtmosphereModel",model},{"F107","160"},{"F107A","155"},{"MagneticIndex","3"},{"HistoricWeatherSource","ConstantFluxAndGeoMag"},{"PredictedWeatherSource","ConstantFluxAndGeoMag"},{"DragModel","Spherical"}};
         editor->setPlainText(fixture("01 Jan 2024 12:00:00.000",parameters)); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Reference atmosphere mission failed");
         const auto expected=results();
         editor->setPlainText(fixture("01 Jan 2024 12:00:00.000",{})); require(window.buildScript(),"No-drag fixture failed");
         configure(parameters,model=="NRLMSISE00");
         require(Moderator::Instance()->GetConfiguredObject("DragSat.DragFM.AtmosDensity")->GetStringParameter("Unit")=="kg/km^3","AtmosDensity units do not match the returned value");
         require(window.runMission()==MainWindow::RunResult::Completed,"GUI atmosphere mission failed"); const auto actual=results();
         for (int row=0;row<2;++row) for (int i=0;i<7;++i) require(std::abs(actual[row][i]-expected[row][i])<(i==0 ? std::max(expected[row][0]*1e-8,1e-30) : i<4 ? 1e-6 : 1e-9),"GUI atmosphere changed density/trajectory calculations");
         auto *runtimeProp=dynamic_cast<PropSetup *>(Moderator::Instance()->GetInternalObject("DragProp"));
         auto *appliedModel=runtimeProp ? runtimeProp->GetODEModel() : nullptr;
         auto *appliedDrag=appliedModel ? drag(*appliedModel) : nullptr;
         require(appliedDrag && appliedDrag->GetStringParameter("AtmosphereModel")==model.toStdString(),"GUI atmosphere model differs from selected model");
         for (const auto &field:QStringList{"F107","F107A","MagneticIndex"})
            require(appliedDrag->GetRealParameter(field.toStdString())==parameters.value(field).toDouble(),"GUI atmosphere parameter readback differs");
         const QJsonObject nasaProof{{"checkpoint","atmosphere."+model},{"model",model},{"F107",160.0},{"F107A",155.0},{"MagneticIndex",3.0},{"outcome","passed"},
            {"facets",QJsonArray{"gui-settings","runtime-model","runtime-parameters","pending-isolation","undo-redo","save-reopen","script-reference-report"}}};
         std::cout<<"NASA_CHECK "<<QJsonDocument(nasaProof).toJson(QJsonDocument::Compact).constData()<<std::endl;
         if (model=="NRLMSISE00") lowDensity=actual[1][0];
         std::cout<<"PASS: "<<model.toStdString()<<" GUI configuration, Undo/Redo, save/reopen, density and 600-second trajectory; density="<<actual[0][0]<<'\n';
      }
      QMap<QString,QString> high={{"AtmosphereModel","NRLMSISE00"},{"F107","220"},{"F107A","210"},{"MagneticIndex","6"}};
      configure(high,false); require(window.runMission()==MainWindow::RunResult::Completed,"High-flux mission failed"); require(results()[1][0]>lowDensity*1.2,"Flux changes did not affect atmosphere density");
      for (const auto &predicted:QStringList{"SchattenFile","CSSISpaceWeatherFile"}) {
         QMap<QString,QString> parameters={{"AtmosphereModel","NRLMSISE00"},{"HistoricWeatherSource","CSSISpaceWeatherFile"},{"PredictedWeatherSource",predicted},{"CSSISpaceWeatherFile",cssi},{"SchattenFile",schatten},{"SchattenErrorModel","PlusTwoSigma"},{"SchattenTimingModel","LateCycle"}};
         for (const auto &epoch:QStringList{"01 Jan 2024 12:00:00.000","01 Jan 2027 12:00:00.000"}) {
            editor->setPlainText(fixture(epoch,parameters)); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Reference weather-file mission failed"); const auto expected=results();
            editor->setPlainText(fixture(epoch,{})); require(window.buildScript(),"Weather no-drag fixture failed"); configure(parameters,false);
            require(window.runMission()==MainWindow::RunResult::Completed,"GUI weather-file propagation failed"); const auto actual=results();
            for (int row=0;row<2;++row) for (int i=0;i<7;++i) require(std::abs(actual[row][i]-expected[row][i])<(i==0 ? std::max(expected[row][0]*1e-8,1e-30) : i<4 ? 1e-6 : 1e-9),"Weather GUI changed calculations");
         }
         std::cout<<"PASS: historic CSSI and "<<predicted.toStdString()<<" prediction GUI/configuration/save/reopen/report execution\n";
      }
      const auto valid=editor->toPlainText(); require(QFile::rename(cssi,cssi+".hidden"),"Weather fixture hide failed");
      require(window.runMission()!=MainWindow::RunResult::Completed,"Missing active weather file unexpectedly executed");
      require(QFile::rename(cssi+".hidden",cssi) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Weather-file runtime recovery failed");
      require(editor->toPlainText()==valid,"Runtime recovery changed script");
      const QMap<QString,QString> exponential={{"AtmosphereModel","Exponential"},{"InputFile",QFileInfo("../data/atmosphere/earth/EarthExponentialAtmosphereData.txt").absoluteFilePath()}};
      editor->setPlainText(fixture("01 Jan 2024 12:00:00.000",exponential)); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Exponential reference mission failed"); const auto expReference=results();
      editor->setPlainText(fixture("01 Jan 2024 12:00:00.000",high)); require(window.buildScript(),"Exponential switch fixture failed"); configure(exponential,false);
      require(window.runMission()==MainWindow::RunResult::Completed,"Replacing NRLMSISE00 with Exponential failed"); const auto expActual=results();
      for (int row=0;row<2;++row) for (int i=0;i<7;++i) require(std::abs(expActual[row][i]-expReference[row][i])<(i==0 ? expReference[row][0]*1e-8 : i<4 ? 1e-6 : 1e-9),"Exponential model switch changed calculations");
      configure(high,false); require(window.runMission()==MainWindow::RunResult::Completed && results()[1][0]>0,"Replacing Exponential with NRLMSISE00 failed"); const auto spherical=results();
      const auto spadPath=files.filePath("SPAD drag.spo"); require(QFile::copy("../data/vehicle/spad/SphericalModel.spo",spadPath),"SPAD fixture copy failed");
      for (const auto &interpolation:QStringList{"Bilinear","Bicubic"}) {
         auto parameters=high; parameters.insert("DragModel","SPADFile");
         auto reference=fixture("01 Jan 2024 12:00:00.000",parameters);
         reference.replace("Create ForceModel DragFM;","DragSat.SPADDragFile = '"+spadPath+"';\nDragSat.SPADDragScaleFactor = 1.5;\nDragSat.SPADDragInterpolationMethod = "+interpolation+";\nCreate ForceModel DragFM;");
         editor->setPlainText(reference); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"SPAD drag reference mission failed"); const auto expected=results();
         editor->setPlainText(fixture("01 Jan 2024 12:00:00.000",{})); require(window.buildScript(),"SPAD no-drag fixture failed");
         const auto source=editor->toPlainText(); QString error="Apply not invoked"; std::exception_ptr failure;
         {
            QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("DragSat"),[&](const auto &changes) {
               error=window.applyResourceChanges("DragSat",changes,source); return error;
            },&owner,source);
            checkLater(&panel,failure,[&] {
               auto *dialog=panel.findChild<QDialog *>("ballisticsMassDialog"); require(dialog,"SPAD ballistics editor missing");
               dialog->findChild<QLineEdit *>("ballistics_SPADDragFile")->setText(spadPath);
               dialog->findChild<QLineEdit *>("ballistics_SPADDragScaleFactor")->setText("1.5");
               auto *combo=dialog->findChild<QComboBox *>("ballistics_SPADDragInterpolationMethod"); require(combo->findText(interpolation)>=0,"SPAD interpolation choice missing"); combo->setCurrentText(interpolation);
               dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); require(dialog->result()==QDialog::Accepted,"SPAD ballistics selection rejected");
            }); panel.findChild<QPushButton *>("spacecraftBallisticsMass")->click(); if (failure) std::rethrow_exception(failure);
            require(editor->toPlainText()==source && panel.hasChanges(),"SPAD ballistics selection bypassed pending Apply");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         configure(parameters,false); require(window.runMission()==MainWindow::RunResult::Completed,"GUI-configured SPAD drag failed"); const auto actual=results();
         require(std::abs(actual[1][1]-spherical[1][1])>1e-6 || std::abs(actual[1][2]-spherical[1][2])>1e-6,"SPAD drag selection did not affect trajectory");
         for (int row=0;row<2;++row) for (int i=0;i<7;++i) require(std::abs(actual[row][i]-expected[row][i])<(i==0 ? expected[row][0]*1e-8 : i<4 ? 1e-6 : 1e-9),"SPAD drag GUI changed trajectory or density");
         std::cout<<"PASS: GUI-configured SPAD drag, "<<interpolation.toStdString()<<" interpolation, scale 1.5, save/reopen and trajectory report\n";
      }
      const auto spadSource=editor->toPlainText(); require(QFile::rename(spadPath,spadPath+".hidden"),"SPAD fixture hide failed");
      require(window.runMission()!=MainWindow::RunResult::Completed,"Missing SPAD drag file unexpectedly executed");
      require(QFile::rename(spadPath+".hidden",spadPath) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed && editor->toPlainText()==spadSource,"SPAD drag runtime file recovery failed");
      std::cout<<"PASS: SPAD file runtime failure and unchanged-script reopen/recovery\n";
      const auto stable=editor->toPlainText();
      require(!window.applyResourceChanges("DragFM",{{"Drag.F107","-1"}},stable).isEmpty() && editor->toPlainText()==stable,"Invalid generic drag edit bypassed rollback");
      bool fileMetadata=false,choiceMetadata=false;
      for (const auto &property:resourceProperties(*Moderator::Instance()->GetConfiguredObject("DragFM"))) {
         if (property.name=="Drag.CSSISpaceWeatherFile") fileMetadata=property.filename && property.fileInput && !property.fileOutput;
         if (property.name=="Drag.PredictedWeatherSource") choiceMetadata=property.choices.contains("CSSISpaceWeatherFile") && property.choices.contains("SchattenFile");
      }
      require(fileMetadata && choiceMetadata,"Generic drag weather fields lack typed choices/input-file metadata");
      configure({{"AtmosphereModel","None"}},false);
      auto *fm=dynamic_cast<ODEModel *>(Moderator::Instance()->GetConfiguredObject("DragFM")); require(fm && !drag(*fm) && fm->GetStringParameter("Drag")=="None","None did not remove owned drag force");
      require(window.runMission()==MainWindow::RunResult::Completed,"Removed-drag mission failed");
      std::cout<<"PASS: validation/chooser Cancel and acceptance, pending-mode retention, flux effect, missing-file runtime recovery and drag removal\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<"Atmosphere check failed: "<<error.what()<<'\n'; return 1; }
}
