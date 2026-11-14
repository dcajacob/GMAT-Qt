//$Id$
//------------------------------------------------------------------------------
//                               GmatConnection
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); 
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.apache.org/licenses/LICENSE-2.0 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either 
// express or implied.   See the License for the specific language
// governing permissions and limitations under the License.
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number S-67573-G
//
// Author: Linda Jun
// Created: 2004/08/27
//
/**
 * Implements GmatConnection which provides service to client.
 */
//------------------------------------------------------------------------------

#include "GmatConnection.hpp"
#include "GmatInterface.hpp"
#include "BaseException.hpp"
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include "InterfaceException.hpp"
#include "wx/strconv.h"
#include "gmatwxdefs.hpp"
#include "MessageInterface.hpp"

#if wxCHECK_VERSION(3, 0, 0)
namespace
{
const wxMBConv &MatlabTextConverter(wxIPCFormat format)
{
   static const wxMBConvUTF16 utf16;
   static const wxMBConvUTF32 utf32;
   switch (format)
   {
      case wxIPC_TEXT: return wxConvLibc;
      case wxIPC_UTF8TEXT: return wxConvUTF8;
      case wxIPC_UTF16TEXT: return utf16;
      case wxIPC_UTF32TEXT: return utf32;
      default: throw InterfaceException("Unsupported MATLAB IPC text format");
   }
}
std::wstring DecodeMatlabText(const wxMBConv &converter, const char *data, size_t size)
{
   const size_t count = converter.ToWChar(nullptr, 0, data, size);
   if (count == wxCONV_FAILED) throw InterfaceException("Invalid MATLAB IPC text encoding");
   std::wstring result(count, L'\0');
   if (count && converter.ToWChar(result.data(), count, data, size) != count)
      throw InterfaceException("Cannot decode MATLAB IPC text");
   return result;
}
std::vector<char> EncodeMatlabText(const wxMBConv &converter, const std::wstring &value)
{
   const size_t count = converter.FromWChar(nullptr, 0, value.data(), value.size());
   const size_t nul = converter.GetMBNulLen();
   if (count == wxCONV_FAILED || nul == 0 || nul > 4 || count > std::numeric_limits<size_t>::max() - nul)
      throw InterfaceException("Cannot encode MATLAB IPC text");
   std::vector<char> result(count + nul, 0);
   if (count && converter.FromWChar(result.data(), count, value.data(), value.size()) != count)
      throw InterfaceException("Cannot finish encoding MATLAB IPC text");
   return result;
}
std::string DecodeMatlabCommand(const void *data, size_t size, wxIPCFormat format)
{
   const auto &converter = MatlabTextConverter(format);
   const size_t nul = converter.GetMBNulLen();
   if (!data || size == wxNO_LEN || !nul || nul > 4 || size < nul || size % nul)
      throw InterfaceException("Invalid MATLAB IPC command size");
   const auto bytes = static_cast<const char*>(data);
   for (size_t i = size - nul; i < size; ++i)
      if (bytes[i] != 0) throw InterfaceException("MATLAB IPC command is missing its text terminator");
   auto wide = DecodeMatlabText(converter, bytes, size - nul);
   if (wide.empty() || wide.find(L'\0') != std::wstring::npos)
      throw InterfaceException("MATLAB IPC command is empty or contains NUL data");
   const auto utf8 = EncodeMatlabText(wxConvUTF8, wide);
   return std::string(utf8.data(), utf8.size() - 1);
}
}
#endif

//#define DEBUG_CONNECTION
//#define DEBUG_CONNECTION_EXECUTE
//#define DEBUG_CONNECTION_POKE
//#define DEBUG_CONNECTION_REQUEST
//#define DEBUG_CONNECTION_ADVISE


//------------------------------------------------------------------------------
// GmatConnection()
//------------------------------------------------------------------------------
GmatConnection::GmatConnection()
   : wxConnection()
{   
   #ifdef DEBUG_CONNECTION
   MessageInterface::ShowMessage
      ("GmatConnection() constructor entered, this=%p\n", this);
   #endif
}


//------------------------------------------------------------------------------
// ~GmatConnection()
//------------------------------------------------------------------------------
GmatConnection::~GmatConnection()
{
   #ifdef DEBUG_CONNECTION
   MessageInterface::ShowMessage
      ("~GmatConnection() destructor entered, this=%p\n", this);
   #endif   
}

//===========================================
// If using wxWidgets 3.0 or higher
#if wxCHECK_VERSION(3, 0, 0)
//===========================================

//------------------------------------------------------------------------------
// bool OnExec(const wxString& WXUNUSED(topic),
//------------------------------------------------------------------------------
bool GmatConnection::OnExec(const wxString &WXUNUSED(topic),const wxString &data)
{
   #ifdef DEBUG_CONNECTION_EXECUTE
   MessageInterface::ShowMessage
      ("GmatConnection::OnExecute() command: %s\n", data.WX_TO_C_STRING);
   #endif
   
   return TRUE;
}


