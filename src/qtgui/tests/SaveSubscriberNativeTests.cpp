#include "OrbitView.hpp"
#include "TextEphemFile.hpp"
#include "FileManager.hpp"
#include "GmatBase.hpp"
#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

static int assertions=0;
static void check(bool condition,const char *message)
{
   ++assertions;
   if (!condition) throw std::runtime_error(message);
}

static int fovId(OrbitView &view,const std::string &name)
{
   check(view.GetParameterID(name)==Gmat::PARAMETER_REMOVED,"OrbitView removed FOV script interface changed");
   int id=-1;
   for (int i=0;i<view.GetParameterCount();++i)
      if (view.GetParameterText(i)==name) {
         check(id==-1,"Duplicate OrbitView native FOV descriptor");
         id=i;
      }
   check(id>=0,"OrbitView native FOV descriptor absent");
   return id;
}

static void fieldOfView(OrbitView &view,const std::array<int,3> &expected)
{
   const std::array<std::string,3> names{{"MinFOV","MaxFOV","InitialFOV"}};
   for (int i=0;i<3;++i) {
      const int id=fovId(view,names[i]);
      check(view.GetParameterType(id)==Gmat::INTEGER_TYPE,"OrbitView FOV native type changed");
      check(view.IsParameterReadOnly(id),"OrbitView deprecated FOV mutability changed");
      check(view.GetIntegerParameter(id)==expected[i],"OrbitView copy/assignment lost a FOV integer");
   }
}

static void resolvedFilename(TextEphemFile &file,const std::filesystem::path &directory,
                             const std::string &expected)
{
   check(file.GetStringParameter("Filename")==expected,"TextEphemFile public filename changed");
   const auto actual=std::filesystem::u8path(file.GetStringParameter("FullPathFileName")).lexically_normal();
   check(actual==(directory/std::filesystem::u8path(expected)).lexically_normal(),
         "TextEphemFile public filename and effective output path disagree");
   check(file.GetParameterType(file.GetParameterID("FullPathFileName"))==Gmat::FILENAME_TYPE,
         "TextEphemFile path descriptor type changed");
   check(file.IsParameterReadOnly(file.GetParameterID("FullPathFileName")),
         "TextEphemFile path alias mutability changed");
}

int main(int argc,char **argv)
{
   try {
      check(argc==3,"Supply startup file and a private output directory");
      const auto startup=std::filesystem::absolute(std::filesystem::u8path(argv[1]));
      const auto output=std::filesystem::absolute(std::filesystem::u8path(argv[2]));
      std::filesystem::create_directories(output);
      std::filesystem::current_path(startup.parent_path());
      FileManager::Instance()->ReadStartupFile(startup.u8string());
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",output.u8string());
      OrbitView source("Source");
      fieldOfView(source,{{0,90,45}});
      source.SetIntegerParameter(fovId(source,"MinFOV"),5);
      source.SetIntegerParameter(fovId(source,"MaxFOV"),80);
      source.SetIntegerParameter(fovId(source,"InitialFOV"),35);
      const std::array<int,3> custom{{5,80,35}};
      fieldOfView(source,custom);
      OrbitView copied(source);
      fieldOfView(copied,custom);
      std::unique_ptr<GmatBase> cloned(source.Clone());
      auto *clone=dynamic_cast<OrbitView *>(cloned.get());
      check(clone!=nullptr,"OrbitView Clone native type changed");
      fieldOfView(*clone,custom);
      OrbitView assigned("Assigned");
      assigned=source;
      fieldOfView(assigned,custom);
      assigned=assigned;
      fieldOfView(assigned,custom);
      OrbitView copiedThroughBase("BaseCopy");
      copiedThroughBase.Copy(&source);
      fieldOfView(copiedThroughBase,custom);
      TextEphemFile defaultFile("TextEphemFile","DefaultFile");
      resolvedFilename(defaultFile,output,".data");
      TextEphemFile copiedDefault(defaultFile);
      resolvedFilename(copiedDefault,output,".data");
      TextEphemFile provided("TextEphemFile","Provided","native header.eph");
      resolvedFilename(provided,output,"native header.eph.data");
      provided.SetStringParameter("Filename","explicit output.txt");
      resolvedFilename(provided,output,"explicit output.txt");
      std::unique_ptr<GmatBase> clonedFile(provided.Clone());
      auto *nativeFile=dynamic_cast<TextEphemFile *>(clonedFile.get());
      check(nativeFile!=nullptr,"TextEphemFile Clone native type changed");
      resolvedFilename(*nativeFile,output,"explicit output.txt");
      TextEphemFile assignedFile("TextEphemFile","AssignedFile");
      assignedFile=provided;
      resolvedFilename(assignedFile,output,"explicit output.txt");
      assignedFile=assignedFile;
      resolvedFilename(assignedFile,output,"explicit output.txt");
      std::cout<<"{\"status\":\"passed\",\"assertions\":"<<assertions
               <<",\"GUI_contexts\":0,\"new_requirement_credit\":0}"<<std::endl;
      return 0;
   } catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}
     catch (...) {std::cerr<<"FAIL native subscriber regression exception"<<std::endl;}
   return 1;
}
