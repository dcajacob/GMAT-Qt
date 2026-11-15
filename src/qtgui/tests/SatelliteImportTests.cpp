#include "MainWindow.hpp"
#include "SatelliteImportData.hpp"
#include "SatelliteImportDialog.hpp"
#include "TLEReader.hpp"
#include "FileManager.hpp"
#include "SpiceInterface.hpp"
#include "Moderator.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "StringUtil.hpp"
#include "QtMessageReceiver.hpp"
#include "MessageInterface.hpp"
#include <QApplication>
#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <stdexcept>
extern "C" {
#include "SpiceUsr.h"
}

static void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
static void write(const QString &path,const QByteArray &data)
{
   QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Cannot write satellite fixture");
}
static QByteArray read(const QString &path)
{
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read satellite result"); return file.readAll();
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("SatelliteImport");
   TestSettings settings;
   try {
      require(argc==2,"Supply a startup file");
      require(GmatStringUtil::NumberOfScientificNotation("1e")==0 &&
              GmatStringUtil::NumberOfScientificNotation("5e1.tle")==0 &&
              GmatStringUtil::NumberOfScientificNotation("1e+7")==1,
              "Scientific-notation boundary parsing is invalid");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      QTemporaryDir scratch; require(scratch.isValid(),"Cannot create satellite test directory");
      qputenv("XDG_DATA_HOME",scratch.filePath("data").toUtf8());
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!evidence.isEmpty()) QDir().mkpath(evidence);
      MainWindow window; require(window.initialize(startup),"Satellite test runtime failed");
      if (auto *messages=dynamic_cast<QtMessageReceiver *>(MessageInterface::GetMessageReceiver()))
         messages->SetMessageCallback(&window,[](const QString &message) { std::cerr<<message.toStdString(); });
      const auto lsk=FileManager::Instance()->GetFullPathname("LSK_FILE");
      SpiceInterface::NotifyKernelPoolChanged(); furnsh_c(lsk.c_str());
      const QByteArray tle="Vanguard\n1 00005U 58002B   00179.78495062  .00000023  00000-0  28098-4 0  4753\n2 00005  34.2682 348.7242 1859667 331.7664  19.3264 10.82419157413667\n";
      QJsonObject fields{{"OBJECT_NAME","Vanguard"},{"NORAD_CAT_ID",5},{"EPOCH","2000-06-27T18:50:19.733568"},
         {"MEAN_MOTION",10.82419157},{"ECCENTRICITY",.1859667},{"INCLINATION",34.2682},
         {"RA_OF_ASC_NODE",348.7242},{"ARG_OF_PERICENTER",331.7664},{"MEAN_ANOMALY",19.3264},
         {"BSTAR",.000028098},{"MEAN_MOTION_DOT",.00000023},{"MEAN_MOTION_DDOT",0}};
      const auto tleData=parseSatelliteElements(tle).front();
      const auto ommData=parseSatelliteElements(QJsonDocument(QJsonArray{fields}).toJson()).front();
      write(scratch.filePath("input.tle"),tleData.elements); write(scratch.filePath("input.omm"),ommData.elements);
      TLEReader tleReader(scratch.filePath("input.tle").toStdString()),ommReader(scratch.filePath("input.omm").toStdString());
      auto nativeTle=tleReader.GetTLEData("00005"),nativeOmm=ommReader.GetTLEData("5");
      tleReader.ParseForSpice(nativeTle); ommReader.ParseForSpice(nativeOmm);
      require(nativeOmm.isOmm && std::abs(nativeTle.secFromJ2k-nativeOmm.secFromJ2k)<1e-5,"OMM epoch differs from the TLE epoch");
      for (int i=0;i<9;++i) require(std::abs(nativeTle.elements[i]-nativeOmm.elements[i])<1e-12,"OMM unit conversion differs from the native TLE reader");
      const double constants[8]={1.082616e-3,-2.53881e-6,-1.65597e-6,7.43669161e-2,120,78,6378.135,1};
      double state[6]; evsgp4_c(nativeOmm.secFromJ2k,constants,nativeOmm.elements,state);
      const double expected[6]={7022.46529266,-1400.08296755,.03995155,1.893841015,6.405893759,4.534807250};
      for (int i=0;i<6;++i) require(std::abs(state[i]-expected[i])<(i<3 ? 1e-5 : 1e-8),"OMM state differs from Vallado's published SGP4 verification vector");
      auto historic=tle; historic.replace("00179.78495062","99365.78495062");
      auto lines=historic.split('\n'); int sum=0;
      for (int i=0;i<68;++i) { const auto ch=lines[1][i]; if (ch>='0' && ch<='9') sum+=ch-'0'; else if (ch=='-') ++sum; }
      lines[1][68]='0'+sum%10; historic=lines.join('\n');
      const auto oldData=parseSatelliteElements(historic).front(); require(oldData.epoch.startsWith("1999-12-31"),"Historical TLE century was not preserved");
      write(scratch.filePath("old.tle"),oldData.elements); TLEReader oldReader(scratch.filePath("old.tle").toStdString());
      auto oldNative=oldReader.GetTLEData("00005"); oldReader.ParseForSpice(oldNative);
      require(oldNative.secFromJ2k<0,"Native TLE propagation shifted a historical epoch into the next century");
      auto bad=tle; bad[bad.indexOf("00179")]='7';
      bool rejected=false; try { parseSatelliteElements(bad); } catch (const std::exception &) { rejected=true; }
      require(rejected,"Corrupt TLE checksum was accepted");
      fields["NORAD_CAT_ID"]=100123;
      const auto largeId=parseSatelliteElements(QJsonDocument(fields).toJson()).front();
      write(scratch.filePath("large.omm"),largeId.elements); TLEReader largeReader(scratch.filePath("large.omm").toStdString());
      require(largeReader.GetTLEData("100123").isOmm,"Six-digit catalog ID was truncated");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString baseline="Create CoordinateSystem ImportICRF;\nImportICRF.Origin = Earth;\nImportICRF.Axes = ICRF;\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nKeep = Keep + 1;\n";
      auto propagate=[&](const SatelliteImportData &data,const QString &tag) {
         editor->setPlainText(baseline); require(window.buildScript(),"Satellite baseline did not build");
         const auto error=window.importSatellite(data,"Imported",baseline,scratch.path());
         QApplication::processEvents();
         if (!error.isEmpty()) std::cerr<<error.toStdString()<<'\n'<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()<<'\n';
         require(error.isEmpty(),"Satellite import failed");
         const auto imported=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==baseline,"Satellite import was not one undoable edit");
         editor->redo(); require(editor->toPlainText()==imported,"Satellite import redo lost its source");
         const auto report=scratch.filePath(tag+".txt");
         const QString declaration="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\n";
         QString tail="Propagate ImportedProp(Imported) {Imported.ElapsedSecs = 60};\nReport Values";
         for (const auto &field:QStringList{"X","Y","Z","VX","VY","VZ"}) tail+=" Imported."+data.coordinateSystem+'.'+field;
         tail+=" Keep;\n";
         editor->setPlainText(declaration+imported+tail);
         require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Imported satellite did not propagate");
         if (!evidence.isEmpty()) { write(evidence+'/'+tag+".txt",read(report)); write(evidence+'/'+tag+".script",editor->toPlainText().toUtf8()); }
         const auto values=QString::fromUtf8(read(report)).trimmed().split(','); require(values.size()==7 && values[6].trimmed()=="42","Imported satellite changed the mission sentinel");
         QVector<double> result; for (int i=0;i<6;++i) { bool valid=false; const auto value=values[i].toDouble(&valid); require(valid && std::isfinite(value),"Imported state is not finite"); result.append(value); }
         return result;
      };
      const auto a=propagate(tleData,"tle"),b=propagate(ommData,"omm"),c=propagate(largeId,"six-digit");
      for (int i=0;i<6;++i) {
         require(std::abs(a[i]-b[i])<1e-5,"TLE and OMM mission propagation disagree");
         require(std::abs(b[i]-c[i])<1e-10,"Six-digit ID changed the propagated orbit");
      }
      QByteArray oem="CCSDS_OEM_VERS = 1.0\nCREATION_DATE = 2000-01-01T12:00:00.000\nORIGINATOR = GMAT\nMETA_START\nOBJECT_NAME = Linear\nOBJECT_ID = Linear\nCENTER_NAME = Earth\nREF_FRAME = EME2000\nTIME_SYSTEM = UTC\nSTART_TIME = 2000-01-01T12:00:00.000\nSTOP_TIME = 2000-01-01T12:10:00.000\nINTERPOLATION = LAGRANGE\nINTERPOLATION_DEGREE = 1\nMETA_STOP\n";
      for (int minute=0;minute<=10;++minute)
         oem+="2000-01-01T12:"+QByteArray::number(minute).rightJustified(2,'0')+":00.000 7000 "+QByteArray::number(minute*450)+" 0 0 7.5 0\n";
      const auto oemPath=scratch.filePath("linear.oem"); write(oemPath,oem);
      SatelliteImportData ephemeris; ephemeris.propagatorType="CCSDS-OEM"; ephemeris.sourceFile=oemPath; ephemeris.epoch="2000-01-01T12:00:00.000Z";
      const auto sampled=propagate(ephemeris,"oem");
      const QVector<double> expectedSample{7000,450,0,0,7.5,0};
      // A1 epochs are stored as doubles. Their sub-microsecond quantization
      // becomes a few millimetres at this fixture's 7.5 km/s velocity.
      for (int i=0;i<6;++i) require(std::abs(sampled[i]-expectedSample[i])<(i<3 ? 1e-5 : 1e-10),"Imported OEM differs from its independent linear-state fixture");
      QByteArray stk="stk.v.10.0\nBEGIN Ephemeris\nNumberOfEphemerisPoints 11\nScenarioEpoch 01 Jan 2000 12:00:00.000\nCentralBody Earth\nCoordinateSystem J2000\nDistanceUnit Kilometers\nEphemerisTimePosVel\n";
      for (int minute=0;minute<=10;++minute) stk+=QByteArray::number(minute*60)+" 7000 "+QByteArray::number(minute*450)+" 0 0 7.5 0\n";
      stk+="END Ephemeris\n";
      const auto stkPath=scratch.filePath("linear.e"); write(stkPath,stk);
      ephemeris.propagatorType="STK"; ephemeris.sourceFile=stkPath;
      const auto stkSample=propagate(ephemeris,"stk");
      for (int i=0;i<6;++i) require(std::abs(stkSample[i]-expectedSample[i])<(i<3 ? 1e-5 : 1e-10),"Imported STK ephemeris differs from the linear-state fixture");
      const auto spkPath=scratch.filePath("linear.bsp");
      double baseEpoch; str2et_c("2000-01-01T12:00:00",&baseEpoch);
      SpiceInt handle; spkopn_c(spkPath.toUtf8().constData(),"IMPORT TEST",0,&handle);
      SpiceDouble states[11][6],epochs[11];
      for (int i=0;i<11;++i) {
         epochs[i]=baseEpoch+i*60;
         states[i][0]=7000; states[i][1]=i*450; states[i][2]=0;
         states[i][3]=0; states[i][4]=7.5; states[i][5]=0;
      }
      spkw09_c(handle,-100123,399,"J2000",epochs[0],epochs[10],"LINEAR IMPORT",1,11,states,epochs); spkcls_c(handle);
      require(!failed_c(),"SPK input fixture could not be created");
      ephemeris.propagatorType="SPK"; ephemeris.sourceFile=spkPath; ephemeris.naifId=-100123;
      // SPICE's J2000 frame follows ICRF. Request that frame explicitly.
      ephemeris.coordinateSystem="ImportICRF";
      const auto spkSample=propagate(ephemeris,"spk");
      for (int i=0;i<6;++i) require(std::abs(spkSample[i]-expectedSample[i])<(i<3 ? 1e-5 : 1e-10),"Imported SPK differs from the linear-state fixture");
      editor->setPlainText(baseline); require(window.buildScript(),"State-vector baseline did not build");
      SatelliteImportData cartesian; cartesian.epoch="2000-01-01T12:00:00.000Z"; cartesian.propagatorType.clear();
      cartesian.state=std::array<double,6>{7000,0,0,0,7.5,1};
      require(window.importSatellite(cartesian,"StateSat",baseline).isEmpty(),"Cartesian state import failed");
      auto *object=Moderator::Instance()->GetConfiguredObject("StateSat"); require(object && object->GetRealParameter("X")==7000,"Imported Cartesian state was not retained");
      std::exception_ptr importFailure;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("satelliteImportDialog");
         try {
            require(dialog,"The Edit-menu import action did not open its dialog");
            dialog->findChild<QTabWidget *>("satelliteImportSource")->setCurrentIndex(1);
            dialog->findChild<QLineEdit *>("satelliteImportFile")->setText(scratch.filePath("input.tle"));
            dialog->findChild<QPushButton *>("readSatelliteFile")->click();
            require(dialog->findChild<QListWidget *>("satelliteFileResults")->count()==1,"File import did not show dated records");
            app.processEvents();
            if (!evidence.isEmpty()) dialog->grab().save(evidence+"/import-dialog.png");
            dialog->findChild<QPushButton *>("importSatellite")->click();
            require(!dialog->isVisible(),"Edit-menu satellite import did not complete");
         } catch (...) { importFailure=std::current_exception(); if (dialog) dialog->reject(); }
      });
      window.findChild<QAction *>("importSatelliteCatalog")->trigger();
      if (importFailure) std::rethrow_exception(importFailure);
      require(Moderator::Instance()->GetConfiguredObject("Vanguard"),"Edit-menu import lost the spacecraft");
      const auto saved=scratch.filePath("imported.script");
      const auto importedSource=editor->toPlainText();
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Imported satellite did not survive save/reopen");
      require(editor->toPlainText()==importedSource && Moderator::Instance()->GetConfiguredObject("Vanguard"),"Save/reopen changed the imported source");
      std::cout<<"PASS TLE/OMM import, six-digit IDs, historical epochs, independent SGP4 state, OEM/STK/SPK inputs, state vectors, undo/redo and file-selection UI\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; }
   return 1;
}
