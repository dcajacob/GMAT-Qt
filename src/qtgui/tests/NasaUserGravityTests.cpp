#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "ResourcePreview.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "CelestialBody.hpp"
#include "PhysicalModel.hpp"
#include "ODEModel.hpp"
#include "PropSetup.hpp"
#include "BaseException.hpp"
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
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static void show(QWidget &widget)
{
   for (auto *page=&widget;page;page=page->parentWidget()) {
      for (auto *tabs:widget.window()->findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea *>(page)) scroll->ensureWidgetVisible(&widget);
   }
   events();require(widget.isVisible() && widget.isEnabled(),"User gravity control is not visible/enabled");
}
static void modal(QObject &owner,const QString &name,const std::function<void()> &trigger,const std::function<void(QDialog &)> &action)
{
   std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,&owner,[&] {
      auto *dialog=owner.findChild<QDialog *>(name);
      try {require(dialog,"User gravity dialog did not open");acceptanceView(*dialog);visited=true;action(*dialog);}
      catch (...) {failure=std::current_exception();if (dialog) dialog->reject();}
   });trigger();if (failure) std::rethrow_exception(failure);require(visited,"User gravity dialog was not exercised");
}
static QMap<QString,QString> properties()
{
   auto *model=Moderator::Instance()->GetConfiguredObject("Gravity");require(model,"User gravity force model missing");QMap<QString,QString> values;
   for (const auto &field:resourceProperties(*model)) values[field.name]=field.value;return values;
}
static std::array<double,6> state(const QString &path)
{
   QFile file(path);require(file.open(QIODevice::ReadOnly),"User gravity state report missing");const auto words=QString::fromUtf8(file.readAll()).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==6,"User gravity report must contain six Cartesian components");
   std::array<double,6> result{};for (int i=0;i<6;++i) {bool valid=false;result[i]=words[i].toDouble(&valid);require(valid && std::isfinite(result[i]),"User gravity state is nonfinite/nonnumeric");}return result;
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaUserGravity");
   try {
      require(argc==2 && acceptanceWidgetPlatform(),"Expected startup and supported GUI platform");TestSettings settings;QTemporaryDir files;require(files.isValid(),"User gravity fixture directory missing");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto kernel=QFileInfo("../data/planetary_ephem/spk/ceres_1900_2100.bsp").canonicalFilePath();require(!kernel.isEmpty(),"NASA Ceres ephemeris fixture missing");
      // Synthetic, axisymmetric degree-two field: C20 = -J2/sqrt(5).
      // File constants intentionally differ from the body's 60 km^3/s^2,
      // 400 km to check NASA's documented potential-file precedence.
      constexpr double mu=63.0,radius=470.0,j2=0.01,r=1000.0,seconds=600.0;
      const double speed=std::sqrt(mu/r*(1.0+1.5*j2*std::pow(radius/r,2)));
      const auto potential=files.filePath("Synthetic Ceres Δ.cof");
      QString cof="COMMENT Synthetic acceptance fixture, not measured Ceres gravity\n";
      cof+=QString("POTFIELD%1%2  1 %3 %4 1.0\n").arg(2,3).arg(2,3).arg(mu*1e9,0,'E',14).arg(radius*1e3,0,'E',14);
      for (int m=0;m<=2;++m) cof+="RECOEF  "+QString::number(2).rightJustified(3)+QString::number(m).rightJustified(3)+"   "+QString::number(m==0 ? -j2/std::sqrt(5.0) : 0.0,'E',14).rightJustified(21)+QString::number(0.0,'E',14).rightJustified(21)+"\n";
      cof+="END\n";{QFile file(potential);require(file.open(QIODevice::WriteOnly) && file.write(cof.toUtf8())==cof.toUtf8().size(),"Could not write synthetic gravity fixture");}
      const auto report=files.filePath("Ceres states.txt");
      const QString definition="% User gravity acceptance: preserve body definition Δ\nCreate Asteroid Ceres;\nCeres.NAIFId = 2000001;\nCeres.OrbitSpiceKernelName = {'"+kernel+"'};\nCeres.Mu = 60;\nCeres.EquatorialRadius = 400;\nCeres.Flattening = 0;\nCeres.SpinAxisRAConstant = 0;\nCeres.SpinAxisRARate = 0;\nCeres.SpinAxisDECConstant = 90;\nCeres.SpinAxisDECRate = 0;\nCeres.RotationConstant = 0;\nCeres.RotationRate = 0;\n";
      const QString fixed="Create CoordinateSystem CeresFrame;\nCeresFrame.Origin = Ceres;\nCeresFrame.Axes = MJ2000Eq;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = CeresFrame;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2024 12:00:00.000';\nSat.X = 1000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+QString::number(speed,'g',17)+";\nSat.VZ = 0;\nCreate ForceModel Gravity;\nGravity.CentralBody = Ceres;\n";
      const QString tail="Gravity.Drag.AtmosphereModel = None;\nGravity.SRP = Off;\nGravity.RelativisticCorrection = Off;\nCreate Propagator Prop;\nProp.FM = Gravity;\nProp.Accuracy = 1e-13;\nProp.InitialStepSize = 10;\nProp.MaxStep = 30;\nCreate ReportFile States;\nStates.Filename = '"+report+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\nCreate Variable Keep;\nKeep = 13; % preserve unrelated source\n";
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 600}; % preserve mission Δ\nReport States Sat.CeresFrame.X Sat.CeresFrame.Y Sat.CeresFrame.Z Sat.CeresFrame.VX Sat.CeresFrame.VY Sat.CeresFrame.VZ;\n";
      const QString source=definition+fixed+"Gravity.PrimaryBodies = {};\nGravity.PointMasses = {Ceres};\n"+tail+mission;
      const QString harmonic="Gravity.PrimaryBodies = {Ceres};\nGravity.PointMasses = {};\nGravity.GravityField.Ceres.PotentialFile = '"+potential+"';\nGravity.GravityField.Ceres.Degree = 2;\nGravity.GravityField.Ceres.Order = 0;\nGravity.GravityField.Ceres.StmLimit = 2;\n";
      const QString reference=definition+fixed+harmonic+tail+mission;
      MainWindow window;require(window.initialize(startup),"User gravity runtime initialization failed");window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(editor && tree && area,"User gravity workspace missing");
      auto build=[&] {if (!window.buildScript()) {events();std::cerr<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()<<std::endl;throw std::runtime_error("User gravity fixture did not build");}};
      auto run=[&] {if (window.runMission()!=MainWindow::RunResult::Completed) {events();std::cerr<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()<<std::endl;throw std::runtime_error("User gravity mission did not complete");}return state(report);};
      editor->setPlainText(reference);build();const auto wanted=run();
      editor->setPlainText(source);build();const auto before=properties();const auto undo=editor->document()->availableUndoSteps();
      const auto items=tree->findItems("Gravity",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"User gravity tree entry missing");tree->itemDoubleClicked(items.front(),0);events();auto *child=area->currentSubWindow();auto *panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr;require(panel,"User gravity resource editor missing");acceptanceView(window);
      auto unchanged=[&] {require(editor->toPlainText()==source && properties()==before && editor->document()->availableUndoSteps()==undo,"User gravity pending edits changed source/model/history");};
      auto *primary=panel->findChild<QLineEdit *>("resource_PrimaryBodies");auto *points=panel->findChild<QLineEdit *>("resource_PointMasses");require(primary && points,"User gravity body selectors missing");show(*points);points->clear();events();
      auto chooseBody=[&](bool accept) {
         show(*primary);const auto buttons=primary->parentWidget()->findChildren<QPushButton *>();require(buttons.size()==1,"Primary-body list action missing");
         modal(*panel,"resourceSelectionDialog",[&]{buttons.front()->click();},[&](QDialog &d) {
            auto *list=d.findChild<QListWidget *>("resourceSelectionList");require(list && list->isVisible(),"Primary-body choices not visible");
            auto *clear=d.findChild<QPushButton *>("resourceClearSelection");require(clear && clear->isVisible(),"Primary-body Clear selection action missing");clear->click();
            const auto matches=list->findItems("Ceres",Qt::MatchExactly);require(matches.size()==1,"User-defined Ceres is absent from the primary-body picker");list->scrollToItem(matches.front());matches.front()->setCheckState(Qt::Checked);events();
            d.findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
         });unchanged();require(primary->text()==(accept ? "Ceres" : ""),"Primary-body chooser Cancel/OK changed wrong state");
      };
      chooseBody(false);chooseBody(true);
      auto *gravity=panel->findChild<QPushButton *>("forceGravity");require(gravity,"User gravity dialog action missing");show(*gravity);
      auto configure=[&](bool accept) {
         modal(*panel,"gravityDialog",[&]{gravity->click();},[&](QDialog &d) {
            auto *body=d.findChild<QComboBox *>("gravity_Body");require(body && body->currentText()=="Ceres","Gravity dialog did not use the pending user primary");
            auto *model=d.findChild<QComboBox *>("gravity_PotentialModel");require(model && model->count()==1 && model->currentText()=="Other","User gravity offered an unrelated built-in potential preset");
            auto *file=d.findChild<QLineEdit *>("gravity_PotentialFile");require(file,"User gravity potential entry missing");const auto initial=file->text();
            for (bool pick:{false,true}) {
               modal(d,"gravityFileDialog",[&]{d.findChild<QPushButton *>("gravityBrowse_PotentialFile")->click();},[&](QDialog &picker) {
                  auto *browser=dynamic_cast<QFileDialog *>(&picker);require(browser && browser->fileMode()==QFileDialog::ExistingFile,"User potential picker permits nonexistent files");
                  if (!pick) {browser->reject();return;}
                  browser->selectFile(potential);auto *path=browser->findChild<QLineEdit *>("fileNameEdit");require(path && path->isVisible(),"User potential filename entry missing");path->setText(potential);events();
                  auto *open=browser->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Open);require(open && open->isEnabled(),"User potential picker Open disabled");open->click();require(browser->result()==QDialog::Accepted,"User potential picker did not accept");
               });require(file->text()==(pick ? potential : initial),"User potential picker Cancel/selection changed wrong value");
            }
            require(d.findChild<QLabel *>("gravity_FileLimits")->text()=="Degree 2 / Order 2","Custom potential limits were not shown");
            auto *degree=d.findChild<QLineEdit *>("gravity_Degree");auto *order=d.findChild<QLineEdit *>("gravity_Order");auto *stm=d.findChild<QLineEdit *>("gravity_StmLimit");require(degree && order && stm,"Custom gravity numeric fields missing");degree->setText("3");order->setText("0");stm->setText("2");
            auto *ok=d.findChild<QDialogButtonBox *>("gravityButtons")->button(QDialogButtonBox::Ok);ok->click();require(d.isVisible() && degree->text()=="3" && !d.findChild<QLabel *>("gravityError")->text().isEmpty(),"Custom gravity over-file degree accepted or pending value lost");unchanged();degree->setText("2");
            if (accept) {ok->click();if (d.result()!=QDialog::Accepted) std::cerr<<"User gravity dialog rejection: "<<d.findChild<QLabel *>("gravityError")->text().toStdString()<<std::endl;require(d.result()==QDialog::Accepted,"Valid custom user gravity settings rejected");} else d.reject();
         });unchanged();
      };
      configure(false);configure(true);
      modal(*panel,"gravityDialog",[&]{gravity->click();},[&](QDialog &d) {require(d.findChild<QLineEdit *>("gravity_PotentialFile")->text()==potential && d.findChild<QLineEdit *>("gravity_Degree")->text()=="2" && d.findChild<QLineEdit *>("gravity_Order")->text()=="0","Pending user gravity settings were not retained on reopen");d.reject();});
      require(panel->hasChanges(),"User gravity settings did not remain pending");unchanged();panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());
      if (!panel || panel->hasChanges()) {if (panel) std::cerr<<panel->findChild<QLabel *>("resourceStatus")->text().toStdString()<<std::endl;throw std::runtime_error("User gravity pending settings did not Apply");}
      const auto applied=editor->toPlainText();const auto accepted=properties();require(applied.startsWith(definition) && applied.endsWith(mission) && applied.contains("Keep = 13; % preserve unrelated source"),"User gravity Apply rewrote body/unrelated source/mission");
      require(accepted.value("GravityField.Ceres.Degree")=="2" && accepted.value("GravityField.Ceres.Order")=="0" && QFileInfo(accepted.value("GravityField.Ceres.PotentialFile")).canonicalFilePath()==potential,"Applied user gravity properties differ");
      require(child->close(),"Applied user gravity editor did not close");events();editor->undo();require(editor->toPlainText()==source,"User gravity Undo was not exact");build();require(properties()==before,"User gravity Undo changed configured state");
      editor->redo();require(editor->toPlainText()==applied,"User gravity Redo was not exact");build();require(properties()==accepted,"User gravity Redo changed accepted state");
      const auto saved=files.filePath("User harmonic Δ.script");require(window.saveScriptTo(saved) && window.loadScript(saved),"User gravity Unicode save/reopen failed");build();QFile savedFile(saved);require(savedFile.open(QIODevice::ReadOnly) && QString::fromUtf8(savedFile.readAll())==applied && editor->toPlainText()==applied && properties()==accepted,"User gravity save/reopen changed source/properties");
      const auto actual=run();for (int i=0;i<6;++i) require(std::abs(actual[i]-wanted[i])<=1e-11*std::max(1.0,std::abs(wanted[i])),"User gravity GUI trajectory differs from equivalent authored mission");
      const double angle=speed/r*seconds;const std::array<double,6> analytic{r*std::cos(angle),r*std::sin(angle),0.0,-speed*std::sin(angle),speed*std::cos(angle),0.0};
      double positionError=0,velocityError=0;
      for (int i=0;i<6;++i) {const double tolerance=i<3 ? 1e-7 : 1e-10;const double error=std::abs(actual[i]-analytic[i]);if (i<3) positionError=std::max(positionError,error);else velocityError=std::max(velocityError,error);if (error>tolerance) std::cerr<<"Analytic component "<<i<<" actual "<<actual[i]<<" expected "<<analytic[i]<<" error "<<actual[i]-analytic[i]<<std::endl;require(error<=tolerance,"User-defined synthetic J2 orbit differs from independent analytic solution");}
      auto *prop=dynamic_cast<PropSetup *>(Moderator::Instance()->GetInternalObject("Prop"));auto *forces=prop ? prop->GetODEModel() : nullptr;require(forces,"User gravity runtime force model missing");int count=0;
      for (int i=0;i<forces->GetNumForces();++i) {auto *force=forces->GetForce(i);require(!force->IsOfType("PointMassForce"),"User primary retained a point-mass contributor");if (force->IsOfType("GravityField")) {++count;require(force->GetBodyName()=="Ceres" && force->GetIntegerParameter("Degree")==2 && force->GetIntegerParameter("Order")==0 && force->GetIntegerParameter("StmLimit")==2 && QFileInfo(QString::fromStdString(force->GetStringParameter("PotentialFileFullPath"))).canonicalFilePath()==potential,"Running user gravity settings differ from GUI selection");require(!force->IsParameterReadOnly(force->GetParameterID("TideModel")),"User body TideModel metadata is still incorrectly read-only");}}
      auto *body=dynamic_cast<CelestialBody *>(Moderator::Instance()->GetConfiguredObject("Ceres"));require(count==1 && body && body->IsUserDefined() && body->GetGravitationalConstant()==60 && body->GetEquatorialRadius()==400,"User gravity changed body identity/constants or force cardinality");
      // Qualify the metadata correction without bypassing the existing model
      // validator: supplied Love numbers permit Solid, never non-Earth pole tides.
      const auto love=files.filePath("Synthetic Ceres love Δ.tide");{QFile file(love);require(file.open(QIODevice::WriteOnly) && file.write("k 2 0 0.1\n")>0,"Could not write custom-body tide metadata fixture");}
      QtResourcePreview probe(forces->GetForce(0)->Clone());require(probe->IsOfType("GravityField"),"Custom-body metadata probe did not clone the gravity field");
      setResourceProperty(*probe,"TideFile",love);setResourceProperty(*probe,"TideModel","Solid");require(probe->GetStringParameter("TideModel")=="Solid","Custom-body Solid choice rejected despite supplied Love numbers");
      bool poleRejected=false;try {setResourceProperty(*probe,"TideModel","SolidAndPole");} catch (BaseException &) {poleRejected=true;}
      require(poleRejected && probe->GetStringParameter("TideModel")=="Solid","Custom-body pole tide accepted or invalid choice changed model");
      setResourceProperty(*probe,"TideModel","None");setResourceProperty(*probe,"TideFile","");bool noDataRejected=false;try {setResourceProperty(*probe,"TideModel","Solid");} catch (BaseException &) {noDataRejected=true;}
      require(noDataRejected && probe->GetStringParameter("TideModel")=="None","Custom-body Solid without Love numbers accepted or invalid choice changed model");
      auto spherical=reference;spherical.replace("Gravity.GravityField.Ceres.Degree = 2;","Gravity.GravityField.Ceres.Degree = 0;");editor->setPlainText(spherical);build();const auto control=run();double difference=0;for (int i=0;i<3;++i) difference+=std::pow(actual[i]-control[i],2);require(std::sqrt(difference)>1e-3,"Non-spherical gravity had no observable effect compared with its monopole control");
      const QJsonObject proof{{"checkpoint","gravity-user.harmonic"},{"body","Ceres"},{"user_defined",true},{"degree",2},{"order",0},{"duration_seconds",600},{"outcome","passed"},
         {"facets",QJsonArray{"visible-primary-picker","cancel-isolation","custom-potential-picker","file-bounds-rejection-correction","pending-apply","exact-undo-redo","unicode-save-reopen","runtime-owned-force","body-definition-preserved","potential-constants-precedence","six-state-script-reference","analytic-equatorial-j2","monopole-control","custom-body-tide-metadata-and-guards"}},
         {"analytic_position_tolerance_km",1e-7},{"analytic_velocity_tolerance_km_per_s",1e-10},{"fixture_scope","synthetic J2 configuration and one circular equatorial orbit; not measured Ceres gravity or general force-model accuracy"}};
      auto measured=proof;measured["max_position_error_km"]=positionError;measured["max_velocity_error_km_per_s"]=velocityError;measured["monopole_position_difference_km"]=std::sqrt(difference);measured["potential_mu_km3_per_s2"]=mu;measured["body_mu_km3_per_s2"]=60;measured["potential_radius_km"]=radius;measured["body_radius_km"]=400;
      std::cout<<"NASA_CHECK "<<QJsonDocument(measured).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}
     catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}
   return 1;
}
