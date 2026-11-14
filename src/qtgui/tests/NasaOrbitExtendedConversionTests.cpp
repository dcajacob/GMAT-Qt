#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "BaseException.hpp"
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &path) {QFile f(path);require(f.open(QIODevice::ReadOnly),"Orbit conversion artifact missing");return f.readAll();}
static void write(const QString &path,const QByteArray &bytes) {QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Orbit conversion artifact write failed");}
static void reveal(QWidget *control) {
   require(control,"Orbit conversion control missing");
   for(QWidget *p=control;p;p=p->parentWidget()) {
      for(auto *tabs:control->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(control);
   }
   events();require(control->isVisible()&&control->isEnabled(),"Orbit conversion control inaccessible");
}
struct Field {QLineEdit *edit;QString unit;};
static Field field(QWidget *panel,const QString &name) {
   Field result{};int count=0;
   for(auto *form:panel->findChildren<QFormLayout *>())for(int r=0;r<form->rowCount();++r) {
      auto *li=form->itemAt(r,QFormLayout::LabelRole),*fi=form->itemAt(r,QFormLayout::FieldRole);
      auto *label=li?qobject_cast<QLabel *>(li->widget()):nullptr;auto *row=fi?fi->widget():nullptr;
      if(!label||label->text()!=name||!row||!row->isVisible())continue;
      auto *edit=qobject_cast<QLineEdit *>(row);if(!edit)edit=row->findChild<QLineEdit *>();if(!edit||edit->isReadOnly())continue;
      QString unit;for(auto *text:row->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly))if(text->isVisible())unit+=text->text();
      result={edit,unit};++count;
   }
   if(count!=1)throw std::runtime_error(("Expected one visible conversion field "+name+", found "+QString::number(count)).toStdString());return result;
}
static void choose(QComboBox *combo,const QString &value) {
   reveal(combo);const int row=combo->findText(value);require(row>=0,"Conversion representation not offered");
   combo->showPopup();events();auto *view=combo->view();const auto index=combo->model()->index(row,0);view->scrollTo(index);events();
   const auto rect=view->visualRect(index);require(view->isVisible()&&rect.isValid(),"Representation popup row unavailable");
   QTest::mouseClick(view->viewport(),Qt::LeftButton,Qt::NoModifier,rect.center());events();require(combo->currentText()==value,"Representation popup selection failed");
}

int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaOrbitExtendedConversion");
   try {
      require(argc==4&&acceptanceWidgetPlatform(),"Expected startup,conversion fixture and source representation");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Conversion fixture directory missing");
      const auto all=QJsonDocument::fromJson(read(argv[2])).object();const QString key=argv[3];QJsonObject spec;
      for(auto c:all["cases"].toArray())if(c.toObject()["key"].toString()==key)spec=c.toObject();const int expectedPairs=spec["expected_pairs"].toInt();require(!spec.isEmpty()&&(expectedPairs==3||expectedPairs==8)&&spec["targets"].toArray().size()==expectedPairs,"Extended conversion source fixture missing");
      const auto artifact=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes){if(!artifact.isEmpty())write(QDir(artifact).filePath(name),bytes);};
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto expected=spec["cartesian"].toArray();const auto output=files.filePath("conversion Δ.csv"),saved=files.filePath("conversion Δ.script");
      QString initial="% NASA direct representation pairs Δ\nEarth.Mu = "+QString::number(spec["mu"].toDouble(),'g',17)+";\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\n";
      const QStringList cartNames{"X","Y","Z","VX","VY","VZ"};for(int i=0;i<6;++i)initial+="Sat."+cartNames[i]+" = "+QString::number(expected[i].toDouble(),'g',17)+";\n";
      initial+="Sat.DisplayStateType = "+key+";\nCreate Variable Keep;\nKeep = 7; % untouched initializer\nCreate ReportFile Values;\nValues.Filename = '"+output+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nKeep = Keep + 2; % untouched mission\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Keep;\n";
      save("initial.script",initial.toUtf8());MainWindow window;require(window.initialize(startup),"Conversion runtime initialization failed");window.show();acceptanceView(window);events();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Conversion workspace missing");
      source->setPlainText(initial);require(window.buildScript(),"Conversion baseline build failed");source->document()->setModified(false);
      QJsonArray pairs,nativeChecks;int displayedFields=0,unitChecks=0;double maxPosition=0,maxVelocity=0;
      auto state=[&](const QString &phase,const QString &display) {
         auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Sat"));require(sat,"Conversion configured spacecraft missing");
         require(QString::fromStdString(sat->GetStringParameter("DisplayStateType"))==display&&sat->GetStringParameter("CoordinateSystem")=="EarthMJ2000Eq","Committed conversion selectors differ");
         const auto value=sat->GetState("Cartesian");QJsonArray actual;
         for(int i=0;i<6;++i){actual.append(value[i]);const double error=std::abs(value[i]-expected[i].toDouble());require(std::isfinite(value[i])&&error<=(i<3?all["position_tolerance_km"].toDouble():all["velocity_tolerance_km_s"].toDouble()),"Conversion changed independent physical Cartesian state");if(i<3)maxPosition=std::max(maxPosition,error);else maxVelocity=std::max(maxVelocity,error);}
         nativeChecks.append(QJsonObject{{"phase",phase},{"display",display},{"actual",actual},{"expected",expected}});
      };
      auto open=[&]() {
         reveal(tree);auto items=tree->findItems("Sat",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Conversion resource tree identity missing");tree->setCurrentItem(items.front());tree->scrollToItem(items.front());events();acceptanceView(window);tree->itemDoubleClicked(items.front(),0);events();
         require(QTest::qWaitFor([&]{auto *child=area->activeSubWindow();return child&&dynamic_cast<ResourceEditor *>(child->widget())&&child->windowTitle().contains("Sat");},1500),"Actual conversion resource editor unavailable");
         auto *child=area->activeSubWindow();child->showNormal();area->setActiveSubWindow(child);events();acceptanceView(*child);return child;
      };
      auto values=[&](ResourceEditor *panel,const QJsonObject &target,const QString &phase) {
         const auto names=target["names"].toArray(),wanted=target["values"].toArray(),units=target["units"].toArray();QJsonArray actual;
         for(int i=0;i<6;++i){auto f=field(panel,names[i].toString());reveal(f.edit);f=field(panel,names[i].toString());bool valid=false;const double observed=f.edit->text().toDouble(&valid);double error=observed-wanted[i].toDouble();if(units[i].toString()=="deg")error=std::remainder(error,360.);
            save("latest-field.json",QJsonDocument(QJsonObject{{"source",key},{"target",target["key"]},{"phase",phase},{"field",names[i]},{"actual",observed},{"expected",wanted[i]},{"unit",f.unit},{"expected_unit",units[i]},{"error",error}}).toJson());
            require(valid&&std::isfinite(observed)&&std::abs(error)<=all["displayed_field_tolerance"].toDouble(),"Displayed conversion field differs from independent equation");require(f.unit==units[i].toString(),"Displayed conversion units differ");++displayedFields;++unitChecks;actual.append(QJsonObject{{"name",names[i]},{"value",observed},{"expected",wanted[i]},{"unit",f.unit}});}
         acceptanceView(*panel);return actual;
      };
      state("baseline",key);
      for(auto entry:spec["targets"].toArray()) {
         const auto target=entry.toObject();const auto name=target["key"].toString();const auto prefix=key+"-to-"+name;const auto baseline=source->toPlainText();const int undo=source->document()->availableUndoSteps();
         auto pristine=[&] {require(source->toPlainText()==baseline&&source->document()->availableUndoSteps()==undo&&!source->document()->isModified(),"Pending conversion altered source/history");state(prefix+"-pending-baseline",key);require(Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")==7,"Pending conversion altered unrelated native variable");};
         auto *child=open();auto *panel=dynamic_cast<ResourceEditor *>(child->widget());auto *combo=panel->findChild<QComboBox *>("resource_DisplayStateType");require(combo&&combo->currentText()==key,"Conversion source representation was not loaded");choose(combo,name);require(panel->hasChanges(),"Pending representation change not dirty");const auto pending=values(panel,target,"pending-cancel");pristine();
         auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");require(buttons,"Conversion dialog actions missing");QPointer<QMdiSubWindow> cancelled=child;QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);events();require(!cancelled||!cancelled->isVisible(),"Conversion Cancel did not close owner");pristine();
         child=open();panel=dynamic_cast<ResourceEditor *>(child->widget());combo=panel->findChild<QComboBox *>("resource_DisplayStateType");require(combo&&combo->currentText()==key,"Cancelled conversion leaked into reopened selector");choose(combo,name);const auto draft=values(panel,target,"pending-apply");pristine();buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");acceptanceView(*child);QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel&&!panel->hasChanges()&&child->isVisible(),"Conversion Apply did not leave clean owner");state(prefix+"-apply",name);const auto committed=values(panel,target,"committed");const auto applied=source->toPlainText();require(applied!=baseline&&applied.mid(applied.indexOf("BeginMissionSequence;"))==baseline.mid(baseline.indexOf("BeginMissionSequence;")),"Conversion changed mission or did not commit");save(prefix+"-applied.script",applied.toUtf8());require(child->close(),"Clean converted owner failed to close");events();
         source->undo();require(source->toPlainText()==baseline&&window.buildScript(),"Conversion exact Undo/build failed");state(prefix+"-undo",key);source->redo();require(source->toPlainText()==applied&&window.buildScript(),"Conversion exact Redo/build failed");state(prefix+"-redo",name);
         require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==applied,"Conversion Unicode save/reopen failed");state(prefix+"-reopen",name);child=open();panel=dynamic_cast<ResourceEditor *>(child->widget());combo=panel->findChild<QComboBox *>("resource_DisplayStateType");require(combo&&combo->currentText()==name,"Reopened conversion selector differs");const auto reopened=values(panel,target,"reopen");require(child->close(),"Reopened clean conversion editor failed to close");events();
         messages->clear();const auto run=window.runMission();events();save(prefix+"-messages.txt",messages->toPlainText().toUtf8());require(run==MainWindow::RunResult::Completed,"Conversion mission failed");const auto raw=read(output);save(prefix+"-values.csv",raw);const auto words=QString::fromUtf8(raw).trimmed().split(',');require(words.size()==7&&words[6].trimmed().toDouble()==9,"Conversion report cardinality/sentinel differs");QJsonArray reported;
         for(int i=0;i<6;++i){bool valid=false;const double value=words[i].trimmed().toDouble(&valid);require(valid&&std::isfinite(value)&&std::abs(value-expected[i].toDouble())<=(i<3?all["position_tolerance_km"].toDouble():all["velocity_tolerance_km_s"].toDouble()),"Conversion mission state differs from independent equation");reported.append(value);}
         pairs.append(QJsonObject{{"source",key},{"target",name},{"pending_cancel",pending},{"pending_apply",draft},{"committed",committed},{"reopened",reopened},{"reported",reported},{"sentinel",9}});save("pairs.json",QJsonDocument(pairs).toJson());save("native-checks.json",QJsonDocument(nativeChecks).toJson());
         source->setPlainText(initial);require(window.buildScript(),"Conversion source reset failed");source->document()->setModified(false);state(prefix+"-reset",key);
      }
      require(pairs.size()==expectedPairs&&displayedFields==24*expectedPairs&&unitChecks==24*expectedPairs,"Conversion pair/field coverage incomplete");
      const QJsonObject proof{{"checkpoint","orbit-extended-conversion-"+key},{"outcome","passed"},{"source_representation",key},{"pairs",pairs},{"directed_pairs",expectedPairs},{"displayed_fields",displayedFields},{"unit_checks",unitChecks},{"native_checks",nativeChecks},{"max_position_error_km",maxPosition},{"max_velocity_error_km_s",maxVelocity},{"numeric_sentinel",9},{"facets",QJsonArray{"actual-primary-representation-popup-directed-pairs","independent-six-representation-field-equations-and-units","pending-cancel-apply-and-native-source-history-isolation","exact-undo-redo-unicode-reopen","mission-cartesian-result-and-sentinel"}},{"scope","Only missing directed pairs for this source;one elliptic orbit in EarthMJ2000Eq and TA conics,including one inclined retrograde Delaunay source. Five other native representations,frames,origins,singular/hyperbolic domains,arbitrary input boundaries,native OS input,full NASA provenance and full newest-build qualification remain open."}};
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;source->document()->setModified(false);return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
