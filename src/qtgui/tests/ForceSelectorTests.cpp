#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "UserParameter.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "ODEModel.hpp"
#include "PropSetup.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read selector report"); return QString::fromUtf8(file.readAll()); }
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name)
{
   auto *table=panel.findChild<QTableWidget *>(); require(table,"Properties table missing");
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) return table->item(row,1);
   throw std::runtime_error(("Missing property "+name).toStdString());
}
static void select(ResourceEditor &panel,const QString &name,const QString &value)
{
   auto *table=panel.findChild<QTableWidget *>(); require(table,"Properties table missing");
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) {
      auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1)); require(choice && choice->findText(value)>=0,"Selector choices missing"); choice->setCurrentText(value); return;
   }
   throw std::runtime_error(("Missing selector "+name).toStdString());
}
static void parserCases()
{
   const QString before="Create ForceModel FM;\nFM.SRP = Off;\nFM.RelativisticCorrection = Off;\nFM.ErrorControl = RSSStep;\n";
   const QString after="Create ForceModel FM;\nFM.SRP = On;\nFM.SRP.Flux = 1400;\nFM.RelativisticCorrection = On;\nFM.ErrorControl = LargestStep;\n";
   const QString fixed="FM.PrimaryBodies = {Earth}; % preserve primary\nFM.GravityField.Earth.Degree = 0; % preserve degree\nFM.Drag.AtmosphereModel = NRLMSISE00; % preserve drag\n";
   const QString source="Create ForceModel FM;\n"+fixed+"  FM.SRP  = ... % preserve continuation\n Off; % preserve selector\nFM.Flux = 1300; % preserve flux comment\nBeginMissionSequence;\nFM.SRP = Off; % preserve runtime\n";
   const auto enabled=patchResourceConfiguration(source,"FM",before,after,{},true);
   require(enabled.contains(fixed) && enabled.contains("FM.SRP  =") && enabled.contains("% preserve continuation") && enabled.contains("% preserve selector") && enabled.contains("% preserve flux comment"),"Toggle rewrote unrelated fields/creator syntax/comments");
   require(!enabled.contains("FM.Flux = 1300") && enabled.contains("FM.SRP.Flux = 1400") && enabled.contains("FM.ErrorControl = LargestStep"),"Mixed leaf/root settings not retained");
   require(enabled.indexOf("FM.RelativisticCorrection = On")<enabled.indexOf("BeginMissionSequence"),"New correction selector reached mission");
   require(enabled.endsWith("BeginMissionSequence;\nFM.SRP = Off; % preserve runtime\n"),"Selector patch changed runtime assignment");
   const auto disabled=patchResourceConfiguration(enabled,"FM",after,before,{},true);
   require(disabled.contains(fixed) && !disabled.contains("FM.SRP.Flux") && disabled.contains("FM.SRP = Off") && disabled.contains("FM.RelativisticCorrection = Off"),"Toggle removal retained dependent fields or rewrote unrelated settings");
   const auto implicit=source.left(source.indexOf("BeginMissionSequence"))+"Propagate P(S);\nFM.SRP = Off;\n";
   require(patchResourceConfiguration(implicit,"FM",before,after,"Propagate P(S);",true).endsWith("Propagate P(S);\nFM.SRP = Off;\n"),"Toggle lost implicit mission boundary");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtForceSelectors");
   try {
      require(argc==2,"Startup argument required"); parserCases(); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto report=files.filePath("states.txt");
      const QString fixed="FM.PrimaryBodies = {Earth}; % exact primary\n GMAT FM.GravityField.Earth.Degree = 0; % exact gravity\nFM.GravityField.Earth.Order = 0;\nFM.PointMasses = {Luna}; % unrelated point mass\n";
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 1800}; % exact mission α\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
      auto fixture=[&](bool srp,bool relat,double flux,const QString &control) {
         QString force="FM.SRP = "+QString(srp ? "On" : "Off")+"; % selector comment\nFM.RelativisticCorrection = "+QString(relat ? "On" : "Off")+"; % correction comment\nFM.ErrorControl = "+control+"; % mixed comment\n";
         if (srp) force+="FM.SRP.Flux = "+QString::number(flux,'g',17)+"; % flux comment\n";
         return "Create Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2024 12:00:00.000';\nSat.X = 7200;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.4;\nSat.VZ = 0;\nSat.SRPArea = 1000;\nCreate ForceModel FM;\n"+fixed+force+
            "Create Propagator Prop;\nProp.FM = FM;\nProp.Accuracy = 1e-13;\nProp.InitialStepSize = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\n"+mission;
      };
      auto run=[&] {
         if (window.runMission()!=MainWindow::RunResult::Completed) { app.processEvents(); throw std::runtime_error("Selector execution failed: "+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); }
         return read(report);
      };
      auto reference=[&](bool srp,bool relat,double flux,const QString &control) { editor->setPlainText(fixture(srp,relat,flux,control)); return run(); };
      const auto base=reference(false,false,1367,"RSSStep"),srp=reference(true,false,1367,"RSSStep"),both=reference(true,true,1400,"LargestStep"),relat=reference(false,true,1367,"LargestStep");
      require(base!=srp && srp!=both && base!=relat,"Independent fixtures did not exercise perturbation effects");
      QSet<QString> runtimeStates;
      int runtimeComparisons=0;
      auto compare=[&](const QString &expected) {
         const auto actual=run().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts),wanted=expected.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(actual.size()==6 && wanted.size()==6,"State report incomplete");
         for (int i=0;i<6;++i) { bool valid=false; const auto a=actual[i].toDouble(&valid),b=wanted[i].toDouble(); require(valid && std::isfinite(a) && std::abs(a-b)<1e-12*std::max(1.0,std::abs(b)),"GUI toggle state differs from independently configured script"); }
         auto *runtimeProp=dynamic_cast<PropSetup *>(Moderator::Instance()->GetInternalObject("Prop"));
         auto *runtime=runtimeProp ? runtimeProp->GetODEModel() : nullptr;
         auto *selected=dynamic_cast<ODEModel *>(Moderator::Instance()->GetConfiguredObject("FM"));
         require(runtime && selected,"Runtime/configured force model missing");
         const auto activeSrp=runtime->GetOnOffParameter("SRP"),activeRelativity=runtime->GetOnOffParameter("RelativisticCorrection");
         require(activeSrp==selected->GetOnOffParameter("SRP") && activeRelativity==selected->GetOnOffParameter("RelativisticCorrection"),"Mission runtime force families differ from GUI configuration");
         if (activeSrp=="On") {
            bool spherical=false;
            for (const auto &property:resourceProperties(*runtime))
               if (property.name=="SRP.SRPModel") spherical=property.value=="Spherical";
            require(spherical,"Mission did not use the expected spherical SRP model");
         }
         runtimeStates.insert(QString::fromStdString(activeSrp+":"+activeRelativity));
         ++runtimeComparisons;
      };
      auto open=[&] {
         auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto matches=tree->findItems("FM",Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"Force model resource missing"); tree->itemDoubleClicked(matches.first(),0);
         for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="FM") return child;
         throw std::runtime_error("Actual force model panel missing");
      };
      auto configured=[&] { return dynamic_cast<ODEModel *>(Moderator::Instance()->GetConfiguredObject("FM")); };
      auto apply=[&](bool srp,bool relat,const QString &control,bool fail) {
         const auto source=editor->toPlainText(); auto *child=open(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Actual editor missing");
         select(*panel,"SRP",srp ? "On" : "Off"); select(*panel,"RelativisticCorrection",relat ? "On" : "Off"); select(*panel,"ErrorControl",control);
         if (fail) field(*panel,"SRP.Flux")->setText("-1");
         require(editor->toPlainText()==source && panel->hasChanges(),"Selector bypassed pending Apply");
         auto *button=panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply); button->click();
         if (fail) {
            require(editor->toPlainText()==source && child->widget()==panel && panel->hasChanges() && configured()->GetOnOffParameter("RelativisticCorrection")=="Off","Invalid mixed selector Apply changed source/configuration/pending panel");
            field(*panel,"SRP.Flux")->setText("1400"); button->click();
         }
         const auto changed=editor->toPlainText(); if (changed==source) throw std::runtime_error("Selector Apply did not change source: "+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString());
         require(configured()->GetOnOffParameter("SRP")==std::string(srp ? "On" : "Off") && configured()->GetOnOffParameter("RelativisticCorrection")==std::string(relat ? "On" : "Off"),"Toggle was silently ignored");
         require(child->isVisible() && child->widget()!=panel && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges(),"Selector Apply did not refresh retained panel");
         require(changed.contains(fixed) && changed.endsWith(mission) && changed.contains("% selector comment") && changed.contains("% correction comment") && changed.contains("% mixed comment"),"Toggle changed unrelated source or comments");
         require(!changed.contains("FM.Drag") && !changed.contains("FM.PolyhedralBodies") && !changed.contains("FM.UserDefined"),"Toggle made other defaults explicit");
         if (!srp) require(!changed.contains("FM.SRP.Flux"),"Disabled SRP retained owned leaf");
         editor->undo(); require(editor->toPlainText()==source,"Toggle Undo was not exact"); editor->redo(); require(editor->toPlainText()==changed,"Toggle Redo was not exact");
         const auto saved=files.filePath(QString("%1-%2 Δ.script").arg(srp).arg(relat)); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Toggle Unicode save/reopen failed");
      };
      editor->setPlainText(fixture(false,false,1367,"RSSStep")); require(window.buildScript(),"Toggle fixture failed");
      apply(true,false,"RSSStep",false); compare(srp);
      apply(true,true,"LargestStep",true); compare(both);
      apply(false,true,"LargestStep",false); compare(relat);
      apply(false,false,"RSSStep",false); compare(base);
      apply(true,false,"RSSStep",false); compare(srp);
      const auto stable=editor->toPlainText(); require(!window.applyResourceChanges("FM",{{"SRP","invalid"}},stable).isEmpty() && editor->toPlainText()==stable,"Invalid toggle bypassed validation");
      // A simultaneous drag removal must retain the newly enabled families,
      // their dependent data and the ordinary field in the same transaction.
      auto dragSource=fixture(false,false,1367,"RSSStep"); dragSource.replace("Create Propagator Prop;","FM.Drag.AtmosphereModel = NRLMSISE00; % drag removal comment\nCreate Propagator Prop;");
      editor->setPlainText(dragSource); require(window.buildScript(),"Mixed drag/toggle fixture failed");
      const auto mixedError=window.applyResourceChanges("FM",{{"Drag.AtmosphereModel","None"},{"SRP","On"},{"SRP.Flux","1400"},{"RelativisticCorrection","On"},{"ErrorControl","LargestStep"}},dragSource);
      if (!mixedError.isEmpty()) throw std::runtime_error(mixedError.toStdString());
      const auto mixed=editor->toPlainText(); require(mixed.contains(fixed) && mixed.endsWith(mission) && mixed.contains("% drag removal comment") && configured()->GetStringParameter("Drag")=="None","Mixed creator families lost source or drag removal");
      editor->undo(); require(editor->toPlainText()==dragSource,"Mixed creator families were not one exact Undo"); editor->redo(); require(editor->toPlainText()==mixed,"Mixed creator families Redo was not exact");
      const auto mixedFile=files.filePath("Mixed creators α.script"); require(window.saveScriptTo(mixedFile) && window.loadScript(mixedFile) && window.buildScript(),"Mixed creator save/reopen failed"); compare(both);
      require(runtimeComparisons>=6 && runtimeStates==QSet<QString>{"Off:Off","On:Off","Off:On","On:On"},"Runtime force validation did not cover every on/off combination");
      for (const auto &family:QStringList{"srp-spherical","relativity"}) {
         const QJsonObject proof{{"checkpoint","force-family."+family},{"family",family},{"outcome","passed"},
            {"facets",QJsonArray{"gui-toggles","all-on-off-combinations","runtime-family","spherical-srp","atomic-mixed-apply","exact-undo-redo","unicode-save-reopen","script-reference-trajectory"}}};
         std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      }
      std::cout<<"PASS: real SRP/relativistic On/Off creation/removal, pending mixed Apply with invalid flux correction, retained panels, unchanged unrelated force source/comments/defaults, exact Undo/Redo/Unicode save/reopen and independent six-state reports; source mapping with continued selectors, legacy leaves and implicit mission boundary.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
