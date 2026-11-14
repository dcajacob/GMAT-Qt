#include "MainWindow.hpp"
#include "AcceptancePlatform.hpp"
#include "CelestialBodyPanel.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include "QtPlotReceiver.hpp"
#include "OrbitRenderer.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QAction>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QElapsedTimer>
#include <QThread>
#include <iostream>
#include <cmath>
#include <limits>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString bytes(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Body report missing"); return QString::fromUtf8(file.readAll()); }
static CelestialBody *body(const QString &name) { return dynamic_cast<CelestialBody *>(Moderator::Instance()->GetConfiguredObject(name.toStdString())); }
static QMap<QString,QString> state(const QString &name) { QMap<QString,QString> values; for (const auto &field:celestialBodyFields(*body(name))) values.insert(field.name,field.value); return values; }
static void events(int milliseconds=250) { QElapsedTimer elapsed; elapsed.start(); while (elapsed.elapsed()<milliseconds) { QApplication::processEvents(); QThread::msleep(5); } }
static void capture(QWidget &widget,const QString &path) { if (!path.isEmpty()) { events(); require(widget.grab().save(path),"Body panel capture failed"); } }
template<class Open,class Inspect> static void modal(QWidget &owner,Open open,Inspect inspect)
{
   bool inspected=false; std::exception_ptr failure; QTimer timer; timer.setSingleShot(true);
   QObject::connect(&timer,&QTimer::timeout,&owner,[&] { auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); try { require(dialog,"Expected modal missing"); inspected=true; inspect(dialog); } catch (...) { failure=std::current_exception(); } if (dialog && dialog->isVisible()) dialog->reject(); });
   timer.start(0); open(); timer.stop(); if (failure) std::rethrow_exception(failure); require(inspected,"Modal did not open");
}
static CelestialBodyPanel *open(MainWindow &window,const QString &name)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Body resource entry missing"); tree->itemDoubleClicked(items.first(),0); QApplication::processEvents();
   auto *workspace=window.findChild<QMdiArea *>("workspace"); require(workspace->activeSubWindow(),"Body MDI window missing"); auto *panel=dynamic_cast<CelestialBodyPanel *>(workspace->activeSubWindow()->widget()); require(panel,"Body editor missing"); return panel;
}
static void close(MainWindow &window) { for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *panel=dynamic_cast<EditablePanel *>(child->widget())) { panel->discardChanges(); child->close(); } QApplication::processEvents(); }
static QLineEdit *edit(CelestialBodyPanel &panel,const QString &field) { auto *value=panel.findChild<QLineEdit *>("body_"+field); require(value,"Body value control missing"); return value; }
static ResourceEditor *resource(MainWindow &window,const QString &name)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Resource entry missing"); tree->itemDoubleClicked(items.first(),0); QApplication::processEvents();
   auto *panel=dynamic_cast<ResourceEditor *>(window.findChild<QMdiArea *>("workspace")->activeSubWindow()->widget()); require(panel,"Resource editor missing"); return panel;
}
static QTableWidgetItem *property(ResourceEditor &panel,const QString &name)
{
   auto *table=panel.findChild<QTableWidget *>(); require(table,"Resource property table missing"); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) return table->item(row,1); throw std::runtime_error("Property row missing");
}
static void section(ResourceEditor &panel,const QString &name)
{
   auto *tabs=panel.findChild<QTabBar *>("propertySections"); require(tabs,"Resource sections missing"); for (int i=0;i<tabs->count();++i) if (tabs->tabText(i)==name) { tabs->setCurrentIndex(i); return; } throw std::runtime_error("Resource section missing");
}
static void applyResource(ResourceEditor &panel)
{
   panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
   QStringList messages; for (auto *label:panel.findChildren<QLabel *>()) messages.append(label->text()); require(!panel.hasChanges(),qPrintable(messages.join('\n')));
}
static void apply(CelestialBodyPanel &panel) { panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(!panel.hasChanges(),qPrintable(panel.findChild<QLabel *>("bodyStatus")->text())); QApplication::processEvents(); }
static const PlotCurve &curve(MainWindow &window,const QString &name) { const auto model=window.plotReceiver()->model("Orb"); require(model!=nullptr,"Body plot data missing"); for (const auto &entry:model->curves) if (entry.name==name) return entry; throw std::runtime_error("Body plot curve missing"); }
static QImage render(MainWindow &window,const QString &path)
{
   if (QApplication::platformName()=="offscreen") return {};
   auto model=std::make_shared<PlotModel>(*window.plotReceiver()->model("Orb")); OrbitRenderer renderer(model); renderer.resize(600,600); renderer.show(); renderer.setView(1,0,0,{},std::numeric_limits<quint64>::max()); events(500); auto image=renderer.captureImage(); require(!image.isNull(),"Native body rendering unavailable"); if (!path.isEmpty()) require(image.save(path),"Native body render capture failed"); renderer.close(); events(50); return image;
}
static int changedPixels(const QImage &first,const QImage &second)
{
   require(first.size()==second.size(),"Native rendering changed image dimensions"); int count=0;
   for (int y=0;y<first.height();++y) for (int x=0;x<first.width();++x) if (first.pixel(x,y)!=second.pixel(x,y)) ++count;
   return count;
}
#include "AcceptanceView.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QPointer>
#include <QTextDocument>

// Included after CelestialBodyTests' existing helpers. These checks extend that
// scenario; its independent scripts, propagation and image assertions still run.
static void bodyProof(const QString &name,const QString &operation)
{
   const QJsonObject record{{"checkpoint","body."+name+"."+operation},
      {"body",name},{"operation",operation},{"outcome","passed"},
      {"facets",QJsonArray{"named-gui-controls","model-readback","source-preservation"}}};
   std::cout<<"NASA_CHECK "<<QJsonDocument(record).toJson(QJsonDocument::Compact).constData()<<std::endl;
}

static void typeBodyValue(QLineEdit &control,const QString &text)
{
   control.setFocus(); control.selectAll();
   if (text.isEmpty()) {
      QKeyEvent press(QEvent::KeyPress,Qt::Key_Backspace,Qt::NoModifier);
      QApplication::sendEvent(&control,&press);
   } else for (const auto ch:text) {
      QKeyEvent press(QEvent::KeyPress,0,Qt::NoModifier,QString(ch));
      QKeyEvent release(QEvent::KeyRelease,0,Qt::NoModifier,QString(ch));
      QApplication::sendEvent(&control,&press); QApplication::sendEvent(&control,&release);
   }
   require(control.text()==text,"Body input did not receive the authored key events");
}

