//$Id: MatlabInterface.cpp 9460 2011-04-21 22:03:28Z lindajun $
//------------------------------------------------------------------------------
//                               MatlabInterface
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool.
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of The National Aeronautics and Space Administration.
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
// number NNG04CC06P
//
// Author: Linda Jun (NASA/GSFC)
// Created: 2010/03/31
//
/**
 * Implements MatlabInterface functions. It is a singleton class -
 * only one instance of this class can be created.
 */
//------------------------------------------------------------------------------
#include <stdlib.h>         // for NULL
#include <string.h>         // for memcpy()
#include <sstream>          // for std::stringstream
#include <algorithm>        // for bounded engine-array copies

// Clear a build error with Visual Studio 2010
#ifdef _CHAR16T
#define CHAR16_T
#endif

#include "engine.h"         // for Matlab Engine
#include "matrix.h"         // for Matlab mxArray
#ifdef GMAT_MATLAB_QT_ENGINE
#include "MatlabLibraryExecutor.hpp"
#endif

#ifdef __WXMAC__
#include <stdlib.h>         // for system() to launch X11 application
#include <unistd.h>         // for gethostname()
#endif

#include "MatlabInterface.hpp"
#include "MatlabCallbackBridge.hpp"
#include "MatlabArrayConversion.hpp"
#include "InterfaceException.hpp"
#include "GmatGlobal.hpp"          // for IsMatlabDebugOn()
#include "StringUtil.hpp"          // for DecomposeBy()
#include "MessageInterface.hpp"
#include <memory>
#include <codecvt>
#include <locale>
#include <atomic>
#include <limits>
#include <cmath>
#include <type_traits>
#if defined(_WIN32) || defined(__WIN32__)
#include <process.h>
#else
#include <unistd.h>
#endif

//#define DEBUG_MATLAB_OPEN_CLOSE
//#define DEBUG_MATLAB_PUT_REAL
//#define DEBUG_MATLAB_GET_REAL
//#define DEBUG_MATLAB_GET_STRING
//#define DEBUG_MATLAB_EVAL

namespace
{
std::string MatlabCharacterData(const mxArray *value)
{
   const size_t count = mxGetNumberOfElements(value);
   const mxChar *characters = mxGetChars(value);
   if (count && !characters)
      throw InterfaceException("MATLAB character output has no character data.");
   std::u16string units;
   units.reserve(count);
   for (size_t i = 0; i < count; ++i) units.push_back(static_cast<char16_t>(characters[i]));
   try
   {
      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
      std::string result = converter.to_bytes(units.data(), units.data() + units.size());
      if (converter.converted() != units.size())
         throw InterfaceException("MATLAB character output contains incomplete UTF-16.");
      return result;
   }
   catch (const std::range_error &)
   {
      throw InterfaceException("MATLAB character output contains invalid UTF-16.");
   }
}

template <typename T>
bool MatlabNumericData(const mxArray *value, RealArray &result)
{
   const size_t count = mxGetNumberOfElements(value);
   const T *data = static_cast<const T*>(mxGetData(value));
   if (count && !data)
      throw InterfaceException("MATLAB numeric output has no data.");
   result.reserve(count);
   for (size_t i = 0; i < count; ++i)
   {
      const Real number = static_cast<Real>(data[i]);
      if constexpr (std::is_integral<T>::value && sizeof(T) >= 8)
      {
         // A 64-bit integer need not fit exactly in native Real. Test range
         // before the round-trip cast, avoiding undefined overflow at 2^63/64.
         const Real upper = std::ldexp(1.0, std::numeric_limits<T>::digits);
         const Real lower = std::is_signed<T>::value ? -upper : 0.0;
         if (number < lower || number >= upper || static_cast<T>(number) != data[i])
         {
            result.clear();
            return false;
         }
      }
      result.push_back(number);
   }
   return true;
}

MatlabValue CaptureMatlabValue(const mxArray *value)
{
   MatlabValue result;
   if (!value) { result.className = "unset"; return result; }
   const char *name = mxGetClassName(value);
   result.className = name ? name : "unknown";
   const size_t rows = mxGetM(value), columns = mxGetN(value);
   const size_t count = mxGetNumberOfElements(value);
   const size_t limit = static_cast<size_t>(std::numeric_limits<Integer>::max());
   if (rows > limit || columns > limit || count > limit)
      throw InterfaceException("MATLAB object output exceeds native dimension capacity.");
   result.rows = static_cast<Integer>(rows);
   result.columns = static_cast<Integer>(columns);
   if (mxGetNumberOfDimensions(value) != 2 || mxIsSparse(value) || mxIsComplex(value))
      return result;
   if (mxIsChar(value))
   {
      result.type = MatlabValue::Type::CHARACTER;
      result.text = MatlabCharacterData(value);
   }
   else if (mxIsStruct(value) && count == 1)
   {
      result.type = MatlabValue::Type::STRUCTURE;
      const int fields = mxGetNumberOfFields(value);
      if (fields < 0) throw InterfaceException("MATLAB structure has an invalid field count.");
      result.fieldNames.reserve(static_cast<size_t>(fields));
      result.children.reserve(static_cast<size_t>(fields));
      for (int i = 0; i < fields; ++i)
      {
         const char *field = mxGetFieldNameByNumber(value, i);
         if (!field) throw InterfaceException("MATLAB structure has an unnamed field.");
         result.fieldNames.emplace_back(field);
         result.children.push_back(CaptureMatlabValue(mxGetFieldByNumber(value, 0, i)));
      }
   }
   else if (mxIsCell(value))
   {
      result.type = MatlabValue::Type::CELL;
      result.children.reserve(count);
      for (size_t i = 0; i < count; ++i)
         result.children.push_back(CaptureMatlabValue(mxGetCell(value, i)));
   }
   else
   {
      bool numeric = false;
      switch (mxGetClassID(value))
      {
         case mxDOUBLE_CLASS: numeric = MatlabNumericData<double>(value, result.numbers); break;
         case mxSINGLE_CLASS: numeric = MatlabNumericData<float>(value, result.numbers); break;
         case mxLOGICAL_CLASS: numeric = MatlabNumericData<mxLogical>(value, result.numbers); break;
         case mxINT8_CLASS: numeric = MatlabNumericData<mxInt8>(value, result.numbers); break;
         case mxUINT8_CLASS: numeric = MatlabNumericData<mxUint8>(value, result.numbers); break;
         case mxINT16_CLASS: numeric = MatlabNumericData<mxInt16>(value, result.numbers); break;
         case mxUINT16_CLASS: numeric = MatlabNumericData<mxUint16>(value, result.numbers); break;
         case mxINT32_CLASS: numeric = MatlabNumericData<mxInt32>(value, result.numbers); break;
         case mxUINT32_CLASS: numeric = MatlabNumericData<mxUint32>(value, result.numbers); break;
         case mxINT64_CLASS: numeric = MatlabNumericData<mxInt64>(value, result.numbers); break;
         case mxUINT64_CLASS: numeric = MatlabNumericData<mxUint64>(value, result.numbers); break;
         default: break;
      }
      if (numeric)
         result.type = mxIsLogical(value) ? MatlabValue::Type::LOGICAL : MatlabValue::Type::NUMERIC;
   }
   return result;
}
}

