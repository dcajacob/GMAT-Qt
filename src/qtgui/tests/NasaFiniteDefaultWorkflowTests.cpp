#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "Debugger.hpp"
#include "FiniteBurn.hpp"
#include <QAbstractItemView>
#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QTabWidget>
#include <QTabBar>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
using namespace formaccept;
namespace {
void reveal(QWidget *widget)
{
 require(widget,"Default workflow widget absent");
 for(auto *parent=widget->parentWidget();parent;parent=parent->parentWidget())
  if(auto *tabs=qobject_cast<QTabWidget *>(parent))for(int i=0;i<tabs->count();++i)
   if(tabs->widget(i)==widget||tabs->widget(i)->isAncestorOf(widget))tabs->setCurrentIndex(i);
 for(auto *parent=widget->parentWidget();parent;parent=parent->parentWidget())
  if(auto *scroll=qobject_cast<QScrollArea *>(parent))scroll->ensureWidgetVisible(widget);
 events();require(widget->isVisible()&&widget->isEnabled(),"Default workflow widget unreachable");acceptanceView(*widget);
}
void paste(QWidget *widget,const QString &text)
{
 reveal(widget);QTest::mouseClick(widget,Qt::LeftButton);QTest::keyClick(widget,Qt::Key_A,Qt::ControlModifier);
 QApplication::clipboard()->setText(text);QTest::keyClick(widget,Qt::Key_V,Qt::ControlModifier);events();
 if(auto *line=qobject_cast<QLineEdit *>(widget))require(line->text()==text,"Default workflow line paste differs");
 else if(auto *source=qobject_cast<QPlainTextEdit *>(widget))require(source->toPlainText()==text,"Default workflow source paste differs");
 else throw std::runtime_error("Unsupported default workflow clipboard target");
}
GmatBase *object(const QString &name)
{
 auto *value=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(value,"Default workflow configured object absent");return value;
}
QMap<QString,QPair<quintptr,QString>> configured()
{
 QMap<QString,QPair<quintptr,QString>> result;
 for(const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT)){
  auto *value=object(QString::fromStdString(name));result[QString::fromStdString(name)]={reinterpret_cast<quintptr>(value),QString::fromStdString(value->GetGeneratingString(Gmat::SCRIPTING))};
 }
 return result;
}
QJsonObject configuredValues()
{
 const auto state=configured();QJsonObject result;for(auto i=state.cbegin();i!=state.cend();++i)result[i.key()]=i.value().second;return result;
}
GmatCommand *native(const QString &type)
{
 QSet<GmatCommand *>seen;GmatCommand *found=nullptr;
 for(auto *command=Moderator::Instance()->GetFirstCommand();command&&!seen.contains(command);command=command->GmatCommand::GetNext()){
  seen.insert(command);if(QString::fromStdString(command->GetTypeName())==type){require(!found,"Default workflow command type ambiguous");found=command;}
 }
 return found;
}
QStringList references(GmatBase *owner,const QString &field)
{
 QStringList result;for(const auto &name:owner->GetStringArrayParameter(field.toStdString()))result.append(QString::fromStdString(name));return result;
}
QStringList commandReferences(GmatCommand *command,UnsignedInt type)
{
 require(command,"Default workflow native command absent");QStringList result;
 for(const auto &name:command->GetRefObjectNameArray(type))result.append(QString::fromStdString(name));return result;
}
QString withoutFields(QString source,const QString &owner,const QStringList &fields)
{
 for(const auto &field:fields)source.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(owner+"."+field)+"[ \\t]*=[^;]*;[ \\t]*(?:\\n|$)",QRegularExpression::MultilineOption));
 return source;
}
}
int main(int argc,char **argv)
{
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaFiniteDefaultWorkflow");
 try{
  require(argc==4,"Supply startup,finite command,and workflow context");const QString command=argv[2],context=argv[3];
  require(QStringList{"BeginFiniteBurn","EndFiniteBurn"}.contains(command)&&QStringList{"MissingSpacecraft","SwitchTemplate","ConfigureAndRun"}.contains(context),"Unknown default workflow case");
  const bool physical=context=="ConfigureAndRun",switching=context=="SwitchTemplate";Rig rig(QFileInfo(argv[1]).absoluteFilePath());const auto saved=rig.files.filePath("Default workflow Ω.script");
  QString initial="% Default workflow header Ω\nCreate Variable Keep;\nKeep = 41;\nCreate GroundStation Site;\nCreate ImpulsiveBurn DecoyImpulse;\nCreate ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\n";
  if(context=="MissingSpacecraft")initial+="Create FiniteBurn KnownBurn;\n";
  if(physical){
   initial+="Earth.Mu = 1e-20;\nCreate ChemicalTank Fuel;\nFuel.FuelMass = 100;\nFuel.PressureModel = PressureRegulated;\nCreate ChemicalThruster Engine ControlEngine;\n";
   for(const auto &engine:QStringList{"Engine","ControlEngine"}){
    initial+=engine+".Tank = {Fuel};\n"+engine+".DecrementMass = false;\n"+engine+".CoordinateSystem = EarthMJ2000Eq;\n"+engine+".ThrustDirection1 = "+(engine=="Engine"?"1":"0")+";\n"+engine+".ThrustDirection2 = 0;\n"+engine+".ThrustDirection3 = "+(engine=="Engine"?"0":"1")+";\n";
    for(int i=1;i<=16;++i)initial+=engine+".C"+QString::number(i)+" = "+(i==1?(engine=="Engine"?QString("100"):QString("250")):QString("0"))+";\n";
    for(int i=1;i<=16;++i)initial+=engine+".K"+QString::number(i)+" = "+(i==1?QString("300"):QString("0"))+";\n";
   }
   initial+="Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.SRP = Off;\nFM.Drag = None;\nCreate Propagator P;\nP.FM = FM;\nP.Type = RungeKutta89;\nP.InitialStepSize = 1;\nP.MinStep = 0.001;\nP.MaxStep = 1;\nP.Accuracy = 1e-12;\n";
  }
  const QString anchor="'Anchor' Keep = Keep + "+QString(physical?"1":"0")+";",continuation="Report 'Continuation' Values Keep;";
  initial+="BeginMissionSequence;\n"+anchor+"\n"+continuation+"\n% Default workflow tail Δ\n";
  rig.source->setPlainText(initial);require(rig.window.buildScript()&&rig.window.saveScriptTo(saved)&&read(saved)==initial.toUtf8(),"Default workflow baseline build/save failed");rig.save("initial.script",initial.toUtf8());const auto initialValues=configuredValues();
  struct State{QString source;int undo;bool modified;QByteArray file;QMap<QString,QPair<quintptr,QString>> objects;GmatCommand *first;};
  auto state=[&]{return State{rig.source->toPlainText(),rig.source->document()->availableUndoSteps(),rig.source->document()->isModified(),read(saved),configured(),Moderator::Instance()->GetFirstCommand()};};
  auto stable=[&](const State &before){require(rig.source->toPlainText()==before.source&&rig.source->document()->availableUndoSteps()==before.undo&&rig.source->document()->isModified()==before.modified&&read(saved)==before.file&&configured()==before.objects&&Moderator::Instance()->GetFirstCommand()==before.first,"Default workflow pending/Cancel action changed source/history/file/native pointers/configuration");};
  auto click=[&](QMdiSubWindow *child,QDialogButtonBox::StandardButton action){auto *buttons=child->findChild<QDialogButtonBox *>("resourceButtons");if(!buttons)buttons=child->findChild<QDialogButtonBox *>();auto *button=buttons?buttons->button(action):nullptr;reveal(button);QTest::mouseClick(button,Qt::LeftButton);events();};
  int commandCommits=0,resourceCommits=0,explicitIdCommits=0,confirmationCancels=0,confirmationAccepts=0,insertionCancels=0;QJsonArray insertions,pickerVisits,resourceEdits;
  auto templateChoice=[&](QMdiSubWindow *child,const QString &type,QString reply={}){
   auto *combo=child->findChild<QComboBox *>("commandTemplate");reveal(combo);const int at=combo->findText(type);require(at>=0,"Default workflow template absent");QTest::mouseClick(combo,Qt::LeftButton,Qt::NoModifier,QPoint(combo->width()-10,combo->height()/2));events();require(combo->view()->isVisible(),"Default workflow template popup absent");acceptanceView(*combo->view()->window());QTest::keyClick(combo->view(),Qt::Key_Home);for(int i=0;i<at;++i)QTest::keyClick(combo->view(),Qt::Key_Down);
   std::exception_ptr failure;bool visited=false;
   if(!reply.isEmpty())QTimer::singleShot(0,child,[&]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());try{require(box&&box->windowTitle()=="Replace command text","Default workflow actual replace confirmation absent");acceptanceView(*box);visited=true;auto *button=box->button(reply=="Cancel"?QMessageBox::Cancel:QMessageBox::Yes);require(button&&button->isEnabled(),"Default workflow confirmation action unavailable");QTest::mouseClick(button,Qt::LeftButton);}catch(...){failure=std::current_exception();if(box)box->reject();}});
   QTest::keyClick(combo->view(),Qt::Key_Return);events();if(failure)std::rethrow_exception(failure);if(!reply.isEmpty()){require(visited,"Default workflow replace confirmation unvisited");if(reply=="Cancel")++confirmationCancels;else ++confirmationAccepts;}
  };
  auto openInsertion=[&](const QString &targetLabel,const QString &type){
   for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==rig.mission||tabs->widget(i)->isAncestorOf(rig.mission))tabs->setCurrentIndex(i);
   int at=-1;const auto snapshot=rig.window.missionSnapshot();for(int i=0;i<snapshot.nodes.size();++i)if(missionCommandLabel(snapshot.nodes[i].statement).name==targetLabel){require(at<0,"Default workflow insertion target ambiguous");at=i;}require(at>=0,"Default workflow insertion target absent");QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(rig.mission);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==at)item=*it;require(item,"Default workflow actual mission item absent");rig.mission->expandAll();rig.mission->scrollToItem(item);rig.mission->setCurrentItem(item);reveal(rig.mission);
   std::exception_ptr failure;bool visited=false;QTimer::singleShot(0,&rig.window,[&]{auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());try{require(menu,"Default workflow insertion menu absent");QAction *choice=nullptr;for(auto *action:menu->actions())if(action->text()=="Insert before…")choice=action;require(choice&&choice->isEnabled(),"Default workflow Insert before disabled");menu->setActiveAction(choice);acceptanceView(*menu);visited=true;QTest::keyClick(menu,Qt::Key_Return);}catch(...){failure=std::current_exception();if(menu)menu->close();}});
   const auto point=rig.mission->visualItemRect(item).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,rig.mission->viewport()->mapToGlobal(point));QApplication::sendEvent(rig.mission->viewport(),&event);events();if(failure)std::rethrow_exception(failure);require(visited,"Default workflow context action unvisited");auto *child=rig.area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Default workflow actual command editor absent");child->showNormal();child->move(0,0);child->resize(rig.area->viewport()->size());events();templateChoice(child,type);acceptanceView(*child);return child;
  };
  auto commit=[&](QPointer<QMdiSubWindow> &child,const QString &statement,const QString &target,const QString &declarations={}){
   const auto before=rig.source->toPlainText();require(before.count(target)==1,"Default workflow source anchor ambiguous");auto expected=before;expected.replace(target,statement+"\n"+target);expected=declarations+expected;click(child,QDialogButtonBox::Apply);rig.save("apply-diagnostic.json",QJsonDocument(QJsonObject{{"actual",rig.source->toPlainText()},{"expected",expected},{"messages",rig.messages->toPlainText()}}).toJson());require(child&&rig.source->toPlainText()==expected&&!dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Default workflow atomic insertion/source or clean editor differs");const int undo=rig.source->document()->availableUndoSteps();auto *first=Moderator::Instance()->GetFirstCommand();click(child,QDialogButtonBox::Apply);require(rig.source->toPlainText()==expected&&rig.source->document()->availableUndoSteps()==undo&&Moderator::Instance()->GetFirstCommand()==first,"Default workflow clean Apply rebuilt or duplicated insertion");click(child,QDialogButtonBox::Close);require(!child,"Default workflow clean Close retained editor");++commandCommits;insertions.append(QJsonObject{{"statement",statement},{"target",target},{"declarations",declarations},{"source_before",before},{"source_after",expected}});rig.save("command-"+QString::number(commandCommits)+".script",expected.toUtf8());return expected;
  };
  const QString burn=context=="MissingSpacecraft"?"KnownBurn":"DefaultFB",defaultStatement=command+" "+burn+"(DefaultSC);";const auto original=state();QPointer<QMdiSubWindow> child=openInsertion("Anchor",command);require(child->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==defaultStatement,"Default workflow initial finite template references differ");stable(original);click(child,QDialogButtonBox::Cancel);require(!child,"Default workflow insertion Cancel retained editor");stable(original);++insertionCancels;child=openInsertion("Anchor",command);stable(original);
  QString accepted;
  if(switching){
   paste(child->findChild<QLineEdit *>("commandName"),"Pending default Ω");const auto pending=child->findChild<QPlainTextEdit *>("commandSource")->toPlainText();templateChoice(child,"Assignment","Cancel");require(child&&child->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==pending,"Default workflow cancelled replacement lost finite draft");stable(original);templateChoice(child,"Assignment","Yes");require(child->findChild<QPlainTextEdit *>("commandSource")->toPlainText()=="VariableName = 1;","Default workflow accepted replacement did not select Assignment");stable(original);const QString replacement="'Switched' Keep = Keep + 1;";paste(child->findChild<QPlainTextEdit *>("commandSource"),replacement);stable(original);accepted=commit(child,replacement,anchor);require(!Moderator::Instance()->GetConfiguredObject("DefaultFB")&&!Moderator::Instance()->GetConfiguredObject("DefaultSC")&&!native(command)&&configuredValues()==initialValues,"Default workflow unrelated command created unused pending defaults or changed resources");
  }else{
   QString statement=defaultStatement;
   if(physical){const QString label=command=="BeginFiniteBurn"?"Start":"Stop";paste(child->findChild<QLineEdit *>("commandName"),label);statement=command+" '"+label+"' "+burn+"(DefaultSC);";}
   const QString declarations=physical?"Create FiniteBurn DefaultFB;\nCreate Spacecraft DefaultSC;\n":"Create Spacecraft DefaultSC;\n";stable(original);accepted=commit(child,statement,anchor,declarations);require(commandReferences(native(command),Gmat::FINITE_BURN)==QStringList{burn}&&commandReferences(native(command),Gmat::SPACECRAFT)==QStringList{"DefaultSC"},"Default workflow native inserted references differ");
   const auto now=configuredValues();for(auto i=initialValues.begin();i!=initialValues.end();++i)require(now[i.key()]==i.value(),"Default workflow insertion changed an existing resource");auto added=now;for(auto i=initialValues.begin();i!=initialValues.end();++i)added.remove(i.key());require(added.keys()==(physical?QStringList{"DefaultFB","DefaultSC"}:QStringList{"DefaultSC"}),"Default workflow automatic resource set differs");
  }
  if(physical){
   auto openResource=[&](const QString &name){
    auto *value=object(name);(void)value;
    for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==rig.resources||tabs->widget(i)->isAncestorOf(rig.resources))tabs->setCurrentIndex(i);
    auto items=rig.resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Default workflow resource tree identity absent/ambiguous");rig.resources->expandAll();rig.resources->scrollToItem(items[0]);rig.resources->setCurrentItem(items[0]);reveal(rig.resources);rig.resources->itemDoubleClicked(items[0],0);events();auto *owner=rig.area->activeSubWindow();require(owner&&dynamic_cast<ResourceEditor *>(owner->widget()),"Default workflow actual resource editor absent");owner->showNormal();owner->move(0,0);owner->resize(rig.area->viewport()->size());events();acceptanceView(*owner);return owner;
   };
   auto configure=[&](const QString &ownerName,const QStringList &fields){
    QPointer<QMdiSubWindow> owner=openResource(ownerName);const auto before=state();const auto beforeValues=configuredValues();QJsonObject fieldValues;
    for(const auto &field:fields){
     auto *line=owner->findChild<QLineEdit *>("resource_"+field);require(line&&!line->isReadOnly(),"Default workflow hardware field absent/readonly");reveal(line);const QString selected=field=="Tanks"?"Fuel":"Engine";const QStringList pool=field=="Tanks"?QStringList{"Fuel"}:QStringList{"ControlEngine","Engine"};const auto nativeBefore=references(object(ownerName),field);
     for(bool accept:{false,true}){
      QPushButton *button=nullptr;for(auto *candidate:line->parentWidget()->findChildren<QPushButton *>(QString(),Qt::FindDirectChildrenOnly))if(candidate->text()=="Select…")button=candidate;reveal(button);const auto draft=line->text();std::exception_ptr failure;bool visited=false;
      QTimer::singleShot(0,owner,[&]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());try{
       require(dialog&&dialog->objectName()=="resourceSelectionDialog","Default workflow actual hardware chooser absent");auto *list=dialog->findChild<QListWidget *>("resourceSelectionList");require(list&&list->isVisible(),"Default workflow hardware list hidden");QStringList offered;for(int i=0;i<list->count();++i)offered.append(list->item(i)->text());auto sorted=offered;sorted.sort();require(sorted==pool,"Default workflow hardware chooser type pool differs");acceptanceView(*dialog);
       for(int i=0;i<list->count();++i){auto *item=list->item(i);const bool checked=item->text()==selected;if((item->checkState()==Qt::Checked)!=checked){list->setCurrentItem(item);list->scrollToItem(item);QTest::keyClick(list,Qt::Key_Space);}require((item->checkState()==Qt::Checked)==checked,"Default workflow hardware checkbox input differs");}
       acceptanceView(*dialog);auto *buttons=dialog->findChild<QDialogButtonBox *>();auto *done=buttons?buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel):nullptr;require(done&&done->isEnabled(),"Default workflow hardware chooser terminal action unavailable");visited=true;pickerVisits.append(QJsonObject{{"owner",ownerName},{"field",field},{"pool",QJsonArray::fromStringList(sorted)},{"selected",selected},{"accepted",accept}});QTest::mouseClick(done,Qt::LeftButton);
      }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});
      QTest::mouseClick(button,Qt::LeftButton);events();if(failure)std::rethrow_exception(failure);require(visited,"Default workflow hardware chooser unvisited");stable(before);require(references(object(ownerName),field)==nativeBefore,"Default workflow pending hardware choice altered native references");if(accept)require(splitResourceReferences(line->text())==QStringList{selected},"Default workflow accepted hardware draft differs");else require(line->text()==draft,"Default workflow cancelled hardware choice altered draft");
     }
     fieldValues[field]=selected;
    }
    require(dynamic_cast<ResourceEditor *>(owner->widget())->hasChanges(),"Default workflow accepted hardware is not pending");click(owner,QDialogButtonBox::Apply);rig.save("hardware-apply-diagnostic.json",QJsonDocument(QJsonObject{{"owner",ownerName},{"before",before.source},{"after",rig.source->toPlainText()},{"messages",rig.messages->toPlainText()}}).toJson());require(owner&&!dynamic_cast<ResourceEditor *>(owner->widget())->hasChanges()&&rig.source->toPlainText()!=before.source&&withoutFields(rig.source->toPlainText(),ownerName,fields)==withoutFields(before.source,ownerName,fields),"Default workflow hardware Apply changed unrelated source or did not become clean");
    for(const auto &field:fields)require(references(object(ownerName),field)==QStringList{fieldValues[field].toString()},"Default workflow applied hardware native reference differs");const auto afterValues=configuredValues();rig.save("native-resource-apply-diagnostic.json",QJsonDocument(QJsonObject{{"owner",ownerName},{"before",beforeValues},{"after",afterValues}}).toJson());for(auto i=beforeValues.begin();i!=beforeValues.end();++i)if(i.key()!=ownerName)require(afterValues[i.key()]==i.value(),"Default workflow hardware Apply changed unrelated resource");require(withoutFields(afterValues[ownerName].toString(),ownerName,fields)==withoutFields(beforeValues[ownerName].toString(),ownerName,fields),"Default workflow hardware Apply changed another field on its owner");++resourceCommits;resourceEdits.append(QJsonObject{{"owner",ownerName},{"fields",fieldValues},{"source_before",before.source},{"source_after",rig.source->toPlainText()},{"native_before",beforeValues},{"native_after",afterValues}});rig.save("resource-"+QString::number(resourceCommits)+".script",rig.source->toPlainText().toUtf8());click(owner,QDialogButtonBox::Close);require(!owner,"Default workflow clean resource Close retained owner");
   };
   configure("DefaultSC",{"Tanks","Thrusters"});configure("DefaultFB",{"Thrusters"});
   // Preserve automatic IDs through the unrelated burn edit above, then prove
   // explicit user ID changes win and survive later configuration rebuilds.
   {
    QPointer<QMdiSubWindow> owner=openResource("DefaultSC");const auto before=state();const auto nativeBefore=configuredValues();
    const QMap<QString,QString> desired{{"NAIFId","-100084"},{"NAIFIdReferenceFrame","-9000084"}};
    auto *table=owner->findChild<QTableWidget *>("resourceProperties");reveal(table);auto *sections=owner->findChild<QTabBar *>("propertySections");reveal(sections);int spice=-1;for(int i=0;i<sections->count();++i)if(sections->tabText(i)=="SPICE")spice=i;require(spice>=0,"Default workflow SPICE section unavailable");QTest::mouseClick(sections,Qt::LeftButton,Qt::NoModifier,sections->tabRect(spice).center());events();acceptanceView(*owner);
    for(auto i=desired.cbegin();i!=desired.cend();++i){int row=-1;for(int r=0;r<table->rowCount();++r)if(table->item(r,0)->text()==i.key())row=r;require(row>=0&&!table->isRowHidden(row)&&bool(table->item(row,1)->flags()&Qt::ItemIsEditable),"Default workflow explicit ID table row unavailable/readonly");auto *value=table->item(row,1);table->setCurrentCell(row,1);table->scrollToItem(value);reveal(table);QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,table->visualItemRect(value).center());QTest::keyClick(table,Qt::Key_F2);events();QList<QLineEdit *> editors;for(auto *editor:table->findChildren<QLineEdit *>())if(editor->isVisible()&&!editor->isReadOnly())editors.append(editor);rig.save("id-editor-diagnostic.json",QJsonDocument(QJsonObject{{"field",i.key()},{"visible_editors",editors.size()},{"focus_class",QApplication::focusWidget()?QString(QApplication::focusWidget()->metaObject()->className()):QString("none")}}).toJson());require(editors.size()==1,"Default workflow advanced ID editor identity ambiguous");auto *line=editors.first();require(line&&!line->isReadOnly(),"Default workflow explicit spacecraft ID control absent/readonly");paste(line,i.value());QTest::keyClick(line,Qt::Key_Return);events();require(value->text()==i.value(),"Default workflow advanced ID editor did not commit typed value");stable(before);}
    click(owner,QDialogButtonBox::Apply);const auto nativeAfter=configuredValues();
    require(owner&&!dynamic_cast<ResourceEditor *>(owner->widget())->hasChanges()&&withoutFields(rig.source->toPlainText(),"DefaultSC",desired.keys())==withoutFields(before.source,"DefaultSC",desired.keys()),"Default workflow explicit ID Apply altered unrelated source or lost clean state");
    for(auto i=desired.cbegin();i!=desired.cend();++i)require(object("DefaultSC")->GetIntegerParameter(i.key().toStdString())==i.value().toInt(),"Default workflow explicit spacecraft ID assignment was overridden");
    require(withoutFields(nativeAfter["DefaultSC"].toString(),"DefaultSC",desired.keys())==withoutFields(nativeBefore["DefaultSC"].toString(),"DefaultSC",desired.keys()),"Default workflow explicit ID Apply changed another spacecraft field");
    for(auto i=nativeBefore.begin();i!=nativeBefore.end();++i)if(i.key()!="DefaultSC")require(nativeAfter[i.key()]==i.value(),"Default workflow explicit ID Apply altered another resource");
    QJsonObject values;for(auto i=desired.cbegin();i!=desired.cend();++i)values[i.key()]=i.value();++explicitIdCommits;resourceEdits.append(QJsonObject{{"owner","DefaultSC"},{"fields",values},{"source_before",before.source},{"source_after",rig.source->toPlainText()},{"native_before",nativeBefore},{"native_after",nativeAfter}});rig.save("explicit-id.script",rig.source->toPlainText().toUtf8());click(owner,QDialogButtonBox::Close);require(!owner,"Default workflow explicit ID clean Close retained editor");
   }

   const QString start="BeginFiniteBurn 'Start' DefaultFB(DefaultSC);",stop="EndFiniteBurn 'Stop' DefaultFB(DefaultSC);";
   const QString opposite=command=="BeginFiniteBurn"?"EndFiniteBurn":"BeginFiniteBurn",oppositeLabel=command=="BeginFiniteBurn"?"Stop":"Start",targetLabel=command=="BeginFiniteBurn"?"Anchor":"Stop",targetSource=command=="BeginFiniteBurn"?anchor:stop;
   child=openInsertion(targetLabel,opposite);const auto pending=state();paste(child->findChild<QLineEdit *>("commandName"),oppositeLabel);stable(pending);accepted=commit(child,opposite=="BeginFiniteBurn"?start:stop,targetSource);
   for(const auto &type:QStringList{"BeginFiniteBurn","EndFiniteBurn"})require(commandReferences(native(type),Gmat::FINITE_BURN)==QStringList{"DefaultFB"}&&commandReferences(native(type),Gmat::SPACECRAFT)==QStringList{"DefaultSC"},"Default workflow paired native references differ");
   auto add=[&](const QString &targetName,const QString &target,const QString &type,const QString &replacement){const auto before=state();child=openInsertion(targetName,type);paste(child->findChild<QPlainTextEdit *>("commandSource"),replacement);stable(before);accepted=commit(child,replacement,target);};
   const QString powered="Propagate 'Powered' P(DefaultSC) {DefaultSC.ElapsedSecs = 10};",coast="Propagate 'Coast' P(DefaultSC) {DefaultSC.ElapsedSecs = 10};";
   add("Stop",stop,"Propagate",powered);add("Anchor",anchor,"Propagate",coast);
   const QString fields="DefaultSC.ElapsedSecs DefaultSC.EarthMJ2000Eq.X DefaultSC.EarthMJ2000Eq.Y DefaultSC.EarthMJ2000Eq.Z DefaultSC.EarthMJ2000Eq.VX DefaultSC.EarthMJ2000Eq.VY DefaultSC.EarthMJ2000Eq.VZ DefaultSC.TotalMass DefaultSC.Fuel.FuelMass Keep";
   for(int phase=0;phase<5;++phase){const QString targetName=phase==0?"Start":phase==1?"Powered":phase==2?"Stop":phase==3?"Coast":"Anchor",target=phase==0?start:phase==1?powered:phase==2?stop:phase==3?coast:anchor;add(targetName,target,"Report","Report 'State"+QString::number(phase)+"' Values "+fields+";");}
  }
  // A single Undo/Redo must reverse the latest accepted command transaction.
  // The earlier per-insertion receipts retain every actual command/source pair.
  const auto last=insertions.last().toObject();const auto final=rig.source->toPlainText();rig.source->undo();require(rig.source->toPlainText()==last["source_before"].toString()&&rig.window.buildScript(),"Default workflow single Undo lost previous source");rig.source->redo();require(rig.source->toPlainText()==final&&rig.window.buildScript(),"Default workflow single Redo lost final source");
  require(rig.window.saveScriptTo(saved)&&read(saved)==final.toUtf8()&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==final,"Default workflow Unicode source/native roundtrip failed");
  if(!switching)require(commandReferences(native(command),Gmat::FINITE_BURN)==QStringList{burn}&&commandReferences(native(command),Gmat::SPACECRAFT)==QStringList{"DefaultSC"},"Default workflow reopened native references differ");
  QJsonArray phases,reported;QJsonObject execution;int missions=0;
  if(switching){
   const auto before=state();const auto run=rig.window.runMission();events();rig.save("messages.txt",rig.messages->toPlainText().toUtf8());const auto raw=read(rig.report);rig.save("values.csv",raw);auto *keep=Moderator::Instance()->GetInternalObject("Keep");require(run==MainWindow::RunResult::Completed&&keep&&keep->GetRealParameter("Value")==42&&raw.trimmed()=="42"&&!Moderator::Instance()->GetConfiguredObject("DefaultFB")&&!Moderator::Instance()->GetConfiguredObject("DefaultSC"),"Default workflow switched assignment native/raw result or unused resource isolation differs");require(rig.source->toPlainText()==before.source&&rig.source->document()->availableUndoSteps()==before.undo&&read(saved)==before.file&&configuredValues()==initialValues,"Default workflow switched Run changed source/history/file/configuration");execution=QJsonObject{{"native_keep",42},{"raw_values",QJsonArray{42}},{"continuation",42}};++missions;acceptanceView(rig.window);
  }else if(physical){
   auto *configuredSat=dynamic_cast<Spacecraft *>(object("DefaultSC"));require(configuredSat,"Default workflow configured spacecraft type differs");const double initialEpoch=configuredSat->GetEpoch();const auto before=state();const auto beforeValues=configuredValues();const double acceleration=100./950000.;
   auto truth=[&](int phase,int keep=41){const double t=phase<2?0:phase<4?10:20,burnTime=std::min(t,10.),displacement=acceleration*(burnTime*burnTime/2+10*std::max(0.,t-10));return QJsonArray{t,7100+displacement,7.35*t,1300+t,acceleration*burnTime,7.35,1,950,100,keep};};
   auto runtime=[&]{auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("DefaultSC"));require(sat,"Default workflow runtime default spacecraft absent");QJsonArray result{(sat->GetEpoch()-initialEpoch)*86400};for(int i=0;i<6;++i)result.append(sat->GetState()[i]);result.append(sat->GetRealParameter("TotalMass"));auto *fuel=sat->GetRefObject(Gmat::FUEL_TANK,"Fuel");require(fuel,"Default workflow runtime mounted fuel absent");result.append(fuel->GetRealParameter("FuelMass"));result.append(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value"));return result;};
   auto compare=[&](const QJsonArray &actual,const QJsonArray &wanted){require(actual.size()==10&&wanted.size()==10,"Default workflow physical witness width differs");for(int i=0;i<10;++i){const double a=actual[i].toDouble(),b=wanted[i].toDouble();if(!std::isfinite(a)||std::abs(a-b)>(i==0?1e-4:1e-7))std::cerr<<"DEFAULT_WORKFLOW_DIFF column="<<i<<" actual="<<a<<" expected="<<b<<std::endl;require(std::isfinite(a)&&std::abs(a-b)<(i==0?1e-4:1e-7),"Default workflow native/report state differs from independent constant-force/default-state/coast/mass oracle");}};
   QMap<int,int> observers;const auto snapshot=rig.window.missionSnapshot();for(int phase=0;phase<5;++phase){int at=-1;for(int i=0;i<snapshot.nodes.size();++i)if(missionCommandLabel(snapshot.nodes[i].statement).name=="State"+QString::number(phase))at=i;require(at>=0&&rig.window.setBreakpoint(at,true),"Default workflow native phase observer absent");observers[at]=phase;}
   Debugger *debug=nullptr;for(auto *owner:rig.window.children())if(auto *candidate=dynamic_cast<Debugger *>(owner))debug=candidate;require(debug,"Default workflow debugger absent");auto previous=debug->paused;std::exception_ptr failure;
   debug->paused=[&](const QString &kind){if(previous)previous(kind);try{auto *dialog=rig.window.findChild<QDialog *>("debuggerDialog");const int at=dialog?dialog->property("commandIndex").toInt():-1;require(dialog&&kind=="Report"&&observers.contains(at)&&observers[at]==phases.size(),"Default workflow native phase order/identity differs");const int phase=phases.size();const auto actual=runtime(),expected=truth(phase);compare(actual,expected);auto *finite=dynamic_cast<FiniteBurn *>(Moderator::Instance()->GetInternalObject("DefaultFB"));const bool firing=phase==1||phase==2;require(finite&&finite->IsFiring()==firing,"Default workflow native default burn firing differs");phases.append(QJsonObject{{"phase",phase},{"native",actual},{"truth",expected},{"firing",firing}});acceptanceView(*dialog);}catch(...){failure=std::current_exception();QTimer::singleShot(0,&rig.window,[&]{rig.window.stopMission();});}QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
   QTimer deadline;deadline.setSingleShot(true);QObject::connect(&deadline,&QTimer::timeout,&rig.window,[&]{failure=std::make_exception_ptr(std::runtime_error("Default workflow physical observer timeout"));rig.window.stopMission();});acceptanceView(rig.window);deadline.start(60000);const auto run=rig.window.runMission();deadline.stop();debug->paused=previous;events();rig.save("messages.txt",rig.messages->toPlainText().toUtf8());if(failure)std::rethrow_exception(failure);require(run==MainWindow::RunResult::Completed&&phases.size()==5&&!rig.window.isRunning(),"Default workflow configured mission completion/phases differ");const auto end=runtime();compare(end,truth(4,42));const auto raw=read(rig.report);rig.save("values.csv",raw);const auto lines=raw.trimmed().split('\n');require(lines.size()==6&&lines.last().trimmed()=="42","Default workflow raw report rows/continuation differ");for(int phase=0;phase<5;++phase){const auto columns=lines[phase].split(',');require(columns.size()==10,"Default workflow raw state report width differs");QJsonArray values;for(const auto &column:columns){bool ok=false;const double number=column.toDouble(&ok);require(ok&&std::isfinite(number),"Default workflow report value invalid");values.append(number);}compare(values,truth(phase));compare(values,phases[phase].toObject()["native"].toArray());reported.append(values);}
   require(rig.source->toPlainText()==before.source&&rig.source->document()->availableUndoSteps()==before.undo&&read(saved)==before.file&&configuredValues()==beforeValues,"Default workflow physical Run changed source/history/file/configured resources");execution=QJsonObject{{"phases",phases},{"reports",reported},{"end_state",end},{"initial_epoch",initialEpoch},{"continuation",42},{"force_newtons",100},{"total_mass_kg",950},{"default_dry_mass_kg",850},{"fuel_mass_kg",100}};++missions;
  }
  const int wantedCommands=physical?9:1;require(commandCommits==wantedCommands&&resourceCommits==(physical?2:0)&&pickerVisits.size()==(physical?6:0)&&insertionCancels==1&&confirmationCancels==(switching?1:0)&&confirmationAccepts==(switching?1:0),"Default workflow mechanism matrix cardinalities differ");
  require(explicitIdCommits==(physical?1:0),"Default workflow explicit ID edit cardinality differs");
  rig.save("mission.script",final.toUtf8());rig.save("configured-initial.json",QJsonDocument(initialValues).toJson());rig.save("configured-final.json",QJsonDocument(configuredValues()).toJson());rig.save("insertions.json",QJsonDocument(insertions).toJson());rig.save("picker-visits.json",QJsonDocument(pickerVisits).toJson());rig.save("resource-edits.json",QJsonDocument(resourceEdits).toJson());rig.save("execution.json",QJsonDocument(execution).toJson());
  QJsonArray facets{"actual-insert-template-pending-cancel-and-native-pointer-isolation","exact-command-and-only-required-default-resource-source-transaction","native-reference-source-history-unicode-roundtrip"};
  if(switching){facets.append("no-unused-defaults-after-confirmed-template-replacement");facets.append("independent-native-and-raw-scalar-continuation");}
  if(physical){facets.append("omitted-spacecraft-ids-stable-and-explicit-id-edits-preserved");facets.append("actual-default-spacecraft-tank-thruster-and-burn-hardware-picker-apply");facets.append("independent-native-raw-result-and-powered-coast-default-state-witness");}
  const QJsonObject proof{{"checkpoint","finite-default-workflow"},{"outcome","passed"},{"command",command},{"context",context},{"command_commits",commandCommits},{"resource_commits",resourceCommits},{"explicit_id_commits",explicitIdCommits},{"picker_visits",pickerVisits.size()},{"insertion_cancels",insertionCancels},{"confirmation_cancels",confirmationCancels},{"confirmation_accepts",confirmationAccepts},{"mission_executions",missions},{"native_phase_checkpoints",phases.size()},{"facets",facets},{"scope","Six explicit follow-up contexts:missing spacecraft only,finite-to-Assignment replacement with Cancel/Yes,and actual GUI hardware completion of new DefaultFB/DefaultSC from either finite command. Physical cases use explicit near-zero gravity,constant100N inertial force,850kg native default dry mass,100kg fuel,no mass decrement and10s powered/10s coast. Each facet is conditional on its declared context;no generic all-facet claim per case. Native paired-command auto-insertion policy,arbitrary resource/default/reference graphs,original private/current NASA recipes,general propagation/frames/mass depletion/science,native OS/GPU/hardware/platform/stress/provenance/full newest-build and technical approval remain open."}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