static void nasaBodyPhysicalChecks(MainWindow &window,QPlainTextEdit &source,const QString &directory)
{
   const QString fixture="% NASA body physical controls - retain this comment\n"
      "Create Variable Keep;\nKeep = 17;\nBeginMissionSequence;\nKeep = Keep + 4; % retain this mission\n";
   const QStringList defaults{"Sun","Mercury","Venus","Earth","Luna","Mars",
                              "Jupiter","Saturn","Uranus","Neptune","Pluto"};
   QStringList names=defaults;
   for (const auto &type:QStringList{"Moon","Asteroid","Comet","Planet"}) names.append("Acceptance"+type);
   for (const auto &name:names) {
      close(window); source.setPlainText(fixture);
      require(window.buildScript(),"Body physical baseline did not build");
      const bool builtin=defaults.contains(name);
      if (!builtin) {
         const auto type=name.mid(QString("Acceptance").size());
         modal(window,[&] { window.findChild<QAction *>("createResource")->trigger(); },[&](QDialog *dialog) {
            auto *types=dialog->findChild<QComboBox *>("resourceType");
            require(types && types->findText(type)>=0,"User celestial-body type missing from creation dialog");
            types->setCurrentText(type); dialog->findChild<QLineEdit *>("resourceName")->setText(name);
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
         });
         require(body(name) && body(name)->IsUserDefined() && body(name)->IsOfType(type.toStdString()),"GUI did not create the selected body type");
         close(window);
         // A new user body is intentionally incomplete (Mu/radius default to
         // zero). Establish a valid applied baseline before testing rejected
         // edits; typing the identical initial zero is a clean no-op.
         auto *created=open(window,name); acceptanceView(*created);
         typeBodyValue(*edit(*created,"Mu"),"60");
         typeBodyValue(*edit(*created,"EquatorialRadius"),"300");
         typeBodyValue(*edit(*created,"Flattening"),"0.01");
         apply(*created); close(window);
         require(state(name).value("CentralBody")== (type=="Moon" ? "Earth" : "Sun"),"New body's central body differs from its documented type default");
      }
      require(body(name) && body(name)->IsUserDefined()!=builtin,"Default/user body identity differs from requested object");
      const auto original=source.toPlainText(); const auto originalState=state(name);
      auto *panel=open(window,name); acceptanceView(*panel);
      auto *pages=panel->findChild<QTabWidget *>("bodyPages");
      require(pages && pages->count()==4,"Body pages missing"); pages->setCurrentIndex(0);
      require(edit(*panel,"Mu")->isVisible() && edit(*panel,"EquatorialRadius")->isVisible() && edit(*panel,"Flattening")->isVisible(),"Physical inputs are not visible on Properties");
      const auto initialUndo=source.document()->availableUndoSteps();
      QPointer<QMdiSubWindow> owner=qobject_cast<QMdiSubWindow *>(panel->parentWidget());
      require(owner,"Body panel has no owning workspace window");
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(!owner && source.toPlainText()==original && source.document()->availableUndoSteps()==initialUndo && state(name)==originalState,"Clean body OK changed source, model or history");

      panel=open(window,name); acceptanceView(*panel); pages=panel->findChild<QTabWidget *>("bodyPages"); pages->setCurrentIndex(0);
      for (const auto &field:QStringList{"Mu","EquatorialRadius","Flattening"}) {
         auto *input=edit(*panel,field); const auto initial=input->text();
         QStringList invalid{"","not-a-number","NaN","Inf","-Inf","1e999","-0.001"};
         if (field!="Flattening") invalid.append("0");
         for (const auto &value:invalid) {
            typeBodyValue(*input,value); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
            require(panel->hasChanges() && input->text()==value && !panel->findChild<QLabel *>("bodyStatus")->text().isEmpty(),qPrintable("Invalid body input "+name+"."+field+"="+value+" (initial="+initial+", actual="+input->text()+", pending="+QString::number(panel->hasChanges())+", diagnostic="+panel->findChild<QLabel *>("bodyStatus")->text()+")"));
            require(source.toPlainText()==original && source.document()->availableUndoSteps()==initialUndo && state(name)==originalState,"Invalid body input mutated model, source or history");
            typeBodyValue(*input,initial);
         }
      }
      require(!panel->hasChanges(),"Restoring rejected body fields left pending changes");
      typeBodyValue(*edit(*panel,"Mu"),"12345.25");
      owner=qobject_cast<QMdiSubWindow *>(panel->parentWidget());
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel)->click();
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(!owner && source.toPlainText()==original && source.document()->availableUndoSteps()==initialUndo && state(name)==originalState,"Body Cancel committed pending physical values");

      // The strict positive boundaries for Mu/radius and inclusive zero for
      // flattening come from Resource_CelestialBody.xml, not current readback.
      for (const auto &values:QList<QStringList>{{"1e-12","1e-12","0"},{"12345.25","987.5","0.125"}}) {
         const auto before=source.toPlainText(); const auto beforeState=state(name);
         panel=open(window,name); acceptanceView(*panel); pages=panel->findChild<QTabWidget *>("bodyPages"); pages->setCurrentIndex(0);
         const QStringList fields{"Mu","EquatorialRadius","Flattening"};
         for (int i=0;i<fields.size();++i) typeBodyValue(*edit(*panel,fields[i]),values[i]);
         require(source.toPlainText()==before && state(name)==beforeState && panel->hasChanges(),"Pending body edit escaped to source or model");
         apply(*panel); const auto after=source.toPlainText();
         for (int i=0;i<fields.size();++i) require(state(name).value(fields[i]).toDouble()==values[i].toDouble(),"Applied body property differs from independently specified value");
         require(after!=before && after.contains("% NASA body physical controls - retain this comment") && after.endsWith("Keep = Keep + 4; % retain this mission\n"),"Body Apply changed unrelated source");
         const auto steps=source.document()->availableUndoSteps(); close(window); panel=open(window,name); apply(*panel);
         require(source.toPlainText()==after && source.document()->availableUndoSteps()==steps,"Repeated body Apply changed source or history");
         close(window); source.undo(); require(source.toPlainText()==before && window.buildScript() && state(name)==beforeState,"Body Undo did not restore exact source and properties");
         source.redo(); require(source.toPlainText()==after && window.buildScript(),"Body Redo did not restore exact source");
         for (int i=0;i<fields.size();++i) require(state(name).value(fields[i]).toDouble()==values[i].toDouble(),"Body Redo did not restore expected properties");
      }
      const auto applied=source.toPlainText(); const auto appliedState=state(name);
      const auto saved=directory+"/physical-"+name+".script";
      require(window.saveScriptTo(saved) && window.loadScript(saved) && source.toPlainText()==applied && state(name)==appliedState,"Body save/reopen did not preserve source and properties");
      panel=open(window,name); acceptanceView(*panel); pages=panel->findChild<QTabWidget *>("bodyPages"); pages->setCurrentIndex(0);
      typeBodyValue(*edit(*panel,"Mu"),"12346.25"); owner=qobject_cast<QMdiSubWindow *>(panel->parentWidget());
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(!owner && state(name).value("Mu").toDouble()==12346.25 && source.toPlainText()!=applied,"Changed body OK failed to commit and close its owner");
      source.undo(); require(source.toPlainText()==applied && window.buildScript() && state(name)==appliedState,"Body OK Undo failed exact restoration");
      bodyProof(name,"physical");
      if (builtin) bodyProof(name,"default"); else bodyProof(name,"create");
   }
   close(window); source.setPlainText(fixture); require(window.buildScript(),"Body physical checks did not restore a clean fixture");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtBodies");
   try {
      require(acceptanceWidgetPlatform(),"Body acceptance requires offscreen or supported visible widgets");
      TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Body fixtures unavailable"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(),captures=argc>2 ? QString::fromLocal8Bit(argv[2]) : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Body runtime initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto report=files.filePath("bodies.txt"),saved=files.filePath("bodies ü.script");
      const QString resources="% body GUI round trip Δ\nCreate Spacecraft Sat;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.FM = FM;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nCreate OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.ViewPointReference = Earth;\nOrb.ViewPointVector = [0 0 25000];\nOrb.ViewDirection = Earth;\nOrb.EnableStars = Off;\nOrb.EnableConstellations = Off;\nOrb.Axes = Off;\nOrb.Grid = Off;\n";
      const QString mission="BeginMissionSequence; % preserve all commands\nPropagate 'Body run' Prop(Sat) {Sat.ElapsedSecs = 120}; % keep propagation\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.Earth.Altitude Sat.EarthFixed.X Sat.EarthFixed.Y Sat.EarthFixed.Z;\n";
      const auto original=resources+mission; editor->setPlainText(original); require(window.saveScriptTo(saved) && window.runMission()==MainWindow::RunResult::Completed,"Body mission failed"); const auto baselineReport=bytes(report); auto *panel=open(window,"Earth"); require(!panel->hasChanges(),"New body editor already dirty"); const auto originalState=state("Earth"); auto *tabs=panel->findChild<QTabWidget *>("bodyPages"); require(tabs && tabs->count()==4 && tabs->tabText(0)=="Properties" && tabs->tabText(1)=="Orbit" && tabs->tabText(2)=="Orientation" && tabs->tabText(3)=="Visualization","wx body pages missing");
      require(panel->findChild<QComboBox *>("body_PosVelSource") && !panel->findChild<QComboBox *>("body_PosVelSource")->isEnabled() && edit(*panel,"CentralBody")->isReadOnly() && edit(*panel,"SpinAxisRAConstant")->isReadOnly(),"Built-in source/central body/pole constants not protected");
      edit(*panel,"Mu")->setText("400000"); edit(*panel,"EquatorialRadius")->setText("6400"); edit(*panel,"Flattening")->setText("0.01"); tabs->setCurrentIndex(2); edit(*panel,"NutationUpdateInterval")->setText("10");
      require(panel->hasChanges() && state("Earth")==originalState && editor->toPlainText()==original,"Pending body settings changed engine/source"); capture(*panel->parentWidget(),captures.isEmpty() ? QString() : captures+".orientation.png"); tabs->setCurrentIndex(0); capture(*panel->parentWidget(),captures.isEmpty() ? QString() : captures+".properties.png");
      modal(*panel,[&] { panel->findChild<QPushButton *>("showScript")->click(); },[](QDialog *dialog) { require(!dialog->findChild<QPlainTextEdit *>("inspectionText")->toPlainText().contains("6400"),"Applied body preview contains pending radius"); }); apply(*panel); const auto configured=editor->toPlainText(); require(configured.endsWith(mission) && !configured.contains("Create Planet Earth"),"Built-in body edit created/replaced body or changed mission"); editor->undo(); require(editor->toPlainText()==original,"Body Undo not exact/atomic"); editor->redo(); require(editor->toPlainText()==configured,"Body Redo not exact"); require(window.runMission()==MainWindow::RunResult::Completed,"GUI physical body mission failed"); const auto physicalReport=bytes(report); require(physicalReport!=baselineReport && curve(window,"Earth").radius==6400,"Physical properties did not affect report/viewer radius");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==physicalReport,"Body Unicode save/reopen changed calculations");
      const QString explicitPhysical="Earth.Mu = 400000;\nEarth.EquatorialRadius = 6400;\nEarth.Flattening = 0.01;\nEarth.NutationUpdateInterval = 10;\n"; editor->setPlainText(explicitPhysical+original); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==physicalReport,"GUI/raw-script physical body calculations differ");
      // Native orbit scene changes from a texture, then a body model/pose.
      const auto texture=files.filePath("texture ü.png"),modelFile=files.filePath("shape ü.obj"); QImage map(64,32,QImage::Format_RGB32); for (int y=0;y<map.height();++y) for (int x=0;x<map.width();++x) map.setPixelColor(x,y,(x/8+y/8)%2 ? QColor(245,25,35) : QColor(20,220,245)); require(map.save(texture),"Texture fixture failed");
      { QFile file(modelFile); require(file.open(QIODevice::WriteOnly),"OBJ fixture failed"); file.write("v -1000 -700 -300\nv 1000 -700 -300\nv 0 1500 -300\nv 0 0 1900\nf 1 3 2\nf 1 2 4\nf 2 3 4\nf 3 1 4\n"); }
      const auto baseImage=render(window,{}); panel=open(window,"Earth"); tabs=panel->findChild<QTabWidget *>("bodyPages"); tabs->setCurrentIndex(3); const auto beforeAppearance=editor->toPlainText();
      modal(*panel,[&] { panel->findChild<QPushButton *>("bodyChoose_TextureMapFileName")->click(); },[](QDialog *dialog) { require(qobject_cast<QFileDialog *>(dialog),"Texture chooser missing"); }); require(!panel->hasChanges(),"Texture picker Cancel changed field");
      modal(*panel,[&] { panel->findChild<QPushButton *>("bodyChoose_TextureMapFileName")->click(); },[&](QDialog *dialog) { auto *chooser=qobject_cast<QFileDialog *>(dialog); chooser->selectFile(texture); static_cast<QDialog *>(chooser)->accept(); }); require(edit(*panel,"TextureMapFileName")->text()==texture && !panel->findChild<QLabel *>("bodyTexturePreview")->pixmap().isNull(),"Texture selection/preview failed");
      require(editor->toPlainText()==beforeAppearance && state("Earth").value("TextureMapFileName")!=texture,"Pending texture changed model/script"); capture(*panel->parentWidget(),captures.isEmpty() ? QString() : captures+".visualization.png"); apply(*panel); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==physicalReport && QFileInfo(curve(window,"Earth").texturePath).canonicalFilePath()==QFileInfo(texture).canonicalFilePath(),"Texture edit changed calculations or did not reach viewer"); const auto textureImage=render(window,captures.isEmpty() ? QString() : captures+".texture.png"); if (!baseImage.isNull()) require(changedPixels(baseImage,textureImage)>2000,"Texture did not change the rendered body surface");
      panel=open(window,"Earth"); tabs=panel->findChild<QTabWidget *>("bodyPages"); tabs->setCurrentIndex(3); modal(*panel,[&] { panel->findChild<QPushButton *>("bodyChoose_3DModelFile")->click(); },[&](QDialog *dialog) { auto *chooser=qobject_cast<QFileDialog *>(dialog); chooser->selectFile(modelFile); static_cast<QDialog *>(chooser)->accept(); }); edit(*panel,"3DModelScale")->setText("3"); edit(*panel,"3DModelOffsetX")->setText("0.1"); edit(*panel,"3DModelRotationY")->setText("35");
      modal(*panel,[&] { panel->findChild<QPushButton *>("bodyChoose_OrbitColor")->click(); },[](QDialog *dialog) { require(qobject_cast<QColorDialog *>(dialog),"Body color dialog missing"); });
      modal(*panel,[&] { panel->findChild<QPushButton *>("bodyChoose_OrbitColor")->click(); },[](QDialog *dialog) { auto *color=qobject_cast<QColorDialog *>(dialog); color->setCurrentColor(QColor(220,150,20)); color->accept(); }); apply(*panel); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==physicalReport,"Model/color changes affected calculation"); const auto &displayed=curve(window,"Earth"); require(displayed.modelPath==modelFile && displayed.modelScale==3 && displayed.modelOffset[0]==0.1 && displayed.modelRotation[1]==35 && displayed.color==QColor(220,150,20),"Body model/pose/color did not reach orbit scene"); const auto modelImage=render(window,captures.isEmpty() ? QString() : captures+".model.png"); if (!textureImage.isNull()) require(changedPixels(modelImage,textureImage)>2000,"Body model/pose did not change native body rendering");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==physicalReport && curve(window,"Earth").modelPath==modelFile,"Model/texture save/reopen failed");
      auto commentedAppearance=editor->toPlainText(); commentedAppearance.replace("Earth.3DModelScale = 3;","Earth.3DModelScale = 3; % preserve model scale explanation"); editor->setPlainText(commentedAppearance); require(window.buildScript(),"Commented body appearance fixture failed");
      require(window.applyResourceChanges("Earth",{{"3DModelScale","4"}},commentedAppearance).isEmpty(),"Repeated model-scale edit failed"); const auto repeatedAppearance=editor->toPlainText(); require(repeatedAppearance.count("Earth.3DModelScale =")==1 && repeatedAppearance.contains("% preserve model scale explanation") && repeatedAppearance.endsWith(mission),"Digit-prefixed body property replacement lost comments/mission or duplicated assignments");
      require(window.applyResourceChanges("Earth",{{"3DModelScale","4"}},repeatedAppearance).isEmpty() && editor->toPlainText()==repeatedAppearance,"Repeated body Apply changed identical source"); require(window.applyResourceChanges("Earth",{{"3DModelScale","3"}},repeatedAppearance).isEmpty() && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==physicalReport,"Repeated appearance edits changed calculation");
      const auto valid=editor->toPlainText(); const auto validBody=state("Earth"); const auto malformed=files.filePath("bad image.txt"); { QFile file(malformed); require(file.open(QIODevice::WriteOnly),"Bad file fixture failed"); file.write("not an image or model"); }
      for (const auto &bad:QList<QMap<QString,QString>>{{{"Mu","0"}},{{"EquatorialRadius","-1"}},{{"Flattening","-0.1"}},{{"NutationUpdateInterval","nan"}},{{"NutationUpdateInterval","-1"}},{{"SpinAxisRAConstant","10"}},{{"PosVelSource","SPICE"}},{{"CentralBody","Mars"}},{{"RotationDataSource","SPICE"}},{{"3DModelScale","0"}},{{"3DModelScale","1001"}},{{"3DModelOffsetY","3.6"}},{{"3DModelRotationZ","181"}},{{"3DModelFile",malformed}},{{"TextureMapFileName",malformed}},{{"TextureMapFileName",files.filePath("missing.png")}},{{"NAIFId","1.5"}}}) require(!window.applyResourceChanges("Earth",bad,valid).isEmpty() && editor->toPlainText()==valid && state("Earth")==validBody,"Invalid body field changed model/script");
      panel=open(window,"Earth"); edit(*panel,"Mu")->setText("nan"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(panel->hasChanges() && !panel->findChild<QLabel *>("bodyStatus")->text().isEmpty(),"Invalid GUI value discarded pending edit"); edit(*panel,"Mu")->setText("400001"); apply(*panel); require(body("Earth")->GetRealParameter(body("Earth")->GetParameterID("Mu"))==400001,"Body invalid-edit correction failed");
      require(window.applyResourceChanges("Earth",{{"Mu","400000"},{"3DModelFile",""},{"TextureMapFileName",""}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==physicalReport && curve(window,"Earth").modelPath.isEmpty() && QFileInfo(curve(window,"Earth").texturePath).canonicalFilePath()!=QFileInfo(texture).canonicalFilePath(),"Clear model/default texture failed or changed calculation"); close(window);
      // Create and configure a user-defined body with the actual New resource
      // dialog. Ceres data, poles and frame reports exercise its orbit/rotation.
      editor->setPlainText(original); require(window.buildScript(),"User body baseline failed"); modal(window,[&] { window.findChild<QAction *>("createResource")->trigger(); },[](QDialog *dialog) { auto *type=dialog->findChild<QComboBox *>("resourceType"); require(type->findText("Asteroid")>=0,"Asteroid missing from New resource"); type->setCurrentText("Asteroid"); dialog->findChild<QLineEdit *>("resourceName")->setText("Ceres"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); }); require(body("Ceres") && body("Ceres")->IsUserDefined(),"GUI user body creation failed"); panel=open(window,"Ceres"); require(!panel->hasChanges() && !edit(*panel,"SpinAxisRAConstant")->isReadOnly() && !edit(*panel,"NAIFId")->isReadOnly(),"User body source/pole/ID controls wrong"); tabs=panel->findChild<QTabWidget *>("bodyPages"); tabs->setCurrentIndex(1);
      const auto ceresKernel=files.filePath("ceres ü.bsp"); require(QFile::copy(QFileInfo(startup).absolutePath()+"/../data/planetary_ephem/spk/ceres_1900_2100.bsp",ceresKernel),"Ceres SPK unavailable"); auto *kernels=panel->findChild<QListWidget *>("body_OrbitSpiceKernelName"); require(kernels,"Ceres SPK editor missing"); modal(*panel,[&] { panel->findChild<QPushButton *>("bodyAdd_OrbitSpiceKernelName")->click(); },[&](QDialog *dialog) { auto *picker=qobject_cast<QFileDialog *>(dialog); picker->selectFile(ceresKernel); static_cast<QDialog *>(picker)->accept(); }); require(kernels->count()==1 && kernels->item(0)->text()==ceresKernel,"Ceres SPK selection failed"); edit(*panel,"NAIFId")->setText("2000001"); edit(*panel,"Mu")->setText("63"); edit(*panel,"EquatorialRadius")->setText("470"); edit(*panel,"Flattening")->setText("0.03"); edit(*panel,"SpinAxisRAConstant")->setText("30"); edit(*panel,"SpinAxisDECConstant")->setText("70"); edit(*panel,"RotationConstant")->setText("20"); edit(*panel,"RotationRate")->setText("40"); capture(*panel->parentWidget(),captures.isEmpty() ? QString() : captures+".orbit.png"); apply(*panel);
      const QString frames="Create CoordinateSystem CeresFrame;\nCeresFrame.Origin = Ceres;\nCeresFrame.Axes = MJ2000Eq;\nCreate CoordinateSystem CeresFixed;\nCeresFixed.Origin = Ceres;\nCeresFixed.Axes = BodyFixed;\n";
      auto userMission=editor->toPlainText(); userMission.insert(userMission.indexOf("BeginMissionSequence"),frames); userMission.replace("Sat.EarthFixed.Z;","Sat.EarthFixed.Z Sat.CeresFrame.X Sat.CeresFrame.Y Sat.CeresFrame.Z Sat.CeresFixed.X Sat.CeresFixed.Y Sat.CeresFixed.Z;"); editor->setPlainText(userMission); require(window.runMission()==MainWindow::RunResult::Completed,"GUI Ceres ephemeris/orientation mission failed"); const auto userReport=bytes(report); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==userReport,"Ceres Unicode save/reopen changed reports");
      const QString explicitCeres="Create Asteroid Ceres;\nCeres.NAIFId = 2000001;\nCeres.OrbitSpiceKernelName = {'"+ceresKernel+"'};\nCeres.Mu = 63;\nCeres.EquatorialRadius = 470;\nCeres.Flattening = 0.03;\nCeres.SpinAxisRAConstant = 30;\nCeres.SpinAxisDECConstant = 70;\nCeres.RotationConstant = 20;\nCeres.RotationRate = 40;\n"; auto reference=explicitCeres+frames+original; reference.replace("Sat.EarthFixed.Z;","Sat.EarthFixed.Z Sat.CeresFrame.X Sat.CeresFrame.Y Sat.CeresFrame.Z Sat.CeresFixed.X Sat.CeresFixed.Y Sat.CeresFixed.Z;"); editor->setPlainText(reference); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==userReport,"GUI/raw-script Ceres orbit/orientation reports differ");
      panel=open(window,"Ceres"); edit(*panel,"SpinAxisRAConstant")->setText("45"); apply(*panel); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)!=userReport,"Pole edit did not affect body-fixed frame report"); const auto rotated=bytes(report); reference.replace("Ceres.SpinAxisRAConstant = 30;","Ceres.SpinAxisRAConstant = 45;"); editor->setPlainText(reference); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==rotated,"GUI/raw-script pole reports differ");
      const auto beforeInvalid=editor->toPlainText(); require(!window.applyResourceChanges("Ceres",{{"PosVelSource","DE405"}},beforeInvalid).isEmpty(),"Unsupported user body DE source accepted"); require(window.applyResourceChanges("Ceres",{{"NAIFId","-1234567"}},beforeInvalid).isEmpty(),"Unknown-ID fixture not accepted for runtime validation"); require(window.runMission()==MainWindow::RunResult::Failed,"Unknown SPICE body ID did not fail at execution"); require(window.applyResourceChanges("Ceres",{{"NAIFId","2000001"}},editor->toPlainText()).isEmpty() && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==rotated,"SPICE body failure did not recover after ID correction");
      // PCK and FK lists use complete replacement, not append-only setters.
      // Exercise order, replacement/removal, kernel-type errors and an imported
      // SPICE rotation source through a body-fixed coordinate report.
      const auto pck=files.filePath("planetary ü.tpc"),pck2=files.filePath("planetary second.tpc"),fk=files.filePath("frame ü.tf"),fk2=files.filePath("frame second.tf");
      require(QFile::copy(QString::fromStdString(FileManager::Instance()->GetFullPathname("PLANETARY_PCK_FILE")),pck) && QFile::copy(pck,pck2),"PCK fixtures unavailable");
      auto writeFrame=[&](const QString &path,int angle) { QFile file(path); require(file.open(QIODevice::WriteOnly),"FK fixture unavailable"); const auto text=QString("KPL/FK\n\\begindata\nFRAME_CERES_TEST = 140001\nFRAME_140001_NAME = 'CERES_TEST'\nFRAME_140001_CLASS = 4\nFRAME_140001_CLASS_ID = 140001\nFRAME_140001_CENTER = 2000001\nTKFRAME_140001_RELATIVE = 'J2000'\nTKFRAME_140001_SPEC = 'ANGLES'\nTKFRAME_140001_UNITS = 'DEGREES'\nTKFRAME_140001_AXES = (3, 2, 1)\nTKFRAME_140001_ANGLES = (%1, 0, 0)\n\\begintext\n").arg(angle); file.write(text.toUtf8()); }; writeFrame(fk,15); writeFrame(fk2,45);
      auto addFile=[&](CelestialBodyPanel &panel,const QString &field,const QString &path) { modal(panel,[&] { panel.findChild<QPushButton *>("bodyAdd_"+field)->click(); },[&](QDialog *dialog) { auto *picker=qobject_cast<QFileDialog *>(dialog); require(picker,"Kernel chooser missing"); picker->selectFile(path); static_cast<QDialog *>(picker)->accept(); }); };
      panel=open(window,"Ceres"); tabs=panel->findChild<QTabWidget *>("bodyPages"); tabs->setCurrentIndex(0); auto *pcks=panel->findChild<QListWidget *>("body_PlanetarySpiceKernelName"); require(pcks && pcks->count()==0,"Unexpected Ceres PCK defaults"); modal(*panel,[&] { panel->findChild<QPushButton *>("bodyAdd_PlanetarySpiceKernelName")->click(); },[](QDialog *) {}); require(!panel->hasChanges(),"Kernel Add Cancel changed panel"); addFile(*panel,"PlanetarySpiceKernelName",pck); addFile(*panel,"PlanetarySpiceKernelName",pck2); pcks->setCurrentRow(1); panel->findChild<QPushButton *>("bodyUp_PlanetarySpiceKernelName")->click(); require(pcks->item(0)->text()==pck2,"PCK Up failed"); panel->findChild<QPushButton *>("bodyDown_PlanetarySpiceKernelName")->click(); require(pcks->item(1)->text()==pck2,"PCK Down failed"); panel->findChild<QPushButton *>("bodyRemove_PlanetarySpiceKernelName")->click(); require(pcks->count()==1 && pcks->item(0)->text()==pck,"PCK Remove failed");
      modal(*panel,[&] { panel->findChild<QPushButton *>("bodyReplace_PlanetarySpiceKernelName")->click(); },[&](QDialog *dialog) { auto *picker=qobject_cast<QFileDialog *>(dialog); picker->selectFile(pck2); static_cast<QDialog *>(picker)->accept(); }); require(pcks->item(0)->text()==pck2,"PCK Replace failed");
      tabs->setCurrentIndex(2); addFile(*panel,"FrameSpiceKernelName",fk); addFile(*panel,"FrameSpiceKernelName",fk2); edit(*panel,"SpiceFrameId")->setText("CERES_TEST"); const auto pendingKernelSource=editor->toPlainText(); const auto pendingKernelMission=pendingKernelSource.mid(pendingKernelSource.indexOf("BeginMissionSequence")); require(state("Ceres").value("FrameSpiceKernelName").isEmpty(),"Pending FK list changed body"); apply(*panel); require(state("Ceres").value("PlanetarySpiceKernelName")==pck2 && state("Ceres").value("FrameSpiceKernelName")==fk+"\n"+fk2 && editor->toPlainText().endsWith(pendingKernelMission),"Complete PCK/FK membership not applied"); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==rotated,"Kernel metadata edit changed simplified-pole calculations");
      const auto validKernels=editor->toPlainText(),kernelReport=bytes(report); const auto kernelState=state("Ceres");
      for (const auto &bad:QList<QMap<QString,QString>>{{{"PlanetarySpiceKernelName",ceresKernel}},{{"FrameSpiceKernelName",pck2}},{{"OrbitSpiceKernelName",fk}},{{"FrameSpiceKernelName",files.filePath("missing.tf")}},{{"PlanetarySpiceKernelName",pck2+"\n"+pck2}}}) require(!window.applyResourceChanges("Ceres",bad,validKernels).isEmpty() && editor->toPlainText()==validKernels && state("Ceres")==kernelState,"Invalid kernel list changed source/body");
      auto imported=editor->toPlainText(); imported.insert(imported.indexOf("BeginMissionSequence"),"Ceres.RotationDataSource = SPICE;\n"); editor->setPlainText(imported); require(window.runMission()==MainWindow::RunResult::Completed,"Imported SPICE rotation with GUI FK/PCK settings failed"); const auto frameReport=bytes(report); require(frameReport!=kernelReport,"SPICE frame ID did not affect body-fixed report");
      const QString kernelSettings="Ceres.PlanetarySpiceKernelName = {'"+pck2+"'};\nCeres.FrameSpiceKernelName = {'"+fk+"', '"+fk2+"'};\nCeres.SpiceFrameId = 'CERES_TEST';\nCeres.RotationDataSource = SPICE;\n"; auto kernelReference=reference; kernelReference.insert(kernelReference.indexOf("BeginMissionSequence"),kernelSettings); editor->setPlainText(kernelReference); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==frameReport,"GUI/raw-script kernel/frame reports differ");
      panel=open(window,"Ceres"); tabs=panel->findChild<QTabWidget *>("bodyPages"); tabs->setCurrentIndex(2); require(edit(*panel,"RotationDataSource")->text()=="SPICE" && edit(*panel,"RotationDataSource")->isReadOnly(),"Imported rotation source not displayed"); auto *fks=panel->findChild<QListWidget *>("body_FrameSpiceKernelName"); fks->setCurrentRow(1); panel->findChild<QPushButton *>("bodyUp_FrameSpiceKernelName")->click(); apply(*panel); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)!=frameReport,"FK order did not select the last loaded frame definition"); const auto orderedFrame=bytes(report); auto orderedReference=kernelReference; orderedReference.replace("{'"+fk+"', '"+fk2+"'}","{'"+fk2+"', '"+fk+"'}"); editor->setPlainText(orderedReference); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==orderedFrame,"GUI/raw-script reordered frame reports differ"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==orderedFrame,"Kernel order/rotation save/reopen failed");
      panel=open(window,"Ceres"); auto *spks=panel->findChild<QListWidget *>("body_OrbitSpiceKernelName"); spks->setCurrentRow(0); panel->findChild<QPushButton *>("bodyRemove_OrbitSpiceKernelName")->click(); apply(*panel); require(state("Ceres").value("OrbitSpiceKernelName").isEmpty() && window.runMission()==MainWindow::RunResult::Failed,"Cleared Ceres SPK did not remove membership/fail execution"); panel=open(window,"Ceres"); addFile(*panel,"OrbitSpiceKernelName",ceresKernel); apply(*panel); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==orderedFrame,"Missing-SPK correction did not recover reports");
      close(window); editor->setPlainText(original); require(window.buildScript(),"Default body kernel removal fixture failed"); panel=open(window,"Luna"); auto *defaultPcks=panel->findChild<QListWidget *>("body_PlanetarySpiceKernelName"); require(defaultPcks && defaultPcks->count()>0,"Luna default kernel list missing"); defaultPcks->selectAll(); panel->findChild<QPushButton *>("bodyRemove_PlanetarySpiceKernelName")->click(); addFile(*panel,"PlanetarySpiceKernelName",pck2); apply(*panel); require(state("Luna").value("PlanetarySpiceKernelName")==pck2,"Default PCK replacement appended/retained startup membership");
      panel=open(window,"Luna"); defaultPcks=panel->findChild<QListWidget *>("body_PlanetarySpiceKernelName"); defaultPcks->selectAll(); panel->findChild<QPushButton *>("bodyRemove_PlanetarySpiceKernelName")->click(); tabs=panel->findChild<QTabWidget *>("bodyPages"); tabs->setCurrentIndex(2); auto *defaultFrames=panel->findChild<QListWidget *>("body_FrameSpiceKernelName"); require(defaultFrames && defaultFrames->count()>0,"Luna default FK list missing"); defaultFrames->selectAll(); panel->findChild<QPushButton *>("bodyRemove_FrameSpiceKernelName")->click(); apply(*panel); require(state("Luna").value("PlanetarySpiceKernelName").isEmpty() && state("Luna").value("FrameSpiceKernelName").isEmpty(),"Default PCK/FK removal retained startup membership"); require(window.saveScriptTo(saved) && window.loadScript(saved) && state("Luna").value("PlanetarySpiceKernelName").isEmpty() && state("Luna").value("FrameSpiceKernelName").isEmpty(),"Default kernel clear did not survive save/reopen");
      editor->setPlainText("Luna.PlanetarySpiceKernelName = { };\nLuna.FrameSpiceKernelName = {};\n"+original); require(window.buildScript() && state("Luna").value("PlanetarySpiceKernelName").isEmpty() && state("Luna").value("FrameSpiceKernelName").isEmpty(),"Separately written empty kernel lists did not clear startup defaults");
      // The wx body-selection dialog serves solar shadows and force-model
      // point masses. Exercise both actual Qt callers and pending exclusions.
      close(window);
      const auto userBody="Create Asteroid Ceres;\nCeres.Mu = 63;\nCeres.EquatorialRadius = 470;\nCeres.NAIFId = 2000001;\nCeres.OrbitSpiceKernelName = {'"+ceresKernel+"'};\n";
      auto selectionSource=userBody+QString("Create SolarPowerSystem SolarPower;\n")+original;
      selectionSource.insert(selectionSource.indexOf("BeginMissionSequence"),"Sat.PowerSystem = SolarPower;\n");
      selectionSource.replace("Sat.EarthFixed.Z;","Sat.EarthFixed.Z Sat.SolarPower.TotalPowerAvailable;");
      editor->setPlainText(selectionSource); require(window.runMission()==MainWindow::RunResult::Completed,"Body selection fixture failed");
      auto *pickerPanel=resource(window,"SolarPower"); section(*pickerPanel,"Shadow"); const auto initialShadow=property(*pickerPanel,"ShadowBodies")->text();
      auto selectBodies=[&](ResourceEditor &panel,const QString &field,const QStringList &selected,const QStringList &hidden,bool accept,const QString &capturePath={}) {
         modal(panel,[&] { panel.findChild<QPushButton *>("chooseProperty_"+field)->click(); },[&](QDialog *dialog) {
            auto *list=dialog->findChild<QListWidget *>("resourceSelectionList"); require(list,"Body selection list missing"); QStringList choices;
            for (int i=0;i<list->count();++i) choices.append(list->item(i)->text());
            require(choices.contains("Luna") && choices.contains("Ceres") && !choices.contains("Sat") && !choices.contains("SolarPower") && !choices.contains("SolarSystemBarycenter"),"Body selection did not filter resource types/include user body");
            for (const auto &name:hidden) require(!choices.contains(name),"wx hidden body appeared in selection");
            dialog->findChild<QPushButton *>("resourceSelectAll")->click(); for (int i=0;i<list->count();++i) require(list->item(i)->checkState()==Qt::Checked,"Body Select all failed");
            dialog->findChild<QPushButton *>("resourceClearSelection")->click(); for (int i=0;i<list->count();++i) { require(list->item(i)->checkState()==Qt::Unchecked,"Body Clear failed"); if (selected.contains(list->item(i)->text())) list->item(i)->setCheckState(Qt::Checked); }
            for (const auto &name:selected) require(choices.contains(name),"Selectable body missing"); capture(*dialog,capturePath);
            if (accept) dialog->accept();
         });
      };
      selectBodies(*pickerPanel,"ShadowBodies",{"Luna","Ceres"},{"Sun"},false); require(property(*pickerPanel,"ShadowBodies")->text()==initialShadow && !pickerPanel->hasChanges() && editor->toPlainText()==selectionSource,"Body picker Cancel changed pending/model/source");
      selectBodies(*pickerPanel,"ShadowBodies",{"Luna","Ceres"},{"Sun"},true,captures.isEmpty() ? QString() : captures+".shadow-selection.png"); require(pickerPanel->hasChanges() && editor->toPlainText()==selectionSource,"Shadow selection applied before Apply"); applyResource(*pickerPanel);
      require(window.runMission()==MainWindow::RunResult::Completed,"GUI shadow selection mission failed"); const auto shadowReport=bytes(report); const auto selectedShadowSource=editor->toPlainText();
      require(selectedShadowSource.endsWith(selectionSource.mid(selectionSource.indexOf("BeginMissionSequence"))) && !selectedShadowSource.contains("SolarPower.InitialEpoch"),"Body selection rewrote unrelated defaults/mission"); editor->undo(); require(editor->toPlainText()==selectionSource,"Shadow selection Undo not exact"); editor->redo(); require(editor->toPlainText()==selectedShadowSource,"Shadow selection Redo not exact"); require(window.buildScript(),"Shadow selection Redo rebuild failed");
      require(!window.applyResourceChanges("SolarPower",{{"ShadowBodies","Sun"}},selectedShadowSource).isEmpty() && editor->toPlainText()==selectedShadowSource,"Typed Sun shadow bypassed wx exclusion");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==shadowReport,"Shadow selection save/reopen changed calculation");
      auto shadowReference=selectionSource; shadowReference.insert(shadowReference.indexOf("BeginMissionSequence"),"SolarPower.ShadowBodies = {Luna, Ceres};\n"); editor->setPlainText(shadowReference); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==shadowReport,"GUI/raw-script shadow calculations differ");
      pickerPanel=resource(window,"SolarPower"); section(*pickerPanel,"Shadow"); selectBodies(*pickerPanel,"ShadowBodies",{},{"Sun"},true); applyResource(*pickerPanel); require(window.runMission()==MainWindow::RunResult::Completed && Moderator::Instance()->GetConfiguredObject("SolarPower")->GetStringArrayParameter("ShadowBodies").empty(),"Clear shadows restored implicit Earth at execution"); const auto clearShadowReport=bytes(report); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==clearShadowReport && Moderator::Instance()->GetConfiguredObject("SolarPower")->GetStringArrayParameter("ShadowBodies").empty(),"Clear shadow selection did not survive save/reopen/run");
      auto clearedReference=selectionSource; clearedReference.insert(clearedReference.indexOf("BeginMissionSequence"),"SolarPower.ShadowBodies = {Earth};\nSolarPower.ShadowBodies = {};\n"); editor->setPlainText(clearedReference); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==clearShadowReport && Moderator::Instance()->GetConfiguredObject("SolarPower")->GetStringArrayParameter("ShadowBodies").empty(),"Raw empty shadow list did not clear prior/default bodies"); close(window); editor->setPlainText(shadowReference); require(window.runMission()==MainWindow::RunResult::Completed,"Point-mass fixture restore failed");
      pickerPanel=resource(window,"FM"); section(*pickerPanel,"Bodies"); property(*pickerPanel,"PrimaryBodies")->setText("Earth");
      selectBodies(*pickerPanel,"PointMasses",{"Sun","Luna","Ceres"},{"Earth"},false); require(property(*pickerPanel,"PointMasses")->text()=="Earth" && editor->toPlainText()==shadowReference,"Point-mass Cancel changed fields/source");
      selectBodies(*pickerPanel,"PointMasses",{"Sun","Luna","Ceres"},{"Earth"},true,captures.isEmpty() ? QString() : captures+".point-selection.png");
      const auto beforeForces=editor->toPlainText(); require(!window.applyResourceChanges("FM",{{"PrimaryBodies","Earth"},{"PointMasses","Earth, Luna"}},beforeForces).isEmpty() && editor->toPlainText()==beforeForces,"Typed overlapping gravity lists bypassed wx exclusion");
      applyResource(*pickerPanel); require(window.runMission()==MainWindow::RunResult::Completed,"GUI point-mass selection mission failed"); const auto forceReport=bytes(report);
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==forceReport,"Force-body selection save/reopen changed calculation");
      auto forceReference=shadowReference; forceReference.replace("FM.PrimaryBodies = {};","FM.PrimaryBodies = {Earth};"); forceReference.replace("FM.PointMasses = {Earth};","FM.PointMasses = {Sun, Luna, Ceres};"); editor->setPlainText(forceReference); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==forceReport,"GUI/raw-script point-mass reports differ");
      require(!window.applyResourceChanges("FM",{{"PointMasses","SolarPower"}},forceReference).isEmpty() && editor->toPlainText()==forceReference,"Non-body point mass accepted or changed source");
      pickerPanel=resource(window,"FM"); section(*pickerPanel,"Bodies"); modal(*pickerPanel,[&] { pickerPanel->findChild<QPushButton *>("chooseProperty_PrimaryBodies")->click(); },[](QDialog *dialog) { auto *list=dialog->findChild<QListWidget *>("resourceSelectionList"); require(list,"Primary-body selection missing"); bool earth=false; for (int i=0;i<list->count();++i) { const auto name=list->item(i)->text(); require(name!="Sun" && name!="Luna" && name!="Ceres","Primary picker included selected point mass"); earth=earth || name=="Earth"; } require(earth,"Primary-body picker omitted Earth"); });
      pickerPanel=resource(window,"FM"); section(*pickerPanel,"Bodies"); selectBodies(*pickerPanel,"PointMasses",{},{"Earth"},true); applyResource(*pickerPanel); require(window.runMission()==MainWindow::RunResult::Completed,"Clear point-mass selection failed");
      close(window); editor->setPlainText(original); require(window.buildScript(),"Body Stop fixture restore failed"); panel=open(window,"Earth"); edit(*panel,"Mu")->setText("400000"); modal(*panel,[&] { panel->parentWidget()->close(); },[](QDialog *dialog) { auto *question=qobject_cast<QMessageBox *>(dialog); require(question,"Body close prompt missing"); question->button(QMessageBox::Cancel)->click(); }); require(panel->hasChanges() && editor->toPlainText()==original,"Body Close Cancel lost pending edits"); close(window);
      const QString loop="Create Variable n;\nBeginMissionSequence;\nWhile n < 1e12;\nn = n + 1;\nEndWhile;\n"; editor->setPlainText(loop); require(window.buildScript(),"Body loop fixture failed"); panel=open(window,"Earth"); bool guarded=false; QTimer::singleShot(20,&window,[&] { guarded=!panel->isEnabled() && !window.applyResourceChanges("Earth",{{"Mu","400000"}},loop).isEmpty(); window.stopMission(); }); require(window.runMission()==MainWindow::RunResult::Stopped && guarded && panel->isEnabled(),"Body editing not guarded during Run/Stop"); close(window); editor->setPlainText(original); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==baselineReport,"Body mission did not recover after Stop");
      nasaBodyPhysicalChecks(window,*editor,files.path());
      std::cout<<"Qt bodies: four wx pages, built-in protection, pending/Cancel/correction/rollback, physical reports, texture/model/pose/color rendering, comment-preserving repeated digit-prefixed properties, exact Undo/Redo, Unicode save/reopen, New Asteroid, SPICE orbit and custom-pole frame reports, PCK/FK selection/replacement/removal/order, startup kernel replacement/clear, independent scripts, invalid-ID/missing-SPK recovery and Run/Stop guards passed\n";
      std::cout<<"Qt body selection: solar-shadow and primary/point-mass callers, user-body choices, hidden Sun/overlapping gravity bodies, Select all/Clear/Cancel/pending/Apply, typed rejection, exact Undo/Redo, explicit empty shadows, Unicode save/reopen and exact raw-script power/propagation reports passed\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