//------------------------------------------------------------------------------
// bool OnPoke(const wxString& WXUNUSED(topic),
//------------------------------------------------------------------------------
bool GmatConnection::OnPoke(const wxString& WXUNUSED(topic),
                            const wxString& item,
                            const void *data,
                            size_t size,
                            wxIPCFormat format)
{
   if (item != "script") return false;
   try
   {
      const auto command = DecodeMatlabCommand(data, size, format);
      return RunPoke(command.c_str());
   }
   catch (BaseException &error)
   {
      MessageInterface::ShowMessage("GMAT MATLAB command rejected: %s\n", error.GetFullMessage().c_str());
   }
   catch (const std::exception &error)
   {
      MessageInterface::ShowMessage("GMAT MATLAB command rejected: %s\n", error.what());
   }
   return false;
}


//------------------------------------------------------------------------------
// const void* OnRequest(const wxString& WXUNUSED(topic), const wxString& item,
//                   size_t * WXUNUSED(size), wxIPCFormat WXUNUSED(format))
//------------------------------------------------------------------------------
/*
 * This method responds to the client application to request data from the server.
 *
 * @param <topic>  Unused
 * @param <item>   Object or parameter name to retrive value from
 * @param <size>   Unused
 * @param <format> Unused
 *
 * @return Object or parameter value string.
 */
//------------------------------------------------------------------------------
const void* GmatConnection::OnRequest(const wxString& WXUNUSED(topic),
                                      const wxString& item,
                                      size_t *size,
                                      wxIPCFormat format)
{
   if (size) *size = 0;
   replyBytes.clear();
   try
   {
      const auto &converter = MatlabTextConverter(format);
      const auto value = ReadReply(item);
      const auto wide = DecodeMatlabText(wxConvUTF8, value.data(), value.size());
      auto encoded = EncodeMatlabText(converter, wide);
      replyBytes.swap(encoded);
      if (size) *size = replyBytes.size();
      return replyBytes.data();
   }
   catch (BaseException &error)
   {
      MessageInterface::ShowMessage("GMAT MATLAB request failed: %s\n", error.GetFullMessage().c_str());
   }
   catch (const std::exception &error)
   {
      MessageInterface::ShowMessage("GMAT MATLAB request failed: %s\n", error.what());
   }
   return nullptr;
}

//===========================================
#else
//===========================================

//------------------------------------------------------------------------------
// bool OnExecute(const wxString& WXUNUSED(topic),
//------------------------------------------------------------------------------
bool GmatConnection::OnExecute(const wxString& WXUNUSED(topic),
                               wxChar *data,
                               int WXUNUSED(size),
                               wxIPCFormat WXUNUSED(format))
{
   #ifdef DEBUG_CONNECTION_EXECUTE
   MessageInterface::ShowMessage
      ("GmatConnection::OnExecute() command: %s\n", data);
   #endif
   
   return TRUE;
}


//------------------------------------------------------------------------------
// bool OnPoke(const wxString& WXUNUSED(topic),
//------------------------------------------------------------------------------
bool GmatConnection::OnPoke(const wxString& WXUNUSED(topic),
                            const wxString& item,
                            wxChar *data,
                            int WXUNUSED(size),
                            wxIPCFormat WXUNUSED(format))
{
   #ifdef DEBUG_CONNECTION_POKE
   MessageInterface::ShowMessage
      ("\nGmatConnection::OnPoke() wx%d.%d entered\n   item = '%s'\n   data = '%s'\n",
       wxMAJOR_VERSION, wxMINOR_VERSION, item.WX_TO_C_STRING, data);
   #endif
   
   const char* itemData = (char*)data;
   
   bool retval = RunPoke(itemData);
   
   #ifdef DEBUG_CONNECTION_POKE
   MessageInterface::ShowMessage
      ("GmatConnection::OnPoke() returning %d\n", retval);
   #endif
   
   return retval;
}


//------------------------------------------------------------------------------
// wxChar* OnRequest(const wxString& WXUNUSED(topic), const wxString& item,
//                   int * WXUNUSED(size), wxIPCFormat WXUNUSED(format))
//------------------------------------------------------------------------------
/*
 * This method responds to the client application to request data from the server.
 *
 * @param <topic>  Unused
 * @param <item>   Object or parameter name to retrive value from
 * @param <size>   Unused
 * @param <format> Unused
 *
 * @return Object or parameter value string.
 */
//------------------------------------------------------------------------------
wxChar* GmatConnection::OnRequest(const wxString& WXUNUSED(topic),
                                  const wxString& item,
                                  int * WXUNUSED(size),
                                  wxIPCFormat WXUNUSED(format))
{
   #ifdef DEBUG_CONNECTION_REQUEST
   MessageInterface::ShowMessage
      ("\nGmatConnection::OnRequest() wx%d.%d entered\n   item = '%s'\n",
       wxMAJOR_VERSION, wxMINOR_VERSION, item.c_stre());
   #endif
   
   char *data = RunRequest(item);
   
   #ifdef DEBUG_CONNECTION_REQUEST
   MessageInterface::ShowMessage
      ("GmatConnection::OnRequest() returning '%s'\n", data);
   #endif
   
   return _T(data);
}

