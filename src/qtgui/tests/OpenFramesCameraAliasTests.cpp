#include "ScriptCompatibility.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static const QString physics="% independently retained calculation α\nCreate Variable Answer;\nAnswer = 3;\n";
static const QString mission="BeginMissionSequence;\nAnswer = Answer + 2; % calculation and comment stay exact\n";
static QString input(const QString &primary,const QString &secondary={})
{
   return physics+"Create OpenFramesView Primary Secondary;\nPrimary.ViewFrame = CoordinateSystem;\nSecondary.ViewFrame = Earth;\n"+
      primary+secondary+"Create OpenFramesInterface Display;\nDisplay.Add = {Earth};\nDisplay.View = {Primary, Secondary};\n"+mission;
}
static QtScriptConversion convert(const QString &source)
{
   const auto converted=convertOpenFramesViews(source);
   require(converted.error.isEmpty(),qPrintable(converted.error));
   require(converted.plots==1 && converted.script.contains(physics) && converted.script.endsWith(mission),"Camera alias conversion changed nonviewer source or calculations");
   return converted;
}
static void primary(const QtScriptConversion &converted,const QString &eye,const QString &center,const std::array<double,3> &up)
{
   require(converted.script.contains("GMAT Display.ViewPointVector = "+eye+";"),"Primary camera eye was lost or became automatic");
   require(converted.script.contains("GMAT Display.ViewDirection = "+center+";"),"Primary camera center was lost");
   const auto setting=qtCameraSettings(converted.script).value("Display");
   require(setting.up && *setting.up==up && setting.automaticTrajectory.isEmpty() && setting.automaticBody.isEmpty(),"Primary alias camera lost its up vector or stored mode");
}
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   try {
      const QString aliases="Primary.SetCameraLocation = On; % stored legacy pose\nPrimary.CameraEye = [11 -23 31];\nPrimary.CameraCenter = [1 2 3];\nPrimary.CameraUp = [1 0 2];\n";
      const QString alternate="Secondary.SetCameraLocation = On;\nSecondary.CameraEye = [41 -53 61];\nSecondary.CameraCenter = [4 5 6];\nSecondary.CameraUp = [2 0 1];\n";
      const auto source=input(aliases,alternate);
      const auto converted=convert(source);
      primary(converted,"[11 -23 31]","[1 2 3]",{1,0,2});
      const auto setting=qtCameraSettings(converted.script).value("Display");
      require(setting.views.size()==1,"Alias conversion lost the secondary view");
      const auto &secondary=setting.views[0];
      require(secondary.name=="Secondary" && secondary.reference=="Earth" && secondary.bodyRelative && secondary.eye==std::array<double,3>{41,-53,61} && secondary.center==std::array<double,3>{4,5,6} && secondary.up==std::array<double,3>{2,0,1} && secondary.automaticBody.isEmpty(),"Secondary body-relative alias pose became automatic or changed components");
      for (const auto &line:(aliases+alternate).split('\n')) if (!line.isEmpty())
         require(converted.script.contains("% Qt conversion: "+line+"\n"),"Conversion lost an original alias setting/comment");
      auto canonical=source;
      canonical.replace("SetCameraLocation","SetDefaultLocation").replace("CameraEye","DefaultEye").replace("CameraCenter","DefaultCenter").replace("CameraUp","DefaultUp");
      const auto canonicalConversion=convert(canonical);
      require(qtCameraDirective("Display",qtCameraSettings(canonicalConversion.script).value("Display"))==qtCameraDirective("Display",setting),"Alias and canonical spellings produce different camera metadata");
      primary(canonicalConversion,"[11 -23 31]","[1 2 3]",{1,0,2});
      const QString defaults="Primary.SetDefaultLocation = On;\nPrimary.DefaultEye = [71 -83 91];\nPrimary.DefaultCenter = [7 8 9];\nPrimary.DefaultUp = [0 2 1];\n";
      primary(convert(input(aliases+defaults,alternate)),"[71 -83 91]","[7 8 9]",{0,2,1});
      primary(convert(input(defaults+aliases,alternate)),"[11 -23 31]","[1 2 3]",{1,0,2});
      const auto disabled=convert(input(aliases+"Primary.SetDefaultLocation = Off;\n",alternate));
      require(qtCameraSettings(disabled.script).value("Display").automaticTrajectory=="CoordinateSystem","Later canonical Off did not disable the alias stored pose");
      const auto disabledAlias=convert(input(defaults+"Primary.SetCameraLocation = Off;\n",alternate));
      require(qtCameraSettings(disabledAlias.script).value("Display").automaticTrajectory=="CoordinateSystem","Later alias Off did not disable the canonical stored pose");
      primary(convert(input(aliases+"Primary.SetCurrentLocation = On;\nPrimary.CurrentEye = [101 -113 121];\nPrimary.CurrentCenter = [10 11 12];\nPrimary.CurrentUp = [0 1 2];\n",alternate)),"[101 -113 121]","[10 11 12]",{0,1,2});
      for (const auto &invalid:QStringList{QString(aliases).replace("SetCameraLocation = On","SetCameraLocation = Maybe"),QString(aliases).replace("CameraEye = [11 -23 31]","CameraEye = [11 -23]"),QString(aliases).replace("CameraUp = [1 0 2]","CameraUp = [0 0 0]")}) {
         const auto bad=input(invalid,alternate);
         const auto rejected=convertOpenFramesViews(bad);
         require(!rejected.error.isEmpty() && rejected.script==bad,"Invalid active alias settings were silently accepted or original source changed");
      }
      QTemporaryDir files; require(files.isValid(),"Alias file-roundtrip directory unavailable");
      QFile saved(files.filePath("legacy camera Δ.script"));
      require(saved.open(QIODevice::WriteOnly) && saved.write(converted.script.toUtf8())==converted.script.toUtf8().size(),"Converted camera write failed");
      saved.close(); require(saved.open(QIODevice::ReadOnly),"Converted camera reopen failed");
      const auto reopened=QString::fromUtf8(saved.readAll());
      require(reopened==converted.script && qtCameraDirective("Display",qtCameraSettings(reopened).value("Display"))==qtCameraDirective("Display",setting),"Unicode file roundtrip changed converted source or retained camera metadata");
      std::cout<<"PASS: legacy stored primary/body-relative secondary cameras; canonical/alias last-write ordering; Current precedence; invalid-setting source protection; original comments/calculations and Unicode conversion-file roundtrip. Source/metadata only, no engine run, rendered scene or host desktop claim.\n";
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
