// Reuse the existing summary driver's real panel/modal, unique numeric-field,
// source-history and artifact helpers. Its main and assertions are unchanged.
#define main originalCommandSummaryMain
#include "NasaCommandSummaryTests.cpp"
#undef main
#include "Moderator.hpp"
#include "GmatCommand.hpp"
#include <QCheckBox>
#include <QLabel>
#include <QMap>

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaSummaryA1");
   try {
      require(argc==2&&acceptanceWidgetPlatform(),"Supply A1 summary startup");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"A1 summary fixture directory missing");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &bytes) { if(!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
      const auto report=files.filePath("epochs.csv"),script=files.filePath("epochs.script");
      const QStringList satellites{"Alpha","Beta","Gamma"};
      const QList<double> initial{21545.25,21600.875,22000.125};
      QString source="% Independent cached A1 epoch display witnesses\n";
      for(int s=0;s<3;++s) {
         const auto name=satellites[s];
         source+="Create Spacecraft "+name+";\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '"+QString::number(initial[s],'g',17)+"';\n"+name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DisplayStateType = Cartesian;\n"+name+".X = 7000;\n"+name+".Y = 0;\n"+name+".Z = 0;\n"+name+".VX = 0;\n"+name+".VY = 7.5460532872678359;\n"+name+".VZ = 0;\n";
      }
      source+="Create ForceModel Gravity;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nGravity.Drag = None;\nGravity.SRP = Off;\nCreate Propagator Coast;\nCoast.FM = Gravity;\nCreate Variable Choose;\nChoose = 1;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n";
      const QString refs="Values Alpha.A1ModJulian Beta.A1ModJulian Gamma.A1ModJulian";
      source+="Report 'Before' "+refs+";\nPropagate 'FirstArc' Coast(Alpha) {Alpha.ElapsedSecs = 20};\nReport 'AfterFirst' "+refs+";\nIf 'BranchChoice' Choose == 1;\nPropagate 'BranchArc' Coast(Alpha) {Alpha.ElapsedSecs = 5};\nReport 'InBranch' "+refs+";\nElse 'Alternate';\nPropagate 'UnusedArc' Coast(Alpha) {Alpha.ElapsedSecs = 100};\nEndIf 'CloseChoice';\nPropagate 'BackArc' BackProp Coast(Beta) {Beta.ElapsedSecs = -10};\nReport 'AfterSecond' "+refs+";\nGMAT 'LateAlpha' Alpha.A1ModJulian = 23000.375;\nGMAT 'LateBeta' Beta.A1ModJulian = 24000.625;\nReport 'AfterAssignments' "+refs+";\n";
      save("mission.script",source.toUtf8());
      const QList<QList<double>> truth{initial,{initial[0]+20./86400.,initial[1],initial[2]},
         {initial[0]+25./86400.,initial[1],initial[2]},
         {initial[0]+25./86400.,initial[1]-10./86400.,initial[2]},
         {23000.375,24000.625,initial[2]}};
      const QStringList checkpoints{"Before","AfterFirst","InBranch","AfterSecond","AfterAssignments"};
      MainWindow window; require(window.initialize(startup),"A1 summary initialization failed");
      window.show(); events(); acceptanceView(window);
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Mission");
      auto *workspace=window.findChild<QMdiArea *>("workspace"); require(editor&&tree&&workspace,"A1 main controls missing");
      editor->setPlainText(source); require(window.saveScriptTo(script),"A1 script save failed");
      if(!window.buildScript()) { if(auto *m=window.findChild<QPlainTextEdit *>("messageWindow")) save("build-messages.txt",m->toPlainText().toUtf8()); throw std::runtime_error("A1 summary fixture build failed"); }
      require(window.runMission()==MainWindow::RunResult::Completed,"A1 summary fixture run failed");
      const auto raw=read(report); save("epochs.csv",raw); const auto rows=QString::fromUtf8(raw).trimmed().split('\n');
      require(rows.size()==truth.size(),"A1 independent report row count differs");
      int reportChecks=0,displayChecks=0,views=0,exports=0;
      for(int row=0;row<rows.size();++row) { const auto cells=rows[row].trimmed().split(','); require(cells.size()==3,"A1 report spacecraft count differs");
         for(int s=0;s<3;++s) { bool ok=false; const auto value=cells[s].toDouble(&ok); require(ok&&std::isfinite(value)&&std::abs(value-truth[row][s])<=1e-10,"A1 report differs from independent initial/elapsed/literal oracle"); ++reportChecks; }
      }
      const int undo=editor->document()->availableUndoSteps();
      auto stable=[&] { require(editor->toPlainText()==source&&!editor->document()->isModified()&&editor->document()->availableUndoSteps()==undo&&read(script)==source.toUtf8()&&read(report)==raw,"A1 inspection changed source/history/report/file"); };
      auto *cached=Moderator::Instance()->GetFirstCommand();
      while(cached&&cached->GetName()!="Before") cached=cached->GetNext();
      require(cached,"A1 exception test command missing"); cached->SetupSummary("MissingSummaryFrame");
      bool threw=false; try { cached->GetSummaryWithA1Epochs(); } catch(BaseException &) { threw=true; }
      cached->SetupSummary("EarthMJ2000Eq");
      require(threw&&QString::fromStdString(cached->GetStringParameter("Summary")).contains("UTC Epoch:")&&
              !QString::fromStdString(cached->GetStringParameter("Summary")).contains("A1 Epoch:"),
              "A1 formatting request leaked after a native rendering exception");
      QJsonArray observations;
      auto check=[&](const QString &text,int stage,const QString &tag) {
         auto scan=QRegularExpression("Spacecraft\\s*:\\s*(\\w+)\\s*\\n").globalMatch(text); QSet<QString> seen; QJsonArray values;
         while(scan.hasNext()) {
            const auto m=scan.next(); const auto name=m.captured(1); const int s=satellites.indexOf(name);
            require(s>=0&&!seen.contains(name),"A1 summary spacecraft duplicated/unknown"); seen.insert(name);
            const auto next=QRegularExpression("Spacecraft\\s*:").match(text,m.capturedEnd());
            const auto block=text.mid(m.capturedEnd(),(next.hasMatch()?next.capturedStart():text.size())-m.capturedEnd());
            auto fields=QRegularExpression("(?m)^\\s*A1 Epoch:\\s*([-+0-9.eE]+)\\s*$").globalMatch(block);
            require(fields.hasNext(),"A1 summary cached epoch missing"); bool ok=false; const double actual=fields.next().captured(1).toDouble(&ok);
            require(ok&&std::isfinite(actual)&&!fields.hasNext()&&std::abs(actual-truth[stage][s])<=6e-11,"A1 summary differs from independent cached epoch");
            values.append(QJsonObject{{"spacecraft",name},{"actual",actual},{"expected",truth[stage][s]}}); ++displayChecks;
         }
         require(seen==QSet<QString>{"Alpha","Beta","Gamma"},"A1 summary omitted configured/unpropagated spacecraft");
         observations.append(QJsonObject{{"stage",stage},{"tag",tag},{"values",values}}); save(tag+".txt",text.toUtf8()); stable();
      };
      auto strip=[](QString text) { text.remove(QRegularExpression("(?m)^        A1 Epoch:[^\\n]*\\n")); return text; };
      auto visit=[&](int index,int stage,const QString &tag,bool unavailable) {
         modal(window,[&] { window.showSummary(index); },[&](QWidget *base) {
            auto *dialog=dynamic_cast<SummaryDialog *>(base); auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText");
            auto *a1=dialog->findChild<QCheckBox *>("summaryA1Epoch"); auto *frame=dialog->findChild<QComboBox *>("summaryFrame");
            require(text&&text->isReadOnly()&&a1&&a1->isVisible()&&!a1->isChecked()&&frame,"A1 summary control/default missing");
            const auto original=text->toPlainText(); require(!original.contains("A1 Epoch:"),"Default summary format changed");
            QTest::mouseClick(a1,Qt::LeftButton); events(); acceptanceView(*dialog); require(a1->isChecked(),"A1 checkbox not selected");
            const auto enabled=text->toPlainText(); require(strip(enabled)==original,"A1 opt-in changed existing summary fields/format");
            save(tag+"-default.txt",original.toUtf8()); save(tag+"-enabled.txt",enabled.toUtf8());
            if(unavailable) require(enabled==original&&!enabled.contains("A1 Epoch:")&&enabled.contains("did not execute"),"Unexecuted A1 summary fabricated a cache");
            else {
               check(enabled,stage,tag);
               const auto path=files.filePath(tag+".txt"); require(dialog->exportText(path).isEmpty()&&read(path)==enabled.toUtf8(),"A1 summary export differs from display"); ++exports;
               frame->setCurrentText("EarthMJ2000Ec"); events(); require(frame->currentText()=="EarthMJ2000Ec","A1 frame change unavailable"); check(text->toPlainText(),stage,tag+"-ecliptic");
               frame->setCurrentText("EarthMJ2000Eq"); events(); require(text->toPlainText()==enabled,"A1 frame round trip changed cached data");
            }
            QTest::mouseClick(a1,Qt::LeftButton); events(); require(!a1->isChecked()&&text->toPlainText()==original,"A1 opt-out did not restore exact original format");
            stable(); ++views;
         });
      };
      // Use the actual command panel button for the first checkpoint, then
      // revisit old and new command caches through the same public modal route.
      auto namedNode=[&](const QString &name) {
         int selected=-1; const auto &nodes=window.missionSnapshot().nodes;
         for(int i=0;i<nodes.size();++i) if(missionCommandLabel(nodes[i].statement).name==name) {
            require(selected<0,"A1 exact command identity duplicated"); selected=i;
         }
         require(selected>=0,"A1 exact command identity missing"); return selected;
      };
      const int before=namedNode("Before"); QTreeWidgetItem *item=nullptr;
      for(QTreeWidgetItemIterator it(tree);*it;++it) if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==before) item=*it;
      require(item,"A1 command tree item missing"); tree->itemDoubleClicked(item,0); events();
      auto *sub=workspace->activeSubWindow(); auto *panel=sub?dynamic_cast<CommandEditor *>(sub->widget()):nullptr;
      require(panel,"A1 actual command editor missing"); auto *button=panel->findChild<QPushButton *>("showCommandSummary"); require(button&&button->isVisible()&&button->isEnabled(),"A1 command panel summary unreachable");
      modal(*panel,[&] { QTest::mouseClick(button,Qt::LeftButton); },[&](QWidget *dialog) {
         auto *a1=dialog->findChild<QCheckBox *>("summaryA1Epoch"); auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText"); require(a1&&text,"A1 actual panel controls absent");
         QTest::mouseClick(a1,Qt::LeftButton); events(); acceptanceView(*dialog); check(text->toPlainText(),0,"panel-before"); ++views;
      });
      sub->close(); events();
      for(int stage=0;stage<checkpoints.size();++stage) visit(namedNode(checkpoints[stage]),stage,checkpoints[stage],false);
      visit(namedNode("FirstArc"),1,"first-propagate",false);
      visit(namedNode("BranchArc"),2,"branch-propagate",false);
      visit(namedNode("BackArc"),3,"backward-propagate",false);
      visit(namedNode("UnusedArc"),0,"unexecuted",true);
      visit(before,0,"old-cache-revisited",false);
      // Full mission recursion and physics-only filtering must use the same
      // per-command formatting request, without leaking it into default reads.
      modal(window,[&] { window.showSummary(-1); },[&](QWidget *base) {
         auto *d=dynamic_cast<SummaryDialog *>(base); auto *a1=d->findChild<QCheckBox *>("summaryA1Epoch"); auto *physics=d->findChild<QCheckBox *>("summaryPhysics"); auto *text=d->findChild<QPlainTextEdit *>("inspectionText");
         require(a1&&physics&&physics->isVisible()&&text,"A1 mission summary controls missing");
         const auto original=text->toPlainText(); QTest::mouseClick(a1,Qt::LeftButton); events(); acceptanceView(*d);
         auto complete=[&](const QString &tag) { const auto contents=text->toPlainText(); const int count=contents.count("Spacecraft       :"); require(count>0&&contents.count("A1 Epoch:")==count,"A1 mission omitted or duplicated a cached spacecraft epoch"); save(tag+".txt",contents.toUtf8()); return count; };
         auto missionValues=[&](bool physicsOnly) {
            QMap<QString,int> expected;
            if(!physicsOnly) for(int i=0;i<checkpoints.size();++i) expected.insert(checkpoints[i],i);
            expected.insert("FirstArc",1); expected.insert("BranchArc",2); expected.insert("BackArc",3);
            const auto contents=text->toPlainText();
            for(auto it=expected.cbegin();it!=expected.cend();++it) {
               const auto start=QRegularExpression("(?m)^======  [A-Za-z]+ Command: "+QRegularExpression::escape(it.key())+"\\s*$").match(contents);
               require(start.hasMatch(),"A1 mission omitted a required command identity");
               const auto end=QRegularExpression("(?m)^======  ").match(contents,start.capturedEnd());
               const auto block=contents.mid(start.capturedEnd(),(end.hasMatch()?end.capturedStart():contents.size())-start.capturedEnd());
               check(block,it.value(),QString(physicsOnly?"physics-":"mission-")+it.key());
            }
         };
         const int fullCount=complete("mission-a1"); missionValues(false); require(strip(text->toPlainText())==original,"Mission A1 opt-in changed existing fields");
         QTest::mouseClick(physics,Qt::LeftButton); events(); acceptanceView(*d); const int physicsCount=complete("mission-physics-a1"); missionValues(true); require(physicsCount==9&&physicsCount<fullCount,"A1 physics summary filter differs from three executed propagations");
         QTest::mouseClick(a1,Qt::LeftButton); events(); require(!text->toPlainText().contains("A1 Epoch:"),"A1 presentation leaked after opt-out");
         QTest::mouseClick(physics,Qt::LeftButton); events(); require(text->toPlainText()==original,"Mission default summary not restored exactly"); ++views; stable();
      });
      save("observations.json",QJsonDocument(observations).toJson());
      const QJsonObject proof{{"checkpoint","summary-a1-cached-epochs"},{"outcome","passed"},{"spacecraft",3},{"command_views",views},{"independent_report_checks",reportChecks},{"display_checks",displayChecks},{"exports",exports},{"native_exception_restored",true},{"default_summary_unchanged",true},{"legacy_assertions_unchanged",true},{"facets",QJsonArray{"actual-panel-checkbox-and-modal","independent-initial-elapsed-backward-and-assignment-epochs","unpropagated-spacecraft-and-old-command-cache","frame-round-trip-export-and-opt-out","branch-unexecuted-and-mission-physics-filter","source-history-report-isolation"}}};
      save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; stable(); return 0;
   } catch(BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
   catch(std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
