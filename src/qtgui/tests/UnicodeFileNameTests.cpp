#include "StringUtil.hpp"
#include <QDir>
#include <QTemporaryDir>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
int main()
{
   try {
      QTemporaryDir files;require(files.isValid(),"Unicode file fixture directory missing");
      const auto folder=files.path()+QString::fromUtf8(u8"/répertoire Δ");require(QDir().mkpath(folder),"Unicode directory could not be created");
      const std::vector<std::string> names{"ASCII target.txt",u8"solver Δ.txt",u8"résultat ü.txt",u8"東京.txt",u8"测定 🚀.txt"};int io=0,rejected=0;
      for (const auto &name:names) {
         require(GmatStringUtil::IsValidFileName(name),"Valid ASCII/UTF-8 basename rejected");const auto path=folder.toStdString()+"/"+name;Integer error=-1;
         require(GmatStringUtil::IsValidFullFileName(path,error) && error==0,"Valid UTF-8 full path rejected");
         {std::ofstream output(path,std::ios::binary);require(bool(output),"Validated Unicode file did not open");output<<"known solver result Δ\n";require(bool(output),"Unicode file write failed");}
         std::ifstream input(path,std::ios::binary);const std::string content((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());require(content=="known solver result Δ\n","Unicode filename reopen changed content");++io;
      }
      Integer error=0;require(!GmatStringUtil::IsValidFileName("") && !GmatStringUtil::IsValidFullFileName("",error),"Empty filename accepted");++rejected;
      for (int value=0;value<=127;++value) if (value<32 || value==127) {
         const auto bad=std::string("Bad")+char(value)+".txt";require(!GmatStringUtil::IsValidFileName(bad),"ASCII control accepted in basename");require(!GmatStringUtil::IsValidFullFileName(folder.toStdString()+"/"+bad,error),"ASCII control accepted in full path");require(!GmatStringUtil::IsValidFullFileName(files.path().toStdString()+"/bad"+char(value)+"/target.txt",error),"ASCII control accepted in directory");rejected+=3;
      }
      for (char value:std::string("\\/|*\":<>?")) {require(!GmatStringUtil::IsValidFileName(std::string("Bad")+value+".txt"),"Reserved basename character accepted");++rejected;}
      for (const auto &bad:std::vector<std::string>{"12:target.txt","1:target.txt",std::string(1,char(0xff))+":target.txt","/bad|directory/target.txt","/bad?directory/target.txt"}) {require(!GmatStringUtil::IsValidFullFileName(bad,error),"Invalid drive/path syntax accepted");++rejected;}
      std::cout<<"PASS Unicode filename validation and real create/reopen for "<<io<<" ASCII/UTF-8 names in a Unicode directory; "<<rejected<<" retained empty/control/reserved/drive rejections. Linux file I/O only; no native Windows/macOS qualification.\n";return 0;
   }catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;return 1;}
}
