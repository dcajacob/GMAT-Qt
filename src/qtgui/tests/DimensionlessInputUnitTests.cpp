// Supplementary display contract. Expectations are independent quantity
// definitions in DimensionlessInputUnitCases.json, not native empty unit tags.
#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CelestialBodyPanel.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
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
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <QTextDocument>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
QByteArray read(const QString &path) {QFile file(path);require(file.open(QIODevice::ReadOnly),"Unit fixture missing");return file.readAll();}
void write(const QString &path,const QByteArray &bytes) {QFile file(path);require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Cannot preserve unit observation");}
void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
void reveal(QWidget *widget) {
   require(widget,"Unit control missing");
   for(auto *parent=widget;parent;parent=parent->parentWidget()) {
      for(auto *tabs:widget->window()->findChildren<QTabWidget *>())if(tabs->indexOf(parent)>=0)tabs->setCurrentWidget(parent);
      if(auto *scroll=qobject_cast<QScrollArea *>(parent))scroll->ensureWidgetVisible(widget);
   }
   events();
   if(!widget->isVisible())throw std::runtime_error(("Unit control is hidden: "+widget->objectName()+" ("+widget->metaObject()->className()+")").toStdString());
   acceptanceView(*widget);
}
}

int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("DimensionlessInputUnits");
   try {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, dimensionless unit cases and group");TestSettings settings;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!output.isEmpty(),"Unit evidence directory required");QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto fixture=QJsonDocument::fromJson(read(argv[2])).object();
      MainWindow window;require(window.initialize(startup),"Unit runtime initialization failed");window.show();acceptanceView(window);events();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");
      auto *resources=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");
      require(editor&&messages&&resources&&area,"Unit workspace missing");QJsonArray observations,cases;int mismatches=0;
      auto preserve=[&]{write(output+"/unit-observations.json",QJsonDocument(observations).toJson());};
      for(const auto &value:fixture["cases"].toArray()) {
         const auto spec=value.toObject();if(spec["group"]!=QString(argv[3]))continue;
         const auto key=spec["key"].toString(),name=spec["object"].toString();
         QString source="% Dimensionless input display Δ\n"+spec["fixture"].toString()+"Create Variable UnitsSentinel;\nUnitsSentinel = 41;\nBeginMissionSequence;\n";
         source.replace("@DATA@",QFileInfo(startup).absoluteDir().absoluteFilePath("../data"));
         editor->setPlainText(source);const bool built=window.buildScript();events();
         write(output+'/'+key+"-build.txt",messages->toPlainText().toUtf8());require(built,"Unit case script did not build");
         write(output+'/'+key+".script",source.toUtf8());editor->document()->setModified(false);
         const int undo=editor->document()->availableUndoSteps();const auto saved=output+'/'+key+" Δ.script";
         require(window.saveScriptTo(saved),"Unit case save failed");
         GmatBase *object=spec["route"]=="body" ? static_cast<GmatBase *>(Moderator::Instance()->GetSolarSystemInUse()->GetBody(name.toStdString())) : Moderator::Instance()->GetConfiguredObject(name.toStdString());
         require(object,"Unit case native object missing");const auto canonical=object->GetGeneratingString(Gmat::SHOW_SCRIPT);
         const auto rawState=spec["route"]=="body" ? QByteArray() : QByteArray::fromStdString(object->GetGeneratingString(Gmat::SCRIPTING));
         auto invariant=[&]{require(editor->toPlainText()==source&&editor->document()->availableUndoSteps()==undo&&read(saved)==source.toUtf8()&&object->GetGeneratingString(Gmat::SHOW_SCRIPT)==canonical,"Unit viewing changed source/history/file/native state");};
         for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)
            if(tabs->widget(i)==resources||tabs->widget(i)->isAncestorOf(resources))tabs->setCurrentIndex(i);
         auto items=resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Unit resource tree item missing");
         resources->scrollToItem(items[0]);reveal(resources);QTest::mouseClick(resources->viewport(),Qt::LeftButton,Qt::NoModifier,resources->visualItemRect(items[0]).center());
         QTest::mouseDClick(resources->viewport(),Qt::LeftButton,Qt::NoModifier,resources->visualItemRect(items[0]).center());events();
         QPointer<QMdiSubWindow> child=area->currentSubWindow();QWidget *panel=child?child->widget():nullptr;require(panel,"Unit editor did not open");
         child->showNormal();child->move(0,0);child->resize(area->viewport()->size());events();
         auto inspectPanel=[&] {
         auto expected=spec["units"].toObject();
         if(spec["route"]=="primary") {
            for(auto it=expected.begin();it!=expected.end();++it) {
               QLineEdit *control=nullptr;
               for(auto *candidate:panel->findChildren<QLineEdit *>())
                  if(spec.contains("control") ? candidate->objectName()==spec["control"].toString() : candidate->objectName().endsWith('_'+it.key())) {
                     require(!control,"Ambiguous primary unit input");control=candidate;
                  }
               require(control&&!control->isReadOnly(),"Writable primary unit input missing");reveal(control);
               QString shown;
               for(auto *label:control->parentWidget()->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly))
                  if(label->isVisible()){reveal(label);shown+=' '+label->text();}
               for(auto *ancestor=static_cast<QWidget *>(control);ancestor&&ancestor!=panel;ancestor=ancestor->parentWidget())
                  for(auto *form:panel->findChildren<QFormLayout *>())
                     if(auto *label=qobject_cast<QLabel *>(form->labelForField(ancestor)))
                        if(label->isVisible()){reveal(label);shown+=' '+label->text();}
               const bool passed=shown.contains(it.value().toString());if(!passed)++mismatches;
               observations.append(QJsonObject{{"case",key},{"route","primary"},{"field",it.key()},
                  {"expected_unit",it.value()},{"actual_label",shown.trimmed()},{"passed",passed}});preserve();invariant();
            }
         }else if(spec["route"]=="body"||spec["route"]=="attitude") {
            for(auto it=expected.begin();it!=expected.end();++it) {
               const auto prefix=spec["route"]=="body"?"body_":"attitude_";
               auto *control=panel->findChild<QLineEdit *>(prefix+it.key());reveal(control);QString label;
               if(spec["route"]=="attitude") {
                  if(auto *unit=panel->findChild<QLabel *>("attitudeUnit_"+it.key())){reveal(unit);label=unit->text();}
               }else for(auto *form:panel->findChildren<QFormLayout *>())if(auto *unit=qobject_cast<QLabel *>(form->labelForField(control))){reveal(unit);label=unit->text();}
               const bool passed=label.contains(it.value().toString());if(!passed)++mismatches;
               observations.append(QJsonObject{{"case",key},{"route",spec["route"]},{"field",it.key()},{"expected_unit",it.value()},{"actual_label",label},{"passed",passed}});preserve();invariant();
            }
         }else {
            auto *table=panel->findChild<QTableWidget *>("resourceProperties");auto *sections=panel->findChild<QTabBar *>("propertySections");
            require(table,"Unit property table missing");int all=-1;
            if(sections&&!sections->isHidden()) {
               for(int i=0;i<sections->count();++i)if(sections->tabText(i)=="All Properties")all=i;
               require(all>=0,"Unit All Properties section missing");reveal(sections);QTest::mouseClick(sections,Qt::LeftButton,Qt::NoModifier,sections->tabRect(all).center());events();
            }
            auto inspect=[&](const QString &phase) {
               for(auto it=expected.begin();it!=expected.end();++it) {
                  int row=-1;for(int i=0;i<table->rowCount();++i)if(table->item(i,0)->text()==it.key())row=i;
                  if(row<0) {
                     QJsonArray rows,tabs;
                     for(int i=0;i<table->rowCount();++i)rows.append(QJsonObject{{"field",table->item(i,0)->text()},{"unit",table->item(i,2)->text()},{"hidden",table->isRowHidden(i)}});
                     if(sections)for(int i=0;i<sections->count();++i)tabs.append(sections->tabText(i));
                     write(output+'/'+key+"-missing-field.json",QJsonDocument(QJsonObject{{"field",it.key()},{"row",row},{"selected_section",sections?sections->currentIndex():-1},{"sections",tabs},{"rows",rows}}).toJson());
                  }
                  require(row>=0,"Pinned dimensionless field is not an available input");
                  auto *unit=table->item(row,2);require(unit&&!unit->flags().testFlag(Qt::ItemIsEditable),"Unit cell missing or editable");
                  QString route="advanced-table",visibleUnit;
                  if(!table->isRowHidden(row)) {
                     reveal(table);table->scrollToItem(unit);events();require(table->visualItemRect(unit).intersects(table->viewport()->rect()),"Unit cell is outside the visible viewport");acceptanceView(*table);visibleUnit=unit->text();
                  }else {
                     route="primary+backing-metadata";QWidget *control=panel->findChild<QWidget *>("resource_"+it.key());
                     if(!control)for(auto *candidate:panel->findChildren<QWidget *>())if(!qobject_cast<QAbstractButton *>(candidate)&&!qobject_cast<QLabel *>(candidate)&&candidate->objectName().endsWith('_'+it.key())){control=candidate;break;}
                     require(control,"Replaced advanced field has no primary input");reveal(control);
                     // Primary forms can wrap a value with a separate unit
                     // label, or put the unit in the associated caption.
                     if(qobject_cast<QHBoxLayout *>(control->parentWidget()->layout()))for(auto *label:control->parentWidget()->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly))if(label->isVisible()) {
                        if(label->text().contains(it.value().toString()))reveal(label);
                        visibleUnit+=' '+label->text();
                     }
                     for(auto *ancestor=static_cast<QWidget *>(control);ancestor&&ancestor!=panel;ancestor=ancestor->parentWidget())
                        for(auto *form:panel->findChildren<QFormLayout *>())if(auto *label=qobject_cast<QLabel *>(form->labelForField(ancestor)))if(label->isVisible()) {
                           if(label->text().contains(it.value().toString()))reveal(label);
                           visibleUnit+=' '+label->text();
                        }
                  }
                  const auto visibleExpected=spec["visible_units"].toObject().value(it.key()).toString(it.value().toString());
                  const auto sensitivity=spec["visible_units"].toObject().contains(it.key()) ? Qt::CaseInsensitive : Qt::CaseSensitive;
                  const bool passed=unit->text()==it.value().toString()&&visibleUnit.contains(visibleExpected,sensitivity);if(!passed)++mismatches;
                  observations.append(QJsonObject{{"case",key},{"route",route},{"phase",phase},{"field",it.key()},{"expected_unit",it.value()},{"actual_unit",unit->text()},{"visible_unit_or_caption",visibleUnit},{"passed",passed}});preserve();invariant();
               }
            };
            inspect("initial");
            if(spec.contains("unit_selector")) {
               const auto selector=spec["unit_selector"].toObject();const auto field=selector["field"].toString();
               auto *choice=panel->findChild<QComboBox *>("resource_"+field);require(choice,"Conditional unit selector missing");
               for(const auto &entry:selector["transitions"].toArray()) {
                  const auto transition=entry.toObject();const auto value=transition["value"].toString();reveal(choice);const int index=choice->findText(value);require(index>=0,"Conditional unit choice missing");
                  QTest::mouseClick(choice,Qt::LeftButton,Qt::NoModifier,QPoint(choice->width()-10,choice->height()/2));events();require(choice->view()->isVisible(),"Conditional unit popup did not open");
                  QTest::keyClick(choice->view(),Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(choice->view(),Qt::Key_Down);QTest::keyClick(choice->view(),Qt::Key_Return);events();
                  require(choice->currentText()==value,"Conditional unit choice failed");expected=transition["units"].toObject();inspect(field+"="+value);
               }
            }
            if(spec["roundtrip_representation"].toBool()) {
               auto *choice=panel->findChild<QComboBox *>("resource_DisplayStateType");require(choice,"Orbit unit selector missing");
               const auto original=choice->currentText();reveal(choice);choice->setCurrentText("Cartesian");events();choice->setCurrentText(original);events();
               reveal(sections);QTest::mouseClick(sections,Qt::LeftButton,Qt::NoModifier,sections->tabRect(all).center());events();reveal(table);inspect("selector-roundtrip");
            }
         }
         if(spec.contains("grid_unit_notice")) {
            auto *input=panel->findChild<QWidget *>("resource_RmatValue");reveal(input);
            auto buttons=input->parentWidget()->findChildren<QPushButton *>(QString(),Qt::FindDirectChildrenOnly);
            require(buttons.size()==1,"Visible array grid action is ambiguous");auto *button=buttons.first();reveal(button);
            std::exception_ptr failure;bool inspected=false;
            QTimer::singleShot(0,panel,[&] {
               auto *dialog=panel->findChild<QDialog *>("numericGridDialog");
               try {
                  require(dialog&&dialog->isVisible(),"Array grid dialog missing");
                  auto *notice=dialog->findChild<QLabel *>("arrayUnitNotice");QString shown;
                  if(notice){reveal(notice);shown=notice->text();}
                  const auto wanted=spec["grid_unit_notice"].toString();const bool passed=shown.contains(wanted);
                  mismatches+=!passed;observations.append(QJsonObject{{"case",key},{"route","array-grid"},{"field","RmatValue"},{"expected_unit",wanted},{"actual_label",shown},{"passed",passed}});preserve();invariant();
                  auto *box=dialog->findChild<QDialogButtonBox *>();require(box,"Array grid dialog buttons missing");QTest::mouseClick(box->button(QDialogButtonBox::Cancel),Qt::LeftButton);inspected=true;
               }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
            });
            QTest::mouseClick(button,Qt::LeftButton);events();if(failure)std::rethrow_exception(failure);require(inspected,"Array grid units were not inspected");
         }
         };
         if(spec["route"]=="nested-axes") {
            auto *owner=panel;auto *button=owner->findChild<QPushButton *>("editCoordinateAxes");reveal(button);
            std::exception_ptr failure;bool inspected=false;
            QTimer::singleShot(0,owner,[&] {
               auto *dialog=owner->findChild<QDialog *>("coordinateAxesDialog");
               try {
                  require(dialog&&dialog->isVisible(),"Nested axes dialog missing");panel=nullptr;
                  for(auto *candidate:dialog->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly))
                     if(dynamic_cast<ResourceEditor *>(candidate))panel=candidate;
                  require(panel,"Nested axes resource editor missing");inspectPanel();inspected=true;dialog->reject();
               }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
            });
            QTest::mouseClick(button,Qt::LeftButton);events();panel=owner;
            if(failure)std::rethrow_exception(failure);require(inspected,"Nested axes units not inspected");
         }else inspectPanel();
         invariant();cases.append(key);child->deleteLater();events();invariant();
         if(!rawState.isEmpty())require(QByteArray::fromStdString(object->GetGeneratingString(Gmat::SCRIPTING))==rawState,"Unit view changed native serialized values");
      }
      require(!cases.isEmpty()&&!observations.isEmpty(),"Unit group has no executed witnesses");
      const QJsonObject proof{{"status",mismatches?"failed":"passed"},{"group",argv[3]},{"cases",cases},{"observations",observations},{"mismatches",mismatches},
         {"mission_executions",0},{"source_native_history_file_isolation",true},{"new_requirement_credit",0},{"scope",fixture["scope"]}};
      write(output+"/proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return mismatches?1:0;
   }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}
    catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';}
   return 1;
}
