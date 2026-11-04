#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "HarmonicGravityOracle.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "GravityField.hpp"
#include "CoordinateSystem.hpp"
#include "CoordinateConverter.hpp"
#include "ODEModel.hpp"
#include "GmatCommand.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
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
#include <QProcess>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace HarmonicOracle;
static void require(bool ok,const char *message) {if(!ok)throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &path) {QFile f(path);require(f.open(QIODevice::ReadOnly),"Harmonic input/report missing");return f.readAll();}
static void write(const QString &path,const QByteArray &bytes) {QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Harmonic evidence write failed");}
template<size_t N>static QJsonArray json(const std::array<double,N> &v) {QJsonArray a;for(double x:v)a.append(x);return a;}
static QJsonArray json(const M &m) {QJsonArray a;for(const auto &r:m)a.append(json(r));return a;}
static void reveal(QWidget *w) {
   require(w,"Harmonic control missing");for(auto *p=w;p;p=p->parentWidget()) {
      for(auto *tabs:w->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(w);
   }
   events();require(w->isVisible()&&w->isEnabled(),"Harmonic control unreachable");
}
static Field parse(const QByteArray &bytes) {
   Field field;
   for(auto line:QString::fromUtf8(bytes).split('\n')) {
      if(line.startsWith("POTFIELD")) {
         field.maximum=line.mid(8,3).toInt();const auto words=line.mid(14).simplified().split(' ');
         require(words.size()>=4&&words[3].toDouble()==1,"Harmonic fixture must be fully normalized");field.mu=words[1].toDouble()/1e9;field.radius=words[2].toDouble()/1000;
      }else if(line.startsWith("RECOEF"))field.coefficients.push_back({line.mid(8,3).toInt(),line.mid(11,3).toInt(),line.mid(17,21).toDouble(),line.size()>38?line.mid(38,21).toDouble():0});
   }
   require(field.mu>0&&field.radius>0&&field.maximum>=8&&!field.coefficients.empty(),"Harmonic model metadata/coefficients missing");return field;
}
struct Trial {QString name;int degree,order;bool shipped;int geometry;};
#include "SampledHarmonicGravityOracle.hpp"
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaHarmonicGravity");
   try {
      require(argc==5&&acceptanceWidgetPlatform(),"Supply startup,Earth/Ceres body,Python and independent oracle");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Harmonic fixture directory missing");
      const QString body=argv[2];require(body=="Earth"||body=="Ceres","Unknown harmonic body");const bool user=body=="Ceres";
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR",files.filePath("evidence"));require(QDir().mkpath(evidence),"Harmonic evidence directory unavailable");auto save=[&](const QString &name,const QByteArray &bytes){write(QDir(evidence).filePath(name),bytes);};
      const auto potential=files.filePath("Synthetic harmonics Δ.cof"),report=files.filePath("Harmonic states Δ.csv"),saved=files.filePath("Harmonic mission Ω.script");
      const double fileMu=user?63:398650,fileRadius=user?470:6300,initialRadius=user?1000:7500;
      QString cof="COMMENT Synthetic normalized degree-eight field,not empirical body gravity\n";
      cof+=QString("POTFIELD%1%2  1 %3 %4 1.0\n").arg(8,3).arg(8,3).arg(fileMu*1e9,0,'E',14).arg(fileRadius*1000,0,'E',14);
      for(int n=2;n<=8;++n)for(int m=0;m<=n;++m) {
         double c=std::pow(-1.,n+m)*.0001/(n*n+m+1),s=m?std::pow(-1.,m)*.00007/(n*n+m+1):0;
         if(n==2) {c=m==0?-.004:m==1?.002:.005;s=m==0?0:m==1?-.003:.004;}
         cof+="RECOEF  "+QString::number(n).rightJustified(3)+QString::number(m).rightJustified(3)+"   "+QString::number(c,'E',14).rightJustified(21)+QString::number(s,'E',14).rightJustified(21)+"\n";
      }
      cof+="END\n";write(potential,cof.toUtf8());save("synthetic.cof",cof.toUtf8());const auto synthetic=parse(cof.toUtf8());
      const auto shipped=QFileInfo("../data/gravity/earth/JGM2.cof").canonicalFilePath();require(!shipped.isEmpty(),"Shipped JGM2 model missing");const auto realEarth=parse(read(shipped));if(!user)save("JGM2.cof",read(shipped));
      const std::vector<Trial> trials{{"Monopole",0,0,false,0},{"Zonal",2,0,false,0},{"Tesseral",2,1,false,0},{"Sectoral",2,2,false,0},{"North",2,2,false,1},{"South",2,2,false,2},{"DegreeFour",4,4,false,0},{"OrderTruncation",4,2,false,3},{"DefaultFile",4,4,!user,3},{"FullFile",user?8:70,user?8:70,!user,0}};
      MainWindow window;require(window.initialize(startup),"Harmonic runtime initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *resources=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&resources&&area,"Harmonic workspace missing");
      QJsonArray observations;int applies=0,dialogs=0;const QStringList components{"X","Y","Z","VX","VY","VZ"};
      for(const auto &trial:trials) {
         const auto field=trial.shipped?realEarth:synthetic;const auto selected=trial.shipped?shipped:potential;
         S initial{initialRadius,-.2*initialRadius,.3*initialRadius,.1,.7*std::sqrt(field.mu/initialRadius),-.2};
         if(trial.geometry==1)initial={0,0,initialRadius,.1,.2,0};if(trial.geometry==2)initial={0,0,-initialRadius,-.1,-.2,0};if(trial.geometry==3)for(int i=3;i<6;++i)initial[i]*=-1;
         QString text="% Independent harmonic acceptance Δ\n";
         if(user) {const auto kernel=QFileInfo("../data/planetary_ephem/spk/ceres_1900_2100.bsp").canonicalFilePath();require(!kernel.isEmpty(),"Ceres ephemeris missing");text+="Create Asteroid Ceres;\nCeres.NAIFId = 2000001;\nCeres.OrbitSpiceKernelName = {'"+kernel+"'};\nCeres.Mu = 60;\nCeres.EquatorialRadius = 400;\nCeres.Flattening = 0;\nCeres.SpinAxisRAConstant = 0;\nCeres.SpinAxisRARate = 0;\nCeres.SpinAxisDECConstant = 90;\nCeres.SpinAxisDECRate = 0;\nCeres.RotationConstant = 37;\nCeres.RotationRate = 15;\n";}
         text+="Create CoordinateSystem Local;\nLocal.Origin = "+body+";\nLocal.Axes = MJ2000Eq;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = Local;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\n";
         for(int i=0;i<6;++i)text+="Sat."+components[i]+" = "+QString::number(initial[i],'g',17)+";\n";
         text+="Create ForceModel FM;\nFM.CentralBody = "+body+";\nFM.PrimaryBodies = {"+body+"};\nFM.PointMasses = {};\nFM.GravityField."+body+".Degree = 1;\nFM.GravityField."+body+".Order = 0;\nFM.Drag.AtmosphereModel = None;\nFM.SRP = Off;\nFM.RelativisticCorrection = Off;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.Accuracy = 1e-13;\nProp.InitialStepSize = 1;\nProp.MinStep = 0;\nProp.MaxStep = 1;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 10};\nReport Values Sat.ElapsedSecs Sat.Local.X Sat.Local.Y Sat.Local.Z Sat.Local.VX Sat.Local.VY Sat.Local.VZ Keep;\n";
         source->setPlainText(text);if(!window.buildScript()){std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Harmonic initial script failed");}save(trial.name+"-initial.script",text.toUtf8());
         auto items=resources->findItems("FM",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Harmonic force resource missing");resources->itemDoubleClicked(items.first(),0);events();auto *child=area->activeSubWindow();auto *panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(panel&&!panel->hasChanges(),"Harmonic force editor missing/dirty");
         auto *gravity=panel->findChild<QPushButton *>("forceGravity");reveal(gravity);acceptanceView(*child);
         auto configure=[&](bool commit) {
            std::exception_ptr failure;bool visited=false;QTimer::singleShot(0,panel,[&] {
               auto *dialog=panel->findChild<QDialog *>("gravityDialog");try {
                  require(dialog,"Harmonic gravity dialog missing");acceptanceView(*dialog);visited=true;
                  auto *which=dialog->findChild<QComboBox *>("gravity_Body");require(which&&which->currentText()==body,"Harmonic primary body mismatch");
                  auto *model=dialog->findChild<QComboBox *>("gravity_PotentialModel");require(model&&model->findText(trial.shipped?"JGM-2":"Other")>=0,"Harmonic model choice missing");model->setCurrentText(trial.shipped?"JGM-2":"Other");
                  auto *file=dialog->findChild<QLineEdit *>("gravity_PotentialFile");reveal(file);file->setText(selected);
                  for(auto pair:QList<QPair<QString,int>>{{"Degree",trial.degree},{"Order",trial.order},{"StmLimit",trial.degree}}) {auto *edit=dialog->findChild<QLineEdit *>("gravity_"+pair.first);reveal(edit);edit->setText(QString::number(pair.second));}
                  acceptanceView(*dialog);require(source->toPlainText()==text,"Harmonic dialog mutated source before Apply");
                  auto *button=dialog->findChild<QDialogButtonBox *>("gravityButtons")->button(commit?QDialogButtonBox::Ok:QDialogButtonBox::Cancel);reveal(button);QTest::mouseClick(button,Qt::LeftButton);
                  if(commit&&dialog->result()!=QDialog::Accepted)std::cerr<<dialog->findChild<QLabel *>("gravityError")->text().toStdString();
                  require(dialog->result()==(commit?QDialog::Accepted:QDialog::Rejected),"Harmonic dialog decision rejected");
               }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
            });QTest::mouseClick(gravity,Qt::LeftButton);if(failure)std::rethrow_exception(failure);require(visited,"Harmonic dialog not visited");++dialogs;
         };
         configure(false);require(!panel->hasChanges()&&source->toPlainText()==text,"Harmonic Cancel changed source/draft");configure(true);require(panel->hasChanges()&&source->toPlainText()==text,"Harmonic pending transaction escaped");
         auto *apply=panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);reveal(apply);acceptanceView(*child);QTest::mouseClick(apply,Qt::LeftButton);events();panel=dynamic_cast<ResourceEditor *>(child->widget());if(panel&&panel->hasChanges())std::cerr<<panel->findChild<QLabel *>("resourceStatus")->text().toStdString();require(panel&&!panel->hasChanges(),"Harmonic Apply failed");++applies;
         const auto applied=source->toPlainText();require(applied!=text&&applied.contains("% Independent harmonic acceptance Δ")&&applied.contains("Keep = 41;")&&child->close(),"Harmonic Apply lost unrelated source or clean close");events();source->undo();require(source->toPlainText()==text&&window.buildScript(),"Harmonic exact Undo failed");source->redo();require(source->toPlainText()==applied&&window.buildScript(),"Harmonic exact Redo failed");require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==applied,"Harmonic Unicode save/reopen failed");save(trial.name+"-mission.script",applied.toUtf8());
         const int history=source->document()->availableUndoSteps();messages->clear();const auto outcome=window.runMission();events();save(trial.name+"-messages.txt",messages->toPlainText().toUtf8());if(outcome!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(outcome==MainWindow::RunResult::Completed,"Harmonic propagation failed");
         const auto raw=read(report);save(trial.name+"-values.csv",raw);const auto words=QString::fromUtf8(raw).trimmed().split(',');require(words.size()==8,"Harmonic report cardinality differs");QJsonArray columns;for(auto word:words){bool valid=false;double value=word.toDouble(&valid);require(valid&&std::isfinite(value),"Harmonic report nonfinite");columns.append(value);}require(std::abs(columns[0].toDouble()-10)<1e-4&&columns[7].toDouble()==41,"Harmonic report time/sentinel differs");S actual{};for(int i=0;i<6;++i)actual[i]=columns[i+1].toDouble();
         GmatCommand *command=Moderator::Instance()->GetFirstCommand();while(command&&!command->IsOfType("Propagate"))command=command->GmatCommand::GetNext();auto *setup=command?dynamic_cast<PropSetup *>(command->GetClone(0)):nullptr;auto *fm=setup?setup->GetODEModel():nullptr;require(fm&&setup->GetPropagator(),"Harmonic live propagation clone missing");
         const double seconds=setup->GetPropagator()->GetTime();save(trial.name+"-clock.json",QJsonDocument(QJsonObject{{"integrator_seconds",seconds},{"reported_seconds",columns[0]}}).toJson());require(std::abs(seconds-10)<1e-9,"Harmonic integrator duration differs");GravityField *gravityForce=nullptr;
         require(fm->GetNumForces()==1,"Harmonic model includes unintended contributors");gravityForce=dynamic_cast<GravityField *>(fm->GetForce(0));require(gravityForce&&gravityForce->GetBodyName()==body.toStdString()&&gravityForce->GetIntegerParameter("Degree")==trial.degree&&gravityForce->GetIntegerParameter("Order")==trial.order,"Harmonic runtime degree/order/body differs");
         std::string nativeBody;double nativeMu=0;require(gravityForce->GetBodyAndMu(nativeBody,nativeMu)&&nativeBody==body.toStdString()&&std::abs(nativeMu-field.mu)<1e-9,"Harmonic potential Mu precedence differs");require(QFileInfo(QString::fromStdString(gravityForce->GetStringParameter("PotentialFileFullPath"))).canonicalFilePath()==selected,"Harmonic runtime potential file differs");
         auto *in=dynamic_cast<CoordinateSystem *>(gravityForce->GetRefObject(Gmat::COORDINATE_SYSTEM,gravityForce->GetStringParameter("InputCoordinateSystem")));auto *fixed=dynamic_cast<CoordinateSystem *>(gravityForce->GetRefObject(Gmat::COORDINATE_SYSTEM,gravityForce->GetStringParameter("FixedCoordinateSystem")));require(in&&fixed,"Harmonic runtime frame definition missing");
         CoordinateConverter converter;Rvector6 zero;zero.Set(0,0,0,0,0,0);Rvector6 unused;std::vector<M> rotations;QJsonArray rotationRecords;
         for(int i=0;i<9;++i){const double t=seconds*i/8;GmatTime epoch(21545);epoch.AddSeconds(t);fixed->SetForGravityUpdate(true);converter.Convert(epoch,zero,in,unused,fixed,false,true);fixed->SetForGravityUpdate(false);const auto matrix=converter.GetLastRotationMatrix();M value{};for(int row=0;row<3;++row)for(int col=0;col<3;++col)value[row][col]=matrix(row,col);for(int row=0;row<3;++row)for(int col=0;col<3;++col){double product=0;for(int k=0;k<3;++k)product+=value[row][k]*value[col][k];require(std::abs(product-(row==col?1:0))<5e-13,"Shared gravity rotation not orthogonal");}rotations.push_back(value);rotationRecords.append(json(value));}
         const auto coarse=integrate(initial,seconds,field,trial.degree,trial.order,rotations,128),expected=integrate(initial,seconds,field,trial.degree,trial.order,rotations,256);S errors{},refinement{};
         for(int i=0;i<6;++i){errors[i]=std::abs(actual[i]-expected[i]);refinement[i]=std::abs(coarse[i]-expected[i]);if(errors[i]>(i<3?1e-6:1e-9))std::cerr<<"HARMONIC_DIFF "<<body.toStdString()<<" "<<trial.name.toStdString()<<" "<<i<<" "<<errors[i]<<std::endl;require(refinement[i]<(i<3?1e-9:1e-12),"Independent harmonic RK4 not converged");require(errors[i]<(i<3?1e-6:1e-9),"Harmonic trajectory differs from independent potential gradient");}
         auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));auto *planet=Moderator::Instance()->GetSolarSystemInUse()->GetBody(body.toStdString());require(sat&&planet,"Harmonic live spacecraft/body missing");const auto satBefore=sat->GetState();const auto origin=planet->GetMJ2000State(sat->GetEpochGT());
         QJsonArray grid;const double oldTime=gravityForce->GetTime();gravityForce->SetTime(0);
         for(V point:std::vector<V>{{initialRadius,0,0},{-initialRadius,0,0},{0,initialRadius,0},{0,-initialRadius,0},{0,0,initialRadius},{0,0,-initialRadius},{.7*initialRadius,-.4*initialRadius,.8*initialRadius}}) {
            const auto local=rotate(rotations.back(),point,true);std::unique_ptr<Spacecraft> probe(dynamic_cast<Spacecraft *>(sat->Clone()));require(bool(probe),"Harmonic read-only probe clone missing");Real state[6];for(int i=0;i<3;++i){state[i]=origin[i]+local[i];state[i+3]=origin[i+3];}probe->GetState().SetState(state,6);
            const auto derivative=gravityForce->GetDerivativesForSpacecraft(probe.get());V actualLocal{};for(int i=0;i<3;++i)actualLocal[i]=state[i]-origin[i];const auto wanted=force(field,trial.degree,trial.order,actualLocal,rotations.back());V observed{},difference{};for(int i=0;i<3;++i){observed[i]=derivative[i+3];difference[i]=std::abs(observed[i]-wanted[i]);if(difference[i]>1e-11)std::cerr<<"HARMONIC_FORCE_DIFF "<<body.toStdString()<<" "<<trial.name.toStdString()<<" "<<i<<" "<<difference[i]<<std::endl;require(difference[i]<1e-11,"Native harmonic force differs from direct potential gradient");}
            grid.append(QJsonObject{{"requested_body_position",json(point)},{"actual_local_position",json(actualLocal)},{"native_acceleration",json(observed)},{"independent_acceleration",json(wanted)},{"errors",json(difference)}});
         }
         const auto sampled=sampledgravity::check(*gravityForce,*sat,in,fixed,field,trial.degree,trial.order,initialRadius);
         save(trial.name+"-sampled-gravity.json",QJsonDocument(sampled).toJson());
         if(!user&&trial.name=="FullFile")sampledgravity::checkTides(*gravityForce,in,fixed,[&](const QJsonArray &rows){save("sampled-tides.json",QJsonDocument(rows).toJson());});
         gravityForce->SetTime(oldTime);require(sat->GetEpochGT()==satBefore.GetEpochGT(),"Harmonic force probe changed live epoch");for(int i=0;i<6;++i)require(sat->GetState()[i]==satBefore[i],"Harmonic force probe changed live spacecraft");require(source->toPlainText()==applied&&source->document()->availableUndoSteps()==history&&!window.isRunning()&&window.findChild<QAction *>("runMission")->isEnabled(),"Harmonic mission changed source/history/lock");
         auto *outputs=window.findChild<QTreeWidget *>("Output");auto values=outputs->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(values.size()==1,"Harmonic report output missing");outputs->itemDoubleClicked(values.first(),0);events();child=area->activeSubWindow();require(child&&dynamic_cast<ReportViewer *>(child->widget()),"Harmonic report viewer missing");acceptanceView(*child);auto *shown=child->findChild<QPlainTextEdit *>();require(shown&&shown->isReadOnly()&&shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Harmonic viewer differs from checked report");require(child->close(),"Harmonic viewer close failed");events();
         observations.append(QJsonObject{{"trial",trial.name},{"degree",trial.degree},{"order",trial.order},{"shipped_model",trial.shipped},{"mu",field.mu},{"radius",field.radius},{"initial_state",json(initial)},{"seconds",seconds},{"reported_seconds",columns[0]},{"rotation_samples",rotationRecords},{"actual_state",json(actual)},{"independent_state",json(expected)},{"state_errors",json(errors)},{"RK4_refinement_errors",json(refinement)},{"grid",grid}});save("observations.json",QJsonDocument(observations).toJson());
      }
      require(applies==10&&dialogs==20,"Harmonic GUI operation count differs");const QJsonObject proof{{"checkpoint","harmonic-gravity-"+body},{"outcome","passed"},{"body",body},{"trial_count",10},{"resource_applies",applies},{"gravity_dialogs",dialogs},{"force_grid_points",70},{"observations",observations},{"facets",QJsonArray{"actual-gravity-controls-cancel-pending-apply","exact-undo-redo-and-unicode-persistence","degree-order-zonal-tesseral-sectoral-truncation","native-axis-and-pole-force-grid","independent-dual-potential-gradient-and-RK4","independent-complex-step-gradient-and-DP5","shipped-Earth70-or-synthetic-user8-model","checked-actual-report-viewer"}},{"scope","Direct normalized harmonic potential differentiated with Cartesian dual numbers,full coefficient and degree/order truncation,axis/pole force samples and independently integrated10s states. Earth uses shipped JGM2 through70/70;Ceres uses synthetic8/8. Native frame rotations are shared explicit inputs,not independent orientation or ephemeris truth. Empirical model truth,tides,arbitrary degrees/files/all bodies,long duration,STM,native OS/GPU/platform/stress/current NASA/full-build remain open."}};
      auto candidate=proof;candidate["outcome"]="candidate";const auto candidatePath=files.filePath("oracle-input.json"),oracleOutput=files.filePath("oracle-output.json");write(candidatePath,QJsonDocument(candidate).toJson());
      QProcess oracle;oracle.start(QString::fromLocal8Bit(argv[3]),QStringList{QString::fromLocal8Bit(argv[4]),"--directory",evidence,"--proof",candidatePath,"--output",oracleOutput});require(oracle.waitForStarted(5000)&&oracle.waitForFinished(120000),"Independent harmonic complex-step/DP5 audit did not finish");save("oracle-stdout.txt",oracle.readAllStandardOutput());const auto error=oracle.readAllStandardError();save("oracle-stderr.txt",error);if(oracle.exitCode()!=0)std::cerr<<error.toStdString();require(oracle.exitStatus()==QProcess::NormalExit&&oracle.exitCode()==0,"Independent harmonic complex-step/DP5 audit failed");save("independent.json",read(oracleOutput));
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