//--------------------------------------
//  initialize static variables
//--------------------------------------
MatlabInterface* MatlabInterface::instance = NULL;
const int MatlabInterface::MAX_OUT_SIZE = 8192;

struct MatlabCallbackState
{
   std::vector<char> output;
   Integer outputSize;
   std::string callingName;
   MatlabCallbackState(Integer capacity,const std::string &name)
      : output(static_cast<size_t>(capacity)+1,0),outputSize(capacity),callingName(name) {}
};

std::shared_ptr<MatlabCallbackState> MatlabInterface::GetCallbackState()
{
   auto state=std::static_pointer_cast<MatlabCallbackState>(MatlabCallbackBridge::LocalState());
   if(!state) {
      state=std::make_shared<MatlabCallbackState>(MAX_OUT_SIZE,callingObjectName);
      MatlabCallbackBridge::LocalState()=state;
   }
   return state;
}


const std::string
MatlabInterface::PARAMETER_TEXT[MatlabInterfaceParamCount - InterfaceParamCount] =
{
   "MatlabMode",
};

const Gmat::ParameterType
MatlabInterface::PARAMETER_TYPE[MatlabInterfaceParamCount - InterfaceParamCount] =
{
   Gmat::INTEGER_TYPE,       //"MatlabMode",
};


//--------------------------------------
//  public functions
//--------------------------------------

//------------------------------------------------------------------------------
// MatlabInterface* Instance()
//------------------------------------------------------------------------------
/*
 * Returns this instance
 */
//------------------------------------------------------------------------------
MatlabInterface* MatlabInterface::Instance()
{
   if (instance == NULL)
      instance = new MatlabInterface("OneInstance");
   
   return instance;
}


//------------------------------------------------------------------------------
// int Open(const std::string &name = "")
//------------------------------------------------------------------------------
/**
 * Opens Matlab engine
 *
 * @param  name  Name used in identifying matlab engine
 * @return  1 = no error, 0 = error
 */
//------------------------------------------------------------------------------
int MatlabInterface::Open(const std::string &name)
{
   if(MatlabCallbackBridge::IsActive()) {
      if(!name.empty() && name!="GmatMatlab")
         throw InterfaceException("A callback must reuse its waiting MATLAB session");
      return matlabMode==NO_MATLAB ? 0 : 1;
   }

#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker()) return MatlabLibraries().Invoke([&]{return Open(name);});
#endif
   if (matlabMode == SINGLE_USE) return OpenSingleEngine(name);
   if (matlabMode != SHARED) return 0;
#ifdef __WXMAC__
   return OpenEngineOnMac();
#else
   return OpenSharedEngine();
#endif
} // end Open()


//------------------------------------------------------------------------------
//  int Close(const std::string &name = "")
//------------------------------------------------------------------------------
/**
 * Closes Matlab engine
 *
 * @param  name  Name used in identifying matlab engine
 * @return  1 = no error, 0 = error
 */
//------------------------------------------------------------------------------
int MatlabInterface::Close(const std::string &name)
{
   if(MatlabCallbackBridge::IsActive())
      throw InterfaceException("Cannot close the waiting MATLAB session from its callback");

#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker()) return MatlabLibraries().Invoke([&]{return Close(name);});
#endif
   if (matlabMode == SINGLE_USE) return CloseSingleEngine(name);
   if (matlabMode != SHARED) return 0;
#ifdef __WXMAC__
   return CloseEngineOnMac();
#else
   return CloseSharedEngine();
#endif
} // end Close()


std::string MatlabInterface::CharacterLiteral(const std::string &value)
{
   std::string result = "['";
   for (unsigned char ch : value)
   {
      if (ch == '\n' || ch == '\r' || ch == 0)
         result += "' char(" + std::to_string(static_cast<unsigned int>(ch)) + ") '";
      else
      {
         result += static_cast<char>(ch);
         if (ch == '\'') result += '\'';
      }
   }
   return result + "']";
}

// Reserve an unused base-workspace name without replacing a user variable.
// The process id separates clients sharing a Windows automation server; the
// serial also separates nested calls and reloads check existing names again.
std::string MatlabInterface::CreateTemporaryVariable()
{
   return CreateTemporaryVariable(StringArray{});
}

std::string MatlabInterface::CreateTemporaryVariable(const StringArray &excluded)
{
#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabCallbackBridge::IsActive() && !MatlabLibraries().OnWorker()) return MatlabLibraries().Invoke([&]{return CreateTemporaryVariable(excluded);});
#endif
   if (!MatlabCallbackBridge::IsActive() && !enginePtr)
      throw MatlabEngineConnectionException("The MATLAB engine is not open for result capture.");
   static std::atomic<unsigned long long> serial{0};
#if defined(_WIN32) || defined(__WIN32__)
   const auto processId = _getpid();
#else
   const auto processId = getpid();
#endif
   for (;;)
   {
      const std::string name = "gmat_io_" + std::to_string(processId) + "_" +
         std::to_string(serial.fetch_add(1, std::memory_order_relaxed));
      if (std::find(excluded.begin(), excluded.end(), name) != excluded.end()) continue;
      if(MatlabCallbackBridge::IsActive()) {
         MatlabCallbackRequest request;request.operation=MatlabCallbackRequest::Operation::RESERVE;request.name=name;
         if(MatlabCallbackBridge::Execute(request).reserved) return name;
         continue;
      }
      std::unique_ptr<mxArray, decltype(&mxDestroyArray)> previous(
         engGetVariable(enginePtr, name.c_str()), &mxDestroyArray);
      if (previous) continue;
      std::unique_ptr<mxArray, decltype(&mxDestroyArray)> placeholder(
         mxCreateDoubleMatrix(0, 0, mxREAL), &mxDestroyArray);
      if (!placeholder)
         throw InterfaceException("Cannot allocate a MATLAB temporary variable.");
      if (engPutVariable(enginePtr, name.c_str(), placeholder.get()) != 0)
         throw MatlabEngineConnectionException("Cannot reserve a MATLAB temporary variable.");
      return name;
   }
}

//------------------------------------------------------------------------------
// void SetCallingObjectName(const std::string &calledFrom)
//------------------------------------------------------------------------------
void MatlabInterface::SetCallingObjectName(const std::string &calledFrom)
{
   if(MatlabCallbackBridge::IsActive()) { GetCallbackState()->callingName=calledFrom;return; }

#ifdef GMAT_MATLAB_QT_ENGINE
   MatlabLibraries().EnsureIdle();
#endif
   callingObjectName = calledFrom;
   #ifdef DEBUG_CALLED_FROM
   MessageInterface::ShowMessage
      ("MatlabInterface::SetCallingObjectName() calledFrom '%s'\n",
       calledFrom.c_str());
   #endif
}


//------------------------------------------------------------------------------
// std::string GetCallingObjectName()
//------------------------------------------------------------------------------
std::string MatlabInterface::GetCallingObjectName()
{
   if(MatlabCallbackBridge::IsActive()) return GetCallbackState()->callingName;

#ifdef GMAT_MATLAB_QT_ENGINE
   MatlabLibraries().EnsureIdle();
#endif
   return callingObjectName;
}


