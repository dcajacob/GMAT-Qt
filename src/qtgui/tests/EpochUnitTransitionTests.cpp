#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "EpochIntervalDialog.hpp"
#include "ParameterInfo.hpp"
#include <QAbstractItemView>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTest>
using namespace formaccept;
static void reveal(QWidget *field)
{
   require(field,"Epoch unit input missing");
   for (auto *parent=field;parent;parent=parent->parentWidget()) {
      for (auto *tabs:field->window()->findChildren<QTabWidget *>())
         if (tabs->indexOf(parent)>=0) tabs->setCurrentWidget(parent);
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(field);
   }
   events();require(field->isVisible()&&field->isEnabled(),"Epoch unit input inaccessible");acceptanceView(*field);
}
static QString captions(QWidget &owner,QWidget *field)
{
   QString text;
   if (qobject_cast<QHBoxLayout *>(field->parentWidget()->layout()))
      for (auto *label:field->parentWidget()->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly))
         if (label->isVisible()) {acceptanceView(*label);text+=' '+label->text();}
   for (auto *parent=field;parent&&parent!=&owner;parent=parent->parentWidget())
      for (auto *form:owner.findChildren<QFormLayout *>())
         if (auto *label=qobject_cast<QLabel *>(form->labelForField(parent)))
            if (label->isVisible()) {acceptanceView(*label);text+=' '+label->text();}
   return text.trimmed();
}
static QJsonObject graph()
{
   QJsonObject result;auto *moderator=Moderator::Instance();
   for (const auto &name:moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT))
      if (auto *object=moderator->GetConfiguredObject(name))
         result[QString::fromStdString(name)]=QString::fromStdString(object->GetGeneratingString(Gmat::SHOW_SCRIPT));
   return result;
}
static QString value(QWidget *field)
{
   if (auto *combo=qobject_cast<QComboBox *>(field)) return combo->currentText();
   if (auto *line=qobject_cast<QLineEdit *>(field)) return line->text();
   throw std::runtime_error("Unsupported epoch input control");
}
static void choose(QWidget *field,const QString &text)
{
   reveal(field);
   if (auto *combo=qobject_cast<QComboBox *>(field)) {
      const int index=combo->findText(text);require(index>=0,"Epoch format option missing");
      QTest::mouseClick(combo,Qt::LeftButton,Qt::NoModifier,QPoint(combo->width()-10,combo->height()/2));events();
      require(combo->view()->isVisible(),"Epoch format menu did not open");QTest::keyClick(combo->view(),Qt::Key_Home);
      for (int i=0;i<index;++i) QTest::keyClick(combo->view(),Qt::Key_Down);
      QTest::keyClick(combo->view(),Qt::Key_Return);
   } else if (auto *line=qobject_cast<QLineEdit *>(field)) {
      QTest::mouseClick(line,Qt::LeftButton);QTest::keyClick(line,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(line,text);QTest::keyClick(line,Qt::Key_Tab);
   } else throw std::runtime_error("Epoch format control has unexpected type");
   events();require(value(field)==text,"Epoch format selection was not retained");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("EpochUnitTransitions");
   try {
      require(argc==3,"Supply startup and epoch unit cases");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto fixture=QJsonDocument::fromJson(read(argv[2])).object();QJsonArray observations;int failures=0;
      for (const auto &entry:fixture["cases"].toArray()) {
         const auto spec=entry.toObject();const auto key=spec["key"].toString();
         const auto source="% Epoch units Δ\n"+spec["fixture"].toString()+"BeginMissionSequence;\n";
         const auto saved=rig.files.filePath(key+" Ω.script");write(saved,source.toUtf8());
         require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Epoch unit fixture failed");
         auto *native=Moderator::Instance()->GetConfiguredObject("Probe");require(native,"Epoch unit native object missing");
         const auto nativeGraph=graph();const auto undo=rig.source->document()->availableUndoSteps();
         auto *info=ParameterInfo::Instance();const auto names=info->GetNamesOfParameters(),types=info->GetTypesOfParameters();
         auto stable=[&] {require(graph()==nativeGraph&&rig.source->toPlainText()==source&&read(saved)==source.toUtf8()
            &&rig.source->document()->availableUndoSteps()==undo&&info->GetNamesOfParameters()==names
            &&info->GetTypesOfParameters()==types,"Epoch unit viewing changed source/native/history/inventory");};
         auto exercise=[&](QWidget &owner,const QString &route,const QString &prefix,const QString &formatName,const QJsonArray &fields) {
            auto *format=owner.findChild<QWidget *>(prefix+formatName);require(format,"Epoch format input missing");
            for (const auto &stateValue:fixture["states"].toArray()) {
               const auto state=stateValue.toObject();const auto wantedFormat=state["format"].toString();
               if (value(format)!=wantedFormat) choose(format,wantedFormat);else reveal(format);
               stable();
               for (const auto &fieldValue:fields) {
                  const auto name=fieldValue.toString();auto *field=owner.findChild<QWidget *>(prefix+name);reveal(field);
                  const auto expected=state["unit"].toString(),actual=captions(owner,field);
                  const bool passed=actual.contains(expected);failures+=!passed;
                  observations.append(QJsonObject{{"case",key},{"route",route},{"format",wantedFormat},
                     {"phase",state["phase"]},{"field",name},{"expected_unit",expected},{"actual_caption",actual},{"passed",passed}});
                  rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
               }
            }
         };
         {
            int applied=0;auto *panel=new ResourceEditor(*native,[&](const QMap<QString,QString> &){++applied;return QString();},nullptr,source);
            auto *child=rig.area->addSubWindow(panel);child->show();child->showNormal();child->resize(rig.area->viewport()->size());events();
            exercise(*panel,"resource-editor",spec["prefix"].toString(),spec["format_field"].toString(),spec["fields"].toArray());
            require(applied==0,"Epoch unit viewing applied pending changes");panel->discardChanges();delete child;events();stable();
         }
         if (spec["interval_dialog"].toBool()) {
            EpochIntervalDialog dialog(*native,{},&rig.window);dialog.show();events();
            exercise(dialog,"interval-dialog","interval","EpochFormat",QJsonArray{"InitialEpoch","FinalEpoch"});
            dialog.reject();stable();
         }
      }
      require(observations.size()==fixture["expected_observations"].toInt(),"Epoch unit observation inventory differs");
      const QJsonObject proof{{"status",failures?"failed":"passed"},{"observations",observations},{"mismatches",failures},
         {"source_native_history_file_and_parameter_inventory_unchanged",true},{"applied_transactions",0},
         {"mission_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return failures?1:0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}
     catch (std::exception &error) {std::cerr<<"FAIL "<<error.what()<<'\n';}
   return 1;
}
