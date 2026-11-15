#include "FormationAcceptance.hpp"
#include "ReportParameterDialog.hpp"
#include "CommandEditor.hpp"
#include "ParameterInfo.hpp"
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTest>
#include <QTimer>
using namespace formaccept;
static void reveal(QWidget *field)
{
   require(field,"Function argument input missing");
   for (auto *parent=field;parent;parent=parent->parentWidget()) {
      for (auto *tabs:field->window()->findChildren<QTabWidget *>()) if(tabs->indexOf(parent)>=0)tabs->setCurrentWidget(parent);
      if(auto *scroll=qobject_cast<QScrollArea *>(parent))scroll->ensureWidgetVisible(field);
   }
   events();require(field->isVisible(),"Function argument input hidden");acceptanceView(*field);
}
static QJsonObject graph()
{
   QJsonObject result;auto *moderator=Moderator::Instance();
   for(const auto &name:moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT))if(auto *object=moderator->GetConfiguredObject(name))
      result[QString::fromStdString(name)]=QString::fromStdString(object->GetGeneratingString(Gmat::SHOW_SCRIPT));
   return result;
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionArgumentUnits");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto function=rig.files.filePath("UnitFunction.gmf");
      write(function,"function [out] = UnitFunction(a, b, c, d)\nCreate Variable a b c d out;\nBeginMissionSequence;\nout = a + b + c + d;\n");
      const QString source="Create Spacecraft Sat;\nSat.NAIFId = -101295;\nSat.NAIFIdReferenceFrame = -911295;\nCreate Variable user;\nuser = 7;\nCreate Array A[2,2];\nCreate GmatFunction UnitFunction;\nUnitFunction.FunctionPath = '"+function+"';\nBeginMissionSequence;\n";
      const auto saved=rig.files.filePath("Function argument units Ω.script");write(saved,source.toUtf8());
      require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Function argument fixture failed");
      const auto native=graph();const int undo=rig.source->document()->availableUndoSteps();auto *info=ParameterInfo::Instance();const auto names=info->GetNamesOfParameters(),types=info->GetTypesOfParameters();
      auto stable=[&] {require(graph()==native&&rig.source->toPlainText()==source&&read(saved)==source.toUtf8()
         &&rig.source->document()->availableUndoSteps()==undo&&info->GetNamesOfParameters()==names&&info->GetTypesOfParameters()==types,
         "Function argument units changed native/source/history/inventory");};
      QJsonArray observations;int failures=0;
      auto observe=[&](const QString &key,const QString &actual,const QString &expected) {
         const bool passed=actual==expected;failures+=!passed;observations.append(QJsonObject{{"key",key},{"actual",actual},{"expected",expected},{"passed",passed}});
         rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
      };
      const QString notice="Units for numeric arguments are defined by the called function; values are passed without unit conversion.";
      const QStringList selected={"7","user","Sat.X","A(1,1)","'text'"};
      {
         ReportParameterDialog dialog(selected,&rig.window,ReportParameterDialog::Mode::FunctionInputs);dialog.show();events();
         auto *label=dialog.findChild<QLabel *>("functionArgumentUnits");if(label)reveal(label);
         observe("direct-picker",label?label->text():QString(),notice);require(dialog.selection()==selected,"Picker display changed arguments");dialog.reject();stable();
      }
      for (const auto &kind:QStringList{"gmat","python"}) {
         const QString statement=kind=="gmat"?"UnitFunction(7, user, Sat.X, A(1,1));":"Python.unit_module.transform(7, user, Sat.X, A(1,1));";
         int applied=0;auto *editor=new CommandEditor(statement,false,{},[&](const QString &){++applied;return QString();});
         auto *child=rig.area->addSubWindow(editor);child->show();child->showNormal();child->resize(rig.area->viewport()->size());events();
         auto *input=editor->findChild<QLineEdit *>("commandField_Inputs");reveal(input);QString caption;
         for(QWidget *parent=input;parent&&parent!=editor;parent=parent->parentWidget())for(auto *form:editor->findChildren<QFormLayout *>())
            if(auto *label=qobject_cast<QLabel *>(form->labelForField(parent)))if(label->isVisible()){reveal(label);caption=label->text();}
         observe(kind+"-input",caption,"Inputs (numeric units defined by function)");
         auto *button=editor->findChild<QPushButton *>("commandChoose_Inputs");reveal(button);std::exception_ptr failure;bool visited=false;
         QTimer::singleShot(0,editor,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            try {require(dialog,"Function argument picker absent");auto *label=dialog->findChild<QLabel *>("functionArgumentUnits");if(label)reveal(label);
               observe(kind+"-picker",label?label->text():QString(),notice);dialog->reject();visited=true;
            }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
         });
         const auto before=input->text();QTest::mouseClick(button,Qt::LeftButton);events();if(failure)std::rethrow_exception(failure);
         require(visited&&applied==0&&input->text()==before,"Function argument inspection changed command");editor->discardChanges();delete child;events();stable();
      }
      require(observations.size()==5,"Function argument observation inventory differs");
      const QJsonObject proof{{"status",failures?"failed":"passed"},{"observations",observations},{"mismatches",failures},
         {"source_native_history_file_and_parameter_inventory_unchanged",true},{"arguments_and_commands_unchanged",true},
         {"mission_executions",0},{"function_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return failures?1:0;
   }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';}return 1;
}