//------------------------------------------------------------------------------
// int PutRealArray(const std::string &matlabVarName, int numRow, int numCol,
//                  const double *inArray)
//------------------------------------------------------------------------------
/**
 * Put arrays to Matlab workspace.
 *
 * @param <matlabVarName> variable name in the MATLAB workspace
 * @param <numRows> number of rows in input array
 * @param <numCols> number of columns in input array
 * @return 1 = no error, 0 = error
 */
//------------------------------------------------------------------------------
int MatlabInterface::PutRealArray(const std::string &matlabVarName,
                                  int numRows, int numCols, const double *inArray)
{
   if(MatlabCallbackBridge::IsActive()) {
      if(numRows<0 || numCols<0) throw InterfaceException("MATLAB numeric input dimensions cannot be negative.");
      const size_t rows=static_cast<size_t>(numRows),columns=static_cast<size_t>(numCols);
      const size_t limit=static_cast<size_t>(std::numeric_limits<Integer>::max());
      if(columns && rows>limit/columns) throw InterfaceException("MATLAB numeric input exceeds native capacity.");
      const size_t count=rows*columns;
      if(count && !inArray) throw InterfaceException("MATLAB numeric input has no data.");
      MatlabValue value;value.type=MatlabValue::Type::NUMERIC;value.className="double";
      value.rows=numRows;value.columns=numCols;value.numbers.resize(count);
      for(size_t row=0;row<rows;++row) for(size_t column=0;column<columns;++column)
         value.numbers[column*rows+row]=inArray[row*columns+column];
      return PutValue(matlabVarName,value);
   }

#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker()) return MatlabLibraries().Invoke([&]{return PutRealArray(matlabVarName,numRows,numCols,inArray);});
#endif
   if (!enginePtr)
      throw MatlabEngineConnectionException("The MATLAB engine is not open for numeric input.");
   if (numRows < 0 || numCols < 0)
      throw InterfaceException("MATLAB numeric input dimensions cannot be negative.");
   const size_t rows = static_cast<size_t>(numRows);
   const size_t columns = static_cast<size_t>(numCols);
   if (columns && rows > std::numeric_limits<size_t>::max() / columns)
      throw InterfaceException("MATLAB numeric input dimensions exceed native capacity.");
   const size_t count = rows * columns;
   if (count && !inArray)
      throw InterfaceException("MATLAB numeric input has no data.");
   std::unique_ptr<mxArray, decltype(&mxDestroyArray)> value(
      mxCreateDoubleMatrix(rows, columns, mxREAL), &mxDestroyArray);
   if (!value)
      throw InterfaceException("Cannot allocate a MATLAB numeric input array.");
   double *data = mxGetPr(value.get());
   if (count && !data)
      throw InterfaceException("Cannot access a MATLAB numeric input array.");
   // GMAT's source matrix is row-major; MATLAB stores columns contiguously.
   for (size_t row = 0; row < rows; ++row)
      for (size_t column = 0; column < columns; ++column)
         data[column * rows + row] = inArray[row * columns + column];
   if (engPutVariable(enginePtr, matlabVarName.c_str(), value.get()) != 0)
      throw MatlabEngineConnectionException("Failed to send numeric input to the MATLAB engine.");
   return 1;
} // end PutRealArray()


//------------------------------------------------------------------------------
// int GetRealArray(const std::string &matlabVarName, int numElements,
//                  double outArray[], Integer &numRowsReceived, Integer &numColsReceived)
//------------------------------------------------------------------------------
/**
 * Get arrays from Matlab workspace.
 *
 * @param <matlabVarName> variable name in the MATLAB workspace
 * @param <numElements> number of elements to receive from MATLAB
 * @param <outArray> array to receive double array from MATLAB
 * @param <numRowsReceived> row dimension of received array to return
 * @param <numColsReceived> column dimension of received array to return
 * @Return 0 on error; else number of elemnets received from MATLAB
 * @exception <InterfaceException> thrown if empty output was received
 */
//------------------------------------------------------------------------------
int MatlabInterface::GetRealArray(const std::string &matlabVarName,
                                  int numElements, double outArray[],
                                  Integer &numRowsReceived, Integer &numColsReceived)
{
   if(MatlabCallbackBridge::IsActive()) {
      numRowsReceived=numColsReceived=0;
      if(numElements<=0 || !outArray) return 0;
      MatlabCallbackRequest request;request.operation=MatlabCallbackRequest::Operation::FETCH;request.name=matlabVarName;
      auto reply=MatlabCallbackBridge::Execute(request);if(!reply.found) return 0;
      const auto &value=reply.value;numRowsReceived=value.rows;numColsReceived=value.columns;
      if(value.type==MatlabValue::Type::NUMERIC && value.className=="double") {
         if(value.numbers.empty()) throw InterfaceException("Received empty real output from MATLAB");
         std::copy_n(value.numbers.data(),std::min(static_cast<size_t>(numElements),value.numbers.size()),outArray);
         return static_cast<Integer>(value.numbers.size());
      }
      if(value.type==MatlabValue::Type::LOGICAL && value.numbers.size()==1) {outArray[0]=value.numbers[0];return 1;}
      return 0;
   }

#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker()) return MatlabLibraries().Invoke([&]{return GetRealArray(matlabVarName,numElements,outArray,numRowsReceived,numColsReceived);});
#endif
   numRowsReceived = 0;
   numColsReceived = 0;
   if (numElements <= 0 || !outArray) return 0;
   if (!enginePtr)
      throw MatlabEngineConnectionException("The MATLAB engine is not open for numeric output.");
   std::unique_ptr<mxArray, decltype(&mxDestroyArray)> value(
      engGetVariable(enginePtr, matlabVarName.c_str()), &mxDestroyArray);
   if (!value) return 0;
   const size_t rows = mxGetM(value.get());
   const size_t columns = mxGetN(value.get());
   const size_t count = mxGetNumberOfElements(value.get());
   const size_t limit = static_cast<size_t>(std::numeric_limits<Integer>::max());
   if (rows > limit || columns > limit || count > limit)
      throw InterfaceException("MATLAB numeric output exceeds native dimension capacity.");
   numRowsReceived = static_cast<Integer>(rows);
   numColsReceived = static_cast<Integer>(columns);
   if (mxIsDouble(value.get()) && !mxIsComplex(value.get()) &&
       !mxIsSparse(value.get()) && mxGetNumberOfDimensions(value.get()) == 2)
   {
      const double *data = mxGetPr(value.get());
      if (!data)
         throw InterfaceException("Received empty real output from MATLAB");
      const size_t copyCount = std::min(static_cast<size_t>(numElements), count);
      std::copy(data, data + copyCount, outArray);
      return static_cast<Integer>(count);
   }
   // Preserve the native logical-scalar compatibility; arrays need doubles.
   if (mxIsLogicalScalar(value.get()))
   {
      outArray[0] = mxIsLogicalScalarTrue(value.get()) ? 1.0 : 0.0;
      return 1;
   }
   return 0;
} // end GetRealArray()


//------------------------------------------------------------------------------
// int GetString(const std::string &matlabVarName, std::string &outStr)
//------------------------------------------------------------------------------
/**
 * Get char array from Matlab workspace.
 *
 * @param <matlabVarName> variable name in the MATLAB workspace
 * @param <outStr> string to receive char array from MATLAB
 * @Return 1 if matlab string variable found, 0 otherwise
 * @exception <InterfaceException> thrown if empty output was received
 */
