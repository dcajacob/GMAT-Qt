#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "HelpDialog.hpp"
#include "Spacecraft.hpp"
#include <QFormLayout>
#include <QComboBox>
#include <QDateTime>
#include <QLocale>
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
static double epochValue(QString text,const QString &format) {
   text=text.trimmed();if(text.startsWith('\'') && text.endsWith('\'')) text=text.mid(1,text.size()-2);
   if(format.endsWith("ModJulian")) {bool ok=false;const double value=text.toDouble(&ok);require(ok && std::isfinite(value),"Displayed numeric epoch is invalid");return value;}
   auto date=QLocale::c().toDateTime(text,"dd MMM yyyy HH:mm:ss.zzz");date.setTimeSpec(Qt::UTC);require(date.isValid(),"Displayed Gregorian epoch is invalid");
   const QDateTime origin(QDate(1941,1,5),QTime(12,0),Qt::UTC);return origin.msecsTo(date)/86400000.0;
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaEpochDefinitions");
   try {
      require(argc==4 && acceptanceWidgetPlatform(),"Expected startup, cases and epoch format");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Epoch fixture directory missing");
      const QString key=argv[3];const auto data=entry(QFileInfo(argv[2]).absoluteFilePath(),key);const auto expected=data["expected"].toObject();const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto report=files.filePath("Epoch results Δ.txt"),saved=files.filePath("Epoch definition Δ.script"),evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &value){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),value);};
      const QString initial="% NASA epoch definition Δ\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = TAIModJulian;\nSat.Epoch = 21545;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 1;\nCreate Variable Keep;\nKeep = 7; % preserve initializer\nCreate ReportFile EpochCheck;\nEpochCheck.Filename = '"+report+"';\nEpochCheck.WriteHeaders = false;\nEpochCheck.Precision = 17;\nEpochCheck.FixedWidth = false;\nEpochCheck.Delimiter = ',';\nBeginMissionSequence;\nKeep = Keep + 2; % preserve mission\nReport EpochCheck Sat.A1ModJulian Sat.TAIModJulian Sat.UTCModJulian Sat.TTModJulian Sat.TDBModJulian Keep;\n";
      save("initial.script",initial.toUtf8());MainWindow window;require(window.initialize(startup),"Epoch initialization failed");window.show();acceptanceView(window);events();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source && tree && area && messages,"Epoch workspace missing");source->setPlainText(initial);require(window.buildScript(),"Epoch baseline rejected");source->document()->setModified(false);const int undo=source->document()->availableUndoSteps();
      auto model=[&] {auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Sat"));require(sat,"Epoch spacecraft missing");const auto state=sat->GetState("Cartesian");const double wanted[6]={7000,0,0,0,7.5,1};for(int i=0;i<6;++i)require(std::abs(state[i]-wanted[i])<1e-12,"Epoch editing changed orbit state");return sat->GetRealParameter("A1Epoch");};
      const double baseline=model();
      auto open=[&](const QString &name) {auto matches=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Epoch resource tree item missing");acceptanceView(window);tree->itemDoubleClicked(matches.front(),0);events();require(QTest::qWaitFor([&]{auto *a=area->activeSubWindow();return a && dynamic_cast<ResourceEditor *>(a->widget()) && a->windowTitle().contains(name);},1500),"Epoch resource activation did not settle");return area->activeSubWindow();};
      QPointer<QMdiSubWindow> sibling=open("Keep");acceptanceView(*sibling);auto *child=open("Sat");auto *panel=dynamic_cast<ResourceEditor *>(child->widget());
      if(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {child->showNormal();child->move(20,12);child->resize(std::min(960,area->viewport()->width()-40),std::min(700,area->viewport()->height()-24));}events();acceptanceView(*child);
      auto *format=panel->findChild<QComboBox *>("resource_DateFormat");auto *epoch=panel->findChild<QLineEdit *>("resource_Epoch");auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");require(format && epoch && buttons,"Primary epoch controls missing");
      auto pristine=[&] {return source->toPlainText()==initial && source->document()->availableUndoSteps()==undo && !source->document()->isModified() && model()==baseline && Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")==7 && sibling && sibling->isVisible() && !dynamic_cast<ResourceEditor *>(sibling->widget())->hasChanges();};
      auto define=[&] {reveal(format);format->setCurrentText(key);events();require(format->currentText()==key,"Requested epoch format unavailable");reveal(epoch);epoch->setFocus();epoch->selectAll();QTest::keyClicks(epoch,data["literal"].toString());events();require(pristine(),"Pending epoch definition changed model/source/history");};
      QJsonArray conversions;
      for(const auto &item:data["conversions"].toArray()) {const auto c=item.toObject();define();reveal(format);format->setCurrentText(c["format"].toString());events();require(format->currentText()==c["format"].toString(),"Epoch format conversion rejected valid input");reveal(epoch);const double actual=epochValue(epoch->text(),format->currentText()),error=std::abs(actual-c["expected_mjd"].toDouble())*86400.;conversions.append(QJsonObject{{"from",key},{"to",c["format"]},{"display",epoch->text()},{"actual_mjd",actual},{"expected_mjd",c["expected_mjd"]},{"error_seconds",error},{"tolerance_seconds",c["tolerance_seconds"]}});save("conversions.json",QJsonDocument(conversions).toJson());require(error<=c["tolerance_seconds"].toDouble(),"Epoch conversion differs from independent time oracle");require(pristine(),"Format conversion changed configured epoch/source/history");acceptanceView(*child);}
      define();QJsonArray rejections;
      auto reject=[&](const QString &bad,QDialogButtonBox::StandardButton action) {reveal(epoch);epoch->setText(bad);events();QPointer<ResourceEditor> previous=panel;acceptanceView(*child);QTest::mouseClick(buttons->button(action),Qt::LeftButton);events();auto *current=dynamic_cast<ResourceEditor *>(child->widget());auto *status=current ? current->findChild<QLabel *>("resourceStatus"):nullptr;save("rejection-diagnostic.json",QJsonDocument(QJsonObject{{"input",bad},{"pristine",pristine()},{"same_panel",current && current==previous},{"diagnostic",status ? status->text():QString()}}).toJson());require(previous && current==previous && pristine() && panel->hasChanges() && epoch->text()==bad && status && !status->text().isEmpty(),"Invalid epoch did not preserve source/model/pending/owner with diagnostic");rejections.append(QJsonObject{{"input",bad},{"action",action==QDialogButtonBox::Apply ? "Apply":"OK"},{"diagnostic",status->text()}});acceptanceView(*child);};
      for(const auto &bad:data["invalid"].toArray())reject(bad.toString(),QDialogButtonBox::Apply);reject(data["invalid"].toArray().first().toString(),QDialogButtonBox::Ok);epoch->setText(data["literal"].toString());events();require(pristine(),"Epoch recovery changed baseline before Apply");acceptanceView(*child);QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges() && child->isVisible(),"Epoch Apply did not stay clean/open");
      QJsonArray checkpoints;auto check=[&](const QString &phase,double wanted,double tolerance){const double actual=model(),error=std::abs(actual-wanted)*86400.;checkpoints.append(QJsonObject{{"phase",phase},{"actual_a1_mjd",actual},{"expected_a1_mjd",wanted},{"error_seconds",error},{"tolerance_seconds",tolerance}});save("checkpoints.json",QJsonDocument(checkpoints).toJson());require(error<=tolerance,"Applied/reloaded epoch differs from independent oracle");};
      const double tolerance=data["epoch_tolerance_seconds"].toDouble(),wanted=expected["A1"].toDouble();check("apply",wanted,tolerance);const auto applied=source->toPlainText();save("applied.script",applied.toUtf8());require(applied!=initial && applied.mid(applied.indexOf("BeginMissionSequence;"))==initial.mid(initial.indexOf("BeginMissionSequence;")),"Epoch Apply changed mission commands");require(child->close(),"Clean epoch editor did not close");events();source->undo();require(source->toPlainText()==initial && window.buildScript(),"Epoch exact Undo failed");check("undo",baseline,0.000001);source->redo();require(source->toPlainText()==applied && window.buildScript(),"Epoch exact Redo failed");check("redo",wanted,tolerance);require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==applied,"Epoch Unicode save/reopen failed");check("reopen",wanted,tolerance);
      child=open("Sat");panel=dynamic_cast<ResourceEditor *>(child->widget());format=panel->findChild<QComboBox *>("resource_DateFormat");epoch=panel->findChild<QLineEdit *>("resource_Epoch");require(format && epoch && format->currentText()==key,"Reopened epoch format differs");reveal(epoch);const auto reopened=epoch->text();require(std::abs(epochValue(reopened,key)-data["source_label_mjd"].toDouble())*86400<=0.000001,"Reopened epoch literal lost precision");acceptanceView(*child);require(child->close(),"Reopened epoch panel did not close");events();save("mission.script",source->toPlainText().toUtf8());messages->clear();const auto result=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());if(result!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Epoch mission failed");const auto raw=read(report);save("values.txt",raw);const auto numbers=QString::fromUtf8(raw).trimmed().split(',');require(numbers.size()==6,"Epoch report requires five time systems and sentinel");QJsonArray reportValues;const QStringList systems={"A1","TAI","UTC","TT","TDB"};
      for(int i=0;i<5;++i){bool ok=false;const double value=numbers[i].trimmed().toDouble(&ok),error=std::abs(value-expected[systems[i]].toDouble())*86400.,limit=(data["system"].toString()=="TDB" || systems[i]=="TDB") ? 0.00005:0.000002;require(ok && std::isfinite(value) && error<=limit,"Mission epoch report differs from independent time oracle");reportValues.append(QJsonObject{{"system",systems[i]},{"actual_mjd",value},{"expected_mjd",expected[systems[i]]},{"error_seconds",error},{"tolerance_seconds",limit}});}require(numbers[5].trimmed().toDouble()==9,"Epoch mission sentinel differs");save("rejections.json",QJsonDocument(rejections).toJson());
      const QJsonObject proof{{"checkpoint","epoch-definition-"+key},{"outcome","passed"},{"family",key},{"literal",data["literal"]},{"conversions",conversions},{"checkpoints",checkpoints},{"rejections",rejections},{"reopened",reopened},{"report",reportValues},{"numeric_sentinel",9},{"facets",QJsonArray{"actual-primary-epoch-definition","independent-time-system-oracle","ten-target-format-conversions","invalid-apply-ok-atomicity","exact-history-unicode-reopen","mission-time-report-and-state-isolation"}},{"scope","Selected2000 epochs; all ten formats and ten conversion targets per source. Independent USNO TDB50us comparison, Gregorian0.51ms display tolerance. Not full epoch-domain/leap-second/precision/native-input/platform qualification."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
