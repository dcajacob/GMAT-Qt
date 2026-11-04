#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTabWidget>
#include <QTest>
#include <QTreeWidgetItemIterator>
using namespace formaccept;

static void reveal(QWidget *widget)
{
   require(widget,"Direction control missing");
   for (QWidget *p=widget;p;p=p->parentWidget()) {
      if (auto *tabs=qobject_cast<QTabWidget *>(p->parentWidget()))
         if (tabs->indexOf(p)>=0) tabs->setCurrentWidget(p);
      if (auto *scroll=qobject_cast<QScrollArea *>(p)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible()&&widget->isEnabled(),"Direction control unreachable"); acceptanceView(*widget);
}

static void checkState(const QJsonArray &row,double seconds)
{
   require(row.size()==7&&std::abs(row[0].toDouble()-seconds)<.001,"Direction sample epoch/cardinality differs");
   const auto truth=circular({"Sat",7000,0,0},seconds);
   for (int i=0;i<6;++i) {
      const double value=row[i+1].toDouble();
      require(std::isfinite(value)&&std::abs(value-truth[i])<(i<3?2e-5:2e-8),
              "Direction result disagrees with independent circular orbit");
   }
}

static QJsonArray oem(const QByteArray &raw,int direction)
{
   const auto epoch=QDateTime::fromString("2000-01-01T12:00:00.000Z",Qt::ISODateWithMs);
   QJsonArray segments,rows;
   auto finish=[&] { if (!rows.isEmpty()) { segments.append(rows); rows={}; } };
   for (const auto &line:QString::fromUtf8(raw).split('\n')) {
      const auto text=line.trimmed();
      if (text=="META_START") finish();
      if (!QRegularExpression("^\\d{4}-\\d\\d-\\d\\dT").match(text).hasMatch()) continue;
      const auto fields=text.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
      require(fields.size()==7,"Direction OEM field count differs");
      auto date=fields[0]; if (!date.endsWith('Z')) date+='Z';
      const auto parsed=QDateTime::fromString(date,Qt::ISODateWithMs);
      require(parsed.isValid(),"Direction OEM epoch unreadable");
      QJsonArray row{epoch.msecsTo(parsed)/1000.};
      for (int i=1;i<7;++i) { bool ok=false; const double value=fields[i].toDouble(&ok); require(ok&&std::isfinite(value),"Direction OEM state nonfinite"); row.append(value); }
      rows.append(row);
   }
   finish(); require(segments.size()==2,"OEM direction change did not produce two independent arcs");
   for (int s=0;s<2;++s) {
      const auto samples=segments[s].toArray(); require(samples.size()==11,"Direction segment lacks exact fixed-step samples");
      for (int i=0;i<11;++i) checkState(samples[i].toArray(),direction*(s==0?i*10:100-i*10));
   }
   return segments;
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaEphemerisDirection");
   try {
      require(argc==3,"Expected ephemeris direction startup and case");
      const QString key=argv[2]; const bool code500=key=="Code500Reversal";
      require(code500||key=="OemForwardReverse"||key=="OemBackwardReverse","Unknown ephemeris direction case");
      const int direction=key=="OemBackwardReverse"?-1:1;
      Rig rig(QFileInfo(argv[1]).absoluteFilePath()); const auto ephemeris=rig.files.filePath("Direction output Ω.eph"),saved=rig.files.filePath("Direction mission Δ.script");
      const double speed=std::sqrt(mu/7000.);
      QString initial="% Direction fixture Ω\nCreate Variable Done;\nDone = 0;\nCreate Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2000 12:00:00.000';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+number(speed)+";\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.Type = RungeKutta89;\nProp.FM = FM;\nProp.InitialStepSize = 5;\nProp.MinStep = 5;\nProp.MaxStep = 5;\nProp.Accuracy = 1e-12;\nCreate EphemerisFile Export;\nExport.Spacecraft = Sat;\nExport.FileFormat = CCSDS-OEM;\nExport.CoordinateSystem = EarthMJ2000Eq;\nExport.StepSize = 10;\nExport.InterpolationOrder = 3;\nExport.WriteEphemeris = false;\nExport.Filename = '"+ephemeris+"';\nCreate ReportFile Truth;\nTruth.Filename = '"+rig.report+"';\nTruth.WriteHeaders = false;\nTruth.FixedWidth = false;\nTruth.Delimiter = ',';\nTruth.Precision = 17;\n";
      const auto report="Report Truth Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Done;\n";
      const QString mission="BeginMissionSequence;\nPropagate 'First direction' "+QString(direction<0?"BackProp ":"")+"Prop(Sat) {Sat.ElapsedSecs = "+QString::number(direction*100)+"};\n"+report+"Propagate 'Reverse direction' "+QString(direction>0?"BackProp ":"")+"Prop(Sat) {Sat.ElapsedSecs = "+QString::number(-direction*100)+"};\nDone = 9;\n"+report+"% Direction tail Δ\n";
      initial+=mission;rig.source->setPlainText(initial);require(rig.window.buildScript(),"Direction fixture failed to build");rig.source->document()->setModified(false);rig.save("initial.script",initial.toUtf8());
      auto open=[&] {
         auto *tree=rig.window.findChild<QTreeWidget *>("Resources");require(tree,"Direction output tree absent");
         for (auto *tabs:rig.window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
         const auto found=tree->findItems("Export",Qt::MatchExactly|Qt::MatchRecursive);require(found.size()==1,"Direction Export item absent");tree->scrollToItem(found[0]);reveal(tree);
         QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(found[0]).center());events();QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(found[0]).center());events();
         auto *child=rig.area->currentSubWindow();require(child&&dynamic_cast<ResourceEditor *>(child->widget()),"Direction actual resource form absent");return child;
      };
      auto configure=[&](const QString &format) {
         auto *child=open();auto *panel=dynamic_cast<ResourceEditor *>(child->widget());
         auto *combo=panel->findChild<QComboBox *>("ephemeris_FileFormat"),*step=panel->findChild<QComboBox *>("ephemeris_StepSize");auto *flag=panel->findChild<QCheckBox *>("ephemeris_WriteEphemeris");
         reveal(combo);combo->setCurrentText(format);require(combo->currentText()==format,"Direction file format choice lost");reveal(step);step->setCurrentText("10");reveal(flag);
         if (!flag->isChecked()) { QStyleOptionButton option;option.initFrom(flag);QTest::mouseClick(flag,Qt::LeftButton,Qt::NoModifier,flag->style()->subElementRect(QStyle::SE_CheckBoxIndicator,&option,flag).center()); }
         require(flag->isChecked(),"Direction writer checkbox stayed disabled");acceptanceView(*child);
         auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");require(buttons,"Direction resource buttons absent");auto *apply=buttons->button(QDialogButtonBox::Apply);reveal(apply);QTest::mouseClick(apply,Qt::LeftButton);events();
         auto *configured=Moderator::Instance()->GetConfiguredObject("Export");require(configured&&configured->GetStringParameter("FileFormat")==format.toStdString()&&configured->GetBooleanParameter("WriteEphemeris")&&!panel->hasChanges(),"Direction Apply did not commit native exporter");
         require(rig.source->toPlainText().endsWith(mission),"Direction Apply changed mission semantics");child->close();events();
      };
      configure(code500?"Code-500":"CCSDS-OEM");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript(),"Direction save/reload failed");
      const auto applied=rig.source->toPlainText();rig.save("mission.script",applied.toUtf8());QJsonArray runs;QByteArray rejectedBinary;
      auto run=[&](bool failure) {
         const auto runSource=rig.source->toPlainText();const int undo=rig.source->document()->availableUndoSteps();rig.messages->clear();acceptanceView(rig.window);const auto outcome=rig.window.runMission();events();acceptanceView(*rig.messages);
         const auto raw=read(rig.report);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==(failure?1:2),"Direction mission continued after error or omitted final result");QJsonArray reports;
         for (int r=0;r<lines.size();++r) {const auto fields=lines[r].split(',');require(fields.size()==8,"Direction report field count differs");QJsonArray row;for(const auto &field:fields){bool ok=false;const double value=field.toDouble(&ok);require(ok&&std::isfinite(value),"Direction report contains nonfinite state");row.append(value);}const auto sentinel=row.takeAt(7);require(sentinel.toDouble()==(r==0?0:9),"Direction terminal assignment executed at wrong time");checkState(row,r==0?direction*100:0);reports.append(row);}
         const auto diagnostic=rig.messages->toPlainText();
         if(failure)require(outcome==MainWindow::RunResult::Failed&&diagnostic.contains("non-monotonic invalid ephemeris")&&Moderator::Instance()->GetInternalObject("Done")->GetRealParameter("Value")==0,"Code500 reversal did not diagnose and stop before terminal assignment");
         else require(outcome==MainWindow::RunResult::Completed&&Moderator::Instance()->GetInternalObject("Done")->GetRealParameter("Value")==9,"Direction recovery/segmented mission did not complete");
         require(rig.source->toPlainText()==runSource,"Direction Run changed source");require(rig.source->document()->availableUndoSteps()==undo,"Direction Run changed Undo history");
         const auto output=QString::fromStdString(Moderator::Instance()->GetConfiguredObject("Export")->GetStringParameter("Filename"));const auto payload=read(output);
         if(failure){require(payload.size()==2800&&payload.startsWith("EPHEM   "),"Code500 reversal emitted data instead of stopping at its incomplete header");rejectedBinary=payload;}
         else if(code500)require(read(ephemeris)==rejectedBinary,"GUI format recovery overwrote the rejected Code500 artifact");
         const QString phase=failure?"rejected":"completed";rig.save(phase+".csv",raw);rig.save(phase+"-messages.txt",diagnostic.toUtf8());rig.save(phase+".eph",payload);
         QJsonObject entry{{"phase",phase},{"reports",reports},{"diagnostic",diagnostic},{"done",failure?0:9},{"configured_filename",output}};if(!failure)entry["segments"]=oem(payload,direction);runs.append(entry);
      };
      run(code500);
      if(code500){configure("CCSDS-OEM");rig.save("recovered.script",rig.source->toPlainText().toUtf8());require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript(),"Direction recovery save/reload failed");run(false);}
      const QJsonObject proof{{"checkpoint","ephemeris-direction-"+key},{"outcome","passed"},{"case",key},{"direction",direction},{"runs",runs},{"facets",QJsonArray{"actual-ephemeris-format-checkbox-apply-save-reload","code500-stops-on-reversal-or-oem-segments-monotonic-arcs","independent-report-oem-circular-orbit-values-and-recovery"}},{"scope","Selected fixed-step direction change with actual resource form and native mission. Code500 refusal is a format-specific subscriber guard; this does not prove every raw interpolator rejects bad data. Independent circular orbit checks every OEM state. Native input/platform/stress/full-build and Code500 internal Toggle gap remain open."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
