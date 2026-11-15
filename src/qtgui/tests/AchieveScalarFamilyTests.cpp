#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include "GmatCommand.hpp"
#include <QApplication>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QComboBox>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *m){if(!ok)throw std::runtime_error(m);}
static void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &p){QFile f(p);require(f.open(QIODevice::ReadOnly),"Missing family artifact");return f.readAll();}
static void write(const QString &p,const QByteArray &b){QFile f(p);require(f.open(QIODevice::WriteOnly)&&f.write(b)==b.size(),"Cannot save family artifact");}
static QString number(double x){return QString::number(x,'g',17);}
int main(int argc,char **argv){
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("AchieveScalarFamily");
 try{
  require(argc==5&&acceptanceWidgetPlatform(),"Supply startup, fixture, key, physical root");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Private fixture unavailable");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(),fixture=QFileInfo(argv[2]).absoluteFilePath(),key=QString::fromLocal8Bit(argv[3]),root=QString::fromLocal8Bit(argv[4]);const auto all=QJsonDocument::fromJson(read(fixture)).object();QJsonObject spec;
  for(auto v:all["cases"].toArray())if(v.toObject()["key"]==key){require(spec.isEmpty(),"Duplicate family key");spec=v.toObject();}require(!spec.isEmpty(),"Unknown family key");
  const auto inputs=all["physical_inputs"].toObject();for(auto it=inputs.begin();it!=inputs.end();++it)require(QString::fromLatin1(QCryptographicHash::hash(read(QDir(root).filePath(it.key())),QCryptographicHash::Sha256).toHex())==it.value().toString(),"Physical input hash changed");
  const auto reference=spec["reference"].toString(),type=spec["parameter_type"].toString();const double truth=spec["expected"].toDouble(),tol=spec["absolute_tolerance"].toDouble(),ctol=spec["solver_control_tolerance"].toDouble(),scale=std::max(1.,std::abs(truth)*.001),requested=tol;require(std::isfinite(truth)&&tol>0,"Invalid independent oracle");
  const auto report=files.filePath("Achieve result Δ.csv"),saved=files.filePath("Achieve accepted Δ.script"),evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &n,const QByteArray &b){if(!evidence.isEmpty())write(QDir(evidence).filePath(n),b);};
  QString initial=all["base_setup"].toString().replace("@ROOT@",root)+spec["setup"].toString();
  initial+="Create Variable Control Bound Seed Keep;\nControl = "+number(truth+scale)+";\nSeed = "+number(truth+scale)+";\nBound = "+number(truth+20*scale)+";\nKeep = 41;\nCreate String Label;\nLabel = 'not numeric';\nCreate Array Grid[1,2];\nGrid(1,1) = 3;\nCreate DifferentialCorrector DC;\nDC.ShowProgress = false;\nDC.MaximumIterations = 20;\nDC.ReportFile = '"+files.filePath("solver.txt")+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n"+all["mission_prefix"].toString()+"Target 'Family target' DC {ShowProgressWindow = false};\nVary 'Independent control' DC(Control = Seed, {Perturbation = "+number(scale*1e-5)+", Lower = "+number(truth-10*scale)+", Upper = "+number(truth+10*scale)+", MaxStep = "+number(5*scale)+", AdditiveScaleFactor = "+number(-truth)+", MultiplicativeScaleFactor = "+number(1/scale)+"});\nAchieve 'Family goal' DC(Bound = "+number(truth)+", {Tolerance = "+number(requested)+"});\nEndTarget;\nKeep = Keep + 1;\nReport Values Control "+reference+" Keep;\n% preserve family tail Ω\n";
  QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;require(window.initialize(startup),"Family runtime initialization failed");window.show();acceptanceView(window);events();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Family workspace missing");source->setPlainText(initial);if(!window.buildScript()){save("build.messages.txt",messages->toPlainText().toUtf8());std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Family baseline did not build");}save("baseline.script",initial.toUtf8());
  int index=-1;const auto snapshot=window.missionSnapshot();for(int i=0;i<snapshot.nodes.size();++i)if(missionCommandLabel(snapshot.nodes[i].statement).name=="Family goal")index=i;require(index>=0,"Achieve goal node missing");QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index)item=*it;require(item,"Achieve goal tree item missing");tree->expandAll();tree->setCurrentItem(item);tree->scrollToItem(item);events();tree->itemDoubleClicked(item,0);events();auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Actual Achieve editor missing");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);acceptanceView(*child);
  auto *field=child->findChild<QLineEdit *>("commandField_Goal");auto *choose=child->findChild<QPushButton *>("commandChoose_Goal");require(field&&choose&&field->text()=="Bound","Actual measured-goal controls missing");std::exception_ptr failure;bool visited=false;
  QTimer::singleShot(0,child,[&]{auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget());try{
   require(dialog,"Actual Achieve parameter picker missing");visited=true;acceptanceView(*dialog);auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry"),*owner=dialog->findChild<QComboBox *>("reportPropertyObject"),*property=dialog->findChild<QComboBox *>("reportPropertyType"),*dep=dialog->findChild<QComboBox *>("reportPropertyDependency");auto *buttons=dialog->findChild<QDialogButtonBox *>();auto *use=dialog->findChild<QPushButton *>("reportUseReference");require(entry&&owner&&property&&dep&&buttons&&use,"Actual family browser missing");
   for(const auto &bad:QStringList{"nan","inf","Label","Grid","Grid(2,1)","MissingScalar.Unknown"}){entry->setEditText(bad);events();require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Achieve picker accepted nonscalar or missing reference");}
   auto parts=reference.split('.');require(owner->findText(parts[0])>=0,"Achieve owner not offered");owner->setCurrentText(parts[0]);events();require(property->findText(type)>=0,"Achieve scalar type not offered");property->setCurrentText(type);events();if(parts.size()==3){require(dep->isVisible()&&dep->findText(parts[1])>=0,"Achieve dependency not offered");dep->setCurrentText(parts[1]);}require(use->isEnabled(),"Achieve use reference disabled");use->click();events();if(parts[0]=="Burn"&&parts.size()==2)entry->setEditText(reference);require(entry->currentText()==reference&&buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Achieve exact reference rejected");buttons->button(QDialogButtonBox::Ok)->click();
  }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});choose->click();if(failure)std::rethrow_exception(failure);require(visited&&field->text()==reference&&source->toPlainText()==initial,"Achieve picker prematurely mutated source");
  const auto before="Achieve 'Family goal' DC(Bound = "+number(truth)+", {Tolerance = "+number(requested)+"});",after="Achieve 'Family goal' DC("+reference+" = "+number(truth)+", {Tolerance = "+number(requested)+"});";QString accepted=initial;require(accepted.count(before)==1,"Goal boundary ambiguous");accepted.replace(before,after);child->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();require(source->toPlainText()==accepted,"Achieve Apply changed unrelated source");auto *panel=dynamic_cast<CommandEditor *>(child->widget());require(panel&&!panel->hasChanges(),"Achieve Apply did not finish");require(child->close(),"Achieve editor did not close");events();source->undo();require(source->toPlainText()==initial&&window.buildScript(),"Achieve Undo failed");source->redo();require(source->toPlainText()==accepted&&window.buildScript(),"Achieve Redo failed");require(window.saveScriptTo(saved)&&read(saved)==accepted.toUtf8()&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==accepted,"Achieve Unicode persistence failed");save("accepted.script",accepted.toUtf8());const auto undo=source->document()->availableUndoSteps();messages->clear();const auto outcome=window.runMission();events();save("run.messages.txt",messages->toPlainText().toUtf8());if(outcome!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(outcome==MainWindow::RunResult::Completed&&source->toPlainText()==accepted&&source->document()->availableUndoSteps()==undo,"Achieve execution failed or changed history");
  const auto raw=read(report);save("results.csv",raw);const auto rows=QString::fromUtf8(raw).trimmed().split('\n');require(rows.size()==1,"Achieve report row count differs");const auto cols=rows[0].split(',');require(cols.size()==3,"Achieve report column count differs");QJsonArray actual;for(auto c:cols){bool ok=false;double v=c.trimmed().toDouble(&ok);require(ok&&std::isfinite(v),"Nonfinite Achieve report");actual.append(v);}require(std::abs(actual[0].toDouble()-(truth+scale))<=ctol&&std::abs(actual[1].toDouble()-truth)<=tol&&actual[2].toDouble()==42,"Achieve result differs from independent physical operand/control/sentinel");require(std::abs(actual[1].toDouble()-truth)<=requested,"Achieve missed requested residual");
  GmatCommand *goal=nullptr;
  for(auto *outer=Moderator::Instance()->GetFirstCommand();outer;outer=outer->GetNext())if(outer->GetTypeName()=="Target")for(auto *c=outer->GetChildCommand(0);c&&c!=outer;c=c->GetNext())if(c->GetTypeName()=="Achieve"){require(!goal,"Duplicate native Achieve goal");goal=c;}
  require(goal,"Executed native Achieve command missing");const double achieved=goal->GetRealParameter("AchievedValue");require(std::isfinite(achieved)&&std::abs(achieved-truth)<=requested&&std::abs(achieved-actual[1].toDouble())<=tol,"Native measured goal differs from independent physical truth");
  const QJsonObject proof{{"checkpoint","achieve-scalar-family"},{"outcome","passed"},{"case",key},{"parameter_type",type},{"reference",reference},{"expected",truth},{"absolute_tolerance",tol},{"solver_control_tolerance",ctol},{"requested_tolerance",requested},{"actual",actual},{"source_rows",spec["source_rows"]},{"picker_visited",visited},{"invalid_picker_rejections",6},{"native_achieved",achieved},{"independent_control_unchanged",std::abs(actual[0].toDouble()-(truth+scale))<=ctol},{"scope","Actual complete scalar-family Achieve input/selector/execution contract; parameter physical truth is reused at its recorded scope, not whole current-build/scientific approval."}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
