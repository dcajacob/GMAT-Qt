#include "FormationAcceptance.hpp"
#include "DynamicDataDialog.hpp"
#include "PlotWidget.hpp"
#include "ParameterInfo.hpp"
#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <QToolBar>
using namespace formaccept;

static void reveal(QWidget *widget)
{
   require(widget,"Data-display unit control missing");
   for (auto *parent=widget;parent;parent=parent->parentWidget()) {
      for (auto *tabs:widget->window()->findChildren<QTabWidget *>())
         if (tabs->indexOf(parent)>=0) tabs->setCurrentWidget(parent);
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   }
   events();require(widget->isVisible(),"Data-display unit control hidden");acceptanceView(*widget);
}
static QJsonObject graph()
{
   QJsonObject result;auto *moderator=Moderator::Instance();
   for (const auto &name:moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT))
      if (auto *object=moderator->GetConfiguredObject(name))
         result[QString::fromStdString(name)]=QString::fromStdString(object->GetGeneratingString(Gmat::SHOW_SCRIPT));
   return result;
}
static QLabel *fieldCaption(QWidget &dialog,QWidget *field)
{
   for (auto *form:dialog.findChildren<QFormLayout *>())
      if (auto *caption=qobject_cast<QLabel *>(form->labelForField(field))) return caption;
   return nullptr;
}
static QJsonObject plotState(const PlotModel &model)
{
   QJsonArray axes,curves;
   for (const auto &axis:{model.xAxis,model.yAxis})
      axes.append(QJsonObject{{"minimum",axis.minimum?QJsonValue(*axis.minimum):QJsonValue()},
         {"maximum",axis.maximum?QJsonValue(*axis.maximum):QJsonValue()},
         {"ticks",axis.ticks},{"precision",axis.precision}});
   for (auto it=model.curves.cbegin();it!=model.curves.cend();++it) {
      const auto &curve=it.value();QJsonArray points;
      for (const auto &point:curve.points) points.append(QJsonArray{point.x,point.y,point.z,point.epoch});
      curves.append(QJsonObject{{"key",it.key()},{"name",curve.name},{"width",curve.width},
         {"markerSize",curve.markerSize},{"visible",curve.visible},{"lines",curve.lines},
         {"markers",curve.markers},{"color",curve.color.name()},{"points",points}});
   }
   return {{"title",model.title},{"xLabel",model.xLabel},{"yLabel",model.yLabel},
      {"axes",axes},{"curves",curves},{"grid",model.grid},{"legend",model.legend}};
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("DataDisplayUnitTests");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString source="% Data display units Δ\nCreate Spacecraft Sat;\nSat.NAIFId = -101293;\nSat.NAIFIdReferenceFrame = -911293;\nCreate DynamicDataDisplay Display;\nDisplay.AddParameters = {1, Sat.X};\nBeginMissionSequence;\n";
      const auto saved=rig.files.filePath("Data display units Ω.script");write(saved,source.toUtf8());
      require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Data display fixture failed");
      const auto native=graph();const int undo=rig.source->document()->availableUndoSteps();
      auto *info=ParameterInfo::Instance();const auto names=info->GetNamesOfParameters(),types=info->GetTypesOfParameters();
      QJsonArray observations;int failures=0;
      auto stable=[&] {
         require(graph()==native&&rig.source->toPlainText()==source&&read(saved)==source.toUtf8()
            &&rig.source->document()->availableUndoSteps()==undo&&info->GetNamesOfParameters()==names
            &&info->GetTypesOfParameters()==types,"Data display inspection changed native/source/history/inventory");
      };
      auto observe=[&](const QString &key,const QString &control,QWidget *field,QWidget *caption,const QString &expected) {
         reveal(field);QString actual;
         if (auto *label=qobject_cast<QLabel *>(caption)) {reveal(label);actual=label->text();}
         else if (auto *check=qobject_cast<QCheckBox *>(caption)) actual=check->text();
         const bool passed=actual==expected;failures+=!passed;
         observations.append(QJsonObject{{"key",key},{"control",control},{"expected",expected},{"actual",actual},{"passed",passed}});
         rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
      };
      auto *object=Moderator::Instance()->GetConfiguredObject("Display");require(object,"Native dynamic display missing");
      DynamicDataDialog dynamic(*object,{},&rig.window);const auto settings=dynamic.settings();
      dynamic.show();events();auto *table=dynamic.findChild<QTableWidget *>("dynamicGrid");
      require(table&&table->rowCount()==1&&table->columnCount()==1,"Dynamic grid fixture differs");
      reveal(table);QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,table->visualItemRect(table->item(0,0)).center());events();
      auto *edit=dynamic.findChild<QPushButton *>("dynamicEditCell");reveal(edit);std::exception_ptr failure;bool visited=false;
      QTimer::singleShot(0,&dynamic,[&] {
         auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
         try {
            require(dialog&&dialog->objectName()=="dynamicCellDialog","Dynamic cell editor missing");
            QMap<QString,QString> bounds;
            for (const auto &name:QStringList{"warnLower","warnUpper","critLower","critUpper"}) {
               auto *field=dialog->findChild<QLineEdit *>("dynamic_"+name);require(field,"Dynamic bound missing");bounds[name]=field->text();
            }
            const QMap<QString,QString> labels={{"warnLower","Warning lower"},{"warnUpper","Warning upper"},
               {"critLower","Critical lower"},{"critUpper","Critical upper"}};
            for (const auto &unit:QStringList{"km","kg"}) {
               if (unit=="kg") {
                  auto *parameter=dialog->findChild<QLineEdit *>("dynamicParameter");reveal(parameter);
                  QTest::mouseClick(parameter,Qt::LeftButton);QTest::keyClick(parameter,Qt::Key_A,Qt::ControlModifier);
                  QTest::keyClicks(parameter,"Sat.DryMass");events();
               }
               for (auto it=labels.cbegin();it!=labels.cend();++it) {
                  auto *field=dialog->findChild<QLineEdit *>("dynamic_"+it.key());
                  observe("dynamic-"+unit,"dynamic_"+it.key(),field,fieldCaption(*dialog,field),it.value()+" ("+unit+")");
                  require(field->text()==bounds[it.key()],"Unit refresh converted a dynamic bound");
               }
            }
            dialog->reject();visited=true;
         } catch (...) {failure=std::current_exception();if(dialog)dialog->reject();}
      });
      QTest::mouseClick(edit,Qt::LeftButton);events();if(failure)std::rethrow_exception(failure);
      require(visited&&dynamic.settings()==settings,"Dynamic Cancel changed pending settings");dynamic.reject();stable();

      auto model=std::make_shared<PlotModel>(PlotModel::Kind::XY);model->title="Unit display fixture";
      model->xLabel="Arbitrary horizontal label";model->yLabel="Arbitrary vertical label";
      model->xAxis.minimum=-10;model->xAxis.maximum=10;model->yAxis.minimum=-20;model->yAxis.maximum=20;
      PlotCurve curve;curve.name="Synthetic data";PlotPoint first;first.x=-1;first.y=2;
      PlotPoint second;second.x=3;second.y=4;curve.points={first,second};model->curves[0]=curve;
      PlotWidget plot(model);plot.resize(1100,750);plot.show();events();const auto originalPlot=plotState(*model);
      auto *action=plot.findChild<QAction *>("plotStyleAction");require(action,"Plot style action missing");QWidget *style=nullptr;
      for (auto *bar:plot.findChildren<QToolBar *>()) if (bar->actions().contains(action)) style=bar->widgetForAction(action);
      reveal(style);visited=false;failure=nullptr;
      QTimer::singleShot(0,&plot,[&] {
         auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
         try {
            require(dialog&&dialog->objectName()=="plotStyleDialog","Plot style editor missing");
            for (const auto &axis:QStringList{"X","Y"}) for (const auto &limit:QStringList{"Minimum","Maximum"}) {
               const auto name="plot"+axis+"Axis"+limit;auto *field=dialog->findChild<QLineEdit *>(name);
               auto *caption=dialog->findChild<QCheckBox *>(name+"Enabled");
               observe("plot-limits",name,field,caption,limit+" ("+axis+"-axis data units)");
            }
            for (const auto &name:QStringList{"Line width","Marker size"}) {
               auto *field=dialog->findChild<QSpinBox *>(name);
               observe("plot-style",name,field,fieldCaption(*dialog,field),name+" (pixels)");
            }
            dialog->reject();visited=true;
         } catch (...) {failure=std::current_exception();if(dialog)dialog->reject();}
      });
      QTest::mouseClick(style,Qt::LeftButton);events();if(failure)std::rethrow_exception(failure);
      require(visited&&plotState(*model)==originalPlot,"Plot Cancel changed axis/style/data settings");plot.close();stable();
      require(observations.size()==14,"Data display observation count differs");
      QJsonObject proof{{"status",failures?"failed":"passed"},{"observations",observations},{"mismatches",failures},
         {"source_native_history_file_and_parameter_inventory_unchanged",true},
         {"dynamic_bounds_and_pending_settings_unchanged",true},{"plot_settings_and_data_unchanged",true},
         {"mission_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return failures?1:0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}
     catch (std::exception &error) {std::cerr<<"FAIL "<<error.what()<<'\n';}
   return 1;
}
