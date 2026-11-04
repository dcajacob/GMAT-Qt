#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDialogButtonBox>
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
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool condition,const char *message) {if(!condition)throw std::runtime_error(message);}
static void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static void save(const QString &name,const QJsonObject &value){const auto root=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(root.isEmpty())return;QFile file(root+"/"+name);require(file.open(QIODevice::WriteOnly),"Brouwer evidence file unavailable");file.write(QJsonDocument(value).toJson());}
static void reveal(QWidget *control){for(auto *p=control;p;p=p->parentWidget()){for(auto *tabs:control->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(control);}events();require(control->isVisible()&&control->isEnabled(),"Brouwer selector is unreachable");}
static QJsonArray choices(QComboBox *control){QJsonArray result;for(int i=0;i<control->count();++i)result.append(control->itemText(i));return result;}
static QString fieldText(QWidget *panel,const QString &name,QString *unit=nullptr){int count=0;QString value;for(auto *form:panel->findChildren<QFormLayout *>())for(int row=0;row<form->rowCount();++row){auto *labelItem=form->itemAt(row,QFormLayout::LabelRole),*valueItem=form->itemAt(row,QFormLayout::FieldRole);auto *label=labelItem?qobject_cast<QLabel *>(labelItem->widget()):nullptr;auto *widget=valueItem?valueItem->widget():nullptr;if(!label||label->text()!=name||!widget||!widget->isVisible())continue;auto *edit=qobject_cast<QLineEdit *>(widget);if(!edit)edit=widget->findChild<QLineEdit *>();if(!edit||edit->isReadOnly())continue;reveal(edit);value=edit->text();if(unit){unit->clear();for(auto *text:widget->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly))if(text->isVisible())*unit+=text->text();}++count;}require(count==1,"Expected one visible Brouwer orbit field");return value;}
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaBrouwerOrigin");try{
 require(argc==3&&acceptanceWidgetPlatform(),"Expected startup and Brouwer representation");TestSettings settings;const QString type=argv[2];require(type=="BrouwerMeanShort"||type=="BrouwerMeanLong","Unknown Brouwer representation");const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
 const QStringList bodies={"Mercury","Venus","Luna","Mars","Jupiter","Saturn","Uranus","Neptune","Pluto","Sun"};QString script="Earth.Mu = 398600.4418;\nCreate Spacecraft OriginSat;\nOriginSat.CoordinateSystem = EarthMJ2000Eq;\nOriginSat.DisplayStateType = Cartesian;\nOriginSat.X = 7000;\nOriginSat.Y = 0;\nOriginSat.Z = 0;\nOriginSat.VX = 0;\nOriginSat.VY = 7.5;\nOriginSat.VZ = 1;\nCreate Variable Keep;\nKeep = 7;\n";
 for(const auto &body:bodies)script+="Create CoordinateSystem Gate"+body+";\nGate"+body+".Origin = "+body+";\nGate"+body+".Axes = MJ2000Eq;\n";
 script+="Create CoordinateSystem GateSSB;\nGateSSB.Origin = SolarSystemBarycenter;\nGateSSB.Axes = MJ2000Eq;\nBeginMissionSequence;\nKeep = Keep + 2;\n";
 MainWindow window;require(window.initialize(startup),"Brouwer origin initialization failed");window.show();acceptanceView(window);auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Brouwer origin workspace missing");source->setPlainText(script);if(!window.buildScript()){std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Brouwer origin baseline rejected");}source->document()->setModified(false);const int undo=source->document()->availableUndoSteps();auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("OriginSat"));require(sat,"Brouwer spacecraft missing");std::array<double,6> baseline{};const auto state=sat->GetState("Cartesian");for(int i=0;i<6;++i)baseline[i]=state[i];
 auto pristine=[&]{if(source->toPlainText()!=script||source->document()->availableUndoSteps()!=undo||source->document()->isModified()||sat->GetStringParameter("CoordinateSystem")!="EarthMJ2000Eq"||sat->GetStringParameter("DisplayStateType")!="Cartesian"||Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")!=7)return false;const auto current=sat->GetState("Cartesian");for(int i=0;i<6;++i)if(current[i]!=baseline[i])return false;return true;};
 auto open=[&](const QString &name){auto found=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(found.size()==1,"Brouwer resource tree item missing");tree->itemDoubleClicked(found.front(),0);events();require(QTest::qWaitFor([&]{return area->activeSubWindow()&&area->activeSubWindow()->windowTitle().contains(name);},1500),"Brouwer resource activation failed");return area->activeSubWindow();};
 QPointer<QMdiSubWindow> sibling=open("Keep");acceptanceView(*sibling);auto *child=open("OriginSat");auto *panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel,"Brouwer resource editor missing");acceptanceView(*child);auto *frame=panel->findChild<QComboBox *>("resource_CoordinateSystem"),*representation=panel->findChild<QComboBox *>("resource_DisplayStateType"),*backing=panel->findChild<QComboBox *>("spacecraftStateRepresentation");require(frame&&representation&&backing,"Brouwer primary/backing selectors missing");QJsonArray matrix;
 QStringList frames={"EarthMJ2000Eq","EarthMJ2000Ec"};for(const auto &body:bodies)frames.append("Gate"+body);frames.append("GateSSB");
 for(const auto &name:frames){reveal(frame);frame->setCurrentText(name);events();acceptanceView(*child);const bool earth=name.startsWith("Earth");const auto visibleChoices=choices(representation),backingChoices=choices(backing);const QJsonObject context{{"frame",name},{"earth_origin",earth},{"visible_choices",visibleChoices},{"backing_choices",backingChoices},{"pristine",pristine()}};save("current-origin-diagnostic.json",context);require(frame->currentText()==name&&visibleChoices==backingChoices,"Brouwer primary/backing frame choices disagree");require((representation->findText("BrouwerMeanShort")>=0)==earth&&(representation->findText("BrouwerMeanLong")>=0)==earth,"Brouwer representations must be offered only for Earth origins");require(pristine()&&sibling&&sibling->isVisible(),"Brouwer frame preview changed source/model/history or sibling");
  if(earth){reveal(representation);representation->setCurrentText(type);events();acceptanceView(*child);require(representation->currentText()==type&&pristine(),"Earth Brouwer preview failed or committed pending state");const auto prefix=type=="BrouwerMeanShort"?"BrouwerShort":"BrouwerLong";const QStringList fields={"SMA","ECC","INC","RAAN","AOP","MA"},units={"km","","deg","deg","deg","deg"};QJsonArray displayed;for(int i=0;i<6;++i){QString unit;const auto value=fieldText(panel,prefix+fields[i],&unit);bool ok=false;const double numeric=value.toDouble(&ok);require(ok&&std::isfinite(numeric)&&unit==units[i],"Earth Brouwer field label/value/unit mismatch");displayed.append(QJsonObject{{"field",prefix+fields[i]},{"value",numeric},{"unit",unit}});acceptanceView(*child);}reveal(representation);representation->setCurrentText("Cartesian");events();acceptanceView(*child);require(pristine(),"Brouwer browsing changed physical applied state");matrix.append(QJsonObject{{"frame",name},{"earth_origin",true},{"choices",visibleChoices},{"displayed_fields",displayed}});
  }else matrix.append(context);
 }
 // A non-Earth body with Earth-like mu must still obey the named-origin rule.
 auto *mars=Moderator::Instance()->GetConfiguredObject("Mars");const double marsMu=mars->GetRealParameter("Mu");mars->SetRealParameter("Mu",398600.4418);reveal(frame);frame->setCurrentText("EarthMJ2000Eq");frame->setCurrentText("GateMars");events();acceptanceView(*child);require(representation->findText(type)<0&&pristine(),"Earth-like mu incorrectly grants Mars a Brouwer representation");mars->SetRealParameter("Mu",marsMu);
 require(panel->hasChanges(),"Non-Earth frame selection did not remain a pending draft");acceptanceView(*child);bool discarded=false;QTimer::singleShot(0,&window,[&]{auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());require(prompt&&prompt->button(QMessageBox::Discard),"Brouwer pending-close discard prompt missing");acceptanceView(*prompt);discarded=true;prompt->button(QMessageBox::Discard)->click();});require(child->close(),"Brouwer pending editor did not close after Discard");events();require(discarded&&pristine()&&sibling&&sibling->isVisible(),"Brouwer Discard changed applied state/source/history or sibling");
 const QJsonObject proof{{"checkpoint","brouwer-origin-"+type},{"outcome","passed"},{"family",type},{"frame_contexts",matrix},{"frame_count",frames.size()},{"Earth_like_Mars_mu_rejected",true},{"pending_discard_pristine",true},{"facets",QJsonArray{"earth-only-brouwer-selector-policy","primary-backing-choice-synchronization","finite-mean-labels-and-units","source-model-history-sibling-isolation","pending-frame-discard-atomicity"}},{"scope","Thirteen frame previews:two Earth inertial frames,ten other celestial origins and SolarSystemBarycenter;both namespaces obey Earth origin identity even with Earth-like Mars mu. Existing Earth displays checked for finite values/units only. Positive six-field definition,independent mean-element science,all origin/axis/domain combinations,native OS input and full current-build/NASA qualification remain open."}};save("proof.json",proof);std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;}