//===========================================
#endif
//===========================================


//------------------------------------------------------------------------------
// bool OnStartAdvise(const wxString& WXUNUSED(topic),
//------------------------------------------------------------------------------
bool GmatConnection::OnStartAdvise(const wxString& WXUNUSED(topic),
                                   const wxString& item)
{
   #ifdef DEBUG_CONNECTION_ADVISE
   MessageInterface::ShowMessage
      ("GmatConnection::OnStartAdvise() %s\n", item.c_str());
   #endif
   
   //#ifdef DEBUG_CONNECTION_ADVISE
   //char* data = GmatInterface::Instance()->GetRunState();
   //MessageInterface::ShowMessage
   //   ("GmatConnection::OnStartAdvise() data=%s\n", data);
   //#endif
   
   return TRUE;
}


//------------------------------------------------------------------------------
// bool OnDisconnect()
//------------------------------------------------------------------------------
bool GmatConnection::OnDisconnect()
{
   #ifdef DEBUG_CONNECTION
   MessageInterface::ShowMessage
      ("GmatConnection::OnDisconnect() entered, this=%p\n", this);
   #endif
   return true;
}


//------------------------------------------------------------------------------
// char* RunRequest(const wxString &item)
//------------------------------------------------------------------------------
char* GmatConnection::RunRequest(const wxString &item)
{
   static thread_local std::string reply;
   reply.clear();
   auto value = ReadReply(item);
   if (value.find('\0') != std::string::npos)
      throw InterfaceException("Legacy MATLAB character requests cannot represent embedded NUL data");
   reply.swap(value);
   return reply.data();
}

std::string GmatConnection::ReadReply(const wxString &item)
{
   if (item.empty()) throw InterfaceException("A MATLAB request item must not be empty");
   const auto bytes = item.ToUTF8();
   if (!bytes.data()) throw InterfaceException("Cannot encode the MATLAB request item as UTF-8");
   std::string name(bytes.data(), bytes.length());
   if (name.empty() || name.find('\0') != std::string::npos)
      throw InterfaceException("A MATLAB request item is empty or contains NUL data");
   auto *interface = GmatInterface::Instance();
   interface->CheckUserInterrupt();
   if (name.back() == '.')
   {
      name.pop_back();
      return interface->GetGmatObjectString(name);
   }
   if (name == "RunState") return interface->GetRunState();
   if (name == "CallbackStatus") return interface->GetCallbackStatus();
   if (name == "CallbackResults") return interface->GetCallbackResults();
   return interface->GetParameterString(name);
}


//------------------------------------------------------------------------------
// bool RunPoke(const char *itemData)
//------------------------------------------------------------------------------
bool GmatConnection::RunPoke(const char *itemData)
{
   if (!itemData) return false;
   if (std::strcmp(itemData, "Close;") == 0) return true;
   try
   {

   const char *data = itemData;
   
   #ifdef DEBUG_CONNECTION_POKE
   MessageInterface::ShowMessage
      ("GmatConnection::RunPoke() entered\n   item = '%s'\n", data);
   #endif
   
   //------------------------------
   // save data to string stream
   //------------------------------
   
   if (strcmp(data, "Open;") == 0)
   {
      GmatInterface::Instance()->OpenScript();
   }
   else if (strcmp(data, "Clear;") == 0)
   {
      GmatInterface::Instance()->ClearScript();
   }
   else if (strcmp(data, "Build;") == 0)
   {
      GmatInterface::Instance()->BuildObject();
   }
   else if (strcmp(data, "Update;") == 0)
   {
      GmatInterface::Instance()->UpdateObject();
   }
   else if (strcmp(data, "Build+Run;") == 0)
   {
      GmatInterface::Instance()->BuildObject();
      GmatInterface::Instance()->RunScript();
   }
   else if (strcmp(data, "Run;") == 0)
   {
      GmatInterface::Instance()->RunScript();
   }
   else if (strcmp(data, "Callback;") == 0)
   {
      GmatInterface::Instance()->ExecuteCallback();
   }
   else if (strncmp(data, "CallbackData", 12) == 0)
   {
      std::string theString(data);
      std::string callbackData = theString.substr(13,1024);
      #ifdef DEBUG_CONNECTION_POKE
         MessageInterface::ShowMessage
            ("   Calling PutCallbackData() with\n"
             "   callbackData = '%s'\n", callbackData.c_str());
      #endif
      GmatInterface::Instance()->PutCallbackData(callbackData);
   }
   else
   {
      GmatInterface::Instance()->PutScript((char*)data);
   }
   
   #ifdef DEBUG_CONNECTION_POKE
   MessageInterface::ShowMessage("GmatConnection::RunPoke() returning true\n");
   #endif
   
   return true;

   }
   catch (BaseException &error)
   {
      MessageInterface::ShowMessage("GMAT MATLAB command rejected: %s\n", error.GetFullMessage().c_str());
      return false;
   }
}