//------------------------------------------------------------------------------
int MatlabInterface::GetString(const std::string &matlabVarName,
                               std::string &outStr)
{
   if(MatlabCallbackBridge::IsActive()) {
      MatlabCallbackRequest request;request.operation=MatlabCallbackRequest::Operation::FETCH_CHARACTER;request.name=matlabVarName;
      auto reply=MatlabCallbackBridge::Execute(request);
      if(!reply.found || reply.value.type!=MatlabValue::Type::CHARACTER) return 0;
      outStr=std::move(reply.value.text);return 1;
   }

#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker()) return MatlabLibraries().Invoke([&]{return GetString(matlabVarName,outStr);});
#endif
   #ifdef DEBUG_MATLAB_GET_STRING
   MessageInterface::ShowMessage
      ("MatlabInterface::GetString() entered, matlabVarName='%s', outStr='%s'\n",
       matlabVarName.c_str(), outStr.c_str());
   #endif

   if (!enginePtr)
      throw MatlabEngineConnectionException("The MATLAB engine is not open for character output.");
   std::unique_ptr<mxArray, decltype(&mxDestroyArray)> value(
      engGetVariable(enginePtr, matlabVarName.c_str()), &mxDestroyArray);
   if (!value || !mxIsChar(value.get())) return 0;
   std::string result = MatlabCharacterData(value.get());
   outStr.swap(result);
   return 1;
} // end GetString()

namespace
{
using MatlabArrayOwner = std::unique_ptr<mxArray, decltype(&mxDestroyArray)>;

MatlabArrayOwner MakeMatlabInput(const MatlabValue &value)
{
   if (value.type == MatlabValue::Type::CHARACTER)
   {
      // Native strings use a canonical MATLAB character row. Decode the full
      // byte range so embedded NUL and non-ASCII text never enter source code.
      std::u16string units;
      try
      {
         std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
         units = converter.from_bytes(value.text.data(), value.text.data() + value.text.size());
         if (converter.converted() != value.text.size())
            throw InterfaceException("MATLAB character input contains incomplete UTF-8.");
      }
      catch (const std::range_error &)
      { throw InterfaceException("MATLAB character input contains invalid UTF-8."); }
      if (units.size() > static_cast<size_t>(std::numeric_limits<Integer>::max()))
         throw InterfaceException("MATLAB character input exceeds native dimension capacity.");
      const mwSize dimensions[2] = {units.empty() ? mwSize(0) : mwSize(1),
                                    static_cast<mwSize>(units.size())};
      MatlabArrayOwner result(mxCreateCharArray(2, dimensions), &mxDestroyArray);
      if (!result) throw InterfaceException("Cannot allocate MATLAB character input.");
      mxChar *data = mxGetChars(result.get());
      if (!units.empty() && !data) throw InterfaceException("Cannot access MATLAB character input.");
      for (size_t i = 0; i < units.size(); ++i) data[i] = static_cast<mxChar>(units[i]);
      return result;
   }
   if (value.rows < 0 || value.columns < 0)
      throw InterfaceException("MATLAB input dimensions cannot be negative.");
   const size_t rows = static_cast<size_t>(value.rows), columns = static_cast<size_t>(value.columns);
   const size_t limit = static_cast<size_t>(std::numeric_limits<Integer>::max());
   if (columns && rows > limit / columns)
      throw InterfaceException("MATLAB input exceeds native element capacity.");
   const size_t count = rows * columns;
   if (value.type == MatlabValue::Type::NUMERIC || value.type == MatlabValue::Type::LOGICAL)
   {
      if (count != value.numbers.size()) throw InterfaceException("MATLAB input data does not match its dimensions.");
      MatlabArrayOwner result(value.type == MatlabValue::Type::LOGICAL ?
         mxCreateLogicalMatrix(rows, columns) : mxCreateDoubleMatrix(rows, columns, mxREAL), &mxDestroyArray);
      if (!result) throw InterfaceException("Cannot allocate MATLAB numeric input.");
      if (value.type == MatlabValue::Type::LOGICAL)
      {
         mxLogical *data = static_cast<mxLogical*>(mxGetData(result.get()));
         if (count && !data) throw InterfaceException("Cannot access MATLAB logical input.");
         for (size_t i = 0; i < count; ++i)
         {
            if (value.numbers[i] != 0.0 && value.numbers[i] != 1.0)
               throw InterfaceException("MATLAB logical input must contain zero or one.");
            data[i] = value.numbers[i] == 1.0;
         }
      }
      else
      {
         double *data = mxGetPr(result.get());
         if (count && !data) throw InterfaceException("Cannot access MATLAB numeric input.");
         for (size_t i = 0; i < count; ++i) data[i] = value.numbers[i];
      }
      return result;
   }
   if (value.type == MatlabValue::Type::CELL)
   {
      if (count != value.children.size()) throw InterfaceException("MATLAB cell data does not match its dimensions.");
      const mwSize dimensions[2] = {static_cast<mwSize>(rows), static_cast<mwSize>(columns)};
      MatlabArrayOwner result(mxCreateCellArray(2, dimensions), &mxDestroyArray);
      if (!result) throw InterfaceException("Cannot allocate MATLAB cell input.");
      for (size_t i = 0; i < count; ++i)
      {
         auto child = MakeMatlabInput(value.children[i]);
         mxSetCell(result.get(), i, child.release());
      }
      return result;
   }
   if (value.type == MatlabValue::Type::STRUCTURE)
   {
      if (count != 1 || value.children.size() != value.fieldNames.size() || value.fieldNames.size() > limit)
         throw InterfaceException("MATLAB structure input must be scalar with matching named fields.");
      std::vector<const char*> fields;
      for (const auto &name : value.fieldNames) fields.push_back(name.c_str());
      MatlabArrayOwner result(mxCreateStructMatrix(1, 1, static_cast<int>(fields.size()), fields.data()), &mxDestroyArray);
      if (!result) throw InterfaceException("Cannot allocate MATLAB structure input.");
      for (size_t i = 0; i < value.children.size(); ++i)
      {
         auto child = MakeMatlabInput(value.children[i]);
         mxSetFieldByNumber(result.get(), 0, static_cast<int>(i), child.release());
      }
      return result;
   }
   throw InterfaceException("Unsupported typed MATLAB input value.");
}
}

mxArray *CreateMatlabArray(const MatlabValue &value)
{
#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker())
      throw InterfaceException("Native MATLAB arrays must be created on the library worker");
#endif
   return MakeMatlabInput(value).release();
}

Integer MatlabInterface::PutValue(const std::string &matlabVarName, const MatlabValue &value)
{
   if(MatlabCallbackBridge::IsActive()) {
      MatlabCallbackRequest request;request.operation=MatlabCallbackRequest::Operation::STORE;
      request.name=matlabVarName;request.value=value;MatlabCallbackBridge::Execute(request);return 1;
   }

#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker())
      return MatlabLibraries().Invoke([&] { return PutValue(matlabVarName, value); });
#endif
   if (!enginePtr) throw MatlabEngineConnectionException("The MATLAB engine is not open for typed input.");
   auto input = MakeMatlabInput(value);
   if (engPutVariable(enginePtr, matlabVarName.c_str(), input.get()) != 0)
      throw MatlabEngineConnectionException("Cannot transfer typed MATLAB input: " + matlabVarName);
   return 1;
}

