#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <functional>
#include <map>
#include <set>
using namespace formaccept;

static void reveal(QWidget *widget) {
   require(widget, "Missing creation control");
   for (QWidget *parent=widget; parent; parent=parent->parentWidget())
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   events(); require(widget->isVisible() && widget->isEnabled(), "Creation control is inaccessible");
   acceptanceView(*widget);
}

int main(int argc, char **argv) {
   QApplication app(argc,argv); app.setOrganizationName("GMATTests");
   app.setApplicationName("MissionCreationPositions");
   try {
      require(argc==4 && acceptanceWidgetPlatform() && QApplication::platformName()!="offscreen", "Supply startup, frozen creation matrix and case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!output.isEmpty(), "Creation evidence directory required");
      const auto plan=QJsonDocument::fromJson(read(argv[2])).object(); QJsonObject spec;
      for (auto v:plan["cases"].toArray()) if (v.toObject()["key"]==QString(argv[3])) spec=v.toObject();
      require(!spec.isEmpty(), "Unknown creation case");
      TestSettings settings; QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto save=[&](const QString &name,const QByteArray &bytes){write(output+'/'+name,bytes);};
      auto expand=[&](QString value) {
         value.replace("@SPEED@",number(std::sqrt(mu/7000.)));
         value.replace("@ROOT@",output);
         value.replace("@DATA_ROOT@",QFileInfo(startup).absoluteDir().absoluteFilePath("../data"));
         value.replace("@FUNCTION@",output+"/Accept.gmf"); value.replace("@MATLAB@",output+"/Accept.m");
         const QRegularExpression pattern("@([A-Z_]+)@"); auto matches=pattern.globalMatch(value);
         QStringList tokens; while(matches.hasNext()) tokens.append(matches.next().captured(0));
         for(const auto &token:tokens) value.replace(token,output+'/'+token.mid(1,token.size()-2).toLower()+".txt");
         require(!value.contains('@'), "Unresolved creation fixture token"); return value;
      };
      const auto support=spec["files"].toObject();
      for(auto it=support.begin();it!=support.end();++it) {
         require(QFileInfo(it.key()).fileName()==it.key(), "Unsafe creation support filename");
         save(it.key(),expand(it.value().toString()).toUtf8());
      }
      save("case.json",QJsonDocument(spec).toJson());
      MainWindow window; window.resize(1200,850); window.show();
      require(window.initialize(startup), "Creation runtime unavailable"); events(); acceptanceView(window);
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",output.toStdString());
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *tree=window.findChild<QTreeWidget *>("Mission");
      auto *area=window.findChild<QMdiArea *>("workspace");
      require(source && tree && area, "Creation workspace missing");
      const auto prefix=expand(spec["prefix"].toString());
      const auto body=spec["body"].toString();
      const auto outer=spec["outer_anchor"].toString(), leaf=spec["leaf_anchor"].toString();
      const auto closing=spec["closing_anchor"].toString();
      QJsonArray results;
      for (auto entry:spec["positions"].toArray()) {
         const auto position=entry.toString();
         const auto path=output+'/'+position+"-mission-Ω.script";
         const auto initial=prefix+(position=="Empty"?QString():body)+"% Preserved creation tail Δ\n";
         write(path,initial.toUtf8());
         require(window.loadScript(path)&&window.buildScript(), "Initial creation fixture failed native Build");
         save(position+"-initial.script",initial.toUtf8()); QString wanted=initial; QJsonArray edits;
         auto named=[&](const QString &name) {
            const auto snapshot=window.missionSnapshot(); int at=-1;
            for(int i=0;i<snapshot.nodes.size();++i) if(missionCommandLabel(snapshot.nodes[i].statement).name==name) {
               require(at<0,"Ambiguous creation command label"); at=i;
            }
            require(at>=0,"Creation command label missing"); return at;
         };
         auto item=[&](int at) {
            for(QTreeWidgetItemIterator it(tree);*it;++it)
               if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==at) return *it;
            throw std::runtime_error("Creation tree item missing");
         };
         auto open=[&]() {
            for(auto *tabs:window.findChildren<QTabWidget *>()) for(int i=0;i<tabs->count();++i)
               if(tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
            QTreeWidgetItem *target=nullptr; QString action;
            if(position=="Empty"||position=="Many") {target=tree->topLevelItem(0);action="Append command…";}
            else if(position=="BranchAppend") {target=item(named("Outer"));action="appendInsideMissionBranch";}
            else if(position=="BlockBefore"||position=="BlockAfter") {target=item(named("Outer"));action=position=="BlockBefore"?"Insert before…":"Insert after…";}
            else if(position=="LeafBefore"||position=="LeafAfter") {target=item(named("Leaf"));action=position=="LeafBefore"?"Insert before…":"Insert after…";}
            else if(position=="ElseBefore"||position=="ElseAfter") {
               const auto snapshot=window.missionSnapshot();int at=-1,parent=named("Outer");
               for(int i=0;i<snapshot.nodes.size();++i) if(snapshot.nodes[i].type=="Else"&&snapshot.nodes[i].parent==parent) at=i;
               require(at>=0,"Creation Else boundary missing");target=item(at);action=position=="ElseBefore"?"Insert before…":"Insert after…";
            }
            else {
               const auto snapshot=window.missionSnapshot();int at=-1,parent=named("Outer");
               for(int i=0;i<snapshot.nodes.size();++i) if(snapshot.nodes[i].type==spec["closing_type"].toString()&&snapshot.nodes[i].parent==parent) at=i;
               require(at>=0,"Creation outer terminator missing");target=item(at);action=position=="EndBefore"?"Insert before…":"Insert after…";
            }
            require(target,"Creation target absent");tree->expandAll();tree->setCurrentItem(target);tree->scrollToItem(target);events();reveal(tree);
            if(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {
               window.raise();window.activateWindow();require(QTest::qWaitForWindowActive(window.windowHandle(),5000),"Creation window did not activate");
            }
            std::exception_ptr failure;bool visited=false;
            QTimer::singleShot(0,&window,[&] {
               auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
               try {
                  require(menu,"Creation context menu absent");QAction *choice=nullptr;
                  for(auto *a:menu->actions()) if(a->text()==action||a->objectName()==action) choice=a;
                  require(choice&&choice->isEnabled(),"Creation action unavailable");menu->setActiveAction(choice);acceptanceView(*menu);visited=true;QTest::keyClick(menu,Qt::Key_Return);
               } catch(...) {failure=std::current_exception();if(menu)menu->close();}
            });
            const auto point=tree->visualItemRect(target).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
            QApplication::sendEvent(tree->viewport(),&event);events();if(failure)std::rethrow_exception(failure);require(visited,"Creation menu was not exercised");
            auto *owner=area->activeSubWindow();require(owner&&dynamic_cast<CommandEditor *>(owner->widget()),"Creation editor missing");
            owner->showNormal();owner->move(0,0);owner->resize(area->viewport()->size());events();acceptanceView(*owner);return owner;
         };
         auto configure=[&](QMdiSubWindow *owner,const QString &statement) {
            auto *choice=owner->findChild<QComboBox *>("commandTemplate");reveal(choice);
            const int at=choice->findText(spec["template"].toString());require(at>=0,"Required creation template missing");
            QTest::mouseClick(choice,Qt::LeftButton,Qt::NoModifier,QPoint(choice->width()-10,choice->height()/2));events();require(choice->view()->isVisible(),"Creation template popup missing");
            acceptanceView(*choice->view()->window());QTest::keyClick(choice->view(),Qt::Key_Home);
            for(int i=0;i<at;++i)QTest::keyClick(choice->view(),Qt::Key_Down);
            QTest::keyClick(choice->view(),Qt::Key_Return);events();require(choice->currentText()==spec["template"].toString(),"Wrong creation template selected");
            auto *draft=owner->findChild<QPlainTextEdit *>("commandSource");reveal(draft);draft->setPlainText(statement);events();
            require(draft->toPlainText()==statement&&dynamic_cast<CommandEditor *>(owner->widget())->hasChanges(),"Creation draft differs");
         };
         auto click=[&](QMdiSubWindow *owner,QDialogButtonBox::StandardButton button) {
            auto *box=owner->findChild<QDialogButtonBox *>();auto *widget=box?box->button(button):nullptr;reveal(widget);QTest::mouseClick(widget,Qt::LeftButton);events();
         };
         const int count=position=="Many"?3:1;
         for(int p=0;p<count;++p) {
            const auto before=wanted;const int undo=source->document()->availableUndoSteps();const bool modified=source->document()->isModified();
            auto statement=spec["statement"].toString().replace("$PROBE$","Probe"+QString::number(p));
            QPointer<QMdiSubWindow> owner=open();configure(owner,statement);
            require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&source->document()->isModified()==modified,"Creation draft changed source/history");
            if(p==0) {click(owner,QDialogButtonBox::Cancel);require(!owner&&source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"Creation Cancel changed source/history");owner=open();configure(owner,statement);}
            statement+='\n';
            if(position=="Empty"||position=="Many") wanted+=statement;
            else {
               const auto anchor=position=="BlockBefore"?outer:(position=="LeafBefore"||position=="LeafAfter")?leaf:(position=="ElseBefore"||position=="ElseAfter")?spec["else_anchor"].toString():(position=="BranchAppend"&&spec.contains("append_anchor"))?spec["append_anchor"].toString():closing;
               require(wanted.count(anchor)==1,"Literal creation anchor is ambiguous");
               const bool after=position=="BlockAfter"||position=="EndAfter"||position=="LeafAfter"||position=="ElseAfter";
               wanted.insert(wanted.indexOf(anchor)+(after?anchor.size():0),statement);
            }
            QPointer<CommandEditor> stale=dynamic_cast<CommandEditor *>(owner->widget());
            click(owner,position=="Many"?QDialogButtonBox::Apply:QDialogButtonBox::Ok);
            save(position+'-'+QString::number(p)+"-committed.script",source->toPlainText().toUtf8());
            save(position+'-'+QString::number(p)+"-expected.script",wanted.toUtf8());
            require(!stale&&source->toPlainText()==wanted&&source->document()->availableUndoSteps()>undo,"Creation committed wrong source/history");
            if(position=="Many") {
               require(owner&&!dynamic_cast<CommandEditor *>(owner->widget())->hasChanges(),"Creation Apply did not refresh owner");
               const int steps=source->document()->availableUndoSteps();click(owner,QDialogButtonBox::Apply);
               require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==steps,"Clean Apply duplicated creation");click(owner,QDialogButtonBox::Ok);
            }
            require(!owner,"Creation editor remained open");
            source->undo();require(source->toPlainText()==before&&window.buildScript(),"Creation Undo did not restore source/model");
            source->redo();require(source->toPlainText()==wanted&&window.buildScript(),"Creation Redo did not restore source/model");
            edits.append(QJsonObject{{"label","Probe"+QString::number(p)},{"statement",statement},{"cancelled_first",p==0},{"undo_redo",true},{"clean_apply",position=="Many"}});
         }
         require(window.saveScriptTo(path)&&read(path)==wanted.toUtf8()&&window.loadScript(path)&&window.buildScript()&&source->toPlainText()==wanted,"Creation Unicode save/reopen differs");
         save(position+"-accepted.script",wanted.toUtf8());
         const auto snapshot=window.missionSnapshot();QJsonArray hierarchy;
         std::map<std::string,std::pair<std::string,std::string>> native;std::map<std::string,int> nativeBranch;std::set<GmatCommand *> seen;
         std::function<void(GmatCommand *,GmatCommand *,std::string,int)> walk=[&](GmatCommand *command,GmatCommand *parent,std::string parentName,int branchIndex) {
            while(command&&command!=parent&&seen.insert(command).second) {
               const auto label=command->GetName();
               if(!label.empty()) {native[label]={command->GetTypeName(),parentName};nativeBranch[label]=branchIndex;}
               for(int i=0;auto *child=command->GetChildCommand(i);++i) walk(child,command,label,i);
               command=command->GetNextWhileExecuting();
            }
         };
         walk(Moderator::Instance()->GetFirstCommand(),nullptr,{},-1);
         QJsonArray nativeEntries;
         for(const auto &entry:native) nativeEntries.append(QJsonObject{{"label",QString::fromStdString(entry.first)},
            {"type",QString::fromStdString(entry.second.first)},{"parent",QString::fromStdString(entry.second.second)},{"branch_index",nativeBranch.at(entry.first)}});
         save(position+"-native-graph.json",QJsonDocument(nativeEntries).toJson());
         const auto expectedParent=(position=="LeafBefore"||position=="LeafAfter")?QString("Inner"):(position=="BranchAppend"||position=="EndBefore"||position=="ElseBefore"||position=="ElseAfter")?QString("Outer"):QString();
         for(int p=0;p<count;++p) {
            const auto label="Probe"+QString::number(p);const int at=named(label);const auto &node=snapshot.nodes[at];
            require(node.type==spec["native_type"].toString(),"Creation native type differs");
            require(expectedParent.isEmpty()?node.parent<0:node.parent==named(expectedParent),"Creation snapshot parent differs");
            require(expectedParent.isEmpty()?item(at)->parent()==tree->topLevelItem(0):item(at)->parent()==item(named(expectedParent)),"Creation visible parent differs");
            const auto match=native.find(label.toStdString());require(match!=native.end()&&match->second.first==spec["native_type"].toString().toStdString()&&match->second.second==expectedParent.toStdString(),"Independent native command traversal differs");
            const auto branchExpected=spec["branch_expectations"].toObject();
            if(branchExpected.contains(position)) require(nativeBranch.at(label.toStdString())==branchExpected[position].toInt(),"Inserted command is in the wrong If/Else arm");
         }
         for(const auto &node:snapshot.nodes) hierarchy.append(QJsonObject{{"type",node.type},{"label",missionCommandLabel(node.statement).name},{"parent",node.parent},{"statement",node.statement}});
         save(position+"-hierarchy.json",QJsonDocument(hierarchy).toJson());
         results.append(QJsonObject{{"position",position},{"commands",count},{"parent",expectedParent},{"native_graph_checked",true},{"source_history_exact",true},{"unicode_reopen",true},{"edits",edits}});
         save("positions.json",QJsonDocument(results).toJson());
      }
      for(auto it=support.begin();it!=support.end();++it)require(read(output+'/'+it.key())==expand(it.value().toString()).toUtf8(),"Creation changed a support file");
      const QJsonObject proof{{"status","passed"},{"native_platform",QApplication::platformName()},{"case",spec["key"]},{"family",spec["family"]},{"outer_context",spec["outer_context"]},{"positions",results},{"native_missions_run",0},{"new_requirement_credit",0},{"scope","Missing GUI creation/configuration positions with independent exact source and native command-graph checks; numerical command execution is a separate reused evidence constituent."}};
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;
   } catch(BaseException &e) {std::cerr<<e.GetFullMessage();} catch(std::exception &e) {std::cerr<<e.what();}
   return 1;
}
