#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "HelpDialog.hpp"
#include "Spacecraft.hpp"
#include "AttitudeDialog.hpp"
#include <QFormLayout>
#include <QComboBox>
#include <array>
#include <algorithm>
#include "Moderator.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAbstractButton>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QSlider>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *m) {if (!ok) throw std::runtime_error(m);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &p) {QFile f(p);require(f.open(QIODevice::ReadOnly),"Panel mechanism artifact missing");return f.readAll();}
static void write(const QString &p,const QByteArray &bytes) {QFile f(p);require(f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size(),"Panel mechanism artifact write failed");}
static QString digest(const QByteArray &bytes) {return QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());}
static void reveal(QWidget *c) {require(c,"Panel mechanism control missing");for (QWidget *p=c;p;p=p->parentWidget()) {for (auto *t:c->window()->findChildren<QTabWidget *>()) if (t->indexOf(p)>=0) t->setCurrentWidget(p);if (auto *s=qobject_cast<QScrollArea *>(p)) s->ensureWidgetVisible(c);}events();require(c->isVisible() && c->isEnabled(),"Panel mechanism control unreachable");}
static QJsonObject entry(const QString &path,const QString &key) {for (auto c:QJsonDocument::fromJson(read(path)).object()["cases"].toArray()) if (c.toObject()["key"].toString()==key) return c.toObject();throw std::runtime_error("Panel mechanism case missing");}
static QString identify(QWidget *w) {if (auto *b=qobject_cast<QAbstractButton *>(w)) return "button:"+QString(b->text()).remove('&');return QString::fromLatin1(w->metaObject()->className())+":"+w->objectName();}
using Pose=std::array<double,12>;
static QJsonArray array(const Pose &p){QJsonArray a;for(double x:p)a.append(x);return a;}
static Pose truth(const QJsonObject &data,double elapsed=0) {
   Pose result{};const auto matrix=data["matrix"].toArray(),rates=data["angular_velocity_degrees"].toArray();double w[3],m[3][3],r[3][3];double magnitude=0;
   for(int i=0;i<3;++i){w[i]=rates[i].toDouble()*std::acos(-1.)/180.;magnitude+=w[i]*w[i];result[9+i]=rates[i].toDouble();for(int j=0;j<3;++j)m[i][j]=matrix[i].toArray()[j].toDouble();}magnitude=std::sqrt(magnitude);
   const double c=std::cos(magnitude*elapsed),s=std::sin(magnitude*elapsed);double u[3]={w[0]/magnitude,w[1]/magnitude,w[2]/magnitude};double skew[3][3]={{0,-u[2],u[1]},{u[2],0,-u[0]},{-u[1],u[0],0}};
   for(int i=0;i<3;++i)for(int j=0;j<3;++j)r[i][j]=(i==j?c:0)+(1-c)*u[i]*u[j]-s*skew[i][j];
   for(int i=0;i<3;++i)for(int j=0;j<3;++j)for(int k=0;k<3;++k)result[3*i+j]+=r[i][k]*m[k][j];return result;
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaAttitudeDefinitions");
   try {
      require(argc==4 && acceptanceWidgetPlatform(),"Expected startup, attitude cases and key");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Attitude fixture missing");const QString key=argv[3];const auto data=entry(QFileInfo(argv[2]).absoluteFilePath(),key);const auto target=truth(data);const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"),report=files.filePath("Attitude results Δ.txt"),saved=files.filePath("Attitude definition Δ.script");auto save=[&](const QString &name,const QByteArray &b){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),b);};
      QString columns="Sat.A1ModJulian";for(int i=1;i<=3;++i)for(int j=1;j<=3;++j)columns+=" Sat.DCM"+QString::number(i)+QString::number(j);columns+=" Sat.AngularVelocityX Sat.AngularVelocityY Sat.AngularVelocityZ Keep";
      const QString initial="% NASA attitude definition Δ\nCreate Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = 21545;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 1;\nSat.Attitude = Spinner;\nSat.AttitudeDisplayStateType = Quaternion;\nSat.AttitudeRateDisplayStateType = AngularVelocity;\nSat.Q1 = 0;\nSat.Q2 = 0;\nSat.Q3 = 0;\nSat.Q4 = 1;\nSat.AngularVelocityX = 0;\nSat.AngularVelocityY = 0;\nSat.AngularVelocityZ = 0;\nCreate ForceModel Gravity;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nGravity.Drag.AtmosphereModel = None;\nGravity.SRP = Off;\nCreate Propagator Prop;\nProp.FM = Gravity;\nCreate Variable Keep;\nKeep = 7; % preserve initializer\nCreate ReportFile AttitudeCheck;\nAttitudeCheck.Filename = '"+report+"';\nAttitudeCheck.WriteHeaders = false;\nAttitudeCheck.Precision = 17;\nAttitudeCheck.FixedWidth = false;\nAttitudeCheck.Delimiter = ',';\nBeginMissionSequence;\nKeep = Keep + 2; % preserve mission\nReport AttitudeCheck "+columns+";\nPropagate Prop(Sat) {Sat.ElapsedSecs = 60};\nReport AttitudeCheck "+columns+";\n";
      save("initial.script",initial.toUtf8());MainWindow window;require(window.initialize(startup),"Attitude initialization failed");window.show();acceptanceView(window);events();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source && messages && tree && area,"Attitude workspace missing");source->setPlainText(initial);if(!window.buildScript()){std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Attitude baseline rejected");}source->document()->setModified(false);const int undo=source->document()->availableUndoSteps();
      auto model=[&]{auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Sat"));require(sat,"Attitude spacecraft missing");Pose pose{};auto a=sat->GetAttitude(sat->GetEpoch());auto w=sat->GetAngularVelocity(sat->GetEpoch());for(int i=0;i<3;++i){for(int j=0;j<3;++j)pose[i*3+j]=a(i,j);pose[9+i]=w[i]*180./std::acos(-1.);}return pose;};const auto baseline=model();
      auto open=[&](const QString &name){auto matches=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Attitude resource missing");acceptanceView(window);tree->itemDoubleClicked(matches.front(),0);events();require(QTest::qWaitFor([&]{auto *a=area->activeSubWindow();return a && dynamic_cast<ResourceEditor *>(a->widget()) && a->windowTitle().contains(name);},1500),"Attitude resource activation did not settle");return area->activeSubWindow();};QPointer<QMdiSubWindow>sibling=open("Keep");acceptanceView(*sibling);auto *child=open("Sat");auto *panel=dynamic_cast<ResourceEditor *>(child->widget());
      if(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible"){child->showNormal();child->move(20,12);child->resize(std::min(960,area->viewport()->width()-40),std::min(700,area->viewport()->height()-24));}events();acceptanceView(*child);auto *editor=dynamic_cast<AttitudeDialog *>(panel->findChild<QDialog *>("attitudeDialog"));auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");require(editor && buttons,"Primary attitude form missing");
      auto field=[&](const QString &name){auto *c=editor->findChild<QLineEdit *>("attitude_"+name);require(c,"Attitude numeric field missing");reveal(c);return c;};auto select=[&](const QString &name,const QString &value){auto *c=editor->findChild<QComboBox *>("attitude_"+name);require(c && c->findText(value)>=0,"Attitude selector value missing");reveal(c);c->setCurrentText(value);events();c=editor->findChild<QComboBox *>("attitude_"+name);require(c && c->currentText()==value,"Attitude selector rejected valid state");};
      auto pristine=[&]{return source->toPlainText()==initial && !source->document()->isModified() && source->document()->availableUndoSteps()==undo && model()==baseline && sibling && sibling->isVisible() && !dynamic_cast<ResourceEditor *>(sibling->widget())->hasChanges();};
      select("AttitudeDisplayStateType",data["representation"].toString());select("EulerAngleSequence",data["sequence"].toString());
      auto fill=[&](const QJsonArray &names,const QJsonArray &values,bool typing){for(int i=0;i<names.size();++i){auto *c=field(names[i].toString());const auto text=QString::number(values[i].toDouble(),'g',17);if(typing){c->setFocus();c->selectAll();QTest::keyClicks(c,text);}else c->setText(text);events();require(pristine(),"Pending attitude/rate fields changed model/source/history");}};
      const auto names=data["names"].toArray(),values=data["values"].toArray(),rateNames=data["rate_names"].toArray(),rateValues=data["rate_values"].toArray();fill(names,values,true);select("AttitudeRateDisplayStateType",data["rate_representation"].toString());fill(rateNames,rateValues,true);
      QJsonArray conversions;
      auto converted=[&](const QJsonObject &entry,const QString &selector,const QString &original){select(selector,original);select(selector,entry["kind"].toString());const auto ns=entry["names"].toArray(),vs=entry["values"].toArray();QJsonArray actual;double minus=0,plus=0;for(int i=0;i<ns.size();++i){bool ok=false;const double value=field(ns[i].toString())->text().toDouble(&ok);require(ok && std::isfinite(value),"Converted attitude field is nonfinite");actual.append(value);minus=std::max(minus,std::abs(value-vs[i].toDouble()));plus=std::max(plus,std::abs(value+vs[i].toDouble()));}const double error=entry["kind"].toString()=="Quaternion"?std::min(minus,plus):minus;conversions.append(QJsonObject{{"selector",selector},{"from",original},{"to",entry["kind"]},{"actual",actual},{"expected",vs},{"max_error",error}});save("conversions.json",QJsonDocument(conversions).toJson());require(error<1e-10 && pristine(),"Attitude conversion differs from independent field oracle");acceptanceView(*child);};
      for(auto item:data["representations"].toArray())converted(item.toObject(),"AttitudeDisplayStateType",data["representation"].toString());select("AttitudeDisplayStateType",data["representation"].toString());
      for(auto item:data["rate_representations"].toArray())converted(item.toObject(),"AttitudeRateDisplayStateType",data["rate_representation"].toString());select("AttitudeRateDisplayStateType",data["rate_representation"].toString());
      QJsonArray units;auto unitCheck=[&](const QString &name,const QString &unit){auto *c=field(name);auto *label=editor->findChild<QLabel *>("attitudeUnit_"+name);require((unit.isEmpty() && (!label || label->text().isEmpty())) || (label && label->text()==unit && label->isVisible()),"Attitude field unit differs");units.append(QJsonObject{{"field",name},{"unit",unit}});};for(int i=0;i<names.size();++i)unitCheck(names[i].toString(),data["units"].toArray()[i].toString());for(auto n:rateNames)unitCheck(n.toString(),"deg/s");
      QJsonArray rejections;auto reject=[&](const QString &name,const QString &value,QDialogButtonBox::StandardButton action){field(name)->setText(value);events();QPointer<ResourceEditor> previous=panel;acceptanceView(*child);QTest::mouseClick(buttons->button(action),Qt::LeftButton);events();auto *current=dynamic_cast<ResourceEditor *>(child->widget());auto *status=current?current->findChild<QLabel *>("resourceStatus"):nullptr;save("rejection-diagnostic.json",QJsonDocument(QJsonObject{{"field",name},{"input",value},{"pristine",pristine()},{"same_panel",current && current==previous},{"diagnostic",status?status->text():QString()}}).toJson());require(previous && current==previous && pristine() && panel->hasChanges() && field(name)->text()==value && status && !status->text().isEmpty(),"Invalid attitude/rate changed source/model/owner or lacked diagnostic");rejections.append(QJsonObject{{"field",name},{"input",value},{"action",action==QDialogButtonBox::Apply?"Apply":"OK"},{"diagnostic",status->text()}});};
      for(const auto &bad:data["invalid"].toArray())reject(data["invalid_field"].toString(),bad.toString(),QDialogButtonBox::Apply);reject(data["invalid_field"].toString(),data["invalid"].toArray().first().toString(),QDialogButtonBox::Ok);fill(names,values,false);for(const auto &bad:QStringList{"nan","inf","not-a-number"})reject(rateNames[0].toString(),bad,QDialogButtonBox::Apply);reject(rateNames[0].toString(),"nan",QDialogButtonBox::Ok);fill(rateNames,rateValues,false);
      QJsonArray checkpoints;auto check=[&](const QString &phase,const Pose &actual,const Pose &wanted){double error=0;for(int i=0;i<12;++i)error=std::max(error,std::abs(actual[i]-wanted[i]));checkpoints.append(QJsonObject{{"phase",phase},{"actual",array(actual)},{"expected",array(wanted)},{"max_error",error}});save("checkpoints.json",QJsonDocument(checkpoints).toJson());require(error<5e-11,"Attitude/rate differs from independent matrix oracle");};acceptanceView(*child);QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges() && child->isVisible(),"Attitude Apply did not stay clean/open");check("apply",model(),target);const auto applied=source->toPlainText();save("applied.script",applied.toUtf8());require(applied!=initial && applied.mid(applied.indexOf("BeginMissionSequence;"))==initial.mid(initial.indexOf("BeginMissionSequence;")),"Attitude Apply changed mission commands");require(child->close(),"Clean attitude editor did not close");events();source->undo();require(source->toPlainText()==initial && window.buildScript(),"Attitude exact Undo failed");check("undo",model(),baseline);source->redo();require(source->toPlainText()==applied && window.buildScript(),"Attitude exact Redo failed");check("redo",model(),target);require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==applied,"Attitude Unicode save/reopen failed");check("reopen",model(),target);
      child=open("Sat");panel=dynamic_cast<ResourceEditor *>(child->widget());editor=dynamic_cast<AttitudeDialog *>(panel->findChild<QDialog *>("attitudeDialog"));require(editor,"Reopened attitude form missing");auto choices=QJsonObject{{"AttitudeDisplayStateType",data["representation"]},{"AttitudeRateDisplayStateType",data["rate_representation"]},{"EulerAngleSequence",data["sequence"]}};for(auto i=choices.begin();i!=choices.end();++i){auto*c=editor->findChild<QComboBox *>("attitude_"+i.key());require(c && c->currentText()==i.value().toString(),"Reopened attitude selector differs");}QJsonArray reopened;auto verify=[&](const QJsonArray &ns,const QJsonArray &vs){for(int i=0;i<ns.size();++i){bool ok=false;double actual=field(ns[i].toString())->text().toDouble(&ok);require(ok && std::abs(actual-vs[i].toDouble())<1e-10,"Reopened attitude/rate field differs");reopened.append(QJsonObject{{"field",ns[i]},{"value",actual}});}};verify(names,values);verify(rateNames,rateValues);acceptanceView(*child);require(child->close(),"Reopened attitude editor did not close");events();save("mission.script",source->toPlainText().toUtf8());messages->clear();auto result=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());if(result!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Attitude mission did not complete");const auto raw=read(report);save("values.txt",raw);auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==2,"Attitude report needs initial/final rows");QJsonArray observations;
      for(int row=0;row<2;++row){auto tokens=lines[row].split(',');require(tokens.size()==14,"Attitude report column count differs");bool ok=false;double epoch=tokens[0].toDouble(&ok);require(ok,"Attitude report epoch invalid");double elapsed=(epoch-21545)*86400.;require(std::abs(elapsed-row*60.)<1e-5,"Attitude report elapsed time differs");Pose actual{};for(int i=0;i<12;++i){actual[i]=tokens[i+1].toDouble(&ok);require(ok && std::isfinite(actual[i]),"Attitude report has nonfinite value");}require(tokens[13].toDouble()==9,"Attitude sentinel differs");check("mission"+QString::number(row),actual,truth(data,elapsed));observations.append(QJsonObject{{"elapsed_seconds",elapsed},{"pose",array(actual)}});}
      save("rejections.json",QJsonDocument(rejections).toJson());const QJsonObject proof{{"checkpoint","attitude-definition-"+key},{"family",key},{"outcome","passed"},{"units",units},{"conversions",conversions},{"reopened",reopened},{"checkpoints",checkpoints},{"observations",observations},{"rejections",rejections},{"numeric_sentinel",9},{"facets",QJsonArray{"actual-primary-attitude-and-rate-definition","all-fields-and-units","independent-all-representation-conversions","invalid-apply-ok-atomicity","independent-dcm-angular-velocity-oracle","exact-history-unicode-reopen","independent-constant-spin-mission"}},{"scope","One nonsingular positive orientation/rate in Spinner/EarthMJ2000Eq.15 cases cover12Euler sequences and quaternion/DCM/MRP. No full domain/singular/all-model/all-frame/native-input qualification."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
