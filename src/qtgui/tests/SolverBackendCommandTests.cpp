// Supplementary solver-context fork; retained physical oracles and bounds.
#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "GmatCommand.hpp"
#include <set>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
QByteArray read(const QString &path) {QFile f(path);require(f.open(QIODevice::ReadOnly),"Function backend artifact missing");return f.readAll();}
void write(const QString &path,const QByteArray &bytes) {QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Function backend artifact write failed");}
QJsonArray rows(const QByteArray &raw) {
   QJsonArray result;for(const auto &line:QString::fromUtf8(raw).trimmed().split('\n')) {
      QJsonArray row;for(const auto &cell:line.trimmed().split(',')) {bool ok=false;const double v=cell.trimmed().toDouble(&ok);require(ok&&std::isfinite(v),"Nonfinite backend report field");row.append(v);}result.append(row);
   }return result;
}
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("SolverBackendCommands");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, case matrix and key");TestSettings settings;
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(),output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
  require(!output.isEmpty(),"Evidence directory required");QDir::setCurrent(QFileInfo(startup).absolutePath());
  QJsonObject spec;for(const auto &entry:QJsonDocument::fromJson(read(argv[2])).object()["cases"].toArray())if(entry.toObject()["key"]==QString(argv[3]))spec=entry.toObject();require(!spec.isEmpty(),"Function backend case missing");
  auto expand=[&](QString text) {
   for(const auto &pair:QList<QPair<QString,QString>>{{"@SPEED@",QString::number(std::sqrt(398600.4415/7000.),'g',17)},
    {"@VALUES@",output+"/values.csv"},{"@EXTRA@",output+"/extra.csv"},{"@OPT@",output+"/optimizer.txt"},
    {"@EVENT@",output+"/events.txt"},{"@HISTORY@",output+"/history.thf"},{"@FUNCTION@",output+"/Scoped.gmf"},
    {"@DATA_ROOT@",QFileInfo(startup).absoluteDir().absoluteFilePath("../data")}})text.replace(pair.first,pair.second);
   require(!text.contains('@'),"Unexpanded function backend input");return text;
  };
  const auto function=expand(spec["function_body"].toString()),script=expand(spec["main_script"].toString());
  write(output+"/Scoped.gmf",function.toUtf8());write(output+"/mission.script",script.toUtf8());
  for(const auto &entry:spec["support_files"].toArray()){const auto file=entry.toObject();write(output+'/'+file["name"].toString(),expand(file["text"].toString()).toUtf8());}
  MainWindow window;require(window.initialize(startup),"Function backend runtime failed");window.show();
  auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");
  require(editor&&messages,"Function backend workspace missing");editor->setPlainText(script);
  if(!window.buildScript()){QApplication::processEvents();write(output+"/build-messages.txt",messages->toPlainText().toUtf8());throw std::runtime_error("Function backend Build failed");}
  const auto saved=output+"/Function backend Δ.script";require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&editor->toPlainText()==script,"Function backend Unicode reopen failed");
  messages->clear();const auto result=window.runMission();QApplication::processEvents();write(output+"/messages.txt",messages->toPlainText().toUtf8());
  require(result==MainWindow::RunResult::Completed,"Function backend Run failed");
  const auto data=rows(read(output+"/values.csv")),expected=spec["expected_report"].toArray(),bounds=spec["column_tolerances"].toArray();
  require(data.size()==1&&data[0].toArray().size()==expected.size()&&bounds.size()==expected.size(),"Function backend report shape differs");
  for(int i=0;i<expected.size();++i)require(std::abs(data[0].toArray()[i].toDouble()-expected[i].toDouble())<bounds[i].toDouble(),"Function backend differs from original independent state/control oracle");
  auto *returned=Moderator::Instance()->GetInternalObject("Returned");require(returned&&returned->GetRealParameter("Value")==42,"Solver backend did not resume after its branch");
  auto *sat=Moderator::Instance()->GetInternalObject("Sat");require(sat,"Function backend caller spacecraft missing");
  const QStringList axes{"X","Y","Z","VX","VY","VZ"};
  for(int i=0;i<6;++i)require(std::abs(sat->GetRealParameter(axes[i].toStdString())-expected[i+1].toDouble())<bounds[i+1].toDouble(),"Solver command did not update its mission spacecraft");
  const QStringList scalars{"Keep","Count","U","Cost","Quantity"};
  for(int i=0;i<scalars.size();++i) {auto *value=Moderator::Instance()->GetInternalObject(scalars[i].toStdString());
   require(value&&std::abs(value->GetRealParameter("Value")-expected[i+7].toDouble())<bounds[i+7].toDouble(),"Solver command scalar effect differs");}
  QJsonArray contacts;
  if(spec["key"].toString().startsWith("FindEvents")) {
   const auto text=QString::fromUtf8(read(output+"/events.txt"));
   require(text.count("Observer: ")==1&&text.contains("Observer: East"),"Function FindEvents did not emit exactly the expected station report");
   const QRegularExpression pattern("(\\d{2} [A-Za-z]{3} \\d{4} \\d{2}:\\d{2}:\\d{2}\\.\\d{3})\\s+(\\d{2} [A-Za-z]{3} \\d{4} \\d{2}:\\d{2}:\\d{2}\\.\\d{3})\\s+([0-9.eE+\\-]+)");
   const auto reference=spec["contact_reference"].toObject();
   const auto wanted=reference["events"].toObject()["East"].toArray();
   auto matches=pattern.globalMatch(text);const QDateTime epoch(QDate(2000,1,1),QTime(12,0),Qt::UTC);
   while(matches.hasNext()) {
      const auto match=matches.next();QJsonObject observed;
      for(int i=1;i<=2;++i) {
         auto time=QLocale::c().toDateTime(match.captured(i),"dd MMM yyyy HH:mm:ss.zzz");require(time.isValid(),"Contact timestamp invalid");time.setTimeSpec(Qt::UTC);
         observed[i==1?"start":"end"]=epoch.msecsTo(time)/1000.;
      }
      bool ok=false;const double duration=match.captured(3).toDouble(&ok);require(ok&&std::isfinite(duration),"Contact duration invalid");observed["duration"]=duration;contacts.append(observed);
   }
   require(contacts.size()==wanted.size()&&contacts.size()==1,"Function contact interval count differs from independent oracle");
   for(int i=0;i<contacts.size();++i)for(const auto &field:QStringList{"start","end","duration"}) {
      const double bound=reference[field=="duration"?"duration_tolerance":"time_tolerance"].toDouble();
      require(bound>0&&std::abs(contacts[i].toObject()[field].toDouble()-wanted[i].toObject()[field].toDouble())<bound,"Function contact differs from original independent time oracle");
   }
   // Exercise repeated reference setup directly as well as through the function.
   // A configured observer occupies one slot even when initialization repeats.
   auto *locator=Moderator::Instance()->GetInternalObject("Events"),*station=Moderator::Instance()->GetInternalObject("East");
   require(locator&&station,"Contact binding objects missing");
   for(int i=0;i<2;++i)require(locator->SetRefObject(station,Gmat::GROUND_STATION,"East")&&locator->Initialize(),"Repeated contact observer binding failed");
  }
  if(!spec["extra_expected"].isNull()) {
   const auto extra=rows(read(output+"/extra.csv")),wanted=spec["extra_expected"].toArray(),limits=spec["extra_tolerances"].toArray();
   require(extra.size()==wanted.size(),"Function file-thrust activation report shape differs");
   for(int row=0;row<wanted.size();++row) {require(extra[row].toArray().size()==wanted[row].toArray().size(),"Function file-thrust activation width differs");
    for(int column=0;column<limits.size();++column)require(std::abs(extra[row].toArray()[column].toDouble()-wanted[row].toArray()[column].toDouble())<limits[column].toDouble(),"Function file-thrust mass flow/coast differs from retained oracle");}
  }
  GmatCommand *branch=nullptr;
  std::set<GmatCommand *> rootSeen;
  for(auto *command=Moderator::Instance()->GetFirstCommand();command && rootSeen.insert(command).second;command=command->GetNext())
   if(command->GetTypeName()==spec["context"].toString().toStdString()){require(!branch,"Duplicate outer solver branch");branch=command;}
  require(branch,"Native solver branch missing");
  QJsonArray nativeTypes;std::set<GmatCommand *> seen;std::set<std::string> present;
  for(auto *command=branch->GetChildCommand(0);command && command!=branch && seen.insert(command).second;command=command->GetNext()){
   nativeTypes.append(QString::fromStdString(command->GetTypeName()));present.insert(command->GetTypeName());
  }
  for(const auto &type:spec["required_context_types"].toArray())require(present.count(type.toString().toStdString()),"Expected backend command is outside the native solver branch");
  const QJsonObject proof{{"status","passed"},{"case",spec["key"]},{"observed",data},{"expected",expected},{"column_tolerances",bounds},
    {"after_branch_sentinel",42},{"contact_intervals",contacts},{"context",spec["context"]},{"native_context_types",nativeTypes},{"executed_source",script},{"solver_mode","RunInitialGuess"},{"whole_requirement_qualified",false}};
  write(output+"/proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;
 }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';}return 1;
}
