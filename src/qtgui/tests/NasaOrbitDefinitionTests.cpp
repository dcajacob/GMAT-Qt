#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "HelpDialog.hpp"
#include "Spacecraft.hpp"
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
using State=std::array<double,6>;
static QJsonArray array(const State &value) {QJsonArray a;for (double x:value) a.append(x);return a;}
static State kepler(const QJsonArray &elements,double mu) {
   const double a=elements[0].toDouble(),e=elements[1].toDouble(),rad=std::acos(-1.0)/180.;
   const double i=elements[2].toDouble()*rad,node=elements[3].toDouble()*rad,peri=elements[4].toDouble()*rad,nu=elements[5].toDouble()*rad;
   const double cN=std::cos(node),sN=std::sin(node),ci=std::cos(i),si=std::sin(i),cw=std::cos(peri),sw=std::sin(peri);
   const std::array<double,3> P{cN*cw-sN*sw*ci,sN*cw+cN*sw*ci,sw*si},Q{-cN*sw-sN*cw*ci,-sN*sw+cN*cw*ci,cw*si};
   const double p=a*(1-e*e),r=p/(1+e*std::cos(nu)),scale=std::sqrt(mu/p);State result{};
   for(int k=0;k<3;++k) {result[k]=r*(P[k]*std::cos(nu)+Q[k]*std::sin(nu));result[k+3]=scale*(-P[k]*std::sin(nu)+Q[k]*(e+std::cos(nu)));}return result;
}
struct OrbitField {QLineEdit *edit;QWidget *row;QString unit;};
static OrbitField field(QWidget *panel,const QString &name) {
   OrbitField found{};int count=0;
   for (auto *form:panel->findChildren<QFormLayout *>()) for(int r=0;r<form->rowCount();++r) {
      auto *labelItem=form->itemAt(r,QFormLayout::LabelRole),*valueItem=form->itemAt(r,QFormLayout::FieldRole);
      auto *label=labelItem ? qobject_cast<QLabel *>(labelItem->widget()):nullptr;auto *row=valueItem ? valueItem->widget():nullptr;
      if(!label || label->text()!=name || !row || !row->isVisible()) continue;
      auto *edit=qobject_cast<QLineEdit *>(row);if(!edit) edit=row->findChild<QLineEdit *>();if(!edit || edit->isReadOnly()) continue;
      QString unit;for(auto *text:row->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly)) if(text->isVisible()) unit+=text->text();found={edit,row,unit};++count;
   }
   if(count!=1) throw std::runtime_error(("Expected one visible orbit field: "+name+", found "+QString::number(count)).toStdString());return found;
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaOrbitDefinitions");
   try {
      require(argc==4 && acceptanceWidgetPlatform(),"Expected startup, orbit cases and key");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Orbit definition fixture missing");
      const QString key=argv[3];const auto data=entry(QFileInfo(argv[2]).absoluteFilePath(),key);const auto names=data["names"].toArray(),values=data["values"].toArray(),units=data["units"].toArray();require(names.size()==6 && values.size()==6 && units.size()==6,"Orbit definition needs six fields");
      const double mu=data["mu"].toDouble();const State target=kepler(data["canonical_keplerian"].toArray(),mu),base{7000,0,0,0,7.5,1};
      for(int i=0;i<6;++i) require(std::abs(target[i]-data["expected_cartesian"].toArray()[i].toDouble())<(i<3 ? 1e-8:1e-11),"Independent orbit fixture matrix/basis calculations disagree");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto report=files.filePath("Orbit definition Δ.txt"),saved=files.filePath("Orbit definition Δ.script");
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes) {if(!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes);};
      const QString initial="% NASA positive orbit definition Δ\nEarth.Mu = 398600.4418;\nCreate Spacecraft DefinedSat;\nDefinedSat.CoordinateSystem = EarthMJ2000Eq;\nDefinedSat.DisplayStateType = Cartesian;\nDefinedSat.X = 7000;\nDefinedSat.Y = 0;\nDefinedSat.Z = 0;\nDefinedSat.VX = 0;\nDefinedSat.VY = 7.5;\nDefinedSat.VZ = 1;\nCreate Variable Keep;\nKeep = 7; % preserve initializer\nCreate ReportFile StateCheck;\nStateCheck.Filename = '"+report+"';\nStateCheck.WriteHeaders = false;\nStateCheck.Precision = 17;\nStateCheck.FixedWidth = false;\nStateCheck.Delimiter = ',';\nBeginMissionSequence;\nKeep = Keep + 2; % preserve mission\nReport StateCheck DefinedSat.X DefinedSat.Y DefinedSat.Z DefinedSat.VX DefinedSat.VY DefinedSat.VZ Keep;\n";
      save("initial.script",initial.toUtf8());MainWindow window;require(window.initialize(startup),"Orbit definition initialization failed");window.show();acceptanceView(window);events();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source && tree && area && messages,"Orbit definition workspace missing");source->setPlainText(initial);if(!window.buildScript()) {std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Orbit definition baseline rejected");}source->document()->setModified(false);const int undo=source->document()->availableUndoSteps();
      auto model=[&] {auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("DefinedSat"));require(sat,"Configured orbit spacecraft missing");State result{};const auto state=sat->GetState("Cartesian");for(int i=0;i<6;++i) result[i]=state[i];return result;};
      auto exactBase=[&] {const auto state=model();for(int i=0;i<6;++i) if(std::abs(state[i]-base[i])>1e-12)return false;return true;};require(exactBase(),"Orbit literal baseline differs");
      auto open=[&](const QString &name) {auto matches=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Orbit resource tree item missing");acceptanceView(window);tree->itemDoubleClicked(matches.front(),0);events();require(QTest::qWaitFor([&] {auto *active=area->activeSubWindow();return active && dynamic_cast<ResourceEditor *>(active->widget()) && active->windowTitle().contains(name);},1500),"Orbit resource editor activation did not settle");return area->activeSubWindow();};
      QPointer<QMdiSubWindow> sibling=open("Keep");acceptanceView(*sibling);auto *child=open("DefinedSat");auto *panel=dynamic_cast<ResourceEditor *>(child->widget());auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");require(buttons,"Orbit Apply actions missing");
      if(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {child->showNormal();child->move(20,12);child->resize(std::min(960,area->viewport()->width()-40),std::min(700,area->viewport()->height()-24));}events();acceptanceView(*child);
      auto *representation=panel->findChild<QComboBox *>("resource_DisplayStateType");require(representation && representation->findText(key)>=0,"Orbit representation not available through primary controls");reveal(representation);representation->setCurrentText(key);events();require(representation->currentText()==key,"Primary orbit representation did not change");
      auto pristine=[&] {return source->toPlainText()==initial && source->document()->availableUndoSteps()==undo && !source->document()->isModified() && exactBase() && Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")==7 && sibling && sibling->isVisible() && !dynamic_cast<ResourceEditor *>(sibling->widget())->hasChanges();};
      QJsonArray unitChecks;
      for(int i=0;i<6;++i) {auto control=field(panel,names[i].toString());reveal(control.edit);control=field(panel,names[i].toString());unitChecks.append(QJsonObject{{"field",names[i]},{"expected_unit",units[i]},{"actual_unit",control.unit}});save("units-before.json",QJsonDocument(unitChecks).toJson());if(control.unit!=units[i].toString()) throw std::runtime_error(("Wrong visible unit for "+names[i].toString()+": expected ["+units[i].toString()+"], got ["+control.unit+"]").toStdString());acceptanceView(*child);}
      for(int i=0;i<6;++i) {auto control=field(panel,names[i].toString());reveal(control.edit);control.edit->setFocus();control.edit->selectAll();QTest::keyClicks(control.edit,QString::number(values[i].toDouble(),'g',17));events();require(pristine(),"Defining orbit fields changed applied model/source/history");acceptanceView(*child);}
      require(panel->hasChanges(),"Positive orbit definition did not produce pending changes");QJsonArray rejected;
      auto reject=[&](const QString &bad,QDialogButtonBox::StandardButton action,const QString &invalidField) {auto control=field(panel,invalidField);reveal(control.edit);control.edit->setText(bad);events();QPointer<ResourceEditor> previous=panel;acceptanceView(*child);QTest::mouseClick(buttons->button(action),Qt::LeftButton);events();auto *current=dynamic_cast<ResourceEditor *>(child->widget());const auto *status=current ? current->findChild<QLabel *>("resourceStatus"):nullptr;save("rejection-diagnostic.json",QJsonDocument(QJsonObject{{"bad",bad},{"same_panel",previous && current==previous},{"pristine",pristine()},{"pending",current && current->hasChanges()},{"diagnostic",status ? status->text():QString()}}).toJson());require(previous && current==previous && pristine() && current->hasChanges() && field(current,invalidField).edit->text()==bad && status && !status->text().isEmpty(),"Invalid orbit definition changed model/source/history, closed owner or omitted diagnostic");rejected.append(QJsonObject{{"action",action==QDialogButtonBox::Apply ? "Apply":"OK"},{"field",invalidField},{"value",bad},{"diagnostic",status->text()}});acceptanceView(*child);};
      for(const auto &bad:data["invalid"].toArray()) reject(bad.toString(),QDialogButtonBox::Apply,data["invalid_field"].toString());reject(data["invalid"].toArray().first().toString(),QDialogButtonBox::Ok,data["invalid_field"].toString());
      if (key.startsWith("Spherical")) {
         field(panel,data["invalid_field"].toString()).edit->setText(QString::number(values[0].toDouble(),'g',17));
         for (const auto &bad:QStringList{"-1","0","nan"}) reject(bad,QDialogButtonBox::Apply,"VMagnitude");
         reject("-1",QDialogButtonBox::Ok,"VMagnitude");
      }
      for(int i=0;i<6;++i) field(panel,names[i].toString()).edit->setText(QString::number(values[i].toDouble(),'g',17));events();require(pristine(),"Orbit recovery changed applied baseline before Apply");acceptanceView(*child);QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges() && child->isVisible(),"Positive orbit Apply did not stay clean/open");
      QJsonArray checkpoints;double positionError=0,velocityError=0;auto check=[&](const QString &phase,const State &actual,const State &expected) {for(int i=0;i<6;++i) {const double error=std::abs(actual[i]-expected[i]);if(i<3) positionError=std::max(positionError,error);else velocityError=std::max(velocityError,error);require(error<=(i<3 ? data["position_tolerance_km"].toDouble():data["velocity_tolerance_km_s"].toDouble()),"Positive orbit Cartesian state differs from independent conic oracle");}checkpoints.append(QJsonObject{{"phase",phase},{"actual",array(actual)},{"expected",array(expected)}});};
      auto verifyType=[&] {auto *sat=Moderator::Instance()->GetConfiguredObject("DefinedSat");require(sat->GetStringParameter("DisplayStateType")==key.toStdString() && sat->GetStringParameter("CoordinateSystem")=="EarthMJ2000Eq","Orbit representation/frame was not committed");};
      verifyType();check("apply",model(),target);const auto applied=source->toPlainText();save("applied.script",applied.toUtf8());require(applied!=initial && applied.mid(applied.indexOf("BeginMissionSequence;"))==initial.mid(initial.indexOf("BeginMissionSequence;")),"Orbit Apply changed mission commands or failed to commit");acceptanceView(*child);require(child->close(),"Clean orbit editor did not close");events();source->undo();require(source->toPlainText()==initial && window.buildScript(),"Orbit definition exact Undo failed");check("undo",model(),base);source->redo();require(source->toPlainText()==applied && window.buildScript(),"Orbit definition exact Redo failed");verifyType();check("redo",model(),target);
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==applied,"Orbit Unicode save/reopen failed");verifyType();check("reopen",model(),target);child=open("DefinedSat");panel=dynamic_cast<ResourceEditor *>(child->widget());representation=panel->findChild<QComboBox *>("resource_DisplayStateType");require(representation && representation->currentText()==key,"Reopened primary representation differs");QJsonArray reopened;
      for(int i=0;i<6;++i) {auto control=field(panel,names[i].toString());reveal(control.edit);control=field(panel,names[i].toString());bool ok=false;const double observed=control.edit->text().toDouble(&ok),wanted=values[i].toDouble();double error=observed-wanted;if(units[i].toString()=="deg") error=std::remainder(error,360.);require(ok && std::abs(error)<1e-6 && control.unit==units[i].toString(),"Reopened orbit field value/label/unit differs");reopened.append(QJsonObject{{"field",names[i]},{"value",observed},{"unit",control.unit}});acceptanceView(*child);}
      require(child->close(),"Reopened clean orbit panel did not close");events();save("mission.script",source->toPlainText().toUtf8());messages->clear();const auto result=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());if(result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Orbit definition mission did not complete");const auto raw=read(report);save("values.txt",raw);const auto numbers=QString::fromUtf8(raw).trimmed().split(',');require(numbers.size()==7,"Orbit report should contain six state components and sentinel");State observed{};for(int i=0;i<6;++i) {bool ok=false;observed[i]=numbers[i].trimmed().toDouble(&ok);require(ok && std::isfinite(observed[i]),"Orbit report has nonfinite state");}require(numbers[6].trimmed().toDouble()==9,"Orbit independent sentinel differs");check("mission",observed,target);save("checkpoints.json",QJsonDocument(checkpoints).toJson());save("rejections.json",QJsonDocument(rejected).toJson());
      const QJsonObject proof{{"checkpoint","orbit-definition-"+key},{"outcome","passed"},{"family",key},{"frame","EarthMJ2000Eq"},{"units",unitChecks},{"inputs",values},{"reopened",reopened},{"state_checkpoints",checkpoints},{"rejections",rejected},{"max_position_error_km",positionError},{"max_velocity_error_km_s",velocityError},{"numeric_sentinel",9},{"facets",QJsonArray{"actual-primary-orbit-six-field-definition","visible-labels-and-units","invalid-definition-apply-ok-atomicity","independent-conic-cartesian-oracle","exact-history-unicode-and-reopened-values","mission-report-and-sibling-isolation"}},{"scope","All six positive fields in this representation, one nonsingular elliptic orbit and EarthMJ2000Eq only. Keplerian inputs use true anomaly. No all-frame, singular/hyperbolic, arbitrary pair conversion, native OS input, independent ephemeris/long propagation or full platform/current NASA corpus qualification."}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
