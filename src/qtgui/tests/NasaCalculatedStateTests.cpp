#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "SpacePoint.hpp"
#include "CelestialBody.hpp"
#include "A1Mjd.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
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

using Vec=std::array<double,3>;
using State=std::array<double,6>;
static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static Vec add(Vec a,const Vec &b,double scale=1) {for (int i=0;i<3;++i) a[i]+=scale*b[i];return a;}
static Vec scale(Vec a,double s) {for (auto &v:a) v*=s;return a;}
static Vec cross(const Vec &a,const Vec &b) {return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
static double norm(const Vec &v) {return std::hypot(v[0],v[1],v[2]);}
static Vec position(const State &s) {return {s[0],s[1],s[2]};}
static Vec velocity(const State &s) {return {s[3],s[4],s[5]};}
static QJsonArray json(const State &s) {QJsonArray a;for (double x:s) a.append(x);return a;}
static void reveal(QWidget &control)
{
   for (auto *page=&control;page;page=page->parentWidget()) {
      for (auto *tabs:control.window()->findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea *>(page)) scroll->ensureWidgetVisible(&control);
   }
   events();require(control.isVisible() && control.isEnabled(),qPrintable("Calculated-point control not visible/enabled: "+control.objectName()));
}
static QLineEdit *entry(QWidget &owner,const QString &field)
{
   auto *value=owner.findChild<QLineEdit *>("resource_"+field);require(value,qPrintable("Calculated-point field missing: "+field));reveal(*value);return value;
}
static void modal(const std::function<void()> &trigger,const std::function<void(QDialog &)> &inspect)
{
   std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,[&] {auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());try {require(dialog,"Calculated-point modal missing");acceptanceView(*dialog);visited=true;inspect(*dialog);} catch (...) {failure=std::current_exception();if (dialog) dialog->reject();}});
   trigger();if (failure) std::rethrow_exception(failure);require(visited,"Calculated-point modal not inspected");
}
static void pick(QWidget &owner,const QString &field,const QStringList &names,bool accept)
{
   auto *value=entry(owner,field);const auto before=value->text();const auto buttons=value->parentWidget()->findChildren<QPushButton *>();require(buttons.size()==1,"Displayed point field picker missing");reveal(*buttons.front());
   modal([&]{buttons.front()->click();},[&](QDialog &dialog) {
      if (field=="BodyNames") {
         auto *list=dialog.findChild<QListWidget *>("resourceSelectionList");auto *clear=dialog.findChild<QPushButton *>("resourceClearSelection");require(list && clear,"Barycenter member picker missing");clear->click();
         require(list->findItems("Ceres",Qt::MatchExactly).size()==1 && list->findItems("SolarSystemBarycenter",Qt::MatchExactly).isEmpty() && list->findItems("Center",Qt::MatchExactly).isEmpty(),"Member picker omitted user body or includes calculated points");
         for (const auto &name:names) {const auto matches=list->findItems(name,Qt::MatchExactly);require(matches.size()==1,"Requested barycenter member missing");list->scrollToItem(matches.front());matches.front()->setCheckState(Qt::Checked);}
         acceptanceView(dialog);dialog.findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
      } else {
         auto *input=dynamic_cast<QInputDialog *>(&dialog);require(input && names.size()==1 && input->comboBoxItems().contains(names.front()),"Requested primary/secondary absent from displayed picker");
         require(!input->comboBoxItems().contains("SolarSystemBarycenter"),"Libration picker offers forbidden built-in barycenter");input->setTextValue(names.front());acceptanceView(dialog);if (accept) input->accept();else input->reject();
      }
   });
   if (!accept) require(value->text()==before,"Calculated-point picker Cancel changed pending field");
   else if (field=="BodyNames") {auto actual=splitResourceReferences(value->text()),expected=names;actual.sort();expected.sort();require(actual==expected,"Member picker selection differs");value->setText(names.join(", "));}
   else require(value->text()==names.front(),"Libration picker selected wrong reference");
}
static State bodyState(const QString &name,double epoch)
{
   auto *body=Moderator::Instance()->GetSolarSystemInUse()->GetBody(name.toStdString());require(body,"Independent oracle body missing");const auto v=body->GetMJ2000State(A1Mjd(epoch));State s{};for (int i=0;i<6;++i) s[i]=v[i];return s;
}
static double gm(const QString &name)
{
   auto *body=Moderator::Instance()->GetSolarSystemInUse()->GetBody(name.toStdString());require(body,"Oracle mass source missing");const auto value=body->GetGravitationalConstant();require(std::isfinite(value) && value>0,"Oracle mass source is invalid");return value;
}
static double totalGm(const QStringList &members) {double sum=0;for (const auto &name:members) sum+=gm(name);return sum;}
static State weighted(const QStringList &members,double epoch)
{
   // Ratios of GM avoid calling Barycenter::GetMass or GetMJ2000State.
   State result{};const double sum=totalGm(members);require(sum>0,"Empty independent barycenter");
   for (const auto &name:members) {const auto s=bodyState(name,epoch);const double weight=gm(name)/sum;for (int i=0;i<6;++i) result[i]+=weight*s[i];}return result;
}
static double equilibrium(double x,double mu)
{
   return x-mu-(1-mu)*x/std::pow(std::abs(x),3)-mu*(x-1)/std::pow(std::abs(x-1),3);
}
static std::array<double,2> pointCoordinates(const QString &point,double mu)
{
   if (point=="L4" || point=="L5") return {0.5,(point=="L4" ? 1 : -1)*std::sqrt(3.)/2};
   // Bracket the physical equilibrium directly. GMAT uses Newton iteration
   // on three separate quintics in gamma; this oracle uses neither.
   double lo=point=="L3" ? -4 : point=="L2" ? 1+1e-12 : 1e-12;
   double hi=point=="L3" ? -1e-12 : point=="L2" ? 4 : 1-1e-12;
   require(equilibrium(lo,mu)<0 && equilibrium(hi,mu)>0,"Libration equilibrium bracket invalid");
   for (int iteration=0;iteration<160;++iteration) {const double mid=(lo+hi)/2;if (mid==lo || mid==hi) break;if (equilibrium(mid,mu)>0) hi=mid;else lo=mid;}
   const double x=(lo+hi)/2;require(std::abs(equilibrium(x,mu))<1e-9,"Independent libration root did not converge");return {x,0};
}
static Vec offset(const Vec &r,const Vec &v,const std::array<double,2> &xy)
{
   const auto h=cross(r,v);require(norm(r)>0 && norm(h)>0,"Degenerate independent libration geometry");const auto transverse=cross(scale(h,1/norm(h)),scale(r,1/norm(r)));
   return add(scale(r,xy[0]),scale(transverse,norm(r)*xy[1]));
}
static State libration(const QString &point,const State &p,const State &s,double mu)
{
   const auto r=add(position(s),position(p),-1),v=add(velocity(s),velocity(p),-1);const auto xy=pointCoordinates(point,mu);
   const auto rOffset=offset(r,v,xy);constexpr double h=16;
   // A five-point derivative of independently constructed positions under
   // locally linear relative motion checks the documented zero-acceleration
   // model without copying GMAT's RDot/axis derivative implementation.
   const auto m2=offset(add(r,v,-2*h),v,xy),m1=offset(add(r,v,-h),v,xy),p1=offset(add(r,v,h),v,xy),p2=offset(add(r,v,2*h),v,xy);
   State answer{};for (int i=0;i<3;++i) {answer[i]=p[i]+rOffset[i];answer[i+3]=p[i+3]+(m2[i]-8*m1[i]+8*p1[i]-p2[i])/(12*h);}return answer;
}
static QMap<QString,QString> properties(const QString &name)
{
   auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(object,"Configured calculated point missing");QMap<QString,QString> values;for (const auto &field:resourceProperties(*object)) values[field.name]=field.value;return values;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaCalculatedStates");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup, calculated-point case and supported GUI platform");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Calculated-point output directory missing");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const QString name=QString::fromLocal8Bit(argv[2]);const bool bary=name.startsWith("Barycenter");
      const auto point=name.left(2);const auto family=bary ? QString("barycenter") : name.mid(2);require(bary || (QStringList{"L1","L2","L3","L4","L5"}.contains(point) && QStringList{"Celestial","BaryPrimary","BarySecondary","Defaults"}.contains(family)),"Unknown calculated-point case");require(!bary || name=="BarycenterMixture" || name=="BarycenterUserOnly" || name=="BarycenterDefault","Unknown barycenter case");
      const QString resource=bary ? "Center" : "Probe",type=bary ? "Barycenter" : "LibrationPoint";const QStringList centerMembers{"Earth","Luna","Ceres"};
      const QString primary=family=="BaryPrimary" ? "Center" : (family=="BarySecondary" || family=="Defaults") ? "Sun" : "Earth",secondary=family=="BaryPrimary" ? "Mars" : family=="BarySecondary" ? "Center" : family=="Defaults" ? "Earth" : "Luna";
      const QStringList members=name=="BarycenterUserOnly" ? QStringList{"Ceres"} : name=="BarycenterDefault" ? QStringList{"Earth","Luna"} : centerMembers,editedMembers=name=="BarycenterUserOnly" ? QStringList{"Ceres","Earth"} : QStringList{"Ceres","Mars","Earth"};
      const auto kernel=QFileInfo("../data/planetary_ephem/spk/ceres_1900_2100.bsp").canonicalFilePath();require(!kernel.isEmpty(),"Shipped Ceres ephemeris missing");
      const QString definition="% Calculated-point state acceptance Δ\nCreate Asteroid Ceres;\nCeres.NAIFId = 2000001;\nCeres.OrbitSpiceKernelName = {'"+kernel+"'};\nCeres.Mu = 64;\nEarth.Mu = 400000;\nLuna.Mu = 5000;\nMars.Mu = 43000;\nSun.Mu = 130000000000;\n";
      const QString keep="Create Variable Keep;\nKeep = 13; % preserve unrelated initializer\n",mission="BeginMissionSequence;\nKeep = Keep + 1; % preserve mission Δ\n";
      const auto baseline=definition+(bary ? QString() : "Create Barycenter Center;\nCenter.BodyNames = {Earth, Luna, Ceres};\n")+keep+mission;
      MainWindow window;require(window.initialize(startup),"Calculated-point runtime initialization failed");window.show();auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(editor && tree && area,"Calculated-point workspace missing");
      auto build=[&] {if (!window.buildScript()) {std::cerr<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()<<std::endl;throw std::runtime_error("Calculated-point fixture build failed");}};
      editor->setPlainText(baseline);build();const auto initialUndo=editor->document()->availableUndoSteps();
      auto unchanged=[&] {require(editor->toPlainText()==baseline && editor->document()->availableUndoSteps()==initialUndo && !Moderator::Instance()->GetConfiguredObject(resource.toStdString()),"Calculated-point draft changed source/model/history");};
      auto fill=[&](QWidget &owner,bool cancelPicker) {
         if (bary) {if (cancelPicker) pick(owner,"BodyNames",members,false);if (name!="BarycenterDefault") pick(owner,"BodyNames",members,true);}
         else {if (cancelPicker) pick(owner,"Primary",{primary},false);if (family!="Defaults") {pick(owner,"Primary",{primary},true);pick(owner,"Secondary",{secondary},true);}auto *selector=owner.findChild<QComboBox *>("resource_Point");if (!selector || selector->isEditable() || selector->count()!=5) std::cerr<<"Displayed Point selector: pointer="<<selector<<" editable="<<(selector ? selector->isEditable() : false)<<" count="<<(selector ? selector->count() : -1)<<std::endl;require(selector && !selector->isEditable() && selector->count()==5,"Five typed libration choices missing");reveal(*selector);selector->setCurrentText(point);require(selector->currentText()==point,"Libration point selection failed");}events();
      };
      int invalidChecks=0;
      auto create=[&](bool accept) {
         auto *action=window.findChild<QAction *>("createResource");require(action && action->isEnabled(),"New calculated-point action unavailable");
         modal([&]{action->trigger();},[&](QDialog &dialog) {
            auto *types=dialog.findChild<QComboBox *>("resourceType");auto *newName=dialog.findChild<QLineEdit *>("resourceName");require(types && types->findText(type)>=0 && newName,"Calculated-point creation controls missing");newName->setText(resource);types->setCurrentText(type);events();
            if (bary) require(splitResourceReferences(entry(dialog,"BodyNames")->text())==QStringList{"Earth","Luna"},"Barycenter draft defaults differ from documented Earth/Luna membership");
            else {auto *selector=dialog.findChild<QComboBox *>("resource_Point");require(entry(dialog,"Primary")->text()=="Sun" && entry(dialog,"Secondary")->text()=="Earth" && selector && selector->currentText()=="L1","Libration draft defaults differ from documented Sun/Earth/L1");}
            fill(dialog,true);unchanged();acceptanceView(dialog);
            auto *buttons=dialog.findChild<QDialogButtonBox *>("resourceCreationButtons");require(buttons,"Calculated-point creation buttons missing");
            if (accept) {auto *value=entry(dialog,bary ? "BodyNames" : "Secondary");const auto valid=value->text();value->setText(bary ? "Ceres, Ceres" : primary);buttons->button(QDialogButtonBox::Ok)->click();events();++invalidChecks;require(dialog.isVisible() && !dialog.findChild<QLabel *>("resourceCreationStatus")->text().isEmpty() && value->text()==(bary ? "Ceres, Ceres" : primary),"Invalid calculated-point creation accepted or lost diagnostic/input");unchanged();value->setText(valid);}
            buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();if (accept && dialog.result()!=QDialog::Accepted) std::cerr<<dialog.findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<std::endl;require(dialog.result()==(accept ? QDialog::Accepted : QDialog::Rejected),"Valid calculated-point Create/Cancel failed");
         });
      };
      create(false);unchanged();create(true);events();const auto created=editor->toPlainText();const auto createdValues=properties(resource);require(created.endsWith(mission) && created.contains(keep) && created.contains("Ceres.Mu = 64;"),"Calculated-point creation changed unrelated source");
      ResourceEditor *panel=nullptr;QMdiSubWindow *child=nullptr;
      auto open=[&](bool trigger) {if (trigger) {const auto items=tree->findItems(resource,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Calculated-point tree entry missing");tree->itemDoubleClicked(items.front(),0);events();}QList<QMdiSubWindow *> matches;for (auto *candidate:area->subWindowList()) if (candidate->property("resourceName").toString()==resource && dynamic_cast<ResourceEditor *>(candidate->widget())) matches.append(candidate);require(matches.size()==1,"Actual calculated-point editor missing or duplicated");child=matches.front();panel=dynamic_cast<ResourceEditor *>(child->widget());child->showNormal();area->setActiveSubWindow(child);child->raise();events();acceptanceView(*child);require(!panel->hasChanges(),"Fresh point editor has pending changes");};
      open(false);require(child->close(),"Created point editor did not close");events();editor->undo();require(editor->toPlainText()==baseline,"Calculated-point creation Undo changed unrelated source");build();require(!Moderator::Instance()->GetConfiguredObject(resource.toStdString()),"Creation Undo retained calculated point");editor->redo();require(editor->toPlainText()==created,"Calculated-point creation Redo not exact");build();require(properties(resource)==createdValues,"Creation Redo changed calculated-point properties");
      auto persist=[&](const QString &label,const QString &source,const QMap<QString,QString> &values) {const auto path=files.filePath(label+" "+name+" Δ.script");require(window.saveScriptTo(path) && window.loadScript(path),"Calculated-point Unicode save/reopen failed");build();QFile file(path);require(file.open(QIODevice::ReadOnly) && QString::fromUtf8(file.readAll())==source && editor->toPlainText()==source && properties(resource)==values,"Calculated-point persistence changed source/properties");};
      double maxPosition=0,maxVelocity=0;QJsonArray samples;
      auto results=[&](const QString &phase,const QStringList &memberNames,const QString &pointName) {
         if (window.runMission()!=MainWindow::RunResult::Completed) {std::cerr<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()<<std::endl;throw std::runtime_error("Calculated-point mission failed");}
         auto *configured=Moderator::Instance()->GetConfiguredObject(resource.toStdString());auto *runtime=dynamic_cast<SpacePoint *>(Moderator::Instance()->GetInternalObject(resource.toStdString()));require(runtime && runtime!=configured,"Calculated-point result is not a fresh sandbox object");
         if (bary) {const auto actual=runtime->GetStringArrayParameter("BodyNames");QStringList names;for (const auto &item:actual) names.append(QString::fromStdString(item));require(names==memberNames,"Runtime barycenter lost ordered membership");}
         else require(runtime->GetStringParameter("Primary")==primary.toStdString() && runtime->GetStringParameter("Secondary")==secondary.toStdString() && runtime->GetStringParameter("Point")==pointName.toStdString(),"Runtime libration configuration differs");
         auto *keepValue=Moderator::Instance()->GetInternalObject("Keep");require(keepValue && keepValue->GetRealParameter("Value")==14,"Calculated-point known mission result differs");
         require(gm("Earth")==400000 && gm("Luna")==5000 && gm("Ceres")==64 && gm("Mars")==43000 && gm("Sun")==130000000000.,"Known synthetic GM constants changed");
         for (double epoch:{21545.,21545.5,21546.,21810.25}) {
            State expected{};QJsonObject sample{{"phase",phase},{"epoch_a1mjd",epoch}};
            if (bary) {expected=weighted(memberNames,epoch);sample["members"]=QJsonArray::fromStringList(memberNames);QJsonArray inputs;for (const auto &member:memberNames) inputs.append(QJsonObject{{"name",member},{"gm",gm(member)},{"state",json(bodyState(member,epoch))}});sample["member_states"]=inputs;}
            else {const auto p=primary=="Center" ? weighted(centerMembers,epoch) : bodyState(primary,epoch),s=secondary=="Center" ? weighted(centerMembers,epoch) : bodyState(secondary,epoch);const double pm=primary=="Center" ? totalGm(centerMembers) : gm(primary),sm=secondary=="Center" ? totalGm(centerMembers) : gm(secondary),mu=sm/(pm+sm);expected=libration(pointName,p,s,mu);sample["primary_state"]=json(p);sample["secondary_state"]=json(s);sample["mass_fraction"]=mu;sample["point"]=pointName;}
            const auto actual=runtime->GetMJ2000State(A1Mjd(epoch));State observed{};for (int i=0;i<6;++i) {observed[i]=actual[i];require(std::isfinite(actual[i]),"Calculated-point state is nonfinite");const double error=std::abs(actual[i]-expected[i]);if (i<3) maxPosition=std::max(maxPosition,error);else maxVelocity=std::max(maxVelocity,error);if (error>(i<3 ? 2e-6 : 2e-8)) std::cerr<<name.toStdString()<<" "<<phase.toStdString()<<" component "<<i<<" actual "<<actual[i]<<" expected "<<expected[i]<<" error "<<error<<std::endl;require(error<=(i<3 ? 2e-6 : 2e-8),"Calculated-point state differs from independent position/velocity oracle");}sample["actual_state"]=json(observed);sample["expected_state"]=json(expected);samples.append(sample);
         }
      };
      persist("created",created,createdValues);results("created",members,point);open(true);const auto undo=editor->document()->availableUndoSteps();const QString editedPoint="L"+QString::number(point.mid(1).toInt()%5+1);
      if (bary) {pick(*panel,"BodyNames",editedMembers,true);entry(*panel,"BodyNames")->clear();}else entry(*panel,"Primary")->setText("SolarSystemBarycenter");
      panel->requestApply();events();++invalidChecks;require(panel->hasChanges() && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && editor->toPlainText()==created && properties(resource)==createdValues && editor->document()->availableUndoSteps()==undo,"Invalid calculated-point Apply changed source/model/history or omitted diagnostic");
      if (bary) entry(*panel,"BodyNames")->setText(editedMembers.join(", "));else {entry(*panel,"Primary")->setText(primary);auto *selector=panel->findChild<QComboBox *>("resource_Point");require(selector,"Displayed point selector missing after correction");reveal(*selector);selector->setCurrentText(editedPoint);}acceptanceView(*child);require(editor->toPlainText()==created && properties(resource)==createdValues,"Pending calculated-point edit escaped to model/source");panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges(),"Corrected point Apply failed");
      const auto applied=editor->toPlainText();const auto appliedValues=properties(resource);require(applied.endsWith(mission) && applied.contains(keep),"Calculated-point Apply changed unrelated source");require(child->close(),"Applied calculated-point editor did not close");events();editor->undo();require(editor->toPlainText()==created,"Point Apply Undo not exact");build();require(properties(resource)==createdValues,"Point Apply Undo changed original properties");editor->redo();require(editor->toPlainText()==applied,"Point Apply Redo not exact");build();require(properties(resource)==appliedValues,"Point Apply Redo changed properties");persist("applied",applied,appliedValues);results("applied",editedMembers,editedPoint);
      QJsonArray facets{"documented-creation-defaults","actual-new-and-resource-editors","visible-typed-reference-pickers","picker-and-create-cancel-isolation","invalid-create-apply-correction","pending-source-model-history-isolation","exact-creation-and-edit-undo-redo","unicode-save-reopen","fresh-sandbox-ordered-references","four-epoch-six-component-states","known-mission-result"};
      if (bary || family.startsWith("Bary")) facets.append("independent-gm-weighted-barycenter");
      if (!bary) {facets.append("independent-bracketed-equilibrium");facets.append("independent-local-linear-velocity-derivative");}
      const QJsonObject proof{{"checkpoint","calculated-state."+name},{"case",name},{"type",type},{"point",bary ? QString() : point},{"primary",bary ? QString() : primary},{"secondary",bary ? QString() : secondary},{"primary_kind",family=="BaryPrimary" ? "Barycenter" : "CelestialBody"},{"secondary_kind",family=="BarySecondary" ? "Barycenter" : "CelestialBody"},{"user_body_in_membership",bary ? members.contains("Ceres") : family.startsWith("Bary")},{"position_tolerance_km",2e-6},{"velocity_tolerance_km_per_s",2e-8},{"maximum_position_error_km",maxPosition},{"maximum_velocity_error_km_per_s",maxVelocity},{"invalid_rejections_checked",invalidChecks},{"epochs_per_phase",4},{"samples",samples},{"outcome","passed"},
         {"facets",facets},
         {"scope","Specified calculated-point configurations and four epochs using shipped ephemeris inputs. Libration velocity checks the documented zero-relative-acceleration model, not the full derivative of changing ephemerides. No independent ephemeris/GPU/native-input/platform/stress qualification."}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