MatlabValue MatlabInterface::GetValue(const std::string &matlabVarName)
{
   if(MatlabCallbackBridge::IsActive()) {
      MatlabCallbackRequest request;request.operation=MatlabCallbackRequest::Operation::FETCH;request.name=matlabVarName;
      auto reply=MatlabCallbackBridge::Execute(request);
      if(!reply.found) throw InterfaceException("MATLAB object output is missing: "+matlabVarName);
      return std::move(reply.value);
   }

#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker())
      return MatlabLibraries().Invoke([&] { return GetValue(matlabVarName); });
#endif
   if (!enginePtr)
      throw MatlabEngineConnectionException("The MATLAB engine is not open for object output.");
   std::unique_ptr<mxArray, decltype(&mxDestroyArray)> value(
      engGetVariable(enginePtr, matlabVarName.c_str()), &mxDestroyArray);
   if (!value)
      throw InterfaceException("MATLAB object output is missing: " + matlabVarName);
   return CaptureMatlabValue(value.get());
}



//------------------------------------------------------------------------------
//  int EvalString(const std::string &evalString)
//------------------------------------------------------------------------------
//  Purpose:
//     Evaluates matlab string.
//
//  Returns:
//     0, if string evaluated was successful
//     nonzero, if unsuccessful
//------------------------------------------------------------------------------
int MatlabInterface::EvalString(const std::string &evalString)
{
   if(MatlabCallbackBridge::IsActive()) {
      auto state=GetCallbackState();
      MatlabCallbackRequest request;request.operation=MatlabCallbackRequest::Operation::EVALUATE;request.expression=evalString;
      auto reply=MatlabCallbackBridge::Execute(request);
      std::fill(state->output.begin(),state->output.end(),0);
      std::copy_n(reply.output.data(),std::min(reply.output.size(),static_cast<size_t>(state->outputSize)),state->output.data());
      return 0;
   }

#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker()) return MatlabLibraries().Invoke([&]{return EvalString(evalString);});
#endif
   #ifdef DEBUG_MATLAB_EVAL
   MessageInterface::ShowMessage
      ("MatlabInterface::EvalString() calling engEvalString with\n"
       "======================================================================\n"
       "%s\n\n", evalString.c_str());
   #endif
   
   // return value is 0 if the command was evaluated by the MATLAB engine,
   // and a nonzero value if unsuccessful. Possible reasons for failure include
   // the engine session is no longer running or the engine pointer is invalid or NULL.
   int retval = engEvalString(enginePtr, evalString.c_str());
   
   #ifdef DEBUG_MATLAB_EVAL
   MessageInterface::ShowMessage("MatlabInterface::EvalString() exiting with %d\n", retval);
   #endif
   return retval;
}


//------------------------------------------------------------------------------
//  int SetOutputBuffer(int size)
//------------------------------------------------------------------------------
/**
 * Sets outBuffer to input size. All result using EvalString() will use this
 * buffer.
 *
 * @param size   size of the buffer, it will use size-1.
 *
 * @return size of the buffer used, 0 = error
 */
//------------------------------------------------------------------------------
int MatlabInterface::SetOutputBuffer(int size)
{
   if(MatlabCallbackBridge::IsActive()) {
      auto state=GetCallbackState();std::fill(state->output.begin(),state->output.end(),0);
      state->outputSize=size>1 ? std::min(size-1,MAX_OUT_SIZE) : 0;return state->outputSize;
   }

#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker()) return MatlabLibraries().Invoke([&]{return SetOutputBuffer(size);});
#endif
   // The C engine does not promise to terminate captured output. Clear the
   // entire storage so shorter captures cannot expose bytes from an old call.
   std::fill(outBuffer, outBuffer + MAX_OUT_SIZE + 1, '\0');
   if (!enginePtr || size <= 1)
      return 0;
   int sizeToUse = size-1;

   if (sizeToUse > MAX_OUT_SIZE)
      sizeToUse = MAX_OUT_SIZE;

   return engOutputBuffer(enginePtr, outBuffer, sizeToUse) == 0 ? sizeToUse : 0;
}


//------------------------------------------------------------------------------
// char* GetOutputBuffer()
//------------------------------------------------------------------------------
/*
 * Returns Matlab output buffer pointer which has a pointer to Matlab result
 * using EvalStaring()
 */
//------------------------------------------------------------------------------
char* MatlabInterface::GetOutputBuffer()
{
   if(MatlabCallbackBridge::IsActive()) return GetCallbackState()->output.data();

#ifdef GMAT_MATLAB_QT_ENGINE
   MatlabLibraries().EnsureIdle();
#endif
   return outBuffer;
}


//------------------------------------------------------------------------------
// bool IsOpen(const std::string &name = "")
//------------------------------------------------------------------------------
/**
 * Checks if engine is open. If name is blank for single use, it will
 * return true if there are more than one engine is opened.
 */
//------------------------------------------------------------------------------
bool MatlabInterface::IsOpen(const std::string &name)
{
   if(MatlabCallbackBridge::IsActive()) return matlabMode!=NO_MATLAB && (name.empty() || name=="GmatMatlab");

#ifdef GMAT_MATLAB_QT_ENGINE
   if (!MatlabLibraries().OnWorker()) return MatlabLibraries().Invoke([&]{return IsOpen(name);});
#endif
   #ifdef DEBUG_MATLAB_OPEN_CLOSE
   MessageInterface::ShowMessage
      ("MatlabInterface::IsOpen() entered, name='%s', matlabMode=%d, enginePtr=<%p>\n",
       name.c_str(), matlabMode, enginePtr);
   #endif
   
   bool isOpen = false;
   if (matlabMode == SINGLE_USE)
   {
      if (name != "")
      {
         std::map<std::string, Engine*>::iterator pos = matlabEngineMap.find(name);
         if (pos == matlabEngineMap.end())
            //return false;
            isOpen = false;
         else
            //return true;
            isOpen = true;
      }
      else
      {
         if (matlabEngineMap.empty())
            //return false;
            isOpen = false;
         else
            //return true;
            isOpen = true;
      }
   }
   else
   {
      //return (MatlabInterface::enginePtr != NULL);
      isOpen = (MatlabInterface::enginePtr != NULL);
   }
   
   #ifdef DEBUG_MATLAB_OPEN_CLOSE
   MessageInterface::ShowMessage
      ("MatlabInterface::IsOpen() returning %d\n", isOpen);
   #endif
   
   return isOpen;
}


