#pragma once
#include "MessageInterface.hpp"
#include "MessageReceiver.hpp"
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

// Test-local receiver: retain the engine's real exception diagnostics without
// introducing a GUI or touching the user's console/log settings.
class NativeMessageCapture final:public MessageReceiver {
public:
   NativeMessageCapture():previous(MessageInterface::GetMessageReceiver()){MessageInterface::SetMessageReceiver(this);}
   ~NativeMessageCapture()override{MessageInterface::SetMessageReceiver(previous);}
   void ShowMessage(const std::string &v)override{text+=v;}
   void ShowMessage(const char *v,...)override{va_list a;va_start(a,v);text+=format(v,a);va_end(a);}
   void PopupMessage(Gmat::MessageType,const std::string &v)override{text+=v;}
   void PopupMessage(Gmat::MessageType,const char *v,...)override{va_list a;va_start(a,v);text+=format(v,a);va_end(a);}
   std::string GetLogFileName()override{return logFile;}
   bool GetLogEnable()override{return enabled;}
   void SetLogEnable(bool v)override{enabled=v;}
   void SetLogPath(const std::string &v,bool=false)override{logFile=v+"/GmatLog.txt";}
   void SetLogFile(const std::string &v)override{logFile=v;}
   void LogMessage(const std::string &v)override{text+=v;}
   void LogMessage(const char *v,...)override{va_list a;va_start(a,v);text+=format(v,a);va_end(a);}
   void ClearMessage()override{queue.clear();}
   std::string GetMessage()override{return queue;}
   void PutMessage(const std::string &v)override{queue+=v;}
   void ClearMessageQueue()override{queue.clear();}
   std::string text;
private:
   static std::string format(const char *v,va_list a){va_list copy;va_copy(copy,a);const int count=std::vsnprintf(nullptr,0,v,copy);va_end(copy);if(count<0)return "Invalid native message format";std::vector<char> b(count+1);std::vsnprintf(b.data(),b.size(),v,a);return std::string(b.data(),count);}
   MessageReceiver *previous;std::string logFile,queue;bool enabled=false;
};
