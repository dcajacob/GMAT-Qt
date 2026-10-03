#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "PropagationColorDialog.hpp"
#include "QtPlotReceiver.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "RgbColor.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QEventLoop>
#include <QWindow>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <iostream>
#include <cmath>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Color report unavailable"); return file.readAll(); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) { QApplication::processEvents(); throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); } }
static const PlotCurve &satellite(const PlotModel &model) { for (const auto &curve:model.curves) if (curve.name=="Sat") return curve; throw std::runtime_error("Colored satellite curve missing"); }
static void sourceCases()
{
   const QString first="% lead α\nPropagate 'keep label' BackProp Synchronized P(Sat, 'STM') Other(Second) {OrbitColor = Green, Sat.ElapsedSecs = 60, StopTolerance = 1e-8}; % tail\n";
   PropagationColorDialog unchanged(first); require(unchanged.statement()==first,"Opening color editor changed source");
   unchanged.findChild<QCheckBox *>("propagationColorOverride")->setChecked(false);
   require(unchanged.statement()==QString(first).replace("OrbitColor = Green,",""),"First color removal changed other command source");
   for (const auto &source:QStringList{
      "Propagate P(Sat) {Sat.ElapsedSecs = 60, OrbitColor = Green, StopTolerance = 1e-8}; % tail",
      "Propagate P(Sat) {Sat.ElapsedSecs = 60, StopTolerance = 1e-8, OrbitColor = Green}; % tail"}) {
      PropagationColorDialog dialog(source); dialog.findChild<QCheckBox *>("propagationColorOverride")->setChecked(false);
      require(dialog.statement()==QString(source).replace(", OrbitColor = Green",""),"Color removal changed tolerance/stop/spacing");
   }
   PropagationColorDialog only("Propagate P(Sat) {OrbitColor = Red}; % one step"); only.findChild<QCheckBox *>("propagationColorOverride")->setChecked(false);
   require(only.statement()=="Propagate P(Sat) ; % one step","Sole color removal left empty braces or lost comment");
   PropagationColorDialog options("Propagate P(Sat) {StopTolerance = 1e-8, OrbitColor = Blue}; % one step"); options.findChild<QCheckBox *>("propagationColorOverride")->setChecked(false);
   require(options.statement()=="Propagate P(Sat) ; % one step","Removing a SingleStep color left invalid inactive-tolerance braces or lost comment");
   PropagationColorDialog step("Propagate P(Sat); % one step"); step.findChild<QCheckBox *>("propagationColorOverride")->setChecked(true); step.findChild<QLineEdit *>("propagationColorValue")->setText("Blue");
   require(step.statement()=="Propagate P(Sat) {OrbitColor = Blue}; % one step","One-step color insertion lost command/comment");
   PropagationColorDialog mapped("Propagate P(Sat) {Sat.ElapsedSecs = 60,\n OrbitColor = [10 % keep RGB comment\n20 30],\n StopTolerance = 1e-8}; % tail");
   mapped.findChild<QLineEdit *>("propagationColorValue")->setText("Orange"); const auto edited=mapped.statement();
   if (!edited.contains("OrbitColor = Orange % keep RGB comment\n") || !edited.contains("StopTolerance = 1e-8") || !edited.endsWith("; % tail")) throw std::runtime_error(("Mapped color lost comment/newline/option: "+edited).toStdString());
   require(!PropagationColorDialog::supports("Propagate P(Sat) {OrbitColor = Red, OrbitColor = Blue};") && !PropagationColorDialog::supports("Report R Sat.X;"),"Unsupported/ambiguous color source offered controls");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtPropagationColor");
   try {
      TestSettings settings; QTemporaryDir files; const bool preview=argc==4 && QString::fromLocal8Bit(argv[2])=="--preview"; require((argc==2 || preview) && files.isValid(),"Color fixture setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Color runtime initialization failed");
      if (preview) {
         PropagationColorDialog dialog("Propagate 'Color preview' P(Sat) {Sat.ElapsedSecs = 60, OrbitColor = [40 140 240]};",&window); dialog.resize(560,180); dialog.show();
         QEventLoop wait; QTimer::singleShot(250,&wait,&QEventLoop::quit); wait.exec(); auto *note=dialog.findChild<QLabel *>("propagationColorNote"); require(note && note->height()>=note->heightForWidth(note->width()),"Native segment color explanation is clipped"); require(dialog.windowHandle() && dialog.windowHandle()->isExposed() && dialog.grab().save(QString::fromLocal8Bit(argv[3])),"Native segment color preview not exposed or saved");
         std::cout<<"PASS: exposed native segment-color dialog preview; no numerical workflow was repeated.\n"; return 0;
      }
      sourceCases(); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("color state.txt"),saved=files.filePath("segment colors Δ.script");
      const QString prefix="% retain force source α\nCreate Spacecraft Sat;\nSat.OrbitColor = Red;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nCreate OrbitView Orb;\nOrb.Add = {Sat, Earth};\nCreate GroundTrack Ground;\nGround.Add = {Sat};\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nBeginMissionSequence;\n";
      const QString first="Propagate 'First' P(Sat) {Sat.ElapsedSecs = 60, StopTolerance = 1e-8}; % first segment\n",second="Propagate 'Second' P(Sat) {Sat.ElapsedSecs = 120, OrbitColor = Purple}; % second segment\n",third="Propagate 'Third' P(Sat) {Sat.ElapsedSecs = 180, OrbitColor = Magenta}; % third segment\n";
      const QString suffix="Report Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
      const auto source=prefix+first+second+third+suffix;
      const auto reference=prefix+QString(first).replace("StopTolerance = 1e-8","StopTolerance = 1e-8, OrbitColor = [40 140 240]")+QString(second).replace("Purple","Orange")+QString(third).replace(", OrbitColor = Magenta","")+suffix;
      editor->setPlainText(reference); run(window); const auto expected=read(report); require(!expected.isEmpty(),"Independent color report empty");
      QMap<QString,std::deque<PlotPoint>> paths; for (const auto &name:QStringList{"Orb","Ground"}) paths[name]=satellite(*window.plotReceiver()->model(name)).points;
      editor->setPlainText(source); require(window.buildScript(),"Color edit fixture failed to build");
      auto open=[&](const QString &label) {
         const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("'"+label+"'")) index=i;
         require(index>=0,"Labeled propagation command missing"); auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree);
         while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item;
         require(*item,"Labeled propagation tree item missing"); tree->itemDoubleClicked(*item,0); auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Actual propagation MDI editor missing"); return child;
      };
      auto colorDialog=[&](CommandEditor &panel,std::function<void(PropagationColorDialog &)> inspect) {
         std::exception_ptr failure; QTimer::singleShot(0,&panel,[&] { auto *dialog=dynamic_cast<PropagationColorDialog *>(panel.findChild<QDialog *>("propagationColorDialog")); try { require(dialog,"Color dialog unavailable"); inspect(*dialog); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
         auto *button=panel.findChild<QPushButton *>("editPropagationColor"); require(button && button->isVisible(),"Segment color control missing"); button->click(); if (failure) std::rethrow_exception(failure);
      };
      auto apply=[&](QMdiSubWindow *child,CommandEditor *panel,const QString &before) {
         require(panel->hasChanges() && editor->toPlainText()==before,"Color edit was not pending"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         require(child->isVisible() && child->widget()!=panel && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges(),"Color Apply did not refresh retained command panel"); const auto accepted=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==before,"Color mission Undo was not exact"); editor->redo(); require(editor->toPlainText()==accepted,"Color mission Redo was not exact");
      };
      auto *child=open("First"); auto *panel=dynamic_cast<CommandEditor *>(child->widget()); const auto originalCommand=panel->findChild<QPlainTextEdit *>("commandSource")->toPlainText();
      colorDialog(*panel,[&](PropagationColorDialog &dialog) { dialog.findChild<QCheckBox *>("propagationColorOverride")->setChecked(true); dialog.findChild<QLineEdit *>("propagationColorValue")->setText("Blue"); dialog.reject(); });
      require(!panel->hasChanges() && panel->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==originalCommand && editor->toPlainText()==source,"Color Cancel changed command or mission");
      colorDialog(*panel,[&](PropagationColorDialog &dialog) {
         dialog.findChild<QCheckBox *>("propagationColorOverride")->setChecked(true); auto *value=dialog.findChild<QLineEdit *>("propagationColorValue"); value->setText("[256 0 0]"); require(!dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled() && !dialog.findChild<QLabel *>("propagationColorError")->text().isEmpty(),"Invalid RGB accepted");
         std::exception_ptr failure; const auto pick=[&](bool accept) { QTimer::singleShot(0,&dialog,[&] { auto *picker=dialog.findChild<QColorDialog *>("propagationColorPicker"); try { require(picker,"Segment color picker unavailable"); picker->setCurrentColor(QColor(40,140,240)); accept ? picker->accept() : picker->reject(); } catch (...) { failure=std::current_exception(); if (picker) picker->reject(); } }); dialog.findChild<QPushButton *>("propagationColorChoose")->click(); if (failure) std::rethrow_exception(failure); };
         pick(false); require(value->text()=="[256 0 0]","Color picker Cancel changed pending value"); pick(true); require(value->text()=="[40 140 240]" && dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Color picker correction failed"); dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      });
      auto *text=panel->findChild<QPlainTextEdit *>("commandSource"); const auto pending=text->toPlainText(); text->undo(); require(text->toPlainText()==originalCommand,"Pending color edit was not one Undo"); text->redo(); require(text->toPlainText()==pending,"Pending color Redo failed"); apply(child,panel,source);
      auto before=editor->toPlainText(); child=open("Second"); panel=dynamic_cast<CommandEditor *>(child->widget()); colorDialog(*panel,[](PropagationColorDialog &dialog) { require(dialog.findChild<QCheckBox *>("propagationColorOverride")->isChecked() && dialog.findChild<QLineEdit *>("propagationColorValue")->text()=="Purple","Existing named segment color did not load"); dialog.findChild<QLineEdit *>("propagationColorValue")->setText("Orange"); dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); }); apply(child,panel,before);
      before=editor->toPlainText(); child=open("Third"); panel=dynamic_cast<CommandEditor *>(child->widget()); colorDialog(*panel,[](PropagationColorDialog &dialog) { dialog.findChild<QCheckBox *>("propagationColorOverride")->setChecked(false); require(!dialog.findChild<QPushButton *>("propagationColorChoose")->isEnabled(),"Default color leaves picker enabled"); dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); }); apply(child,panel,before);
      require(editor->toPlainText().startsWith(prefix) && editor->toPlainText().endsWith(suffix) && editor->toPlainText().contains("% first segment") && editor->toPlainText().contains("StopTolerance = 1e-8"),"Color edits changed configuration/report/comments/tolerance");
      require(window.saveScriptTo(saved) && window.loadScript(saved),"Color Unicode save/reopen failed"); run(window); require(read(report)==expected,"Segment color edit changed six-state calculation report");
      const QColor blue(40,140,240),orange=QColor::fromRgb(RgbColor::ToIntColor("Orange")&0xffffff);
      for (const auto &name:QStringList{"Orb","Ground"}) {
         const auto &points=satellite(*window.plotReceiver()->model(name)).points; const auto &reference=paths.value(name); require(points.size()==reference.size() && points.size()>15,"Colored trajectory sample count changed");
         int blueCount=0,orangeCount=0,redCount=0; for (size_t i=0;i<points.size();++i) { require(points[i].color==reference[i].color && std::abs(points[i].x-reference[i].x)<1e-8 && std::abs(points[i].y-reference[i].y)<1e-8 && std::abs(points[i].z-reference[i].z)<1e-8,"GUI segment colors/history differ from independent script"); blueCount+=points[i].color==blue; orangeCount+=points[i].color==orange; redCount+=points[i].color==Qt::red; }
         if (blueCount<=3 || orangeCount<=3 || redCount<=3) throw std::runtime_error((name+" color samples: blue="+QString::number(blueCount)+", orange="+QString::number(orangeCount)+", red="+QString::number(redCount)).toStdString());
      }
      // Check display metadata alone: a subset reset leaves the other object's
      // override intact, and solver samples keep their target-color palette.
      auto *receiver=window.plotReceiver(); const auto count=satellite(*receiver->model("Ground")).points.size();
      const auto metadata=("SegmentColor=On|"+QString::number(blue.rgb()&0xffffff)+"|Sat|Second").toStdString();
      require(receiver->TakeGroundTrackAction("Ground",metadata) && receiver->TakeGroundTrackAction("Ground","SegmentColor=Off|0|Second") && receiver->TakeGroundTrackAction("Ground","SolverData=Off"),"Subset segment metadata rejected");
      require(satellite(*receiver->model("Ground")).color==blue,"Subset reset cleared another spacecraft's override");
      require(receiver->TakeGroundTrackAction("Ground","SolverData=On") && satellite(*receiver->model("Ground")).color!=blue,"Solver trial color inherited accepted segment override");
      require(receiver->TakeGroundTrackAction("Ground","SolverData=Off") && satellite(*receiver->model("Ground")).color==blue,"Accepted color did not restore after solver samples");
      require(!receiver->TakeGroundTrackAction("Ground","SegmentColor=On|invalid|Sat") && satellite(*receiver->model("Ground")).color==blue,"Invalid color metadata changed the palette");
      require(receiver->TakeGroundTrackAction("Ground","SegmentColor=Off|0|Sat") && receiver->TakeGroundTrackAction("Ground","SolverData=Off") && satellite(*receiver->model("Ground")).color==Qt::red && satellite(*receiver->model("Ground")).points.size()==count,"Override reset changed history or lost the default color");
      std::cout<<"PASS: actual MDI segment-color override, named/RGB picker and Cancel/invalid correction, pending and applied exact Undo/Redo, retained refreshed panels, unrelated source/tolerance/comments, Unicode save/reopen, unchanged independent six-state report and Orbit/Ground per-sample colored trajectories; mapped source including one-step/options/flags/comment cases.\n";
   } catch (BaseException &failure) { std::cerr<<"FAIL: "<<failure.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &failure) { std::cerr<<"FAIL: "<<failure.what()<<'\n'; return 1; }
}
