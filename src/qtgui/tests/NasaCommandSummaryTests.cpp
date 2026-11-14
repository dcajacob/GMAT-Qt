#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "InspectionDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &p) { QFile f(p); require(f.open(QIODevice::ReadOnly),"Summary artifact missing"); return f.readAll(); }
static void write(const QString &p,const QByteArray &b) { QFile f(p); require(f.open(QIODevice::WriteOnly)&&f.write(b)==b.size(),"Summary artifact write failed"); }
static double number(const QString &text,const QString &label) {
   const auto matches=QRegularExpression("(?:^|\\s)"+QRegularExpression::escape(label)+"\\s*=\\s*([-+0-9.eE]+)").globalMatch(text);
   auto scan=matches; require(scan.hasNext(),qPrintable("Summary numeric field missing: "+label));
   bool ok=false; const auto value=scan.next().captured(1).toDouble(&ok);
   require(ok&&std::isfinite(value)&&!scan.hasNext(),"Summary numeric field invalid or duplicated"); return value;
}
static double epoch(const QString &text,const QString &system) {
   const auto match=QRegularExpression("(?m)^\\s*"+system+"\\s+Epoch:\\s+.+?\\s+([-+0-9.eE]+)\\s*$").match(text);
   bool ok=false; const double value=match.captured(1).toDouble(&ok);
   require(match.hasMatch()&&ok&&std::isfinite(value),"Summary epoch row invalid or missing"); return value;
}
template<class Open,class Inspect> static void modal(QWidget &owner,Open open,Inspect inspect) {
   std::exception_ptr error; bool visited=false; QTimer watcher; watcher.setInterval(20);
   QObject::connect(&watcher,&QTimer::timeout,&owner,[&] {
      auto *dialog=QApplication::activeModalWidget(); if(!dialog) return; watcher.stop();
      try { require(dynamic_cast<SummaryDialog *>(dialog),"Actual summary dialog missing"); visited=true; acceptanceView(*dialog); inspect(dialog); }
      catch(...) { error=std::current_exception(); }
      if(dialog->isVisible()) static_cast<QDialog *>(dialog)->reject();
   });
   watcher.start(); open(); watcher.stop(); if(error) std::rethrow_exception(error); require(visited,"Summary modal was not visited");
}
static int node(const MainWindow &window,const QString &name) {
   const auto &nodes=window.missionSnapshot().nodes;
   for(int i=0;i<nodes.size();++i) if(nodes[i].statement.contains("'"+name+"'")) return i;
   throw std::runtime_error("Named summary command missing");
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaCommandSummary");
   try {
      require(argc==2&&acceptanceWidgetPlatform(),"Supply command-summary startup"); TestSettings settings; QTemporaryDir files;
      require(files.isValid(),"Summary fixture directory missing"); const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath()); const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &bytes) { if(!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
      const auto report=files.filePath("summary Δ.csv"),saved=files.filePath("summary Ω.script");
      const QStringList satellites={"Alpha","Beta"};
      const QStringList parameters={"EarthMJ2000Eq.X","EarthMJ2000Eq.Y","EarthMJ2000Eq.Z","EarthMJ2000Eq.VX","EarthMJ2000Eq.VY","EarthMJ2000Eq.VZ",
         "Earth.SMA","Earth.ECC","EarthMJ2000Eq.INC","EarthMJ2000Eq.RAAN","EarthMJ2000Eq.AOP","Earth.TA","Cd","DragArea","Cr","SRPArea","DryMass","TotalMass",
         "UTCModJulian","TAIModJulian","TTModJulian","TDBModJulian","A1ModJulian"};
      const QList<QList<double>> elements={{9000,.2,35,40,50,60},{13000,.35,65,120,210,150}};
      const QList<QList<double>> properties={{2.3,14,1.7,5,810,847},{1.9,8,1.3,11,420,439}};
      QString source="% NASA command summary field and cached-state witnesses Ω\nCreate ChemicalTank FuelA FuelB;\nFuelA.FuelMass = 37;\nFuelB.FuelMass = 19;\nCreate Spacecraft Alpha Beta;\n";
      const QStringList orbit={"SMA","ECC","INC","RAAN","AOP","TA"},props={"Cd","DragArea","Cr","SRPArea","DryMass"};
      for(int s=0;s<2;++s) {
         const auto sat=satellites[s]; source+=sat+".CoordinateSystem = EarthMJ2000Eq;\n"+sat+".DateFormat = A1ModJulian;\n"+sat+".Epoch = '21545.25';\n"+sat+".DisplayStateType = Keplerian;\n"+sat+".Tanks = {"+(s==0?"FuelA":"FuelB")+"};\n";
         for(int i=0;i<6;++i) source+=sat+"."+orbit[i]+" = "+QString::number(elements[s][i],'g',17)+";\n";
         for(int i=0;i<5;++i) source+=sat+"."+props[i]+" = "+QString::number(properties[s][i],'g',17)+";\n";
      }
      source+="Create ForceModel Gravity;\nGravity.CentralBody = Earth;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nGravity.Drag = None;\nGravity.SRP = Off;\nCreate Propagator Coast;\nCoast.FM = Gravity;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n";
      QStringList reports; for(const auto &sat:satellites) for(const auto &p:parameters) reports.append(sat+"."+p);
      source+="Report 'Initial values' Values "+reports.join(' ')+";\n";
      source+="Alpha.Cd = 2.6;\nAlpha.DragArea = 16;\nAlpha.Cr = 1.6;\nAlpha.SRPArea = 7;\nAlpha.DryMass = 900;\nAlpha.FuelA.FuelMass = 31;\nPropagate 'Two spacecraft coast' Coast(Alpha, Beta) {Alpha.ElapsedSecs = 120};\nReport 'Final values' Values "+reports.join(' ')+";\n";
      MainWindow window; require(window.initialize(startup),"Summary initialization failed"); window.show(); events(); acceptanceView(window);
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Summary main source missing"); editor->setPlainText(source);
      save("mission.script",source.toUtf8());
      require(window.saveScriptTo(saved),"Summary fixture save failed");
      const bool built=window.buildScript();
      if(!built) if(auto *messages=window.findChild<QPlainTextEdit *>("messageWindow")) std::cerr<<messages->toPlainText().toStdString()<<std::endl;
      require(built,"Summary fixture build failed");
      require(window.runMission()==MainWindow::RunResult::Completed,"Summary fixture execution failed");
      const auto raw=read(report); save("values.csv",raw); const auto lines=QString::fromUtf8(raw).trimmed().split('\n'); require(lines.size()==2,"Summary reference report row count differs");
      QList<QList<double>> values; for(const auto &line:lines) { QList<double> row; for(const auto &field:line.split(',')) { bool ok=false; const double v=field.trimmed().toDouble(&ok); require(ok&&std::isfinite(v),"Summary reference report invalid"); row.append(v); } require(row.size()==46,"Summary reference report column count differs"); values.append(row); }
      QJsonArray observations; int compared=0,initialChecks=0; const int undo=editor->document()->availableUndoSteps();
      auto stable=[&] { require(editor->toPlainText()==source&&!editor->document()->isModified()&&editor->document()->availableUndoSteps()==undo&&read(saved)==source.toUtf8()&&read(report)==raw,"Summary inspection changed source/history/mission/report"); };
      const QStringList labels={"X","Y","Z","VX","VY","VZ","SMA","ECC","INC","RAAN","AOP","TA","Cd","Drag area","Cr","Reflective (SRP) area","Dry mass","Total mass"};
      auto check=[&](const QString &text,int stage,const QString &tag) {
         save(tag+".txt",text.toUtf8()); require(text.contains("Cartesian State")&&text.contains("Keplerian State"),"Summary state headings missing");
         const QRegularExpression marker("Spacecraft\\s*:\\s*(\\w+)\\s*\\n"); auto scan=marker.globalMatch(text); QSet<QString> seen;
         while(scan.hasNext()) {
            const auto m=scan.next(); const auto name=m.captured(1); const int s=satellites.indexOf(name); require(s>=0&&!seen.contains(name),"Summary cross-spacecraft name duplicated or unknown"); seen.insert(name);
            const auto next=marker.match(text,m.capturedEnd()); const auto block=text.mid(m.capturedEnd(),(next.hasMatch()?next.capturedStart():text.size())-m.capturedEnd());
            require(block.contains("Coordinate System: EarthMJ2000Eq"),"Summary coordinate label wrong"); QJsonArray observed;
            for(int i=0;i<18;++i) { const double actual=number(block,labels[i]),expected=values[stage][s*23+i]; require(std::abs(actual-expected)<=std::max(1e-7,std::abs(expected)*1e-10),qPrintable("Summary field disagrees with command report: "+name+" "+labels[i])); observed.append(actual); ++compared; }
            for(int i=0;i<4;++i) { const double actual=epoch(block,QStringList{"UTC","TAI","TT","TDB"}[i]),expected=values[stage][s*23+18+i]; require(std::abs(actual-expected)<=6e-11,"Summary modified-Julian epoch differs from command report"); observed.append(actual); ++compared; }
            require(!block.contains("A1 Epoch:"),"A1 source discrepancy changed; review epoch gap before qualification");
            observations.append(QJsonObject{{"stage",stage},{"tag",tag},{"spacecraft",name},{"values",observed},{"a1_displayed",false}});
         }
         require(seen==QSet<QString>{"Alpha","Beta"},"Summary omitted an unpropagated/configured spacecraft"); stable();
      };
      // Non-default initial elements and properties establish concrete expected values,
      // rather than trusting two display paths that might share a wrong default.
      for(int s=0;s<2;++s) {
         for(int i=0;i<6;++i) { require(std::abs(values[0][s*23+6+i]-elements[s][i])<1e-8,"Initial independent Keplerian fixture differs"); ++initialChecks; }
         for(int i=0;i<6;++i) { require(std::abs(values[0][s*23+12+i]-properties[s][i])<1e-10,"Initial independent mass/ballistic fixture differs"); ++initialChecks; }
         require(std::abs(values[0][s*23+22]-21545.25)<1e-12&&std::abs((values[1][s*23+22]-values[0][s*23+22])*86400-120)<1e-5,"Two-spacecraft epoch progression differs"); initialChecks+=2;
      }
      const QList<double> finalProps={2.6,16,1.6,7,900,931};
      for(int i=0;i<6;++i) { require(std::abs(values[1][12+i]-finalProps[i])<1e-10&&std::abs(values[1][23+12+i]-properties[1][i])<1e-10,"Summary property mutation leaked or tank-inclusive mass wrong"); initialChecks+=2; }
      require(std::abs(values[0][0]-values[1][0])>1&&std::abs(values[0][23]-values[1][23])>1,"Propagated summary state did not change");
      auto inspect=[&](QWidget *dialog,int stage,const QString &tag) { auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText"); auto *frame=dialog->findChild<QComboBox *>("summaryFrame"); require(text&&text->isReadOnly()&&frame&&frame->currentText()=="EarthMJ2000Eq","Actual summary controls/default/read-only missing"); check(text->toPlainText(),stage,tag); text->moveCursor(QTextCursor::End); acceptanceView(*dialog); stable(); };
      auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *workspace=window.findChild<QMdiArea *>("workspace"); require(tree&&workspace,"Summary mission tree/workspace missing");
      const int first=node(window,"Initial values"),last=node(window,"Final values"),coast=node(window,"Two spacecraft coast");
      auto panel=[&](int index,int stage,const QString &tag) { QTreeWidgetItem *target=nullptr; for(QTreeWidgetItemIterator it(tree);*it;++it) if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index) target=*it; require(target,"Summary command tree item missing"); tree->itemDoubleClicked(target,0); events(); auto *sub=workspace->activeSubWindow(); auto *command=sub?dynamic_cast<CommandEditor *>(sub->widget()):nullptr; require(command,"Actual summary command panel missing"); auto *button=command->findChild<QPushButton *>("showCommandSummary"); require(button&&button->isVisible()&&button->isEnabled(),"Summary command-panel button unreachable"); acceptanceView(*command); modal(*command,[&] { QTest::mouseClick(button,Qt::LeftButton); },[&](QWidget *d) { inspect(d,stage,tag); }); sub->close(); events(); stable(); };
      panel(last,1,"final-summary"); panel(first,0,"initial-summary-after-run");
      modal(window,[&] { window.showSummary(coast); },[&](QWidget *d) { inspect(d,1,"coast-summary"); });
      // Revisit the first command after other views: cached fields must stay at its
      // own epoch and retain the old Alpha properties and tank-inclusive mass.
      modal(window,[&] { window.showSummary(first); },[&](QWidget *d) { inspect(d,0,"initial-summary-revisited"); });
      save("observations.json",QJsonDocument(observations).toJson()); stable();
      const QJsonObject proof{{"checkpoint","command-summary-fields"},{"outcome","passed"},{"family","command-summary"},{"spacecraft",2},{"command_views",4},{"compared_values",compared},{"independent_fixture_checks",initialChecks},{"a1_gap",true},{"facets",QJsonArray{"actual-command-panel-summary-after-execution","all-two-spacecraft-names-and-earthmj2000eq","cartesian-and-keplerian-values-at-command-epoch","cached-ballistic-area-dry-and-tank-inclusive-total-mass","four-current-modified-julian-time-rows","old-command-cache-revisit-and-source-history-report-isolation"}},{"scope","Two Earth-centred nondegenerate elliptic spacecraft, initial Report and 120-second point-mass Propagate/Report summaries; Alpha property/tank mutation and unchanged Beta, old-command revisit. Selected reported values and fixed initial expectations; independent orbit/time science, A1 display discrepancy, arbitrary commands/orbits/frames/native OS input/platform/hardware/stress and full current-build qualification remain open."}};
      save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   } catch(BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
   catch(std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