//------------------------------------------------------------------------------
// void RunMatlabString(const std::string &evalString)
//------------------------------------------------------------------------------
void MatlabInterface::RunMatlabString(const std::string &evalString)
{
#ifdef GMAT_MATLAB_QT_ENGINE
   if(!MatlabCallbackBridge::IsActive()) MatlabLibraries().EnsureIdle();
#endif
   #ifdef DEBUG_MATLAB_EVAL
   MessageInterface::ShowMessage
      ("MatlabInterface::RunMatlabString() entered\n"
       "======================================================================\n"
       "%s\n\n", evalString.c_str());
   #endif
   
   if (!IsOpen())
   {
      // Let's try to open it first (loj: 2008.03.06)
      if (Open() == 0)
         throw InterfaceException("**** ERROR **** Failed to open MATLAB engine\n");
   }
   
   const auto callingName=GetCallingObjectName();
   std::string newEvalStr = evalString;
   
   // Internal directory/path operations use CharacterLiteral and functional
   // MATLAB syntax. Execute other expressions as supplied; token splitting
   // here used to truncate command-form paths containing whitespace.
   // Track this evaluation independently of any preceding error message.
   // Do not use a scratch name referenced by the expression's inputs/outputs.
   std::string statusName;
   for (size_t suffix = 0; ; ++suffix)
   {
      statusName = "gmat_eval_status_" + std::to_string(suffix);
      if (newEvalStr.find(statusName) == std::string::npos) break;
   }
   newEvalStr = "try,\n  beep off;\n  " + newEvalStr + "\n  " + statusName +
      " = 0;\ncatch\n  errormsg = lasterr;\n  " + statusName + " = 1;\nend";
   
   bool errorReturned = false;
   std::string errorStr;
   // call to evaluate string
   // Initialize separately: a syntax error can prevent the entire try/catch
   // command from parsing. Such an evaluation must not reuse an old success.
   if (EvalString(statusName + " = -1;") == 0 && EvalString(newEvalStr) == 0)
   {
      // if there was an error throw an exception
      #ifdef DEBUG_MATLAB_EVAL
      MessageInterface::ShowMessage("   Now checking if there was an error\n");
      #endif
      Real status = -1;
      Integer rows = 0, columns = 0;
      if (GetRealArray(statusName, 1, &status, rows, columns) != 1 ||
          rows != 1 || columns != 1 || (status != 0 && status != 1))
      {
         errorReturned = true;
         errorStr = "MATLAB evaluation did not report completion";
      }
      else if (status == 1)
      {
         if (GetString("errormsg", errorStr) != 1)
            errorStr = "MATLAB reported an error without a character message";
         #ifdef DEBUG_MATLAB_EVAL
         MessageInterface::ShowMessage
            ("Error occurred in Matlab function '%s'\n'%s'\n",
             callingName.c_str(), errorStr.c_str());
         #endif
         errorReturned = true;
      }
      #ifdef DEBUG_MATLAB_EVAL
      MessageInterface::ShowMessage("   No error found\n");
      #endif
   }
   else
   {
      throw MatlabEngineConnectionException("\"" + callingName +
         ".m\", Error encountered during engEvalString of \"" + evalString + "\"");
   }
   
   if (errorReturned)
   {
      throw InterfaceException("\"" + callingName + ".m\", " + errorStr);
   }
   
   #ifdef DEBUG_MATLAB_EVAL
   MessageInterface::ShowMessage("MatlabInterface::RunMatlabString() exiting\n\n");
   #endif
}


//------------------------------------------------------------------------------
// void SetMatlabMode(int mode)
//------------------------------------------------------------------------------
/**
 * Sets matlab run mode.
 *
 * @param  mode  The matlab run mode
 *               SINGLE_USE, SHARED or NO_MATLAB
 */
//------------------------------------------------------------------------------
void MatlabInterface::SetMatlabMode(int mode)
{
   if(MatlabCallbackBridge::IsActive()) {
      if(mode!=matlabMode) throw InterfaceException("Cannot change a waiting MATLAB callback session mode");
      return;
   }

#ifdef GMAT_MATLAB_QT_ENGINE
   MatlabLibraries().EnsureIdle();
#endif
   if (mode != SINGLE_USE && mode != SHARED && mode != NO_MATLAB)
      throw InterfaceException("Invalid MATLAB engine mode");
#if !defined(_WIN32) && !defined(__WIN32__)
   if (mode == SINGLE_USE)
      throw InterfaceException("MATLAB engOpenSingleUse mode is supported only on Windows.");
#endif
   if (mode == matlabMode)
      return;
   // Handles belong to the mode that opened them. Changing dispatch while an
   // engine is live can double-close it or leave named engines unreachable.
   if (enginePtr || !matlabEngineMap.empty())
      throw InterfaceException("Close MATLAB engines before changing engine mode");
   matlabMode = mode;
}

//------------------------------------------------------------------------------
// int GetMatlabMode()
//------------------------------------------------------------------------------
/**
 * Return matlab run mode.
 *
 * @return  SINGLE_USE, SHARED or NO_MATLAB
 */
//------------------------------------------------------------------------------
int MatlabInterface::GetMatlabMode()
{
   if(MatlabCallbackBridge::IsActive()) return matlabMode;

#ifdef GMAT_MATLAB_QT_ENGINE
   MatlabLibraries().EnsureIdle();
#endif
   return matlabMode;
}


//------------------------------------------------------------------------------
//  GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the MatlabInterface.
 *
 * @return clone of the MatlabInterface.
 */
//------------------------------------------------------------------------------
GmatBase* MatlabInterface::Clone() const
{
#ifdef GMAT_MATLAB_QT_ENGINE
   MatlabLibraries().EnsureIdle();
#endif
   return (new MatlabInterface(*this));
}


//---------------------------------------------------------------------------
//  void Copy(const GmatBase* orig)
//---------------------------------------------------------------------------
/**
 * Sets this object to match another one.
 * 
 * @param orig The original that is being copied.
 */
//---------------------------------------------------------------------------
void MatlabInterface::Copy(const GmatBase* orig)
{
   operator=(*((MatlabInterface *)(orig)));
}


//--------------------------------------
//  private functions
//--------------------------------------

//------------------------------------------------------------------------------
//  <constructor>
//  MatlabInterface()
//------------------------------------------------------------------------------
MatlabInterface::MatlabInterface(const std::string &name) :
   Interface ("MatlabInterface", name)
{
   enginePtr = 0;
   accessCount = 0;
   matlabMode = SHARED;
   outBuffer = new char[MAX_OUT_SIZE+1];
   for (int i=0; i<=MAX_OUT_SIZE; i++)
      outBuffer[i] = '\0';
   debugMatlabEngine = GmatGlobal::Instance()->IsMatlabDebugOn();
}


//------------------------------------------------------------------------------
//  <destructor>
//  MatlabInterface()
//------------------------------------------------------------------------------
MatlabInterface::~MatlabInterface()
{
   if (enginePtr != NULL || !matlabEngineMap.empty())
      Close();
   delete [] outBuffer;
}


//------------------------------------------------------------------------------
// MatlabInterface(const MatlabInterface &mi)
//------------------------------------------------------------------------------
MatlabInterface::MatlabInterface(const MatlabInterface &mi) :
   Interface (mi),
   enginePtr (NULL),
   callingObjectName (mi.callingObjectName),
   accessCount (0),
   matlabMode (mi.matlabMode),
   outBuffer (new char[MAX_OUT_SIZE+1]()),
   debugMatlabEngine (mi.debugMatlabEngine)
{
}


