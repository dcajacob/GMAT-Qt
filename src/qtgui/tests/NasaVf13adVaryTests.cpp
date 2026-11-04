#include "Vf13adAcceptance.hpp"
#include "CommandEditor.hpp"
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <QTreeWidgetItemIterator>
using namespace vfaccept;
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaVf13adVary");try{
   require(argc==3,"VF13ad startup and control case required");Rig rig(QFileInfo(argv[1]).absoluteFilePath());const QString mode=argv[2];Options base,changed;QString field;
   if(mode=="Initial"){field="Initial value";changed.initial=1;}
   else if(mode=="Perturbation"){field="Perturbation";changed.perturbation=.02;}
   else if(mode=="MaxStep"){field="MaxStep";changed.maxStep=.25;}
   else if(mode=="Additive"){field="AdditiveScaleFactor";changed.additive=3;}
   else if(mode=="Multiplicative"){field="MultiplicativeScaleFactor";changed.multiplicative=2;}
   else throw std::runtime_error("Unknown VF13ad control case");
   const auto original=rig.script(base);rig.source->setPlainText(original);rig.save("authored.script",original.toUtf8());const bool built=rig.window.buildScript();events();if(!built)rig.save("build.failure.txt",rig.window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toUtf8());require(built,"VF13ad baseline source did not build");rig.run("baseline",base,mode=="MaxStep");
   QTreeWidgetItem *entry=nullptr;for(QTreeWidgetItemIterator it(rig.mission);*it;++it)if((*it)->text(0).contains("VF control")){require(!entry,"VF13ad control label ambiguous");entry=*it;}require(entry,"VF13ad actual mission-tree Vary missing");
   rig.mission->expandAll();rig.mission->scrollToItem(entry);rig.mission->setCurrentItem(entry);events();rig.mission->itemDoubleClicked(entry,0);events();auto *child=rig.area->activeSubWindow();auto *panel=child?dynamic_cast<CommandEditor *>(child->widget()):nullptr;require(panel,"VF13ad actual Vary MDI unavailable");child->showNormal();child->resize(rig.area->viewport()->size());child->move(0,0);acceptanceView(*child);
   for(const auto &name:QStringList{field}){auto *input=child->findChild<QLineEdit *>("commandField_"+name);require(input&&input->isVisible()&&input->isEnabled(),"Selected VF13ad supported Vary option hidden/disabled");}
   for(const auto &name:QStringList{"Lower","Upper"}){auto *input=child->findChild<QLineEdit *>("commandField_"+name);require(input&&input->isVisible()&&!input->isEnabled()&&input->toolTip().contains("does not use"),"VF13ad unsupported bound control incorrectly enabled or unexplained");}
   auto *input=child->findChild<QLineEdit *>("commandField_"+field);const auto changedValue=mode=="Initial"?changed.initial:mode=="Perturbation"?changed.perturbation:mode=="MaxStep"?changed.maxStep:mode=="Additive"?changed.additive:changed.multiplicative;
   input->setFocus();input->selectAll();QTest::keyClicks(input,number(changedValue));require(input->text()==number(changedValue)&&panel->hasChanges()&&rig.source->toPlainText()==original,"VF13ad typed option escaped pending state");acceptanceView(*child);
   const auto expected=QString(original).replace(vary(base),vary(changed));auto *apply=child->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);require(apply&&apply->isEnabled(),"VF13ad Vary Apply unavailable");QTest::mouseClick(apply,Qt::LeftButton);events();require(rig.source->toPlainText()==expected,"VF13ad option Apply changed unrelated source or lost value");const int steps=rig.source->document()->availableUndoSteps();
   child=rig.area->activeSubWindow();require(child,"VF13ad Vary panel disappeared after Apply");apply=child->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);QTest::mouseClick(apply,Qt::LeftButton);events();require(rig.source->toPlainText()==expected&&rig.source->document()->availableUndoSteps()==steps,"VF13ad repeated Apply changed source/history");rig.closePanels();rig.run("gui",changed,mode=="MaxStep");
   rig.source->undo();require(rig.source->toPlainText()==original&&rig.window.buildScript(),"VF13ad Vary Undo not exact");rig.run("undo",base,mode=="MaxStep");rig.source->redo();require(rig.source->toPlainText()==expected&&rig.window.buildScript(),"VF13ad Vary Redo not exact");rig.run("redo",changed,mode=="MaxStep");
   const auto saved=rig.files.filePath(mode+" VF control Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.source->toPlainText()==expected,"VF13ad control Unicode save/reopen changed source");rig.run("reopened",changed,mode=="MaxStep");
   const auto reference=rig.script(changed);rig.source->setPlainText(reference);rig.run("script",changed,mode=="MaxStep");
   const QJsonObject proof{{"checkpoint","vf13ad-vary-"+mode},{"outcome","passed"},{"family","vf13ad-vary"},{"control",mode},{"phases",6},{"facets",QJsonArray{"actual-vf13ad-vary-mdi-and-capability-controls","typed-single-field-pending-apply-idempotence-history-reopen","independent-initial-probe-quadratic-optimum-and-native-scaled-state","retained-raw-trials-and-independent-script-route"}},{"scope","Selected live VF13ad scalar constrained quadratic, forward differences and one field per case. Raw physical trials, documented scaled native variable, analytic constrained optimum and explicit MaxStep activation where requested. Not every optimizer/problem/conditioning/domain/name/backend, physical mission science, OS input/platform/hardware/stress or complete current-build/NASA provenance."}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
}catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;}
