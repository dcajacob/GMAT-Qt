#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "Debugger.hpp"
#include "ResourceEditor.hpp"
#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <functional>
using namespace formaccept;

static QJsonObject entry(const QString &file,const QString &key)
{
   for (const auto &value:QJsonDocument::fromJson(read(file)).object()["cases"].toArray())
      if (value.toObject()["key"]==key) return value.toObject();
   throw std::runtime_error("Control insertion case absent");
}
static void reveal(QWidget *widget)
{
   require(widget,"Control insertion widget absent");
   for (QWidget *parent=widget;parent;parent=parent->parentWidget())
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   events();require(widget->isVisible()&&widget->isEnabled(),"Control insertion widget unreachable");
   acceptanceView(*widget);
}
using State=std::array<double,7>;
static QJsonArray array(const State &state)
{
   QJsonArray result;for (double value:state) result.append(value);return result;
}
static State nativeState()
{
   State result;int i=0;
   for (const char *name:{"I","J","W","Gate","Score","Seen","Keep"}) {
      auto *object=Moderator::Instance()->GetInternalObject(name);
      require(object,"Control insertion runtime variable absent");
      result[i++]=object->GetRealParameter("Value");
   }
   return result;
}
static void equal(const QJsonArray &actual,const QJsonArray &expected)
{
   require(actual.size()==expected.size(),"Control insertion observation width differs");
   for (int i=0;i<actual.size();++i)
      require(std::isfinite(actual[i].toDouble())&&actual[i].toDouble()==expected[i].toDouble(),
              "Control insertion result differs from independent integer oracle");
}
struct Oracle {QJsonArray samples;State final;};
static Oracle truth(const QString &family,const QString &position,const QString &parentKind="If")
{
   State state{-9,-7,0,-1,0,0,41};QJsonArray samples;
   auto observe=[&](int probe,const QString &branch) {
      samples.append(QJsonObject{{"probe",probe},{"branch",branch},{"values",array(state)}});++state[5];
   };
   auto execute=[&](int probe) {
      if (family=="For") for (int j=1;j<=3;++j) {state[1]=j;state[4]+=2;observe(probe,"loop");}
      else if (family=="While") {int guard=0;while (state[2]<3) {require(++guard<=3,"Oracle loop bound exceeded");++state[2];state[4]+=3;observe(probe,"loop");}}
      else if (state[3]>0) {state[4]+=family=="IfElse"?7:5;observe(probe,"true");}
      else if (family=="IfElse") {state[4]+=11;observe(probe,"false");}
   };
   if (position=="Empty") execute(0);
   else {
      if (position=="BlockBefore") execute(0);
      for (int i=1;i<=2;++i) {
         state[0]=i;state[3]=i-1;state[2]=0;
         if(parentKind=="IfElse") {if(i==((position=="ElseBefore"||position=="BranchAppend")?1:2))execute(0);}
         else {
         if (position=="LeafBefore") execute(0);
         if (position=="LeafAfter") execute(0);
         if (position=="BranchAppend"||position=="EndBefore") execute(0);
         }
      }
      if (position=="BlockAfter"||position=="EndAfter") execute(0);
      ++state[6];
      if (position=="Many") for (int p=0;p<3;++p) execute(p);
   }
   return {samples,state};
}
static QString block(const QString &family,int probe)
{
   const auto name="Probe"+QString::number(probe);
   auto body=[&](const QString &branch,int delta) {
      return "Score = Score + "+QString::number(delta)+";\n'Observe "+name+" "+branch+"' Seen = Seen + 1;\n";
   };
   if (family=="For") return "For '"+name+"' J = 1:1:3;\n"+body("loop",2)+"EndFor;";
   if (family=="While") return "While '"+name+"' W < 3;\nW = W + 1;\n"+body("loop",3)+"EndWhile;";
   return "If '"+name+"' Gate > 0;\n"+body("true",family=="IfElse"?7:5)+
      (family=="IfElse"?"Else;\n"+body("false",11):QString())+"EndIf;";
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaControlTree");
   try {
      require(argc==5&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen",
              "Supply startup, reused command cases, insertion cases and key on a native widget platform");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      const auto spec=entry(QFileInfo(argv[3]).absoluteFilePath(),argv[4]);
      const auto reused=entry(QFileInfo(argv[2]).absoluteFilePath(),spec["recipe_key"].toString());
      require(reused["legacy_object"]==spec["legacy_object"],"Control insertion reused branch identity differs");
      const auto family=spec["family"].toString(),id=spec["key"].toString();
      const auto parentLabel=spec["parent"].toString("Outer");require(parentLabel=="Outer"||parentLabel=="Inner","Explicit insertion parent identity differs");
      const auto parentKind=spec["parent_type"].toString("If");require(parentKind=="If"||parentKind=="While"||parentKind=="IfElse","Explicit inner parent type differs");require(parentKind!="While"||parentLabel=="Inner","While parent requires explicit Inner target");require(parentKind!="IfElse"||parentLabel=="Inner","IfElse parent requires explicit Inner target");
      require(QStringList{"For","If","IfElse","While"}.contains(family),"Unknown insertion family");
      TestSettings settings;QTemporaryDir files;require(files.isValid(),"Control insertion fixture absent");
      QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;window.resize(1200,850);window.show();
      require(window.initialize(startup),"Control insertion runtime unavailable");events();acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");
      auto *tree=window.findChild<QTreeWidget *>("Mission"),*resources=window.findChild<QTreeWidget *>("Resources");
      auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&resources&&area,"Control insertion workspace missing");
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &bytes) {if (!evidence.isEmpty()) write(evidence+"/"+name,bytes);};
      save("case.json",QJsonDocument(spec).toJson());
      QString prefix="% Control insertion witness Ω\nCreate Variable I J W Gate Score Seen Keep;\nI = -9;\nJ = -7;\nW = 0;\nGate = -1;\nScore = 0;\nSeen = 0;\nKeep = 41;\nBeginMissionSequence;\n";
      if(parentKind=="While")prefix.replace("BeginMissionSequence;\n","Create Variable ParentPass ParentEntries;\nParentPass = 0;\nParentEntries = 0;\nBeginMissionSequence;\n");
      if(parentKind=="IfElse")prefix.replace("BeginMissionSequence;\n","Create Variable ParentTrue ParentFalse;\nParentTrue = 0;\nParentFalse = 0;\nBeginMissionSequence;\n");
      const QString outer="For 'Outer' I = 1:1:2;\n",inner=parentKind=="IfElse"?"If 'Inner' I == 1;\nParentTrue = ParentTrue + 1;\n":parentKind=="While"?"While 'Inner' ParentPass < 1;\nParentPass = ParentPass + 1;\nParentEntries = ParentEntries + 1;\n":"If 'Inner' I > 0;\n",leaf="'Leaf' Keep = Keep + 0;\n";
      const QString endIf="EndIf;\n",endWhile="EndWhile;\n",endFor="EndFor;\n";
      const QString body=outer+"Gate = I - 1;\nW = 0;\n"+(parentKind=="While"?QString("ParentPass = 0;\n"):QString())+inner+leaf+(parentKind=="IfElse"?QString("Else;\nParentFalse = ParentFalse + 1;\n'Alternate' Keep = Keep + 0;\n"):QString())+(parentKind=="While"?endWhile:endIf)+endFor+"'After outer' Keep = Keep + 1;\n";
      int commits=0,cancels=0,rawValues=0;QJsonArray qualified;
      for (const auto &value:spec["positions"].toArray()) {
         const auto positionSpec=value.toObject();const auto position=positionSpec["position"].toString();
         require(parentLabel=="Outer"||position=="BranchAppend"||position=="EndBefore"||(parentKind=="IfElse"&&(position=="ElseBefore"||position=="ElseAfter")),"Inner parent only supports explicitly qualified positions");
         const auto saved=files.filePath(position+" Ω.script");
         const QString initial=prefix+(position=="Empty"?QString():body)+"% Preserve tail Δ\n";
         write(saved,initial.toUtf8());require(window.loadScript(saved)&&window.buildScript(),"Control insertion initial build failed");
         save(position+"-initial.script",initial.toUtf8());QString wanted=initial;QJsonArray edits;
         auto named=[&](const QString &name) {
            const auto snapshot=window.missionSnapshot();int at=-1;
            for (int i=0;i<snapshot.nodes.size();++i) if (missionCommandLabel(snapshot.nodes[i].statement).name==name) {
               require(at<0,"Control insertion label ambiguous");at=i;
            }
            require(at>=0,"Control insertion named node absent");return at;
         };
         auto item=[&](int at) {
            for (QTreeWidgetItemIterator it(tree);*it;++it)
               if ((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==at) return *it;
            throw std::runtime_error("Control insertion tree item absent");
         };
         auto insertion=[&]() {
            for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i)
               if (tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
            QTreeWidgetItem *target=nullptr;QString action;
            if (position=="Empty"||position=="Many") {target=tree->topLevelItem(0);action="Append command…";}
            else if (position=="BranchAppend") {target=item(named(parentLabel));action="appendInsideMissionBranch";}
            else if(position=="ElseBefore"||position=="ElseAfter") {
               const auto snapshot=window.missionSnapshot();const int parent=named("Inner");int selected=-1;
               for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].type=="Else"&&snapshot.nodes[i].parent==parent){require(selected<0,"Parent Else identity ambiguous");selected=i;}
               require(selected>=0,"Parent Else absent");target=item(selected);action=position=="ElseBefore"?"Insert before…":"Insert after…";
            }
            else if (position=="BlockBefore"||position=="BlockAfter") {target=item(named("Outer"));action=position=="BlockBefore"?"Insert before…":"Insert after…";}
            else if (position=="LeafBefore"||position=="LeafAfter") {target=item(named("Leaf"));action=position=="LeafBefore"?"Insert before…":"Insert after…";}
            else {
               const auto snapshot=window.missionSnapshot();const int parent=named(parentLabel);int closing=-1;
               for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type==(parentLabel=="Inner"?(parentKind=="While"?"EndWhile":"EndIf"):"EndFor")&&snapshot.nodes[i].parent==parent) closing=i;
               require(closing>=0,"Control insertion outer terminator absent");target=item(closing);action=position=="EndBefore"?"Insert before…":"Insert after…";
            }
            require(target,"Control insertion target absent");tree->expandAll();tree->setCurrentItem(target);tree->scrollToItem(target);events();reveal(tree);
            std::exception_ptr failure;bool visited=false;
            QTimer::singleShot(0,&window,[&] {
               auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
               try {
                  require(menu,"Actual insertion context menu absent");QAction *choice=nullptr;
                  for (auto *candidate:menu->actions()) if (candidate->text()==action||candidate->objectName()==action) choice=candidate;
                  require(choice&&choice->isEnabled(),"Actual insertion action unavailable");menu->setActiveAction(choice);acceptanceView(*menu);visited=true;QTest::keyClick(menu,Qt::Key_Return);
               } catch (...) {failure=std::current_exception();if (menu) menu->close();}
            });
            const auto point=tree->visualItemRect(target).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
            QApplication::sendEvent(tree->viewport(),&event);events();if (failure) std::rethrow_exception(failure);require(visited,"Insertion action not visited");
            auto *owner=area->activeSubWindow();require(owner&&dynamic_cast<CommandEditor *>(owner->widget()),"Actual insertion command editor absent");
            owner->showNormal();owner->move(0,0);owner->resize(area->viewport()->size());events();acceptanceView(*owner);return owner;
         };
         auto configure=[&](QMdiSubWindow *owner,int p) {
            auto *choice=owner->findChild<QComboBox *>("commandTemplate");reveal(choice);
            const auto wantedType=family=="IfElse"?QString("If"):family;const int index=choice->findText(wantedType);require(index>=0,"Control command template absent");
            QTest::mouseClick(choice,Qt::LeftButton,Qt::NoModifier,QPoint(choice->width()-10,choice->height()/2));events();require(choice->view()->isVisible(),"Control command template popup absent");
            acceptanceView(*choice->view()->window());QTest::keyClick(choice->view(),Qt::Key_Home);
            for (int i=0;i<index;++i) QTest::keyClick(choice->view(),Qt::Key_Down);
            QTest::keyClick(choice->view(),Qt::Key_Return);events();require(choice->currentText()==wantedType,"Control command template selection differs");
            auto *draft=owner->findChild<QPlainTextEdit *>("commandSource");reveal(draft);draft->setPlainText(block(family,p));events();
            require(draft->toPlainText()==block(family,p)&&dynamic_cast<CommandEditor *>(owner->widget())->hasChanges(),"Control insertion source draft differs");acceptanceView(*owner);
         };
         auto click=[&](QMdiSubWindow *owner,QDialogButtonBox::StandardButton which) {
            auto *box=owner->findChild<QDialogButtonBox *>();auto *button=box?box->button(which):nullptr;reveal(button);QTest::mouseClick(button,Qt::LeftButton);events();
         };
         const int count=position=="Many"?3:1;
         for (int p=0;p<count;++p) {
            const auto before=wanted;const int undo=source->document()->availableUndoSteps();
            const bool modified=source->document()->isModified();QPointer<QMdiSubWindow> owner=insertion();configure(owner,p);
            require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&source->document()->isModified()==modified,"Pending insertion escaped draft/history isolation");
            if (p==0) {
               click(owner,QDialogButtonBox::Cancel);require(!owner&&source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"Insertion Cancel changed source/history or retained owner");
               ++cancels;owner=insertion();configure(owner,p);
            }
            const auto statement=block(family,p)+"\n";
            if (position=="Empty"||position=="Many") wanted+=statement;
            else {
               const auto anchor=position=="ElseBefore"||position=="ElseAfter"||(parentKind=="IfElse"&&position=="BranchAppend")?QString("Else;\n"):position=="BlockBefore"?outer:position=="LeafBefore"||position=="LeafAfter"?leaf:parentLabel=="Inner"?(parentKind=="While"?endWhile:endIf):endFor;
               require(wanted.count(anchor)==1,"Independent insertion source anchor ambiguous");
               const bool after=position=="BlockAfter"||position=="EndAfter"||position=="LeafAfter"||position=="ElseAfter";
               wanted.insert(wanted.indexOf(anchor)+(after?anchor.size():0),statement);
            }
            QPointer<CommandEditor> old=dynamic_cast<CommandEditor *>(owner->widget());
            click(owner,position=="Many"?QDialogButtonBox::Apply:QDialogButtonBox::Ok);
            require(!old&&source->toPlainText()==wanted&&source->document()->availableUndoSteps()>undo,"Control insertion committed wrong source/history or retained stale editor");
            if (position=="Many") {
               require(owner&&!dynamic_cast<CommandEditor *>(owner->widget())->hasChanges(),"Insertion Apply lost refreshed clean owner");
               const int steps=source->document()->availableUndoSteps();
               auto *clean=owner->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);
               require(clean&&clean->isVisible(),"Clean insertion Apply absent");
               acceptanceView(*owner);QTest::mouseClick(clean,Qt::LeftButton);events();
               require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==steps,"Repeated insertion Apply duplicated command/history");
               click(owner,QDialogButtonBox::Ok);
            }
            require(!owner,"Insertion OK did not close original owner");++commits;
            source->undo();require(source->toPlainText()==before&&window.buildScript(),"Insertion Undo failed exact source/model restoration");
            source->redo();require(source->toPlainText()==wanted&&window.buildScript(),"Insertion Redo failed exact source/model restoration");
            edits.append(QJsonObject{{"probe",p},{"statement",statement},{"mode",position=="Many"?"apply-clean-apply-ok":"positive-ok"},{"cancel_first",p==0},{"source_history_exact",true},{"stale_editor_retired",true},{"owner_closed",true}});
         }
         require(window.saveScriptTo(saved)&&read(saved)==wanted.toUtf8()&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted,"Control insertion Unicode save/reload changed exact source/model");
         save(position+"-accepted.script",wanted.toUtf8());save(position+"-edits.json",QJsonDocument(edits).toJson());
         auto parentLoopState=[&]{auto value=[](const char *name){auto *v=Moderator::Instance()->GetInternalObject(name);require(v,"Native parent loop counter absent");return v->GetRealParameter("Value");};return QJsonObject{{"i",value("I")},{"pass",value("ParentPass")},{"entries",value("ParentEntries")}};};QJsonArray parentLoopSamples;
         auto parentBranchState=[&]{auto value=[](const char *name){auto *v=Moderator::Instance()->GetInternalObject(name);require(v,"Native parent branch counter absent");return v->GetRealParameter("Value");};return QJsonObject{{"i",value("I")},{"true",value("ParentTrue")},{"false",value("ParentFalse")}};};QJsonArray parentBranchSamples;
         const auto snapshot=window.missionSnapshot();QJsonArray hierarchy;QMap<int,QPair<int,QString>> observers;
         for (int i=0;i<snapshot.nodes.size();++i) {
            const auto &node=snapshot.nodes[i];const auto label=missionCommandLabel(node.statement).name;
            hierarchy.append(QJsonObject{{"type",node.type},{"label",label},{"parent",node.parent},{"statement",node.statement}});
            for (int p=0;p<count;++p) for (const auto &branch:QStringList{"loop","true","false"})
               if (label=="Observe Probe"+QString::number(p)+" "+branch) observers[i]={p,branch};
         }
         const QString parent=position=="LeafBefore"||position=="LeafAfter"||position=="ElseBefore"||position=="ElseAfter"?"Inner":position=="BranchAppend"||position=="EndBefore"?parentLabel:QString();
         for (int p=0;p<count;++p) {
            const int index=named("Probe"+QString::number(p));const auto &node=snapshot.nodes[index];
            require(node.type==(family=="IfElse"?QString("If"):family),"Inserted native control command type differs");
            require(parent.isEmpty()?node.parent<0:node.parent==named(parent),"Inserted native branch has wrong parent");
            require(parent.isEmpty()?item(index)->parent()==tree->topLevelItem(0):item(index)->parent()==item(named(parent)),"Visible insertion tree parent differs from literal hierarchy");
         }
         require(observers.size()==count*(family=="IfElse"?2:1),"Inserted native body observer count differs");
         save(position+"-hierarchy.json",QJsonDocument(hierarchy).toJson());tree->expandAll();events();acceptanceView(*tree);
         const auto expected=truth(family,position,parentKind);QJsonArray samples;std::exception_ptr failure;Debugger *debug=nullptr;
         for (auto *object:window.children()) if (auto *candidate=dynamic_cast<Debugger *>(object)) debug=candidate;
         require(debug,"Control insertion actual debugger absent");const auto previous=debug->paused;
         for (auto it=observers.begin();it!=observers.end();++it) require(window.setBreakpoint(it.key(),true),"Native body observation breakpoint rejected");
         debug->paused=[&](const QString &type) {
            if (previous) previous(type);
            try {
               auto *dialog=window.findChild<QDialog *>("debuggerDialog");require(dialog&&dialog->isVisible(),"Actual control insertion debugger dialog absent");acceptanceView(*dialog);
               const int index=dialog->property("commandIndex").toInt();require(observers.contains(index),"Unselected command paused during insertion witness");
               const auto selected=observers[index];const QJsonObject sample{{"probe",selected.first},{"branch",selected.second},{"values",array(nativeState())}};
               samples.append(sample);save(position+"-samples.json",QJsonDocument(samples).toJson());if(parentKind=="IfElse"){const auto q=parentBranchState();require(q["i"].toDouble()==((position=="ElseBefore"||position=="BranchAppend")?1:2)&&q["true"].toDouble()==1&&q["false"].toDouble()==(position=="ElseBefore"||position=="ElseAfter"||position=="BranchAppend"?0:1),"Native parent branch order/count differs");parentBranchSamples.append(q);}if(parentKind=="While"){const auto q=parentLoopState();require(q["pass"].toDouble()==1&&q["entries"]==q["i"],"Native While parent entry/order differs");parentLoopSamples.append(q);}
               require(samples.size()<=expected.samples.size()&&sample==expected.samples[samples.size()-1].toObject(),"Inserted branch ran at wrong position/order/state or took wrong path");
            } catch (...) {failure=std::current_exception();QTimer::singleShot(0,&window,[&] {window.stopMission();});}
            QTimer::singleShot(0,&window,[&] {debug->resume();});
         };
         QTimer deadline;deadline.setSingleShot(true);QObject::connect(&deadline,&QTimer::timeout,&window,[&] {failure=std::make_exception_ptr(std::runtime_error("Control insertion watchdog expired"));window.stopMission();});
         const int beforeRun=source->document()->availableUndoSteps();messages->clear();deadline.start(20000);acceptanceView(window);
         const auto status=window.runMission();deadline.stop();debug->paused=previous;events();save(position+"-messages.txt",messages->toPlainText().toUtf8());
         if (failure) std::rethrow_exception(failure);
         require(status==MainWindow::RunResult::Completed&&samples==expected.samples,"Control insertion mission failed or body observation count differs");
         const auto final=array(nativeState());equal(final,array(expected.final));
         require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==beforeRun&&read(saved)==wanted.toUtf8()&&!window.isRunning(),"Control insertion Run changed source/history/file or completion state");
         save(position+"-results.json",QJsonDocument(QJsonObject{{"position",position},{"family",family},{"samples",samples},{"expected_samples",expected.samples},{"final",final},{"expected_final",array(expected.final)},{"completed",true}}).toJson());
         rawValues+=7*(samples.size()+1);qualified.append(positionSpec);if(parentKind=="IfElse"){const auto q=parentBranchState();require(q["i"].toDouble()==2&&q["true"].toDouble()==1&&q["false"].toDouble()==1,"Native parent final true/false counts differ");save(position+"-parent-branches.json",QJsonDocument(QJsonObject{{"samples",parentBranchSamples},{"final",q}}).toJson());}if(parentKind=="While"){const auto q=parentLoopState();require(q["pass"].toDouble()==1&&q["entries"].toDouble()==2&&q["i"].toDouble()==2,"Native While parent final entry/count differs");save(position+"-parent-loop.json",QJsonDocument(QJsonObject{{"samples",parentLoopSamples},{"final",q}}).toJson());}
         for (auto it=observers.begin();it!=observers.end();++it) require(window.setBreakpoint(it.key(),false),"Native observer breakpoint cleanup failed");
      }
      const QJsonObject proof{{"checkpoint","control-tree-"+id},{"outcome","passed"},{"case",id},{"family",family},{"group",spec["group"]},{"parent",parentLabel},{"parent_type",parentKind},{"positions",qualified},{"missions",qualified.size()},{"insertions",commits},{"cancelled_drafts",cancels},{"raw_native_values",rawValues},{"facets",QJsonArray{"actual-context-menu-and-control-template-selection","positive-insertion-ok-and-apply-becomes-replacement","cancel-and-clean-apply-source-history-isolation","literal-source-undo-redo-unicode-save-reload","native-and-visible-parent-and-own-terminator-placement","independent-integer-branch-loop-body-boundary-and-final-results"}},{"scope","Finite documented insertion positions for For/plain If/If-with-Else/While using an outer For and inner If. Native body assignment breakpoints and final scalar objects match independent exact integer simulation. Original private keyword recipes, arbitrary expressions/parent types/filters/solver contexts, native OS input/GPU/hardware/platform/stress/current private NASA corpus and full newest-build qualification remain open."}};
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}
     catch (std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}
   return 1;
}