//------------------------------------------------------------------------------
// MatlabInterface& operator=(const MatlabInterface& mi)
//------------------------------------------------------------------------------
MatlabInterface& MatlabInterface::operator=(const MatlabInterface& mi)
{
   if (&mi != this)
   {
#ifdef GMAT_MATLAB_QT_ENGINE
      MatlabLibraries().EnsureIdle();
#endif
      if (enginePtr || !matlabEngineMap.empty())
         throw InterfaceException("Close MATLAB engines before replacing interface configuration");
      Interface::operator=(mi);
      callingObjectName = mi.callingObjectName;
      matlabMode = mi.matlabMode;
      debugMatlabEngine = mi.debugMatlabEngine;
      accessCount = 0;
      lastEngineName.clear();
      outBuffer[0] = '\0';
   }
   
   return *this;
}


//------------------------------------------------------------------------------
// int OpenEngineOnMac()
//------------------------------------------------------------------------------
/**
 * Opens Matlab engine on Mac.
 *
 * @return  1 = no error, 0 = error
 */
//------------------------------------------------------------------------------
int MatlabInterface::OpenEngineOnMac()
{
#ifdef __WXMAC__
   if (debugMatlabEngine)
   {
      MessageInterface::ShowMessage
         ("MatlabInterface::OpenEngineOnMac() enginePtr=%p\n", enginePtr);
   }
   
   if (enginePtr == NULL)
      MessageInterface::ShowMessage("Please wait while MATLAB engine opens...\n");
   
   /// Check if MATLAB engine is still running then doesn't need to re-launch
   if (enginePtr != NULL)
   {
      accessCount = 1;
      if (debugMatlabEngine)
      {
         MessageInterface::ShowMessage
            ("Attempting to reopen MATLAB engine ... accessCount = %d\n",
             accessCount);
      }
      return 1;
   }
   
//   // open the X11 application before launching the matlab
//   system("open -a X11");
   // need to get IP address or hostname here
   char hName[128];
   int OK = gethostname(hName, 128);
   
   if (OK != 0)
   {
      //MessageInterface::ShowMessage("Error getting host name\n");
      return 0;
   }
   
   std::string hNameStr(hName);
//   std::string runString = "matlab -maci ";  // for 32-bit ONLY for now!!!
   // do we want -nodisplay here as well??
   std::string runString = "matlab -nosplash -nodesktop";  // for 64-bit ONLY for now!!!
   Engine *opened = engOpen(runString.c_str());
   if (opened)
   {
      if (engEvalString(opened, "format long") != 0)
      {
         engClose(opened);
         return 0;
      }
      enginePtr = opened;
      MessageInterface::ShowMessage(
               "Successfully opened MATLAB engine using startcmd \"%s\"\n",
               runString.c_str());
      
      accessCount = 1;
      if (debugMatlabEngine)
      {
         MessageInterface::ShowMessage
            ("Attempting to open MATLAB engine ... accessCount = %d\n",
             accessCount);
      }
      //engSetVisible(enginePtr,1);  // rats!
      return 1;
   }
   else
   {
      MessageInterface::ShowMessage(
               "Failed to open MATLAB engine using startcmd \"%s\"\n",
               runString.c_str());
      return 0;
   }
#else
   return 0;
#endif
}


//------------------------------------------------------------------------------
// int CloseEngineOnMac()
//------------------------------------------------------------------------------
/**
 * Closes shared MATLAB engine on Mac.
 *
 * @return 1 if successfully closed, 0 otherwise
 */
//------------------------------------------------------------------------------
int MatlabInterface::CloseEngineOnMac()
{
#ifdef __WXMAC__

   int retval = 1; // set to success
   
   // Check if MATLAB engine is still running then close it.
   if (enginePtr != NULL)
   {
      if (debugMatlabEngine)
      {
         MessageInterface::ShowMessage
            ("MatlabInterface::Close() enginePtr=%p\n", enginePtr);
      }
      
      accessCount = 0;
      
      if (debugMatlabEngine)
      {
         MessageInterface::ShowMessage
            ("Attempting to close MATLAB engine ... accessCount = %d\n",
             accessCount);
      }
      
//      // need to close X11 here ------------ **** TBD ****
//      MessageInterface::ShowMessage
//         ("Closing MATLAB engine ... please close X11 ...\n");
      
      //==============================================================
      // int engClose(Engine *ep);
      // 0 on success, and 1 otherwise. Possible failure includes
      // attempting to terminate a MATLAB engine that was
      // already terminated.
      //==============================================================
      retval = engClose(enginePtr);
      if (retval == 0)
      {
         retval = 1;
         MessageInterface::ShowMessage("MATLAB engine successfully closed\n");
      }
      else
      {
         retval = 0;;
         MessageInterface::ShowMessage("\nError closing MATLAB\n");
      }
      
      enginePtr = NULL;      // set to NULL, so it can be reopened
   }
   else
   {
      if (debugMatlabEngine)
      {
         MessageInterface::ShowMessage
            ("\nUnable to close MATLAB engine because it is not currently running\n");
      }
   }
   return retval;
#else
   return 0;
#endif
}


//------------------------------------------------------------------------------
// int OpenSharedEngine()
//------------------------------------------------------------------------------
/**
 * Opens shared Matlab engine on Windows.
 *
 * @return  1 = no error, 0 = error
 */
//------------------------------------------------------------------------------
int MatlabInterface::OpenSharedEngine()
{
   if (debugMatlabEngine)
      MessageInterface::ShowMessage
         ("MatlabInterface::OpenSharedEngine() enginePtr=%p\n", enginePtr);
   
   if (enginePtr == NULL)
   {
      MessageInterface::ShowMessage("Please wait while MATLAB engine opens...\n");
   }
   else
   {
      if (debugMatlabEngine)
      {
         MessageInterface::ShowMessage("Connecting to current MATLAB engine\n");
         MessageInterface::ShowMessage
            ("MatlabInterface::OpenSharedEngine() returning 1\n");;
      }
      
      return 1;
   }
   
   // open new MATLAB engine
   if ((enginePtr = engOpen(NULL)))
   {
      accessCount = 1;
      
      if (debugMatlabEngine)
      {
         MessageInterface::ShowMessage
            ("Attempting to open MATLAB engine ... accessCount = %d, "
             "enginePtr=%p\n", accessCount, enginePtr);
      }
      
      // set precision to long
      if (EvalString("format long") != 0)
      {
         CloseSharedEngine();
         return 0;
      }
      MessageInterface::ShowMessage("MATLAB engine successfully opened\n");
      if (debugMatlabEngine)
         MessageInterface::ShowMessage
            ("MatlabInterface::OpenSharedEngine() returning 1\n");;
      return 1;
   }
   else
   {
      MessageInterface::ShowMessage("Failed to open MATLAB engine ...\n");
      if (debugMatlabEngine)
         MessageInterface::ShowMessage
            ("MatlabInterface::OpenSharedEngine() returning 0\n");;
      return 0;
   }
}


//------------------------------------------------------------------------------
// int CloseSharedEngine()
//------------------------------------------------------------------------------
/**
 * Closes shared MATLAB engine on Windows.
 *
 * @return 1 if successfully closed, 0 otherwise
 */
