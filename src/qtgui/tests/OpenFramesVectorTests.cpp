#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "OrbitVectors.hpp"
#include "PlotWidget.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "CoordinateSystem.hpp"
#include "CoordinateConverter.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Vector source/report missing"); return file.readAll(); }
static osg::Vec3d xyz(const std::array<double,3> &value) { return {value[0],value[1],value[2]}; }
static osg::Vec3d origin(const PlotPoint &value) { return {value.x,value.y,value.z}; }
static osg::Vec3d rotate(const PlotPoint &pose,const osg::Vec3d &value) {
   const auto &r=pose.bodyToView; return {r[0]*value.x()+r[1]*value.y()+r[2]*value.z(),r[3]*value.x()+r[4]*value.y()+r[5]*value.z(),r[6]*value.x()+r[7]*value.y()+r[8]*value.z()};
}
static int coloredPixels(const QImage &image) {
   int count=0; for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) { const auto c=image.pixelColor(x,y); if (c.green()>c.red()*2 && c.green()>c.blue()*2 && c.green()>80) ++count; } return count;
}
static void geometryAndPixels()
{
   auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit); model->axes=model->grid=model->labels=model->legend=false;
   PlotVector vector; vector.name="Arrow"; vector.source="Sat"; vector.destination="Sun"; vector.color=QColor(0,255,0,204);
   PlotVectorSample first; first.source.frame=1; first.sourceRadius=3; first.destination={50,0,0};
   PlotVectorSample last=first; last.source.frame=2; last.destination={0,50,0}; vector.samples={first,last}; model->vectors.append(vector);
   const auto a=orbitVectorArrow(*model,vector,quint64(1)),b=orbitVectorArrow(*model,vector,quint64(2));
   require(a && b && (a->start-osg::Vec3d(3,0,0)).length()<1e-12 && (a->end-osg::Vec3d(6,0,0)).length()<1e-12 && (b->start-osg::Vec3d(0,3,0)).length()<1e-12,"Relative Position Auto arrow lost pointing or source radius offset");
   QMap<QString,OrbitObjectBounds> bounds{{"Sat",{{1,2,3},5}}};
   const auto asset=orbitVectorArrow(*model,vector,quint64(1),&bounds);
   require(asset && (asset->start-osg::Vec3d(6,2,3)).length()<1e-12 && (asset->end-asset->start).length()==5,"Vector Auto framing ignored actual model center/radius");
   vector.bodyFixed=true; vector.automaticLength=false; vector.length=12; vector.start={2,3,4}; vector.direction={0,2,0};
   first.source.bodyToView={0,-1,0,1,0,0,0,0,1};
   const auto fixed=orbitVectorArrow(*model,vector,first);
   require(fixed && (fixed->start-osg::Vec3d(-3,2,4)).length()<1e-12 && (fixed->end-fixed->start-osg::Vec3d(-12,0,0)).length()<1e-12,"Body-Fixed start/direction did not use retained source attitude and Manual length");
   auto unavailable=last; unavailable.valid=false; unavailable.source.frame=3; model->vectors[0].samples.push_back(unavailable);
   require(!orbitVectorArrow(*model,model->vectors[0],quint64(0)) && !orbitVectorArrow(*model,model->vectors[0],quint64(3)),"Missing/invalid vector samples replayed stale geometry");
   model->vectors[0].samples.pop_back();
   PlotCanvas canvas(model); canvas.resize(640,480); canvas.show(); canvas.setFrame(1); const auto early=canvas.captureImage();
   canvas.setFrame(2); const auto late=canvas.captureImage();
   require(!early.isNull() && coloredPixels(early)>10 && coloredPixels(late)>10 && early!=late,"Fallback vector shaft/head missing or replay ignored its selected frame");
   model->vectors.clear(); canvas.refresh(); require(coloredPixels(canvas.captureImage())==0,"Fallback retained vector geometry persisted after removal");
}
static void conversion(const QString &startup)
{
   const auto samples=QDir(QFileInfo(startup).absolutePath()).absoluteFilePath("../samples");
   for (const auto &path:QStringList{"Ex_DynamicDisplay_QuaternionPropagation.script","Ex_ElectricPropulsion.script","Ex_ForceModel_ExtraShadowBodies.script"}) {
      const auto source=QString::fromUtf8(read(QDir(samples).filePath(path))); const auto converted=convertOpenFramesViews(source);
      require(converted.error.isEmpty(),qPrintable(converted.error));
      require(converted.script.endsWith(source.mid(source.indexOf("BeginMissionSequence;"))),"Vector sample conversion altered mission source");
      int vectors=0;
      for (const auto &plot:qtCameraSettings(converted.script)) for (const auto &vector:plot.vectors) {
         ++vectors; require(vector.type=="Relative Position" && vector.automaticLength && !vector.source.isEmpty() && !vector.destination.isEmpty() && !vector.label.isEmpty(),"Actual sample vector metadata lost required effect");
         require(converted.script.contains("% Qt conversion: Create OpenFramesVector "+vector.name),"Original vector declaration was not retained");
      }
      require(vectors>0,"Shipped sample silently discarded all vectors");
   }
   const QString setup="Create Spacecraft Sat;\nCreate OpenFramesInterface Display;\nDisplay.Add = {Sat, Earth};\nCreate OpenFramesVector A B;\nA.SourceObject = Sat;\nA.VectorType = 'Relative Position';\nA.DestinationObject = Sun;\nB.SourceObject = Earth;\nB.VectorColor = 'Yellow';\nDisplay.Vector = A;\nDisplay.Vector = {B, , A, B};\nDisplay.Vector = A;\n";
   const auto converted=convertOpenFramesViews(setup+"BeginMissionSequence;\n"); require(converted.error.isEmpty(),qPrintable(converted.error));
   const auto list=qtCameraSettings(converted.script).value("Display").vectors;
   require(list.size()==2 && list[0].name=="A" && list[1].name=="B" && list[1].type=="Body-Fixed" && list[1].color==0xffff00 && list[1].label=="DefaultVector","Script vector append/dedup order, color or BF defaults differ from OF");
   require(qtCameraSettings(convertOpenFramesViews(setup+"Display.Vector = {};\nBeginMissionSequence;\n").script).value("Display").vectors.size()==2,"Empty script vector list changed selected arrows");
   for (const auto &bad:QStringList{
         setup+"A.VectorType = 'Thrust Vector';\nBeginMissionSequence;\n",
         setup+"A.VectorLengthType = Manual;\nA.VectorLength = 0;\nBeginMissionSequence;\n",
         setup+"B.BFDirection = [0 0 0];\nBeginMissionSequence;\n",
         setup+"A.VectorColor = [256 0 0];\nBeginMissionSequence;\n",
         setup+"A.ExtraMode = On;\nBeginMissionSequence;\n",
         setup+"BeginMissionSequence;\nA.VectorLength = 2;\n",
         setup+"BeginMissionSequence;\nCreate Variable V;\nV = A.VectorLength;\n"}) {
      const auto rejected=convertOpenFramesViews(bad); require(!rejected.error.isEmpty() && rejected.script==bad,"Unsupported/malformed/dynamic vector conversion changed original source");
   }
   auto invalid=qtCameraSettings(converted.script).value("Display"); invalid.vectors[0].length=-1; bool rejected=false;
   try { qtCameraSettings(qtCameraDirective("Display",invalid)); } catch (const std::exception &) { rejected=true; }
   require(rejected,"Invalid persisted vector metadata accepted");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtOpenFramesVectors");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Vector focused check requires offscreen and a startup path");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Vector temporary directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      conversion(startup); geometryAndPixels();
      const auto report=files.filePath("vector-independent state Δ.txt"),saved=files.filePath("retained vectors Δ.script");
      const QString resources="% vector-independent physics α\nCreate Spacecraft Sat;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 10;\nProp.MaxStep = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 17;\nValues.WriteHeaders = false;\n";
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 60}; % unchanged numerical mission\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
      const QString displays="Create CoordinateSystem Rotating;\nRotating.Origin = Earth;\nRotating.Axes = BodyFixed;\nCreate OpenFramesInterface Display Rotated;\nDisplay.Add = {Sat, Earth};\nDisplay.CoordinateSystem = EarthMJ2000Eq;\nDisplay.EnableStars = Off;\nRotated.Add = {Sat, Earth};\nRotated.CoordinateSystem = Rotating;\nRotated.EnableStars = Off;\nCreate OpenFramesVector ToSun BF ToMoon;\nToSun.SourceObject = Sat;\nToSun.VectorType = 'Relative Position';\nToSun.DestinationObject = Sun;\nToSun.VectorColor = [255 255 128];\nToSun.VectorLabel = 'Sun α';\nBF.SourceObject = Sat;\nBF.VectorType = 'Body-Fixed';\nBF.BFStartPoint = [2 3 4];\nBF.BFDirection = [0 2 0];\nBF.VectorLengthType = Manual;\nBF.VectorLength = 12;\nBF.VectorColor = Red;\nBF.VectorLabel = 'Body Y';\nToMoon.SourceObject = Earth;\nToMoon.VectorType = 'Relative Position';\nToMoon.DestinationObject = Luna;\nToMoon.VectorColor = [192 192 192];\nToMoon.VectorLabel = Moon;\nDisplay.Vector = {ToSun, BF};\nRotated.Vector = {ToMoon};\n";
      MainWindow window; window.show(); require(window.initialize(startup),"Vector runtime initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Vector script editor unavailable");
      editor->setPlainText(resources+mission); require(window.runMission()==MainWindow::RunResult::Completed,"Independent vector-free mission failed"); const auto expected=read(report);
      const auto converted=convertOpenFramesViews(resources+displays+mission); require(converted.error.isEmpty() && converted.script.endsWith(mission),qPrintable(converted.error));
      editor->setPlainText(converted.script); require(window.buildScript(),"Vector-bearing OrbitView build failed");
      require(Moderator::Instance()->GetConfiguredObject("Display")->GetStringArrayParameter("Add")==StringArray({"Sat","Earth"}),"Vector conversion changed Add membership");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed,"Vector save/reopen/execution failed");
      require(editor->toPlainText()==converted.script && read(report)==expected,"Vector display changed saved source or independent numerical state");
      auto retained=window.plotReceiver()->model("Display"),rotated=window.plotReceiver()->model("Rotated");
      require(retained && rotated && retained->vectors.size()==2 && rotated->vectors.size()==1,"Selected vectors were not retained");
      auto *solar=Moderator::Instance()->GetSolarSystemInUse(); auto *view=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetInternalObject("Rotating"));
      auto *inertial=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetInternalObject("EarthMJ2000Eq"));
      require(view && inertial,"Rotating vector coordinate systems unavailable");
      for (const auto *model:{retained.get(),rotated.get()}) for (const auto &vector:model->vectors) {
         require(vector.samples.size()>2 && vector.samples.front().valid && vector.samples.back().valid,"Vector epoch history incomplete");
         for (const auto *sample:{&vector.samples.front(),&vector.samples.back()}) {
            const auto arrow=orbitVectorArrow(*model,vector,*sample); require(bool(arrow),"Captured vector pose produced no arrow");
            if (vector.bodyFixed) {
               require((arrow->start-origin(sample->source)-rotate(sample->source,{2,3,4})).length()<1e-8 && (arrow->end-arrow->start-rotate(sample->source,{0,12,0})).length()<1e-8,"Actual Body-Fixed arrow ignored published source pose");
            } else {
               auto state=solar->GetBody(vector.destination.toStdString())->GetMJ2000State(sample->source.epoch);
               if (model==rotated.get()) { Rvector6 changed; CoordinateConverter converter; converter.Convert(sample->source.epoch,state,inertial,changed,view); state=changed; }
               const auto destination=osg::Vec3d(state[0],state[1],state[2]);
               require((xyz(sample->destination)-destination).length()<1e-7,"Unplotted vector destination differs from ephemeris in plot coordinates");
               auto direction=destination-origin(sample->source); direction.normalize(); auto shaft=arrow->end-arrow->start; const auto length=shaft.normalize();
               require((shaft-direction).length()<1e-8 && std::abs(length-sample->sourceRadius)<1e-7,"Actual Relative Position Auto arrow direction/length incorrect");
            }
         }
      }
      require(retained->vectors[0].color.rgb()==QColor(255,255,128).rgb() && retained->vectors[0].label=="Sun α" && std::abs(retained->vectors[0].color.alphaF()-.8)<.01,"Vector color/label was lost in save/reopen");
      auto metadata=qtCameraSettings(converted.script); auto invalid=metadata; invalid["Display"].vectors[0].destination="MissingBody"; bool rejected=false;
      try { QtPlotReceiver::validateCameraReferences(invalid); } catch (const std::exception &) { rejected=true; } require(rejected,"Unknown vector endpoint accepted");
      invalid=metadata; invalid["Display"].objectAxes["Luna"]=true; rejected=false;
      try { QtPlotReceiver::validateCameraReferences(invalid); } catch (const std::exception &) { rejected=true; } require(rejected,"Vector endpoint relaxation also weakened strict object flags");
      const auto before=orbitVectorArrow(*retained,retained->vectors[0],retained->vectors[0].samples.front());
      editor->setPlainText("Create Spacecraft Replacement;\nBeginMissionSequence;\n"); require(window.buildScript(),"Replacement engine build failed");
      const auto replay=orbitVectorArrow(*retained,retained->vectors[0],retained->vectors[0].samples.front());
      require(replay && before && (replay->start-before->start).length()==0 && (replay->end-before->end).length()==0,"Vector replay consulted discarded engine objects");
      auto copy=*retained; copy.maxPoints=2; copy.trim(); require(copy.vectors[0].samples.size()==2 && copy.vectors[1].samples.size()==2,"Vector histories ignored retention limit");
      copy.clear(); require(copy.vectors[0].samples.empty() && copy.vectors[1].samples.empty(),"Vector histories survived model clear");
      std::cout<<"PASS: all three regular shipped vector conversions; original mission/source and independent numerical report; scalar/list selection, Relative Position/Body-Fixed, named/RGB color and labels, Auto/Manual lengths, model bounds, hidden Sun/Luna endpoints in inertial/rotating frames, replay after engine replacement, retention/clear and fallback colored shaft/head replay. Unsupported/dynamic vectors remain precise errors. Offscreen only; native renderer pixels not exercised.\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
