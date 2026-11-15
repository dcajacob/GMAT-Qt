#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "SpiceInterface.hpp"
#include <QAbstractItemView>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTest>
using namespace formaccept;
static void reveal(QWidget *w){require(w,"Auxiliary event input missing");for(auto *p=w;p;p=p->parentWidget())if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(w);events();require(w->isVisible()&&w->isEnabled(),"Auxiliary event input inaccessible");acceptanceView(*w);}
static void click(QDialogButtonBox *b,QDialogButtonBox::StandardButton which){auto *button=b?b->button(which):nullptr;reveal(button);QTest::mouseClick(button,Qt::LeftButton);events();}
#include "EventRootSettingsAcceptance.hpp"
static QVector<qint64> eventEpochs(const QString &text){
   QVector<qint64> result;const QRegularExpression stamp("(\\d{2} [A-Za-z]{3} \\d{4} \\d{2}:\\d{2}:\\d{2}\\.\\d{3})");
   for(const auto &line:text.split('\n')){
      auto matches=stamp.globalMatch(line);QVector<qint64> row;
      while(matches.hasNext()){const auto value=QDateTime::fromString(matches.next().captured(1),"dd MMM yyyy HH:mm:ss.zzz");require(value.isValid(),"Auxiliary event timestamp invalid");row.append(value.toMSecsSinceEpoch());}
      if(row.size()>=2 || (row.size()==1 && (line.trimmed().endsWith("Start") || line.trimmed().endsWith("End"))))result+=row;
   }
   return result;
}
int main(int argc,char **argv){
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("EventBrentAuxiliary");
   try{
      require(argc==3,"Supply startup and Intrusion/Region");Rig rig(QFileInfo(argv[1]).absoluteFilePath());const QString kind=argv[2];require(kind=="Intrusion"||kind=="Region","Unknown auxiliary event kind");
      const QString sample=kind=="Intrusion"?"Ex_IntrusionLocator_Mercury_Sun_Transit.script":"Ex_PlanetographicRegion.script";
      const auto originalSample=read("../samples/"+sample);QString source=QString::fromUtf8(originalSample);
      source.replace(QRegularExpression(kind=="Intrusion"?"\\bIL\\b":"\\bCL\\b"),"Events");
      const auto report=rig.files.filePath("Auxiliary Brent Ω.txt");const QRegularExpression fileLine("^[ \\t]*Events\\.Filename[ \\t]*=[^\\r\\n]*$",QRegularExpression::MultilineOption);
      require(source.contains(fileLine),"Sample event filename assignment absent");source.replace(fileLine,"Events.Filename = '"+report+"';");
      const int start=source.indexOf("BeginMissionSequence");require(start>=0,"Sample mission boundary absent");source.insert(start,"Events.StepSize = 120;\nEvents.RunMode = Automatic;\nEvents.WriteReport = true;\n");
      rig.source->setPlainText(source);require(rig.window.buildScript(),"Auxiliary SPICE baseline build failed");
      const auto baselineStatus=rig.window.runMission();events();rig.save("SPICE-baseline.messages.txt",rig.messages->toPlainText().toUtf8());rig.save("adapted-SPICE.script",source.toUtf8());
      if(baselineStatus!=MainWindow::RunResult::Completed){
         SpiceInt count=0;ktotal_c("ALL",&count);QJsonArray kernels;
         for(SpiceInt i=0;i<count;++i){char file[2048],type[64],origin[2048];SpiceInt handle;SpiceBoolean found;kdata_c(i,"ALL",2048,64,2048,file,type,origin,&handle,&found);if(found){kernels.append(QJsonObject{{"file",file},{"type",type}});if(QString(file).endsWith(".tsc"))rig.save("failed-clock-"+QString::number(i)+".tsc",read(file));}}
         rig.save("failed-kernel-inventory.json",QJsonDocument(kernels).toJson());
      }
      require(baselineStatus==MainWindow::RunResult::Completed,"Auxiliary SPICE baseline run failed");const auto reference=read(report);rig.save("SPICE-reference.txt",reference);rig.save("adapted-SPICE.script",source.toUtf8());
      const auto expected=eventEpochs(QString::fromUtf8(reference));require(expected.size()>=2,"Sample baseline produced no interval rows");
      if(kind=="Intrusion")require(reference.contains("Mercury")&&!reference.contains("No intrusion events"),"Sample baseline lacks Mercury intrusion");
      require(rig.window.buildScript(),"Auxiliary source reset failed");auto *native=Moderator::Instance()->GetConfiguredObject("Events");require(native,"Auxiliary configured locator absent");const auto original=native->GetGeneratingString(Gmat::SCRIPTING);const int undo=rig.source->document()->availableUndoSteps();
      auto pristine=[&]{return rig.source->toPlainText()==source&&Moderator::Instance()->GetConfiguredObject("Events")==native&&native->GetGeneratingString(Gmat::SCRIPTING)==original&&rig.source->document()->availableUndoSteps()==undo;};
      auto open=[&]{auto items=rig.resources->findItems("Events",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Auxiliary locator tree item ambiguous");rig.resources->itemDoubleClicked(items.first(),0);events();auto *child=rig.area->activeSubWindow();auto *p=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(p,"Actual auxiliary event editor absent");return p;};
      eventrootaccept::Witness witness;
      for(bool cancel:{true,false}){
         auto *panel=open();auto *form=panel->findChild<QDialog *>("eventLocatorDialog");require(form&&form->isVisible(),"Auxiliary locator form not visible");
         auto combo=[&](const QString &name,const QString &wanted){auto *box=form->findChild<QComboBox *>("event_"+name);reveal(box);const int index=box->findText(wanted);require(index>=0,"Auxiliary event choice missing");if(box->currentIndex()!=index){QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,QPoint(box->width()-10,box->height()/2));events();require(box->view()->isVisible(),"Auxiliary choice popup did not open");acceptanceView(*box->view()->window());QTest::keyClick(box->view(),Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(box->view(),Qt::Key_Down);QTest::keyClick(box->view(),Qt::Key_Return);events();}require(box->currentText()==wanted,"Auxiliary event choice changed");};
         witness.configure(panel,form,cancel,combo,pristine);
         if(cancel){rig.closePanels();require(pristine(),"Auxiliary draft discard changed native/source/history");}
         else {click(panel->findChild<QDialogButtonBox *>("resourceButtons"),QDialogButtonBox::Apply);require(!panel->hasChanges(),"Auxiliary Brent Apply failed");rig.closePanels();}
      }
      const auto applied=rig.source->toPlainText();require(applied!=source,"Auxiliary root settings did not serialize");
      rig.source->undo();require(rig.source->toPlainText()==source&&rig.window.buildScript(),"Auxiliary exact Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Auxiliary exact Redo failed");
      const auto saved=rig.files.filePath("Auxiliary Brent Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Auxiliary Unicode roundtrip failed");
      require(rig.window.runMission()==MainWindow::RunResult::Completed,"Auxiliary Brent run failed");events();const auto observed=read(report);rig.save("Brent-report.txt",observed);rig.save("Brent-applied.script",applied.toUtf8());
      const auto actual=eventEpochs(QString::fromUtf8(observed));require(actual.size()==expected.size(),"Brent changed auxiliary event interval count");QJsonArray comparisons;
      for(int i=0;i<actual.size();++i){require(std::abs(actual[i]-expected[i])<=2,"Brent auxiliary event differs from SPICE by over2ms");comparisons.append(QJsonObject{{"SPICE_epoch_ms",double(expected[i])},{"Brent_epoch_ms",double(actual[i])}});}
      require(rig.source->toPlainText()==applied,"Auxiliary event execution changed source");auto proof=witness.observe();proof["case"]=kind;proof["outcome"]="passed";proof["event_epoch_comparisons"]=comparisons;proof["input_sample_sha256"]=QString::fromLatin1(QCryptographicHash::hash(originalSample,QCryptographicHash::Sha256).toHex());proof["scope"]="Actual additional locator/region Brent path and SPICE coherence; no independent physical-orbit or geometry approval";
      rig.save("proof.json",QJsonDocument(proof).toJson());return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