//------------------------------------------------------------------------------
int MatlabInterface::CloseSharedEngine()
{
   #ifdef DEBUG_MATLAB_OPEN_CLOSE
   MessageInterface::ShowMessage
      ("MatlabInterface::CloseSharedEngine() entered, enginePtr=<%p>\n", enginePtr);
   #endif
   
   int retval = 0; // set to failed
   
   // Check if MATLAB engine is still running then close it.
   if (enginePtr != NULL)
   {
      if (debugMatlabEngine)
      {
         MessageInterface::ShowMessage
            ("MatlabInterface::CloseSharedEngine() enginePtr=%p\n", enginePtr);
      }
      
      accessCount = 0;
      
      if (debugMatlabEngine)
      {
         MessageInterface::ShowMessage
            ("Attempting to close MATLAB engine ... accessCount = %d\n", accessCount);
      }
      
      //==============================================================
      // int engClose(Engine *ep);
      // 0 on success, and 1 otherwise. Possible failure includes
      // attempting to terminate a MATLAB engine that was
      // already terminated.
      //==============================================================
      retval = engClose(enginePtr);
      if (retval == 0)
      {
         retval = 1;
         MessageInterface::ShowMessage("MATLAB engine successfuly closed\n");
      }
      else
      {
         retval = 0;
         MessageInterface::ShowMessage("Error closing MATLAB\n");
      }
      
      enginePtr = NULL; // set to NULL, so it can be reopened
   }
   else
   {
      retval = 0;
      if (debugMatlabEngine)
      {
         MessageInterface::ShowMessage
            ("\nUnable to close MATLAB engine because it is not currently running\n");
      }
   }
   
   #ifdef DEBUG_MATLAB_OPEN_CLOSE
   MessageInterface::ShowMessage
      ("MatlabInterface::CloseSharedEngine() returning retval=%d\n", retval);
   #endif
   return retval;
}


//------------------------------------------------------------------------------
// int OpenSingleEngine(const std::string &name);
//------------------------------------------------------------------------------
/**
 * Opens single Matlab engine on Windows.
 *
 * @return  1 = no error, 0 = error
 */
//------------------------------------------------------------------------------
int MatlabInterface::OpenSingleEngine(const std::string &name)
{
#if !defined(_WIN32) && !defined(__WIN32__)
   throw InterfaceException("MATLAB engOpenSingleUse mode is supported only on Windows.");
#endif
   // The default belongs to the GMAT process. Explicit names retain the native
   // API's optional additional-engine behavior without splitting ordinary users.
   const std::string engineName = name.empty() ? "GmatMatlab" : name;

   auto existing = matlabEngineMap.find(engineName);
   if (existing != matlabEngineMap.end())
   {
      enginePtr = existing->second;
      lastEngineName = engineName;
      return 1;
   }

   // This vendor API supplies the Windows single-use process contract. Do not
   // substitute engOpen on platforms where engOpenSingleUse is unsupported.
   int status = -99;
   Engine *opened = engOpenSingleUse(NULL, NULL, &status);
   if (!opened || status != 0)
   {
      if (opened)
         engClose(opened);
      MessageInterface::ShowMessage("Failed to open MATLAB engine for single use...\n");
      return 0;
   }

   // A failed open/setup must not lose the previously selected engine.
   if (engEvalString(opened, "format long") != 0)
   {
      engClose(opened);
      return 0;
   }
   try
   {
      matlabEngineMap.emplace(engineName, opened);
   }
   catch (...)
   {
      engClose(opened);
      throw;
   }
   enginePtr = opened;
   lastEngineName = engineName;
   accessCount = static_cast<Integer>(matlabEngineMap.size());
   outBuffer[0] = '\0';
   MessageInterface::ShowMessage("MATLAB engine '%s' successfully opened\n", engineName.c_str());
   return 1;
}

//------------------------------------------------------------------------------
// int CloseSingleEngine(const std::string &name)
//------------------------------------------------------------------------------
/**
 * Closes single MATLAB engine on Windows.
 *
 * @param  name  Name used in identifying matlab engine
 * @return 1 if successfully closed, 0 otherwise
 */
//------------------------------------------------------------------------------
int MatlabInterface::CloseSingleEngine(const std::string &name)
{
   auto closeEngine = [&](std::map<std::string, Engine*>::iterator position)
   {
      Engine *closing = position->second;
      const std::string closedName = position->first;
      const bool selected = enginePtr == closing;
      const int status = engClose(closing);

      // Retire every attempted handle, including a connection whose process
      // has already terminated. Never select or close that handle again.
      matlabEngineMap.erase(position);
      accessCount = static_cast<Integer>(matlabEngineMap.size());
      if (selected)
      {
         if (matlabEngineMap.empty())
         {
            enginePtr = NULL;
            lastEngineName.clear();
         }
         else
         {
            enginePtr = matlabEngineMap.begin()->second;
            lastEngineName = matlabEngineMap.begin()->first;
         }
         outBuffer[0] = '\0';
      }
      MessageInterface::ShowMessage(status == 0 ?
            "MATLAB engine '%s' successfully closed\n" :
            "Error closing MATLAB engine '%s'\n", closedName.c_str());
      return status == 0;
   };

   if (!name.empty())
   {
      auto position = matlabEngineMap.find(name);
      if (position == matlabEngineMap.end())
         return 0;
      return closeEngine(position) ? 1 : 0;
   }

   bool allClosed = true;
   while (!matlabEngineMap.empty())
   {
      const bool closed = closeEngine(matlabEngineMap.begin());
      allClosed = closed && allClosed;
   }
   enginePtr = NULL;
   lastEngineName.clear();
   accessCount = 0;
   outBuffer[0] = '\0';
   return allClosed ? 1 : 0;
}

//------------------------------------------------------------------------------
// Integer GetParameterID(const std::string &str) const
//------------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
Integer MatlabInterface::GetParameterID(const std::string &str) const
{
   for (Integer i = InterfaceParamCount; i < MatlabInterfaceParamCount; i++)
   {
      if (str == PARAMETER_TEXT[i - InterfaceParamCount])
         return i;
   }
   
   return Interface::GetParameterID(str);
}


//---------------------------------------------------------------------------
//  bool IsParameterReadOnly(const Integer id) const
//---------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
bool MatlabInterface::IsParameterReadOnly(const Integer id) const
{
   // All parameters are read only
   return true;
}


//------------------------------------------------------------------------------
// Integer GetIntegerParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
Integer MatlabInterface::GetIntegerParameter(const Integer id) const
{
   switch (id)
   {
   case MATLAB_MODE:
      return matlabMode;
   default:
      return Interface::GetIntegerParameter(id);
   }
}


//------------------------------------------------------------------------------
// Integer SetIntegerParameter(const Integer id, const Integer value)
//------------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
Integer MatlabInterface::SetIntegerParameter(const Integer id, const Integer value)
{
   switch (id)
   {
   case MATLAB_MODE:
      SetMatlabMode(value);
      return matlabMode;
   default:
      return Interface::SetIntegerParameter(id, value);
   }
}


//---------------------------------------------------------------------------
//  Integer GetIntegerParameter(const std::string &label) const
//---------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
Integer MatlabInterface::GetIntegerParameter(const std::string &label) const
{
   return GetIntegerParameter(GetParameterID(label));
}


//---------------------------------------------------------------------------
//  Integer SetIntegerParameter(const std::string &label, const Integer value)
//---------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
Integer MatlabInterface::SetIntegerParameter(const std::string &label,
                                             const Integer value)
{
   return SetIntegerParameter(GetParameterID(label), value);
}

