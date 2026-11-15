#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "EphemWriterCCSDS.hpp"
#include "EphemerisFile.hpp"
#include "CCSDSOEMReader.hpp"
#include "CCSDSOEMSegment.hpp"
#include "CCSDSEphemerisFile.hpp"
#include "Interpolator.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
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
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <memory>
#include <limits>
#include <stdexcept>

static void require(bool value, const char *message)
{
   if (!value) throw std::runtime_error(message);
}
static void events()
{
   QApplication::processEvents();
   QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}
static QByteArray read(const QString &path)
{
   QFile file(path);
   require(file.open(QIODevice::ReadOnly), "Cannot read Hermite witness");
   return file.readAll();
}
static void write(const QString &path, const QByteArray &data)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(data) == data.size(), "Cannot save Hermite witness");
}

class WriterProbe : public EphemWriterCCSDS
{
public:
   WriterProbe(const std::string &method, int degree) : EphemWriterCCSDS("Probe")
   {
      interpolatorName = method;
      interpolationOrder = degree;
      CreateInterpolator();
   }
   Interpolator &samples() { return *interpolator; }
   int supported(int count) { return GetSupportedInterpolationOrder(count); }
};

static void nativeChecks()
{
   WriterProbe writer("Hermite", 7);
   auto &samples = writer.samples();
   samples.SetForceInterpolation(true);
   const double anchor = 1861488032.;
   // A quintic and its analytic derivative exercise all position constraints.
   auto polynomial = [](double t, double *state) {
      state[0] = 7000 + .02*t*t - .000003*std::pow(t,5);
      state[1] = 7*t + .00002*std::pow(t,4);
      state[2] = .001*t*t*t;
      state[3] = .04*t - .000015*std::pow(t,4);
      state[4] = 7 + .00008*t*t*t;
      state[5] = .003*t*t;
   };
   for (int i = 0; i < 100; ++i) {
      double state[34] = {};
      polynomial(i, state);
      state[6] = 2*i+1;
      samples.AddPoint(anchor+i, state);
   }
   double actual[34] = {}, expected[6];
   polynomial(98.25, expected);
   require(samples.Interpolate(anchor+98.25, actual), "Rolling Hermite interpolation failed");
   for (int i = 0; i < 6; ++i)
      require(std::abs(actual[i]-expected[i]) < 2e-7, "Hermite differs from analytic quintic or derivative");
   require(std::abs(actual[6]-197.5) < 1e-10, "Ancillary interpolation changed");
   std::unique_ptr<Interpolator> copy(samples.Clone());
   require(copy->Interpolate(anchor+98.25, actual), "Hermite clone lost its sample buffer");
   for (int i = 0; i < 6; ++i)
      require(std::abs(actual[i]-expected[i]) < 2e-7, "Hermite clone changed results");
   samples.Clear();
   require(samples.IsInterpolationFeasible(anchor+1) == -1, "Hermite reset retained old points");
   for (int i = 0; i < 2; ++i) {
      double state[34] = {};
      state[0] = i*i*i;
      state[3] = 3*i*i;
      samples.AddPoint(anchor+i, state);
      samples.AddPoint(anchor+i, state);
   }
   require(samples.GetPointCount()==2, "Duplicate epochs increased Hermite support");
   double invalid[34] = {};
   invalid[3] = std::numeric_limits<double>::quiet_NaN();
   bool rejected = false;
   try { samples.AddPoint(anchor+2, invalid); } catch (BaseException &) { rejected = true; }
   require(rejected && samples.GetPointCount()==2, "Invalid Hermite input changed the sample buffer");
   require(samples.AdjustOrderToNumPoints(), "Short Hermite stencil could not reduce order");
   require(samples.Interpolate(anchor+.5, actual), "Reduced Hermite interpolation failed");
   require(std::abs(actual[0]-.125)<1e-12 && std::abs(actual[3]-.75)<1e-12,
         "Reduced cubic did not use endpoint velocities");
   require(writer.supported(2)==3 && writer.supported(3)==5 && writer.supported(4)==7,
         "Hermite metadata overstates sample support");
   for (int degree : {3, 5, 7, 9}) {
      for (int direction : {1, -1}) {
         WriterProbe polynomialWriter("Hermite", degree);
         auto &interpolation = polynomialWriter.samples();
         interpolation.SetForceInterpolation(true);
         const int count = (degree+1)/2;
         for (int i=0; i<count; ++i) {
            const double t=direction*i;
            double state[34] = {};
            state[0]=std::pow(t,degree);
            state[3]=degree*std::pow(t,degree-1);
            interpolation.AddPoint(anchor+t,state);
         }
         const double query=direction*.5;
         require(interpolation.Interpolate(anchor+query,actual), "Hermite degree or direction failed");
         require(std::abs(actual[0]-std::pow(query,degree))<1e-10 &&
               std::abs(actual[3]-degree*std::pow(query,degree-1))<1e-10,
               "Hermite degree differs from analytic polynomial and derivative");
      }
   }
   WriterProbe lagrange("Lagrange", 7);
   require(lagrange.supported(2)==1 && lagrange.supported(3)==2, "Lagrange degree policy changed");
}

