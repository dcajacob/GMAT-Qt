#include "FormationAcceptance.hpp"
#include "GmatType.hpp"
#include <QAction>
#include <QSet>
using namespace formaccept;

static QStringList strings(const StringArray &values) {
 QStringList result;for(const auto &v:values)result.append(QString::fromStdString(v));return result;
}
static QStringList unique(QStringList values) {values.removeDuplicates();values.sort();return values;}

int main(int argc,char **argv) {
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionResourceCensus");
 try {
  require(argc==2&&acceptanceWidgetPlatform(),"Supply native census startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());require(rig.window.buildScript(),"Factory census baseline Build failed");auto *moderator=Moderator::Instance();const auto before=rig.source->toPlainText();const int undo=rig.source->document()->availableUndoSteps();const auto objects=moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT);
  const auto categories=moderator->GetAllObjectTypeArrayMap();const auto all=strings(moderator->GetListOfAllFactoryItems());QJsonArray rows;QStringList flattened;QMap<QString,QJsonArray> memberships;
  for(const auto &entry:categories) {
   const auto factory=strings(entry.second),viewable=strings(moderator->GetListOfViewableItems(entry.first)),hidden=strings(moderator->GetListOfUnviewableItems(entry.first));QJsonArray entries;QStringList uncategorized;
   for(const auto &name:factory){entries.append(QJsonObject{{"factory_type",name},{"viewable",viewable.contains(name)},{"explicit_unviewable",hidden.contains(name)}});memberships[name].append(int(entry.first));if(!viewable.contains(name)&&!hidden.contains(name))uncategorized.append(name);}
   flattened.append(factory);rows.append(QJsonObject{{"category_id",int(entry.first)},{"category_name",QString::fromStdString(GmatType::GetTypeName(entry.first))},{"factory_types",QJsonArray::fromStringList(factory)},{"viewable_types",QJsonArray::fromStringList(viewable)},{"unviewable_types",QJsonArray::fromStringList(hidden)},{"unclassified_visibility",QJsonArray::fromStringList(uncategorized)},{"entries",entries}});
  }
  require(!rows.isEmpty()&&unique(flattened)==unique(all),"Full factory list/type-map census differs");QJsonObject duplicates,firstResolution;
  for(auto i=memberships.cbegin();i!=memberships.cend();++i){firstResolution[i.key()]=i.value().first();if(i.value().size()>1)duplicates[i.key()]=i.value();}
  require(rig.source->toPlainText()==before&&rig.source->document()->availableUndoSteps()==undo&&objects==moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT),"Factory census changed source/history/configuration");acceptanceView(rig.window);
  const QJsonObject proof{{"checkpoint","full-native-script-factory-census"},{"outcome","passed"},{"categories",rows},{"category_count",rows.size()},{"all_factory_items",QJsonArray::fromStringList(all)},{"unique_factory_types",QJsonArray::fromStringList(unique(all))},{"unique_factory_type_count",unique(all).size()},{"factory_entry_count",flattened.size()},{"duplicate_name_categories",duplicates},{"interpreter_first_category_by_name",firstResolution},{"configured_source_history_unchanged",true},{"scope","Current loaded factory/type-map census used by Interpreter.Initialize,including hidden and owned component classes. No construction,use,global eligibility,function scope,science or whole NASA requirement qualification is inferred."}};rig.save("native-factory-census.json",QJsonDocument(proof).toJson());rig.save("baseline.script",before.toUtf8());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
