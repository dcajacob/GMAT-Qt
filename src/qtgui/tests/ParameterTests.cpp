#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "UserParameter.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include "Array.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTimer>
#include <QFileDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QAction>
#include <QRegularExpression>
#include <QTableWidget>
#include <QHeaderView>
#include <QElapsedTimer>
#include <QThread>
#include <iostream>
#include <stdexcept>
#include <cmath>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Report missing"); return QString::fromUtf8(file.readAll()); }
template<class Open,class Inspect> static void modal(QWidget &owner,Open open,Inspect inspect)
{
   std::exception_ptr failure;
   QTimer::singleShot(0,&owner,[&] { auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      try { require(dialog,"Expected dialog missing"); inspect(dialog); }
      catch (...) { failure=std::current_exception(); }
      if (dialog && dialog->isVisible()) dialog->reject();
   }); open(); if (failure) std::rethrow_exception(failure);
}
static QTreeWidgetItem *item(QTreeWidget *tree,const QString &name)
{
   QTreeWidgetItemIterator it(tree); while (*it) { if ((*it)->text(0)==name) return *it; ++it; } throw std::runtime_error("Parameter tree item missing");
}
static ResourceEditor *open(MainWindow &window,const QString &name)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources"); tree->itemDoubleClicked(item(tree,name),0); QApplication::processEvents();
   auto *workspace=window.findChild<QMdiArea *>("workspace"); auto *panel=dynamic_cast<ResourceEditor *>(workspace->activeSubWindow()->widget()); require(panel,"Parameter MDI panel missing"); return panel;
}
static void closePanels(MainWindow &window)
{
   auto *workspace=window.findChild<QMdiArea *>("workspace");
   for (auto *child:workspace->subWindowList()) if (auto *panel=dynamic_cast<EditablePanel *>(child->widget())) { panel->discardChanges(); child->close(); }
   QApplication::processEvents();
}
static void capture(QWidget *widget,const QString &path)
{
   if (path.isEmpty()) return;
   QElapsedTimer elapsed; elapsed.start(); while (elapsed.elapsed()<300) { QApplication::processEvents(); QThread::msleep(5); }
   require(widget->grab().save(path),"Cannot capture parameter layout");
}
static QLineEdit *scalarField(QDialog *dialog)
{
   for (auto *field:dialog->findChildren<QLineEdit *>("parameterValue")) if (field->isVisible()) return field;
   return nullptr;
}
static void nasaParameterProof(const char *checkpoint,const char *family,const QStringList &facets)
{
   QJsonArray values; for (const auto &facet:facets) values.append(facet);
   std::cout<<"NASA_CHECK "<<QJsonDocument(QJsonObject{{"checkpoint",checkpoint},{"family",family},
      {"outcome","passed"},{"facets",values}}).toJson(QJsonDocument::Compact).constData()<<std::endl;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtParameters");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Fixture setup failed");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto screenshots=argc>2 ? QString::fromLocal8Bit(argv[2]) : QString();
      MainWindow window; window.show(); require(window.initialize(startup),"Initialization failed");
      const auto output=FileManager::Instance()->GetAbsPathname("OUTPUT_PATH"); struct RestoreOutput { std::string path; ~RestoreOutput() { FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",path); } } restore{output}; FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",files.path().toStdString());
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto numericReport=files.filePath("numbers.txt"),stringReport=files.filePath("strings.txt"),saved=files.filePath("parameters ü.script");
      const QString source="% scalar configuration Δ\nCreate Variable n m; % keep grouped declaration\nn = 2; % keep earlier initializer\nGMAT n = 3; % keep effective initializer\nCreate String Text Spare; % keep strings\nText = 'old'; % keep literal comment\nCreate ReportFile Numbers Texts;\nNumbers.Filename = '"+numericReport+"';\nNumbers.WriteHeaders = false;\nNumbers.Precision = 16;\nTexts.Filename = '"+stringReport+"';\nTexts.WriteHeaders = false;\nBeginMissionSequence; % preserve the entire mission\nReport Numbers n m;\nReport Texts Text Spare;\nn = n * 2; % calculation remains exact\nm = n + 7;\nReport Numbers n m;\nText = 'runtime'; % keep later assignment\nReport Texts Text Spare;\n";
      editor->setPlainText(source); require(window.saveScriptTo(saved) && window.buildScript(),"Parameter fixture build failed");
      auto *panel=open(window,"n"); auto *value=panel->findChild<QLineEdit *>("parameterValue"); require(value && value->text()=="3" && !panel->hasChanges(),"Variable initial value missing or dirty"); value->setText("5.125");
      require(panel->hasChanges() && Moderator::Instance()->GetConfiguredObject("n")->GetRealParameter("Value")==3 && editor->toPlainText()==source,"Pending numeric value modified model");
      modal(*panel,[&] { panel->findChild<QPushButton *>("showScript")->click(); },[](QDialog *dialog) { const auto script=dialog->findChild<QPlainTextEdit *>("inspectionText")->toPlainText(); require(script.contains("n = 3") && !script.contains("5.125"),"Scalar preview included pending edits"); });
      capture(panel->parentWidget(),screenshots.isEmpty() ? QString() : screenshots+".variable.png"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::processEvents();
      QString configured=source; configured.replace("GMAT n = 3;","GMAT n = 5.125;"); require(editor->toPlainText()==configured && Moderator::Instance()->GetConfiguredObject("n")->GetRealParameter("Value")==5.125,"Variable Apply changed source outside effective initializer");
      editor->undo(); require(editor->toPlainText()==source,"Numeric Undo not exact"); editor->redo(); require(editor->toPlainText()==configured,"Numeric Redo not exact");
      panel=open(window,"Text"); value=panel->findChild<QLineEdit *>("parameterValue"); require(value && value->text()=="old","String initializer missing"); const QString text="O'Brien Δ"; value->setText(text); require(panel->hasChanges() && Moderator::Instance()->GetConfiguredObject("Text")->GetStringParameter("Expression")=="old","Pending string value modified model");
      capture(panel->parentWidget(),screenshots.isEmpty() ? QString() : screenshots+".string.png"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::processEvents(); configured.replace("Text = 'old';","Text = '"+text+"';"); require(editor->toPlainText()==configured && Moderator::Instance()->GetConfiguredObject("Text")->GetStringParameter("Expression")==text.toStdString(),"String initializer did not apply or altered mission assignment");
      require(window.runMission()==MainWindow::RunResult::Completed,"GUI parameter execution failed"); const auto expectedNumbers=read(numericReport),expectedStrings=read(stringReport); const auto numbers=expectedNumbers.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(numbers.size()==4 && numbers[0].toDouble()==5.125 && numbers[1].toDouble()==0 && numbers[2].toDouble()==10.25 && numbers[3].toDouble()==17.25 && expectedStrings.contains(text) && expectedStrings.contains("runtime"),"Parameter reports are wrong");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && read(numericReport)==expectedNumbers && read(stringReport)==expectedStrings,"Unicode scalar save/reopen changed calculations/text");
      // Independently scripted initial values exercise the same calculations.
      QString reference=source; reference.replace("GMAT n = 3;","GMAT n = 5.125;"); reference.replace("Text = 'old';","Text = 'O'Brien Δ';"); editor->setPlainText(reference); require(window.runMission()==MainWindow::RunResult::Completed && read(numericReport)==expectedNumbers && read(stringReport)==expectedStrings,"GUI/raw-script parameter reports differ");
      panel=open(window,"n"); value=panel->findChild<QLineEdit *>("parameterValue"); value->setText("bad"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(panel->hasChanges() && value->text()=="bad" && editor->toPlainText()==reference && Moderator::Instance()->GetConfiguredObject("n")->GetRealParameter("Value")==5.125,"Invalid value discarded pending edit or changed model"); value->setText("5.125"); closePanels(window);
      for (const auto &bad:QStringList{"","nan","inf","-inf","1e9999","1 + 2","n","3; n=5"}) require(!window.applyResourceChanges("n",{{"Value",bad}},reference).isEmpty() && editor->toPlainText()==reference,"Invalid numeric initializer changed source");
      for (const auto &bad:QStringList{"O'Brien; 50% Δ","x'; % comment","line\nbreak",QString(QChar(0))}) require(!window.applyResourceChanges("Text",{{"Value",bad}},reference).isEmpty() && editor->toPlainText()==reference && Moderator::Instance()->GetConfiguredObject("Text")->GetStringParameter("Expression")==text.toStdString(),"Unrepresentable string did not restore exact source/model");
      for (const auto &literal:QStringList{""," leading and trailing ","semi; 50% Δ","O''Brien","quote\" and back\\slash","x'; n=666; Text='y"}) {
         const auto before=editor->toPlainText(); const auto error=window.applyResourceChanges("Text",{{"Value",literal}},before); QApplication::processEvents(); require(error.isEmpty(),qPrintable("Literal ["+literal+"] rejected: "+error+"\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().right(2000)+"\n"+setUserParameterValue(before,"Text","String",literal))); require(Moderator::Instance()->GetConfiguredObject("Text")->GetStringParameter("Expression")==literal.toStdString() && Moderator::Instance()->GetConfiguredObject("n")->GetRealParameter("Value")==5.125,"String literal changed numeric configuration"); require(window.runMission()==MainWindow::RunResult::Completed && read(numericReport)==expectedNumbers,"String literal changed mission calculations");
      }
      // Default/omitted initializers must be inserted, including empty strings.
      const auto defaultSource=editor->toPlainText(); require(window.applyResourceChanges("m",{{"Value","-0.25"}},defaultSource).isEmpty() && Moderator::Instance()->GetConfiguredObject("m")->GetRealParameter("Value")==-0.25,"Default numeric initializer could not be inserted"); require(window.applyResourceChanges("Spare",{{"Value","new spare"}},editor->toPlainText()).isEmpty() && Moderator::Instance()->GetConfiguredObject("Spare")->GetStringParameter("Expression")=="new spare","Default string initializer could not be inserted");
      // Numeric spelling and omitted semicolons still preserve commands/comments.
      for (const auto &variant:QStringList{
         "Create Variable n;\nn = -2.5e-3; % keep scientific\nBeginMissionSequence;\nn = n + 10; % untouched\n",
         "Create Variable n\nn = 2\nBeginMissionSequence\nn = n + 10\n",
         "Create Variable n;\nn = 2;\nn = n + 10; % implicit mission\n",
         "Create Variable n; % no initializer\nBeginMissionSequence;\nn = 10; % keep executable assignment\n"}) {
         editor->setPlainText(variant); require(window.buildScript(),"Initializer syntax fixture failed"); const auto missionAt=variant.indexOf("n = n + 10"); const auto missionSuffix=missionAt<0 ? QString() : variant.mid(missionAt); require(window.applyResourceChanges("n",{{"Value","1.2345678901234567"}},variant).isEmpty(),"Initializer syntax edit rejected"); require(Moderator::Instance()->GetConfiguredObject("n")->GetRealParameter("Value")==QString("1.2345678901234567").toDouble(),"Numeric precision changed"); if (!missionSuffix.isEmpty()) require(editor->toPlainText().endsWith(missionSuffix),"Implicit/existing mission assignment was rewritten");
      }
      editor->setPlainText(reference); require(window.buildScript(),"Creation fixture restore failed"); const auto create=window.findChild<QAction *>("createResource"); require(create,"New resource action missing");
      modal(window,[&] { create->trigger(); },[&](QDialog *dialog) { dialog->findChild<QComboBox *>("resourceType")->setCurrentText("Variable"); dialog->findChild<QLineEdit *>("resourceName")->setText("Canceled"); scalarField(dialog)->setText("15"); }); require(editor->toPlainText()==reference && !Moderator::Instance()->GetConfiguredObject("Canceled"),"Create Cancel changed model");
      modal(window,[&] { create->trigger(); },[&](QDialog *dialog) {
         auto *type=dialog->findChild<QComboBox *>("resourceType"); auto *name=dialog->findChild<QLineEdit *>("resourceName");
         type->setCurrentText("Variable"); auto *initial=scalarField(dialog); require(initial->text()=="0","New Variable default incorrect"); initial->setText("-17.75"); type->setCurrentText("String"); initial=scalarField(dialog); initial->setText("retained text"); type->setCurrentText("Variable"); initial=scalarField(dialog); require(initial->text()=="-17.75","Type switch lost pending numeric value"); type->setCurrentText("String"); initial=scalarField(dialog); require(initial->text()=="retained text","Type switch lost pending string value"); type->setCurrentText("Variable"); initial=scalarField(dialog); name->setText("NewNumber"); initial->setText("bad"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); require(dialog->isVisible() && !Moderator::Instance()->GetConfiguredObject("NewNumber") && editor->toPlainText()==reference,"Invalid Create changed model/source or closed dialog"); initial->setText("-17.75"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      }); require(Moderator::Instance()->GetConfiguredObject("NewNumber")->GetRealParameter("Value")==-17.75,"Create initial numeric value ignored"); const auto afterCreation=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==reference,"Create Undo not atomic"); editor->redo(); require(editor->toPlainText()==afterCreation,"Create Redo not exact"); closePanels(window);
      modal(window,[&] { create->trigger(); },[&](QDialog *dialog) { dialog->findChild<QComboBox *>("resourceType")->setCurrentText("String"); dialog->findChild<QLineEdit *>("resourceName")->setText("NewString"); scalarField(dialog)->setText("new literal Δ"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); }); require(Moderator::Instance()->GetConfiguredObject("NewString")->GetStringParameter("Expression")=="new literal Δ","Create initial string value ignored"); closePanels(window);
      for (const auto &bad:QStringList{"","1Bad","bad-name","n","If","Propagate","GMAT"}) require(!window.createResource("Variable",bad,editor->toPlainText(),1,1,QString("2")).isEmpty(),"Invalid/reserved/duplicate resource name accepted");
      for (const auto &size:QList<QPair<int,int>>{{0,1},{1,0},{1001,1},{1,1001}}) require(!window.createResource("Array","InvalidArray",editor->toPlainText(),size.first,size.second).isEmpty(),"Invalid array dimensions accepted");
      require(window.createResource("Variable","InvalidNumber",editor->toPlainText(),1,1,QString("nan")).size()>0 && !Moderator::Instance()->GetConfiguredObject("InvalidNumber"),"Invalid create initialized resource");
      // wx creation accepts 1..1000 in either dimension. Test the full boundary
      // plus direct cell navigation, pending edits, cancellation and Apply.
      modal(window,[&] { create->trigger(); },[&](QDialog *dialog) {
         dialog->findChild<QComboBox *>("resourceType")->setCurrentText("Array"); auto *rows=dialog->findChild<QSpinBox *>("arrayRows"),*columns=dialog->findChild<QSpinBox *>("arrayColumns");
         require(rows->maximum()==1000 && columns->maximum()==1000 && !scalarField(dialog),"Array creation range/value visibility differs from wx");
         rows->setValue(1000); columns->setValue(1000); dialog->findChild<QLineEdit *>("resourceName")->setText("Boundary"); capture(dialog,screenshots.isEmpty() ? QString() : screenshots+".create.png"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      });
      auto *array=dynamic_cast<Array *>(Moderator::Instance()->GetConfiguredObject("Boundary")); require(array && array->GetIntegerParameter("NumRows")==1000 && array->GetIntegerParameter("NumCols")==1000 && array->GetRealParameter("SingleValue",999,999)==0,"Boundary array initialization incorrect");
      panel=open(window,"Boundary"); auto *editCells=panel->findChild<QPushButton *>("editCells_RmatValue"); require(editCells,"Boundary array editor missing");
      modal(*panel,[&] { editCells->click(); },[&](QDialog *dialog) {
         auto *row=dialog->findChild<QSpinBox *>("arrayCellRow"),*column=dialog->findChild<QSpinBox *>("arrayCellColumn"); auto *cell=dialog->findChild<QLineEdit *>("arrayCellValue"); auto *grid=dialog->findChild<QTableWidget *>("numericGrid"); require(row && column && cell && row->maximum()==1000 && column->maximum()==1000 && grid->horizontalHeader()->sectionResizeMode(0)==QHeaderView::Interactive,"Array cell controls/widths missing");
         row->setValue(1000); column->setValue(1000); cell->setText("-6.125"); dialog->findChild<QPushButton *>("arraySetCell")->click(); require(grid->currentRow()==999 && grid->currentColumn()==999 && grid->item(999,999)->text()=="-6.125" && Moderator::Instance()->GetConfiguredObject("Boundary")->GetRealParameter("SingleValue",999,999)==0,"Direct cell selection modified model or missed cell");
      }); require(!panel->hasChanges(),"Array cell Cancel changed pending matrix");
      modal(*panel,[&] { editCells->click(); },[&](QDialog *dialog) {
         dialog->findChild<QSpinBox *>("arrayCellRow")->setValue(1000); dialog->findChild<QSpinBox *>("arrayCellColumn")->setValue(1000); auto *cell=dialog->findChild<QLineEdit *>("arrayCellValue"); cell->setText("nan"); dialog->findChild<QPushButton *>("arraySetCell")->click(); require(dialog->findChild<QTableWidget *>("numericGrid")->item(999,999)->text()=="0","Invalid direct cell value changed grid"); cell->setText("-6.125"); dialog->findChild<QPushButton *>("arraySetCell")->click(); capture(dialog,screenshots.isEmpty() ? QString() : screenshots+".array.png"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      }); require(panel->hasChanges() && Moderator::Instance()->GetConfiguredObject("Boundary")->GetRealParameter("SingleValue",999,999)==0,"Accepted grid edited engine before Apply"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::processEvents(); require(Moderator::Instance()->GetConfiguredObject("Boundary")->GetRealParameter("SingleValue",999,999)==-6.125,"Full wx boundary cell Apply failed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && Moderator::Instance()->GetConfiguredObject("Boundary")->GetRealParameter("SingleValue",999,999)==-6.125,"Boundary array save/reopen lost cell");
      require(window.deleteResource("Boundary",editor->toPlainText()).isEmpty() && window.deleteResource("NewNumber",editor->toPlainText()).isEmpty() && window.deleteResource("NewString",editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed && read(numericReport)==expectedNumbers && read(stringReport)==expectedStrings,"Create/delete changed original calculations/text");
      std::cout<<"Qt parameters: numeric/string initial values, pending/Cancel/rollback, exact Undo, Unicode save/reopen, independent reports, implicit/no-semicolon source, quote fidelity, typed creation, reserved names and 1000x1000 array cell navigation/Apply passed\n";
      nasaParameterProof("parameters.variable-name","Variable",{"named-gui-create","configured-value","pending-before-apply","invalid-name-rejection","exact-undo-redo"});
      nasaParameterProof("parameters.variable-real","Variable",{"typed-value","pending-before-apply","nonfinite-rejection","precision","numeric-report","unicode-save-reopen"});
      nasaParameterProof("parameters.array-name-dimensions","Array",{"named-gui-create","dimension-boundaries","zero-initialization","configured-dimensions"});
      nasaParameterProof("parameters.array-real-cell","Array",{"typed-cell-value","zero-initialization","nonfinite-rejection","pending-before-apply","cancel","apply","unicode-save-reopen"});
      nasaParameterProof("parameters.string-create","String",{"named-gui-create","configured-value"});
      nasaParameterProof("parameters.string-quotes","String",{"typed-value","quote-fidelity","invalid-literal-rejection","unicode","exact-source","numeric-report-unchanged"});
      nasaParameterProof("parameters.runtime-string","String",{"runtime-string-assignment","report-text"});
      nasaParameterProof("parameters.multiply","Variable",{"scalar-multiplication","numeric-report"});
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