int main(int argc, char **argv)
{
   QApplication app(argc, argv);
   QLocale::setDefault(QLocale::c());
   app.setOrganizationName("GMATTests");
   app.setApplicationName("OemHermite");
   try {
      require(argc==4 && acceptanceWidgetPlatform(), "Expected startup, original fixture and case");
      nativeChecks();
      const QString startup=QFileInfo(argv[1]).absoluteFilePath(), key=argv[3];
      const auto catalog=QJsonDocument::fromJson(read(argv[2])).object();
      QJsonObject spec;
      for (const auto &item:catalog["cases"].toArray())
         if (item.toObject()["key"]==key) spec=item.toObject();
      require(!spec.isEmpty(), "Original numerical fixture missing");
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      TestSettings settings;
      QTemporaryDir temporary;
      const QString output=temporary.filePath("Hermite Δ.oem"), saved=temporary.filePath("Hermite Δ.script");
      MainWindow window;
      require(window.initialize(startup), "Hermite GUI initialization failed");
      window.show(); events();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");
      auto *area=window.findChild<QMdiArea *>("workspace");
      require(source && messages && area, "Hermite workspace missing");
      const QString initial=QString(
         "Create Spacecraft Sat;\nSat.Id = '%1';\nSat.DateFormat = UTCGregorian;\n"
         "Sat.Epoch = '01 Jan 2000 12:00:00.000';\nSat.CoordinateSystem = EarthMJ2000Eq;\n"
         "Sat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\n"
         "Sat.VX = 0;\nSat.VY = 7.546053287267836;\nSat.VZ = 0;\n"
         "Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\n"
         "Create Propagator PropA;\nPropA.FM = FM;\nPropA.Type = RungeKutta89;\n"
         "PropA.InitialStepSize = 5;\nPropA.MinStep = 5;\nPropA.MaxStep = 5;\nPropA.Accuracy = 1e-12;\n"
         "Create Propagator PropB;\nPropB.FM = FM;\nPropB.Type = PrinceDormand78;\n"
         "PropB.InitialStepSize = 5;\nPropB.MinStep = 5;\nPropB.MaxStep = 5;\nPropB.Accuracy = 1e-12;\n"
         "Create EphemerisFile Export;\nExport.Spacecraft = Sat;\nExport.FileFormat = CCSDS-OEM;\n"
         "Export.FormatVersion = '1.0';\nExport.Filename = '%2';\nExport.CoordinateSystem = EarthMJ2000Eq;\n"
         "Export.StepSize = IntegratorSteps;\nExport.InterpolationOrder = 7;\n"
         "Export.EpochFormat = UTCGregorian;\nExport.InitialEpoch = '%3';\nExport.FinalEpoch = '%4';\n"
         "BeginMissionSequence;\n%5").arg(catalog["object_id"].toString(), output,
               spec["initial_epoch"].toString(), spec["final_epoch"].toString(), spec["mission"].toString());
      source->setPlainText(initial);
      require(window.buildScript(), "Initial Lagrange mission failed to build");
      auto *tree=window.findChild<QTreeWidget *>("Resources");
      for (auto *tabs:window.findChildren<QTabWidget *>())
         for (int i=0; i<tabs->count(); ++i)
            if (tabs->widget(i)==tree || tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
      const auto found=tree->findItems("Export",Qt::MatchExactly|Qt::MatchRecursive);
      require(found.size()==1, "Ephemeris resource missing");
      tree->scrollToItem(found.first()); events();
      QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(found.first()).center()); events();
      QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(found.first()).center()); events();
      auto *child=area->currentSubWindow();
      auto *panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr;
      auto *dialog=panel ? panel->findChild<QDialog *>("ephemerisDialog") : nullptr;
      auto *method=dialog ? dialog->findChild<QComboBox *>("ephemeris_Interpolator") : nullptr;
      auto *degree=dialog ? dialog->findChild<QLineEdit *>("ephemeris_InterpolationOrder") : nullptr;
      require(method && degree && method->currentText()=="Lagrange" && method->findText("Hermite")>=0,
            "OEM editor must default to Lagrange and offer Hermite");
      method->setCurrentText("Hermite");
      dialog->findChild<QComboBox *>("ephemeris_StepSize")->setCurrentText(spec["sampling"].toString());
      degree->setText("4"); events();
      auto apply=[&] {
         auto *button=panel->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Apply);
         require(button && button->isEnabled(), "Hermite Apply unavailable");
         QTest::mouseClick(button,Qt::LeftButton); events();
      };
      apply();
      require(source->toPlainText()==initial && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty(),
            "Invalid Hermite degree mutated mission or lost diagnostic");
      degree->setText("1"); events(); apply();
      require(source->toPlainText()==initial, "Unsupported Hermite degree one changed mission");
      degree->setText("7");
      auto *format=dialog->findChild<QComboBox *>("ephemeris_FileFormat");
      format->setCurrentText("SPK"); events();
      require(!method->isEnabled() && method->currentText()=="Hermite", "SPK method changed");
      format->setCurrentText("CCSDS-OEM"); events();
      require(method->isEnabled() && method->currentText()=="Hermite", "OEM format switch lost Hermite");
      apply();
      const QString applied=source->toPlainText();
      require(applied.contains("Export.Interpolator = Hermite") && applied!=initial, "Hermite Apply did not update script");
      child->close(); events();
      source->undo(); require(source->toPlainText()==initial && window.buildScript(), "Hermite Undo failed");
      source->redo(); require(source->toPlainText()==applied && window.buildScript(), "Hermite Redo failed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(), "Hermite save/reopen failed");
      require(Moderator::Instance()->GetConfiguredObject("Export")->GetStringParameter("Interpolator")=="Hermite",
            "Reopened mission lost Hermite selection");
      const auto result=window.runMission(); events();
      if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();
      require(result==MainWindow::RunResult::Completed, "Hermite propagation failed");
      const QByteArray payload=read(output);
      const QString evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!evidence.isEmpty()) {
         write(QDir(evidence).filePath("mission.script"),source->toPlainText().toUtf8());
         write(QDir(evidence).filePath("mission.oem"),payload);
         write(QDir(evidence).filePath("messages.txt"),messages->toPlainText().toUtf8());
      }
      CCSDSOEMReader reader;
      reader.SetFile(output.toStdString());
      const auto expected=spec["expected_segments"].toArray();
      require(reader.GetNumberOfSegments()==expected.size(), "Hermite segment count changed");
      double maxPosition=0, maxVelocity=0;
      QJsonArray degrees;
      for (int segment=0; segment<expected.size(); ++segment) {
         auto *data=reader.GetSegment(segment);
         require(data->GetStringMetaData("INTERPOLATION")=="HERMITE", "Hermite metadata missing");
         const auto wanted=expected[segment].toArray();
         const int actualDegree=data->GetIntegerMetaData("INTERPOLATION_DEGREE");
         require(actualDegree==std::min(7,2*static_cast<int>(wanted.size())-1), "Hermite degree exceeds serialized support");
         degrees.append(actualDegree);
         require(data->GetNumberOfDataPoints()==wanted.size(), "Hermite output cadence changed");
         for (int row=0; row<wanted.size(); ++row) {
            GmatTime epoch; Rvector values;
            require(data->GetEpochAndData(row,epoch,values), "Hermite OEM data missing");
            const auto reference=wanted[row].toArray();
            for (int component=0; component<6; ++component) {
               const double error=std::abs(values[component]-reference[component+1].toDouble());
               if (error>=spec[component<3 ? "position_tolerance" : "velocity_tolerance"].toDouble())
                  std::cerr<<"row "<<row<<" component "<<component<<" error "<<error<<std::endl;
               require(error<spec[component<3 ? "position_tolerance" : "velocity_tolerance"].toDouble(),
                     "Hermite exceeds original independent orbit accuracy bound");
               if (component<3) maxPosition=std::max(maxPosition,error);
               else maxVelocity=std::max(maxVelocity,error);
            }
         }
      }
      // Check both readers against an independent confluent divided-difference
      // reference at an off-grid epoch. Export accuracy is checked above against
      // the protected circular-orbit fixture without changing its tolerances.

      CCSDSEphemerisFile ephemeris;
      require(ephemeris.OpenForRead(output.toStdString(), "TimePosVel"), "Hermite ephemeris open failed");
      require(ephemeris.ReadDataRecords(), "Hermite ephemeris read failed");
      for (int segment=0; segment<expected.size(); ++segment) {
         const auto &points=ephemeris.theEphem[segment].points;
         if (points.size()<2) continue;
         GmatTime middle=points.front().GetEpoch();
         middle.AddSeconds((points[1].GetEpoch()-points.front().GetEpoch()).GetTimeInSec()*.5);
         const auto state=ephemeris.InterpolatePoint(middle);
         const auto direct=dynamic_cast<CCSDSOEMSegment *>(reader.GetSegment(segment))->GetState(middle.GetMjd());
         const int count=std::min(static_cast<int>(points.size()),
               (reader.GetSegment(segment)->GetIntegerMetaData("INTERPOLATION_DEGREE")+1)/2);
         const GmatTime anchor=points.front().GetEpoch();
         const long double query=(middle-anchor).GetTimeInSec();
         for (int component=0; component<3; ++component) {
            // Independent confluent divided differences with repeated nodes.
            std::vector<long double> nodes(2*count);
            std::vector<std::vector<long double>> table(2*count,std::vector<long double>(2*count));
            for (int i=0; i<count; ++i) {
               nodes[2*i]=nodes[2*i+1]=(points[i].GetEpoch()-anchor).GetTimeInSec();
               table[2*i][0]=table[2*i+1][0]=points[i].posvel[component];
               table[2*i+1][1]=points[i].posvel[component+3];
               if (i>0) table[2*i][1]=(table[2*i][0]-table[2*i-1][0])/(nodes[2*i]-nodes[2*i-1]);
            }
            for (int column=2; column<2*count; ++column)
               for (int row=column; row<2*count; ++row)
                  table[row][column]=(table[row][column-1]-table[row-1][column-1])/(nodes[row]-nodes[row-column]);
            long double position=table[2*count-1][2*count-1],velocity=0;
            for (int i=2*count-2; i>=0; --i) {
               velocity=velocity*(query-nodes[i])+position;
               position=position*(query-nodes[i])+table[i][i];
            }
            require(std::abs(state[component]-position)<2e-9 &&
                  std::abs(state[component+3]-velocity)<2e-11,
                  "Ephemeris reader differs from independent Hermite polynomial");
            // The legacy Real-MJD entry point has its documented clock rounding.
            const auto preciseQuery=(GmatTime(middle.GetMjd())-anchor).GetTimeInSec();
            position=table[2*count-1][2*count-1]; velocity=0;
            for (int i=2*count-2; i>=0; --i) {
               velocity=velocity*(preciseQuery-nodes[i])+position;
               position=position*(preciseQuery-nodes[i])+table[i][i];
            }
            require(std::abs(direct[component]-position)<2e-9 &&
                  std::abs(direct[component+3]-velocity)<2e-11,
                  "OEM reader differs from independent Hermite polynomial");
         }
      }
      // Strict mode must reject inadequate source support and output support.
      if (key=="NarrowAtLast") {
         QString strict=applied;
         strict.replace("BeginMissionSequence;", "Export.InsufficientDataAction = Error;\nBeginMissionSequence;");
         source->setPlainText(strict);
         require(window.buildScript(), "Strict Hermite configuration failed to build");
         const auto strictResult=window.runMission(); events();
         if (!evidence.isEmpty()) write(QDir(evidence).filePath("strict-input-messages.txt"),messages->toPlainText().toUtf8());
         if (strictResult==MainWindow::RunResult::Completed || !messages->toPlainText().contains("requires at least 4 points"))
            std::cerr<<messages->toPlainText().toStdString();
         require(strictResult!=MainWindow::RunResult::Completed &&
               messages->toPlainText().contains("requires at least 4 points"),
               "Strict Hermite did not reject three source samples for degree seven");
         strict.replace("Export.InterpolationOrder = 7", "Export.InterpolationOrder = 5");
         source->setPlainText(strict);
         require(window.buildScript(), "Strict reduced Hermite configuration failed to build");
         const auto outputStrictResult=window.runMission(); events();
         if (!evidence.isEmpty()) write(QDir(evidence).filePath("strict-output-messages.txt"),messages->toPlainText().toUtf8());
         require(outputStrictResult!=MainWindow::RunResult::Completed &&
               messages->toPlainText().contains("exported OEM records requires at least 3 points"),
               "Strict Hermite did not reject two serialized records for degree five");
      }
      if (!evidence.isEmpty()) {
         write(QDir(evidence).filePath("mission.script"),applied.toUtf8());
         write(QDir(evidence).filePath("mission.oem"),payload);
         write(QDir(evidence).filePath("messages.txt"),messages->toPlainText().toUtf8());
         const QJsonObject proof{{"outcome","passed"},{"case",key},{"interpolator","Hermite"},
            {"degrees",degrees},{"max_position_error_km",maxPosition},{"max_velocity_error_km_s",maxVelocity},
            {"original_position_tolerance",spec["position_tolerance"]},{"original_velocity_tolerance",spec["velocity_tolerance"]},
            {"gui_apply_undo_redo_save_reopen",true},{"native_quintic_rolling_clone_reset",true},{"reader_roundtrip",true},{"readback_reference","independent confluent divided differences"},
            {"strict_input_and_output_rejection",key=="NarrowAtLast"}};
         write(QDir(evidence).filePath("proof.json"),QJsonDocument(proof).toJson());
         std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      }
      source->document()->setModified(false);
      return 0;
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<std::endl; }
   catch (std::exception &error) { std::cerr<<error.what()<<std::endl; }
   return 1;
}
