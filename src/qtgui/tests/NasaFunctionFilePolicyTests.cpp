#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "FunctionFileDialog.hpp"
#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTest>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
static void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Function policy input unreadable");return f.readAll();}
static void write(const QString &path,const QByteArray &data){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"Function policy artifact write failed");}
static void replace(QPlainTextEdit *editor,const QString &value){require(editor&&editor->isVisible()&&!editor->isReadOnly(),"Function policy editor not exposed/editable");acceptanceView(*editor);editor->setFocus();QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QApplication::clipboard()->setText(value);QTest::keyClick(editor,Qt::Key_V,Qt::ControlModifier);events();require(editor->toPlainText()==value,"Function policy keyboard edit differs");}
int main(int argc,char **argv){
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaFunctionFilePolicy");
 try{
  require(argc==3&&acceptanceWidgetPlatform(),"Supply function policy startup and case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Function policy fixture directory missing");
  const QString mode=argv[2];require(QStringList{"Utf8Literal","PrefixRecovery","TrailingSuffix","InvalidUtf8Comment"}.contains(mode),"Unknown function file policy case");
  const bool string=mode=="Utf8Literal",prefix=mode=="PrefixRecovery",invalid=mode=="InvalidUtf8Comment",trailing=mode=="TrailingSuffix";
  const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto directory=files.filePath("Function policies Ω");require(QDir().mkpath(directory),"Function policy Unicode directory missing");
  const auto function=directory+(trailing?"/Encoded.gmf.txt":"/Encoded.gmf"),report=files.filePath("policy report Ω.csv"),saved=files.filePath("policy mission Ω.script");
  const QString header="function [out] = Encoded()\nCreate "+QString(string?"String":"Variable")+" out;\nBeginMissionSequence;\n";
  const QString originalText=header+(string?QString::fromUtf8("out = 'café Ω';\n"):QString("out = 14;\n"));
  QByteArray original=originalText.toUtf8();if(prefix)original.prepend(QString::fromUtf8("% Ω\n").toUtf8());if(invalid)original.prepend(QByteArray("% ascii prefix ")+QByteArray(1,char(0xff))+"\n");write(function,original);
  const QString wanted=prefix?QString::fromUtf8("% ascii prefix Ω\n")+originalText:string?QString(originalText).replace(QString::fromUtf8("café Ω"),QString::fromUtf8("naïve Δ")):QString(originalText).replace("out = 14;","out = 21;");
  const auto artifact=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &data){if(!artifact.isEmpty())write(QDir(artifact).filePath(name),data);};save("initial-function.bin",original);save("intended-edited-function.gmf",wanted.toUtf8());
  const QString mission="% Keep the caller unchanged Ω\nCreate GmatFunction Exercise;\nExercise.FunctionPath = '"+function+"';\nCreate "+QString(string?"String":"Variable")+" Result;\nResult = "+QString(string?"'caller'":"-901")+";\nCreate Variable Keep;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n[Result] = Exercise();\nKeep = Keep + 1;\nReport Values Result Keep;\n% Retain this tail Ω\n";save("initial.script",mission.toUtf8());
  MainWindow window;window.resize(1100,850);window.show();require(window.initialize(startup),"Function policy native runtime unavailable");acceptanceView(window);auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");require(source&&messages,"Function policy main controls missing");replace(source,mission);int history=source->document()->availableUndoSteps();
  auto stable=[&]{require(source->toPlainText()==mission&&source->document()->availableUndoSteps()==history,"Function policy file operation changed caller/history");};
  const bool built=window.buildScript();save("initial-build-messages.txt",messages->toPlainText().toUtf8());stable();require(built!=prefix,"Native function prefix policy differs from declared boundary");if(prefix)require(messages->toPlainText().contains("ASCII"),"Leading UTF-8 rejection lost native ASCII diagnostic");
  QJsonArray runs,decisions;auto run=[&](const QString &phase,const QJsonValue &expected){stable();acceptanceView(window);messages->clear();const auto status=window.runMission();events();save(phase+"-messages.txt",messages->toPlainText().toUtf8());if(status!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(status==MainWindow::RunResult::Completed,"Function file policy mission did not complete");auto *result=Moderator::Instance()->GetInternalObject("Result");const QJsonValue native=string?QJsonValue(QString::fromStdString(result->GetStringParameter("Value"))):QJsonValue(result->GetRealParameter("Value"));require(native==expected&&Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Function policy native value differs from independent literal");const auto raw=read(report);save(phase+"-values.csv",raw);const auto fields=QString::fromUtf8(raw).trimmed().split(',');require(fields.size()==2&&fields[1].toDouble()==42&&(string?fields[0].trimmed()==expected.toString():fields[0].toDouble()==expected.toDouble()),"Function policy raw report differs from checked native literal");stable();require(!window.isRunning()&&window.findChild<QAction *>("runMission")->isEnabled(),"Function policy run did not restore controls");auto *configured=Moderator::Instance()->GetConfiguredObject("Result");require(string?configured->GetStringParameter("Value")=="caller":configured->GetRealParameter("Value")==-901,"Function policy run changed configured prototype");runs.append(QJsonObject{{"phase",phase},{"native",native},{"keep",42},{"configured_prototype_preserved",true}});};
  const QJsonValue first=string?QJsonValue(QString::fromUtf8("café Ω")):QJsonValue(14);if(!prefix)run("initial",first);
  // These are real file-editor widgets; their launch from the resource panel
  // is covered by preserved existing cases. Here the new scope is file bytes
  // versus native consumption, including files rejected by the current Build.
  auto edit=[&](bool commit){FunctionFileDialog dialog(function,&window);dialog.show();events();acceptanceView(dialog);auto *text=dialog.findChild<QPlainTextEdit *>("functionFileText");auto *buttons=dialog.findChild<QDialogButtonBox *>();require(text&&buttons,"Function policy file editor controls missing");
   if(invalid){require(text->isReadOnly()&&!buttons->button(QDialogButtonBox::Save)->isEnabled()&&!dialog.findChild<QPushButton *>("functionSaveAs")->isEnabled(),"Invalid UTF-8 file left saving/editing enabled");bool message=false;for(auto *label:dialog.findChildren<QLabel *>())message=message||label->text().contains("not valid UTF-8");require(message,"Invalid UTF-8 editor diagnostic missing");require(read(function)==original,"Opening malformed file changed its bytes");QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);decisions.append(QJsonObject{{"readonly_invalid_utf8",true},{"saved",false}});stable();return;}
   const auto expected=QString::fromUtf8(original);require(!text->isReadOnly()&&text->toPlainText()==expected&&!text->document()->isModified(),"Function policy editor decoded wrong content");replace(text,wanted);require(text->document()->isModified()&&read(function)==original,"Function policy draft wrote file before Save");stable();acceptanceView(dialog);QTest::mouseClick(buttons->button(commit?QDialogButtonBox::Save:QDialogButtonBox::Cancel),Qt::LeftButton);require(dialog.result()==(commit?QDialog::Accepted:QDialog::Rejected)&&read(function)==(commit?wanted.toUtf8():original),"Function policy Save/Cancel changed wrong bytes");stable();decisions.append(QJsonObject{{"saved",commit},{"readonly_invalid_utf8",false}});
  };
  edit(false);if(!invalid){edit(true);save("saved-function.bin",read(function));messages->clear();require(window.buildScript(),"Function policy edited file failed native rebuild/recovery");save("edited-build-messages.txt",messages->toPlainText().toUtf8());run(prefix?"recovered":"edited",string?QJsonValue(QString::fromUtf8("naïve Δ")):QJsonValue(prefix?14:21));}
  require(window.saveScriptTo(saved)&&read(saved)==mission.toUtf8()&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==mission,"Function file policy caller save/reopen changed source");history=source->document()->availableUndoSteps();run("reopened",string?QJsonValue(QString::fromUtf8("naïve Δ")):QJsonValue(invalid||prefix?14:21));save("final-function.bin",read(function));save("native-results.json",QJsonDocument(runs).toJson());
  const QJsonObject proof{{"checkpoint","function-file-policy-"+mode},{"outcome","passed"},{"case",mode},{"filename",QFileInfo(function).fileName()},{"initial_native_build_accepted",built},{"editor_rejects_invalid_utf8",invalid},{"native_prefix_rejection_recovered",prefix},{"non_gmf_final_suffix_accepted",trailing},{"runs",runs},{"decisions",decisions},{"facets",QJsonArray{"actual-file-editor-native-byte-interoperability","caller-history-and-file-save-cancel-isolation","independent-literal-native-and-raw-report","native-rebuild-and-Unicode-reopen"}},{"scope","Characterization of current editor/parser behavior against unresolved historical .gmf/ASCII-only requirement. Passing checks do not qualify NASA's restriction:UTF-8 literals,malformed comment bytes and a .gmf.txt filename expose acceptance beyond that wording. No universal encoding/filesystem/platform/native OS/science/stress/current-private NASA or newest-build qualification."}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
