#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "CelestialBody.hpp"
#include "SolarSystem.hpp"
#include "A1Mjd.hpp"
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
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QTimer>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
using V=std::array<double,3>;
using State=std::array<double,6>;
using Matrix=std::array<V,3>;
using Axes=std::array<QString,3>;
static const State input{1000,-200,300,1.25,-.5,.75};
static const State anchor{1000,2000,-500,1,-.3,.2};
static const State reference{8000,800,400,1.7,6.5,-1};
static const QStringList components{"X","Y","Z","VX","VY","VZ"};
static const QStringList variables{"RefX","RefY","RefZ","RefVX","RefVY","RefVZ"};
static V add(V a,const V &b) {for (int i=0;i<3;++i) a[i]+=b[i];return a;}
static V scale(V a,double s) {for (auto &x:a) x*=s;return a;}
static double dot(const V &a,const V &b) {double r=0;for (int i=0;i<3;++i) r+=a[i]*b[i];return r;}
static V cross(const V &a,const V &b) {return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
static V unit(const V &v) {const double n=std::sqrt(dot(v,v));require(n>1e-12,"Independent reference vector is degenerate");return scale(v,1/n);}
static QJsonArray json(const State &v) {QJsonArray a;for (auto x:v) a.append(x);return a;}
static QJsonArray json(const Matrix &m) {QJsonArray a;for (auto v:m) {QJsonArray row;for (double x:v) row.append(x);a.append(row);}return a;}
static QJsonArray json(const Axes &v) {QJsonArray a;for (const auto &x:v) a.append(x);return a;}
struct Basis {Matrix rows{},rates{};};
static Basis basis(const Axes &axes,bool reverse)
{
   V r{},v{};for (int i=0;i<3;++i) {r[i]=reference[i]-anchor[i];v[i]=reference[i+3]-anchor[i+3];}
   if (reverse) {r=scale(r,-1);v=scale(v,-1);}
   const V radial=unit(r),velocity=unit(v),normal=unit(cross(r,v));
   // NASA's guide explicitly neglects all acceleration contributions. Thus
   // V and N have zero derivative; the R derivative still uses known velocity.
   const V radialRate=scale(add(v,scale(radial,-dot(radial,v))),1/std::sqrt(dot(r,r)));
   Basis result;int missing=-1;
   for (int i=0;i<3;++i) {
      if (axes[i].isEmpty()) {missing=i;continue;}
      const auto key=axes[i].right(1);const double sign=axes[i].startsWith('-') ? -1:1;
      result.rows[i]=scale(key=="R" ? radial : key=="V" ? velocity : normal,sign);
      result.rates[i]=key=="R" ? scale(radialRate,sign) : V{};
   }
   require(missing>=0,"Independent reference basis needs one derived axis");
   const int a=(missing+1)%3,b=(missing+2)%3;
   result.rows[missing]=cross(result.rows[a],result.rows[b]);
   result.rates[missing]=add(cross(result.rates[a],result.rows[b]),cross(result.rows[a],result.rates[b]));
   return result;
}
static QTableWidgetItem *cell(QWidget *owner,const QString &key)
{
   auto *table=owner->findChild<QTableWidget *>("resourceProperties");if (!table) return nullptr;
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==key) return table->item(row,1);
   return nullptr;
}
static QString field(QWidget *owner,const QString &key)
{
   if (auto *c=owner->findChild<QComboBox *>("resource_"+key)) return c->currentText();
   if (auto *e=owner->findChild<QLineEdit *>("resource_"+key)) return e->text();
   if (auto *v=cell(owner,key)) return v->text();throw std::runtime_error(("Missing object-axis field "+key).toStdString());
}
static void setField(QWidget *owner,const QString &key,const QString &value)
{
   if (auto *c=owner->findChild<QComboBox *>("resource_"+key)) {require(c->isEnabled() && c->findText(value)>=0,"Object-axis choice absent or disabled");c->setCurrentText(value);}
   else if (auto *e=owner->findChild<QLineEdit *>("resource_"+key)) {require(e->isEnabled() && !e->isReadOnly(),"Object-axis field not editable");e->setText(value);}
   else if (auto *v=cell(owner,key)) {require(v->flags() & Qt::ItemIsEditable,"Object-axis table field not editable");v->setText(value);}
   else throw std::runtime_error(("Missing object-axis control "+key).toStdString());
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaObjectPicker");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and object-axis case");
      TestSettings settings;QTemporaryDir files;require(files.isValid(),"Object-axis fixture directory missing");
      const QString mode=argv[2];require(QStringList{"Primary","Secondary","Unknown","Origin"}.contains(mode),"Unknown object reference-picker case");
      const QString initialOrigin=mode=="Origin" ? "Reference":"Earth";
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto report=files.filePath("Object axes Δ.txt");
      QString initial="% Retain object referenced acceptance Δ\nCreate Spacecraft Anchor Reference Sat;\n";
      for (const auto &name:QStringList{"Anchor","Reference","Sat"}) {
         const State state=name=="Anchor" ? anchor : name=="Reference" ? reference : input;
         initial+=name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545';\n"+name+".DisplayStateType = Cartesian;\n";
         for (int i=0;i<6;++i) initial+=name+"."+components[i]+" = "+QString::number(state[i],'g',17)+";\n";
      }
      initial+="Create Barycenter Center;\nCenter.BodyNames = {Earth, Luna};\nCreate LibrationPoint LP;\nLP.Primary = Earth;\nLP.Secondary = Luna;\nLP.Point = L1;\nCreate GroundStation Site;\nSite.CentralBody = Earth;\nCreate Variable "+variables.join(' ')+" Keep;\n";
      for (int i=0;i<6;++i) initial+=variables[i]+" = "+QString::number(reference[i],'g',17)+"; % reference input\n";
      initial+="Keep = 41; % preserve initializer\nCreate ReportFile States;\nStates.Filename = '"+report+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\nBeginMissionSequence;\n";
      for (double epoch:{21545.,21545.5,21546.25}) {
         for (const auto &name:QStringList{"Anchor","Reference","Sat"}) {
            initial+=name+".A1ModJulian = "+QString::number(epoch,'g',17)+";\n";
            for (int i=0;i<6;++i) initial+=name+".EarthMJ2000Eq."+components[i]+" = "+(name=="Reference" ? variables[i] : QString::number((name=="Anchor" ? anchor : input)[i],'g',17))+";\n";
         }
         initial+="Report States Sat.A1ModJulian";
         for (const auto &name:QStringList{"Sat","Reference","Anchor","Sat"}) for (const auto &c:components) initial+=" "+name+".EarthMJ2000Eq."+c;
         initial+=";\n";
      }
      initial+="Keep = Keep + 1; % preserve mission\n";
      MainWindow window;require(window.initialize(startup),"Object-axis runtime initialization failed");window.show();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");
      auto *area=window.findChild<QMdiArea *>("workspace");auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");
      require(source && tree && area && messages,"Object-axis workspace missing");source->setPlainText(initial);require(window.buildScript(),"Object-axis baseline failed");
      const auto initialUndo=source->document()->availableUndoSteps();int cancels=0,invalid=0,runtimeFailures=0,phase=0;
      double maxp=0,maxv=0;QJsonArray samples,failures,configurations,pickerEvents;int pickerCancels=0;
      Axes currentAxes{"R","","N"};
      const Axes positive=currentAxes;
      auto configured=[&] {return Moderator::Instance()->GetConfiguredObject("CreatedFrame");};
      auto unchanged=[&] {require(!configured() && source->toPlainText()==initial && source->document()->availableUndoSteps()==initialUndo,"Object-axis draft changed registration/source/history");};
      auto view=[&](QMdiSubWindow *child) {
         if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);child->showNormal();events();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();require(area->activeSubWindow()==child,"Object-axis view is obscured");}
         acceptanceView(*child);
      };
      auto pick=[&](ResourceEditor *owner,const QString &name,const QString &value) {
         auto *line=owner->findChild<QLineEdit *>("resource_"+name);
         require(line && line->isVisible() && line->isEnabled(),"Actual reference text field unavailable");
         QPushButton *button=nullptr;
         for (auto *candidate:line->parentWidget()->findChildren<QPushButton *>(QString(),Qt::FindDirectChildrenOnly))
            if (candidate->text()=="Select…") button=candidate;
         require(button && button->isVisible() && button->isEnabled(),"Actual visible reference Select action unavailable");
         const auto before=line->text(),sourceBefore=source->toPlainText();const auto undo=source->document()->availableUndoSteps();
         const QString modelBefore=configured() ? QString::fromStdString(configured()->GetGeneratingString(Gmat::SHOW_SCRIPT)) : QString();
         auto invoke=[&](bool accept) {
            // Paint before scheduling: the callback must run inside the actual
            // QInputDialog loop, never during an earlier exposure checkpoint.
            acceptanceView(*owner->window());std::exception_ptr error;
            QTimer::singleShot(0,owner,[&] {
               auto *dialog=owner->findChild<QInputDialog *>();
               try {
                  require(dialog && dialog->isComboBoxEditable(),"Actual editable single-reference picker missing");
                  require(dialog->windowTitle()=="Select "+name,"Reference picker title does not match field");
                  const auto names=dialog->comboBoxItems();
                  for (const auto &required:QStringList{"Anchor","Reference","Earth","Center","LP","Site"}) require(names.contains(required),"Reference picker omits an allowed SpacePoint type");
                  for (const auto &excluded:QStringList{"States","Keep","EarthMJ2000Eq","CreatedFrame"}) require(!names.contains(excluded),"Reference picker includes a non-SpacePoint resource");
                  dialog->setTextValue(value);acceptanceView(*dialog);
                  pickerEvents.append(QJsonObject{{"field",name},{"accepted",accept},{"value",value},{"choices",QJsonArray::fromStringList(names)}});
                  if (accept) dialog->accept(); else {dialog->reject();++pickerCancels;}
               } catch (...) {error=std::current_exception();if (dialog) dialog->reject();}
            });button->click();if (error) std::rethrow_exception(error);
         };
         invoke(false);require(line->text()==before,"Reference picker Cancel changed its pending field");
         invoke(true);require(line->text()==value,"Reference picker acceptance lost selected/typed value");
         require(source->toPlainText()==sourceBefore && source->document()->availableUndoSteps()==undo,"Reference picker committed before parent Apply/Create");
         require(!configured() || QString::fromStdString(configured()->GetGeneratingString(Gmat::SHOW_SCRIPT))==modelBefore,"Reference picker mutated the configured coordinate system");
      };
      auto axes=[&](ResourceEditor *parent,const Axes &selection,bool reversed,bool accept,const QString &unknown={}) {
         auto *button=parent->findChild<QPushButton *>("editCoordinateAxes");auto *tabs=parent->findChild<QTabWidget *>("resourceEditorTabs");
         require(button && button->isEnabled() && tabs,"Actual object-axis action unavailable");
         for (int i=0;i<tabs->count();++i) if (tabs->widget(i)->isAncestorOf(button)) tabs->setCurrentIndex(i);
         events();acceptanceView(*parent->window());require(button->isVisible(),"Object axes action not reachable");
         std::exception_ptr error;QTimer::singleShot(0,parent,[&] {
            auto *dialog=parent->findChild<QDialog *>("coordinateAxesDialog");
            try {
               require(dialog,"Actual object-axis dialog missing");auto *type=dialog->findChild<QComboBox *>("coordinateAxisType");
               require(type && type->findText("ObjectReferenced")>=0,"ObjectReferenced selector missing");type->setCurrentText("ObjectReferenced");events();
               ResourceEditor *nested=nullptr;for (auto *widget:dialog->findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<ResourceEditor *>(widget);candidate && candidate->isVisible()) nested=candidate;require(nested,"Actual nested reference form missing");
               for (const auto &name:QStringList{"Primary","Secondary"}) {
                  const QString value=unknown==name ? "Missing "+name+" Ω" : name=="Primary" ? (reversed ? "Reference":"Anchor") : (reversed ? "Anchor":"Reference");
                  if (mode==name || mode=="Unknown" || mode=="Origin") pick(nested,name,value);else setField(dialog,name,value);
               }
               for (int i=0;i<3;++i) setField(dialog,QString("XYZ")[i]+QString("Axis"),selection[i]);
               require(!dialog->findChild<QLineEdit *>("resource_Epoch"),"ObjectReferenced exposes an irrelevant Epoch");acceptanceView(*dialog);
               if (accept) {dialog->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Apply)->click();events();require(dialog->result()==QDialog::Accepted,"Nested object axes OK failed");}
               else {dialog->reject();++cancels;}
            } catch (...) {error=std::current_exception();if (dialog) dialog->reject();}
         });button->click();if (error) std::rethrow_exception(error);
      };
      // Composite constraints deliberately belong to the parent, preserving
      // the existing ability to keep and correct incomplete nested drafts.
      auto create=[&](bool commit) {
         std::exception_ptr error;QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(dialog,"Actual object-axis creator missing");dialog->findChild<QComboBox *>("resourceType")->setCurrentText("CoordinateSystem");dialog->findChild<QLineEdit *>("resourceName")->setText("CreatedFrame");
               ResourceEditor *panel=nullptr;for (auto *widget:dialog->findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<ResourceEditor *>(widget);candidate && candidate->isVisible()) panel=candidate;
               require(panel,"Object-axis creator form missing");if (mode=="Origin") pick(panel,"Origin",initialOrigin);else setField(panel,"Origin",initialOrigin);axes(panel,currentAxes,false,false);require(field(panel,"Axes")=="MJ2000Eq","Nested object-axis Cancel changed parent");unchanged();
               auto *ok=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);
               if (mode=="Unknown" && commit) for (const auto &name:QStringList{"Primary","Secondary"}) {
                  axes(panel,currentAxes,false,true,name);ok->click();events();
                  const auto diagnostic=dialog->findChild<QLabel *>("resourceCreationStatus")->text();
                  require(dialog->isVisible() && dialog->result()!=QDialog::Accepted && !diagnostic.isEmpty(),"Unknown picked reference accepted at Create or lacks diagnostic");
                  unchanged();++invalid;acceptanceView(*dialog);
               }
               axes(panel,currentAxes,false,true);unchanged();
               if (!commit) {dialog->reject();++cancels;return;}
               acceptanceView(*dialog);ok->click();events();if (dialog->result()!=QDialog::Accepted) std::cerr<<dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<std::endl;require(dialog->result()==QDialog::Accepted,"Valid object-axis Create failed");
            } catch (...) {error=std::current_exception();if (dialog) dialog->reject();}
         });window.findChild<QAction *>("createResource")->trigger();if (error) std::rethrow_exception(error);
      };
      create(false);unchanged();create(true);require(configured() && configured()->GetStringParameter("Axes")=="ObjectReferenced","Created object-axis family differs");
      QString stripped=source->toPlainText();stripped.remove(QRegularExpression("^(?:Create CoordinateSystem CreatedFrame;|(?:GMAT[ \\t]+)?CreatedFrame\\.[^;\\n]*;)[ \\t]*\\n",QRegularExpression::MultilineOption));require(stripped==initial,"Object-axis Create changed unrelated source");
      for (auto *child:area->subWindowList()) if (child->property("resourceName").toString()=="CreatedFrame") {require(child->close(),"Clean object-axis creator did not close");events();break;}
      // Replace only each Report's final six expressions, after GUI creation.
      {QTextCursor group(source->document());group.beginEditBlock();const QRegularExpression pattern("(Report States[^;\\n]*Anchor\\.EarthMJ2000Eq\\.VZ )((?:Sat\\.EarthMJ2000Eq\\.[A-Z]+ ?){6});");QString text=source->toPlainText();auto matches=pattern.globalMatch(text);int count=0;QVector<QPair<int,QPair<int,QString>>> edits;while (matches.hasNext()) {const auto m=matches.next();QString tail=m.captured(2);tail.replace("Sat.EarthMJ2000Eq.","Sat.CreatedFrame.");edits.append({static_cast<int>(m.capturedStart(2)),{static_cast<int>(m.capturedLength(2)),tail}});++count;}require(count==3,"Object-axis report binding count differs");for (auto i=edits.crbegin();i!=edits.crend();++i) {QTextCursor cursor(source->document());cursor.setPosition(i->first);cursor.setPosition(i->first+i->second.first,QTextCursor::KeepAnchor);cursor.insertText(i->second.second);}group.endEditBlock();}require(window.buildScript(),"GUI-created object frame did not build");
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &data) {QFile file(QDir(evidence.isEmpty() ? QDir::currentPath() : evidence).filePath(name));require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Cannot retain object-axis evidence");};
      auto run=[&](const Axes &selection,bool reversed,const QString &origin,const QString &label) {
         messages->clear();const auto result=window.runMission();events();save(QString::number(phase)+"-"+label+".script",source->toPlainText().toUtf8());save(QString::number(phase)+"-"+label+"-messages.txt",messages->toPlainText().toUtf8());if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Object-axis report mission failed");
         QFile file(report);require(file.open(QIODevice::ReadOnly),"Object-axis report missing");const auto raw=file.readAll();save(QString::number(phase)+"-"+label+"-states.txt",raw);const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==75,"Object-axis report needs three dates and four states");const Basis model=basis(selection,reversed);
         for (int row=0;row<3;++row) {
            bool ok=false;const double epoch=words[row*25].toDouble(&ok);require(ok && std::abs(epoch-std::array<double,3>{21545,21545.5,21546.25}[row])<=1e-10,"Object-axis report clock differs");
            for (int group=0;group<3;++group) for (int i=0;i<6;++i) {const double value=words[row*25+1+group*6+i].toDouble(&ok);require(ok && std::abs(value-(group==0 ? input : group==1 ? reference : anchor)[i])<=1e-8,"Object-axis known subject/reference inputs changed");}
            State offset{};QJsonObject originInputs;
            if (origin=="Reference") offset=reference;
            else if (origin=="Center") {
               auto *solar=Moderator::Instance()->GetSolarSystemInUse();auto *earth=solar->GetBody("Earth"),*moon=solar->GetBody("Luna");const auto e=earth->GetMJ2000State(A1Mjd(epoch)),m=moon->GetMJ2000State(A1Mjd(epoch));const double muE=earth->GetGravitationalConstant(),muM=moon->GetGravitationalConstant();State relative{};for (int i=0;i<6;++i) {relative[i]=m[i]-e[i];offset[i]=relative[i]*muM/(muE+muM);}originInputs={{"earth_mu",muE},{"luna_mu",muM},{"luna_relative_state",json(relative)}};
            }
            State actual{},expected{};V p{},v{};for (int i=0;i<3;++i) {p[i]=input[i]-offset[i];v[i]=input[i+3]-offset[i+3];}
            for (int i=0;i<3;++i) {expected[i]=dot(model.rows[i],p);expected[i+3]=dot(model.rows[i],v)+dot(model.rates[i],p);}
            for (int i=0;i<6;++i) {actual[i]=words[row*25+19+i].toDouble(&ok);const double error=std::abs(actual[i]-expected[i]);require(ok && std::isfinite(actual[i]),"Object-axis report nonfinite");if (i<3) maxp=std::max(maxp,error);else maxv=std::max(maxv,error);if (error>(i<3 ? 2e-7:2e-9)) std::cerr<<"OBJECT_DIFF "<<mode.toStdString()<<" phase="<<phase<<" row="<<row<<" component="<<i<<" actual="<<actual[i]<<" expected="<<expected[i]<<" error="<<error<<std::endl;require(error<=(i<3 ? 2e-7:2e-9),"Object axes six-state differs from independent geometry and frame-rate oracle");}
            samples.append(QJsonObject{{"phase",phase},{"label",label},{"axes",json(selection)},{"reversed",reversed},{"origin",origin},{"epoch_a1mjd",epoch},{"origin_offset",json(offset)},{"origin_inputs",originInputs},{"basis_rows",json(model.rows)},{"basis_rates",json(model.rates)},{"actual",json(actual)},{"expected",json(expected)}});
         }
         configurations.append(QJsonObject{{"axes",json(selection)},{"reversed",reversed},{"origin",origin}});require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Object-axis mission did not continue");
         auto *output=window.findChild<QTreeWidget *>("Output");const auto reports=output->findItems("States",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Object-axis output tree missing");output->itemDoubleClicked(reports.front(),0);events();QMdiSubWindow *viewer=nullptr;for (auto *child:area->subWindowList()) if (dynamic_cast<ReportViewer *>(child->widget())) viewer=child;require(viewer,"Actual object-axis report viewer missing");view(viewer);auto *shown=viewer->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Object-axis viewer differs from checked report");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*viewer);require(viewer->close(),"Object-axis report did not close");events();++phase;
      };
      auto open=[&] {const auto items=tree->findItems("CreatedFrame",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Object-axis resource tree entry missing");tree->itemDoubleClicked(items.front(),0);events();QMdiSubWindow *child=nullptr;for (auto *w:area->subWindowList()) if (w->property("resourceName").toString()=="CreatedFrame") child=w;require(child,"Actual object-axis resource editor missing");view(child);return child;};
      auto edit=[&](const Axes &selection,bool reversed,const QString &origin,bool reject) {
         const auto before=source->toPlainText();const auto undo=source->document()->availableUndoSteps();auto *child=open();auto *panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges(),"Object-axis editor missing or dirty");
         if (mode=="Origin") pick(panel,"Origin",origin);else setField(panel,"Origin",origin);axes(panel,selection,reversed,true);require(source->toPlainText()==before && source->document()->availableUndoSteps()==undo,"Nested object-axis edit escaped parent Apply");panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());
         if (reject) {require(panel && panel->hasChanges() && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && source->toPlainText()==before && source->document()->availableUndoSteps()==undo,"Invalid object-axis Apply accepted or was not atomic");++invalid;panel->discardChanges();require(child->close(),"Invalid object-axis draft did not close");events();return;}
         if (panel && panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("resourceStatus")->text().toStdString()<<std::endl;require(panel && !panel->hasChanges(),"Valid object-axis Apply failed");const auto applied=source->toPlainText();require(applied!=before && configured()->GetStringParameter("Origin")==origin.toStdString(),"Object-axis Apply did not change its configuration");auto *owned=configured()->GetOwnedObject(0);require(owned->GetStringParameter("Primary")==std::string(reversed ? "Reference":"Anchor") && owned->GetStringParameter("Secondary")==std::string(reversed ? "Anchor":"Reference"),"Object-axis references differ after Apply");for (int i=0;i<3;++i) require(owned->GetStringParameter((QString("XYZ")[i]+QString("Axis")).toStdString())==selection[i].toStdString(),"Applied object-axis direction differs");view(child);require(child->close(),"Clean object-axis editor did not close");events();source->undo();require(source->toPlainText()==before && window.buildScript(),"Object-axis Undo did not restore exact source");source->redo();require(source->toPlainText()==applied && window.buildScript(),"Object-axis Redo did not restore exact source");const auto path=files.filePath("Object axes Δ.script");require(window.saveScriptTo(path) && window.loadScript(path) && window.buildScript() && source->toPlainText()==applied,"Object-axis Unicode persistence changed source/model");
      };
      run(currentAxes,false,initialOrigin,"created");
      if (mode=="Unknown") {
         for (const auto &name:QStringList{"Primary","Secondary"}) {
            const auto before=source->toPlainText();const auto undo=source->document()->availableUndoSteps();
            const auto modelBefore=QString::fromStdString(configured()->GetGeneratingString(Gmat::SHOW_SCRIPT));
            auto *child=open();auto *panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel,"Unknown-reference edit panel missing");
            axes(panel,currentAxes,false,true,name);panel->requestApply();events();
            require(panel->hasChanges() && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && source->toPlainText()==before && source->document()->availableUndoSteps()==undo && QString::fromStdString(configured()->GetGeneratingString(Gmat::SHOW_SCRIPT))==modelBefore,"Unknown picked reference Apply was not rejected atomically");
            ++invalid;acceptanceView(*child);panel->discardChanges();require(child->close(),"Unknown reference draft did not close");events();
         }
         const Axes negative{"-R","","N"};edit(negative,false,"Earth",false);run(negative,false,"Earth","unknown-references-recovered");
      } else if (mode=="Origin") {
         edit(currentAxes,false,"Center",false);run(currentAxes,false,"Center","picked-barycenter-origin");
      } else {
         edit(currentAxes,true,"Earth",false);run(currentAxes,true,"Earth","picked-references-reversed");
      }
      const QJsonObject proof{{"checkpoint","object-picker-"+mode},{"outcome","passed"},{"mode",mode},{"cancels",cancels},{"picker_cancels",pickerCancels},{"picker_events",pickerEvents},{"invalid_rejections",invalid},{"runtime_failures",runtimeFailures},{"failures",failures},{"configurations",configurations},{"samples",samples},{"max_position_error_km",maxp},{"max_velocity_error_km_s",maxv},{"facets",QJsonArray{"actual-coordinate-create","actual-reference-direction-selectors","actual-single-reference-picker","visible-axes-action","nested-and-parent-cancel","pending-apply-isolation","exact-source-history","unicode-save-reopen","independent-basis-rate-six-state","actual-report-viewer"}},{"scope","Actual Primary/Secondary/Origin single-reference Select dialogs with eligible SpacePoint types, non-SpacePoint exclusion, Cancel/accept/typed-unknown recovery, nested/parent atomicity, exact source history and independent six-state results. Uses known spacecraft geometry, selected spacecraft and barycenter origins. Acceleration terms neglected per guide; lunar ephemeris/masses are shared explicit translation inputs. Broad input/dynamics/time/native-input/platform/stress/current NASA corpus and final regression remain open."}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}
   catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
