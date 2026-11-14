//$Id$
//------------------------------------------------------------------------------
//                                 PythonInterface
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
// FDSS II.
//
// Author: Farideh Farahnak
// Created: 2015/05/22
//
/**
 * Definition for the PythonInterface command class
 */
//------------------------------------------------------------------------------
#include "stdlib.h"

#include "PythonInterface.hpp"
#include "MessageInterface.hpp"
#include "InterfaceException.hpp"
#include "Array.hpp"
#include <iostream>
#include <memory>
#include <pyconfig.h>
#ifdef __linux__
#include <dlfcn.h>
#endif

//#define DEBUG_INITIALIZATION
//#define DEBUG_EXECUTION

/// Initialize the singleton pointer
PythonInterface* PythonInterface::instance = NULL;

//------------------------------------------------------------------------------
// PythonInterface* PyInstance()
//------------------------------------------------------------------------------
/**
 * Access method for the Python interface singleton
 *
 * @return The singleton pointer
 */
//------------------------------------------------------------------------------
PythonInterface* PythonInterface::PyInstance()
{
   if (instance == NULL)
	   instance = new PythonInterface("PythonInterface");

   return instance;
}


//------------------------------------------------------------------------------
// PythonInterface(const std::string &name)
//------------------------------------------------------------------------------
/**
 * Constructor
 */
//------------------------------------------------------------------------------
PythonInterface::PythonInterface(const std::string &name) : 
   Interface            ("PythonInterface", name),
   isPythonInitialized  (false),
   numPyCommands        (0),
   // Default to the windows path separator
   plF                  (";")
{
}


//------------------------------------------------------------------------------
// ~PythonInterface()
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
PythonInterface::~PythonInterface()
{
}


//------------------------------------------------------------------------------
// void Copy(const GmatBase* orig)
//------------------------------------------------------------------------------
/**
 * Copies settings from the input PythonInterface to this one
 *
 * @param orig The Interface being copied
 *
 * @return The new PythonInterface
 *
 * @note Since this is a singleton, we need to determine if this method is ever
 * called
 */
//------------------------------------------------------------------------------
void PythonInterface::Copy(const GmatBase* orig)
{
   throw InterfaceException("The PythonInterface is a singleton and cannot be "
         "copied");
}


//------------------------------------------------------------------------------
// GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * This method clones the current PythonInterface to make a new one.
 *
 * @return The clone
 *
 * @note Since this is a singleton, we need to determine if this methos is ever
 * called
 */
//------------------------------------------------------------------------------
GmatBase* PythonInterface::Clone() const
{
   throw InterfaceException("The PythonInterface is a singleton and cannot be "
         "cloned");
}


//------------------------------------------------------------------------------
// bool PyInitialize()
//------------------------------------------------------------------------------
/**
 * Load Python engine
 *
 * This method will initialize python by loading Python engine.
 *
 * @return bool
 */
//------------------------------------------------------------------------------
bool PythonInterface::PyInitialize()
{
#ifdef DEBUG_INITIALIZATION
   MessageInterface::ShowMessage("PythonInterface::PyInitialize() start.\n");
#endif

#ifdef __linux__
   // GMAT loads plugins locally.  Python extension modules (e.g. NumPy)
   // require the symbols of this plugin's linked interpreter in global scope.
   // Promote that already-loaded library, never a guessed Python version.
   // Retain one handle for the interpreter's lifetime, including shutdown.
   static void *pythonLibrary = NULL;
   if (!pythonLibrary)
   {
      Dl_info libraryInfo;
      if (!dladdr(reinterpret_cast<void *>(&Py_Initialize), &libraryInfo) ||
          !libraryInfo.dli_fname)
         throw InterfaceException("Cannot locate the linked Python library.");
      dlerror();
      pythonLibrary = dlopen(libraryInfo.dli_fname,
            RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL);
      if (!pythonLibrary)
      {
         const char *error = dlerror();
         throw InterfaceException("Cannot expose the linked Python library "
               "to extension modules: " + std::string(error ? error :
               "unknown dynamic loader error"));
      }
   }
#endif

   // Initialize Python only once.
   if (!isPythonInitialized)
   {
      bool pyLibFound = false;

      #ifdef _WIN32
         ////// This code only work on Windows:
         ////wchar_t *pythonHomePath = Py_GetPythonHome();
         ////if (pythonHomePath == NULL)
         ////   throw InterfaceException("Error: The value of the system "
         ////         "environment variable PYTHONHOME is not set.\nThe Python"
         ////         "interface cannot be accessed");


         ////char homePath[1000];
         ////size_t len = wcstombs(&homePath[0], pythonHomePath, sizeof(homePath));
         ////homePath[len] = '\0';
         ////std::string homePaths;
         ////homePaths.assign(&homePath[0]);

         ////// Check the existance of python37.dll file
         ////std::string pythonDll = homePaths + "/python37.dll";
         ////FILE *fp = fopen(pythonDll.c_str(), "r");

         ////if (fp != NULL)
         ////{
         ////   fclose(fp);
         ////   pyLibFound = true;
         ////}
         ////else
         ////   throw InterfaceException("Error: PYTHONHOME = " + homePaths +
         ////         ".  PYTHONHOME contains an incorrect path or a path pointing "
         ////         "to the wrong version of Python.  Set PYTHONPATH to point to "
         ////         "your Anaconda 3.7 folder.\n");
         pyLibFound = true;
      #else
         /// @todo Mac and Linux configuration

//         // Locate Python and validate version
//         wchar_t *pythonHomePath = Py_GetPythonHome();
//         if (pythonHomePath)
//         {
//            MessageInterface::ShowMessage("Python home: %ls\n", pythonHomePath);
//            Py_SetProgramName(pythonHomePath);
//         }
         // Temporarily assume they are located
         pyLibFound = true;
      #endif

      if (pyLibFound)
      {
         try
         {
            Py_Initialize();
         }
         catch (...)
         {
            PyObject* error = PyErr_Occurred();
            if (error)
               throw InterfaceException("The Python interpreter failed to "
                     "initialize.  You may need to set the PYTHONPATH "
                     "environment variable.");
            // Rethrow other exceptions
            throw;
         }
      }
   }
   
   if (Py_IsInitialized())
   {
      if (!isPythonInitialized)

      #ifdef DEBUG_INITIALIZATION
         MessageInterface::ShowMessage("Python is initialized/Loaded.\n");
      #endif

	   isPythonInitialized = true;
      numPyCommands++;
   }
   else
   {
	   throw CommandException ("Python failed to load properly.");
   }
		
#ifdef DEBUG_INITIALIZATION
   MessageInterface::ShowMessage("PythonInterface::PyInitialize() end.\n");
#endif
   return isPythonInitialized;
}


//------------------------------------------------------------------------------
// bool PyFinalize()
//------------------------------------------------------------------------------
/**
 * UnLoad Python engine
 *
 * This method will shut down/unload python engine.
 *
 * @return bool
 */
//------------------------------------------------------------------------------
bool PythonInterface::PyFinalize()
{
#ifdef DEBUG_INITIALIZATION
   MessageInterface::ShowMessage("PythonInterface::PyFinalize() start.\n");
#endif

   // When all the Python commands in the Gmat script are run and completed,
   // close and finalize Python.
   if (--numPyCommands == 0)
   {
//	   Py_Finalize();

      #ifdef DEBUG_EXECUTION
         MessageInterface::ShowMessage("Python was Finalized and Unloaded.\n");
      #endif
///
//      isPythonInitialized = false;
   }
	
   #ifdef DEBUG_EXECUTION
      MessageInterface::ShowMessage("numPyCommands: %d  (Python is unloaded "
            "when this counter reaches 0)\n", numPyCommands);
   #endif

#ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage("PythonInterface::PyFinalize() end.\n");
#endif

      return isPythonInitialized;
}


//------------------------------------------------------------------------------
// void PyPathSep()
//------------------------------------------------------------------------------
/**
 * Operating system environment/system path delimiter
 *
 * This method will differentiate the path delimiter for different OS.
 *
 * @return void
 */
//------------------------------------------------------------------------------
void PythonInterface::PyPathSep()
{
   // Unix and Mac separator is ":"
   // Windows separator is ";"
   #ifdef _WIN32
      plF = ";";
   #else
      plF = ":";
   #endif
}

//------------------------------------------------------------------------------
// void PyAddModulePath(const StringArray& path)
//------------------------------------------------------------------------------
/**
 * Add Python module to the system path.
 *
 * @param path List of paths added in the startup file
 */
//------------------------------------------------------------------------------
void PythonInterface::PyAddModulePath(const StringArray& path)
{
   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage("Entered PythonInterface::PyAddModulePath()\n");
   #endif

   UnsignedInt pathCount = path.size();
   std::string cmd, line;

   cmd = "import sys\n";
   PyRun_SimpleString(cmd.c_str());
   for (UnsignedInt it = 0; it < pathCount; ++it)
   {
      // Handle potential of backslash separators coming from Windows
      line =  path[it];
      std::replace(line.begin(), line.end(), '\\', '/');

      cmd = "sys.path.append(\"" + line + "\")\n";

      #ifdef DEBUG_INITIALIZATION
         MessageInterface::ShowMessage("   Adding %s to the Python path\n",
                 path[it].c_str());
         MessageInterface::ShowMessage("   Python command: '%s'\n",
                 cmd.c_str());
      #endif

      PyRun_SimpleString(cmd.c_str());
   }

   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage("  Leaving PyAddModulePath( ) \n");
   #endif
}

//------------------------------------------------------------------------------
// PyObject* PyFunctionWrapper(const std::string &modName, 
//                             const std::string &funcName,
//                             const std::vector<PyObject *> &argIn)
//------------------------------------------------------------------------------
/**
 * Method that calls the scripted Python function
 *
 * @param modName The name of teh Python file being called
 * @param funcName The Python function in the module
 * @param argIn The input parameters
 *
 * @return The PyObject containing the returned data
 */
//------------------------------------------------------------------------------
PyObject* PythonInterface::PyFunctionWrapper(const std::string &modName, 
                                             const std::string &funcName,
                                             const std::vector<PyObject *> &argIn)
{
   PyObject* pyModule = NULL;
   PyObject* pyPluginModule = NULL;
   PyObject* pyFuncAttr = NULL;
   PyObject* pyArgs = NULL;
   PyObject* pyFunc = NULL;
   PyObject* pyTupleObj = NULL;

   //error messages
   PyObject* pType = NULL;
   PyObject* pValue = NULL;
   PyObject* pTraceback = NULL;

   std::string msg;

   // Build the Python Tuple object based on the format string
   pyTupleObj = PyTuple_New(argIn.size());

#ifdef IS_PY3K
   // create a python Unicode object from an UTF-8 encoded null terminated char buffer
   pyModule = PyUnicode_FromString(modName.c_str() );
#else
   pyModule = PyBytes_FromString(modName.c_str() );
#endif
  
   if (!pyModule)
   {
      PyErrorMsg(pType, pValue, pTraceback, msg);
      throw InterfaceException(" Python Exception: " + msg + "\n");
   }

   // import the python module
   pyPluginModule = PyImport_Import(pyModule);   
   Py_DECREF(pyModule);

   if (!pyPluginModule)
   {
      PyErrorMsg(pType, pValue, pTraceback, msg);
      throw InterfaceException(" Python Exception: " + msg + "\n");
   }

   // Set standard output of this module to a parameter
   std::string redirectPyOutput =
      "from io import StringIO\n"
      "import sys\n"
      "storageObj = StringIO()\n"
      "sys.stdout = storageObj";
   PyRun_String(redirectPyOutput.c_str(), Py_file_input,
      PyModule_GetDict(pyPluginModule), PyModule_GetDict(pyPluginModule));
   PyObject *catcher = PyObject_GetAttrString(pyPluginModule, "storageObj");

   // retrieve an attribute named 'funcName' from the python module
   pyFuncAttr = PyObject_GetAttrString(pyPluginModule, funcName.c_str() );
   Py_DECREF(pyPluginModule);

   if (!pyFuncAttr)
   {
      PyErrorMsg(pType, pValue, pTraceback, msg);
      throw InterfaceException(" Python Exception: " + msg + "\n");
   }

   /*
    * GMAT supports passing data to Python using the following rules:
    *
    * Variable data is passed into a Python float
    *
    * Array data is passed in memoryview objects for one dimensional arrays
    *
    * Two dimensional arrays are not supported
    *
    * Strings are passed to Python strings
    */

   for (UnsignedInt i = 0; i < argIn.size(); ++i)
      PyTuple_SetItem(pyTupleObj, i, argIn.at(i));
   
   #ifdef DEBUG_EXECUTION
      MessageInterface::ShowMessage("Executing the function\n", *(Real*)argIn.at(n));
   #endif

   // Call the python function   
   pyFunc = PyObject_CallObject(pyFuncAttr, pyTupleObj);

   #ifdef DEBUG_EXECUTION
      MessageInterface::ShowMessage("Function executed\n", *(Real*)argIn.at(n));
   #endif
  
   Py_DECREF(pyFuncAttr);
   Py_DECREF(pyTupleObj);

   if (!pyFunc)
   {
      PyErrorMsg(pType, pValue, pTraceback, msg);
      throw InterfaceException("Python Exception: " + msg + "\n");
   }

   // Grab any standard output that has been generated
   PyObject* getValuePyFunc = PyObject_GetAttrString(catcher, "getvalue");
   PyObject* emptyArgs = PyTuple_Pack(0);
   PyObject *stdOutFromPy = PyObject_CallObject(getValuePyFunc, emptyArgs);

   std::string outFromPyString;
   PyObject *bytes;
   bytes = PyUnicode_AsUTF8String(stdOutFromPy);
   outFromPyString = PyBytes_AsString(bytes);
   MessageInterface::ShowMessage(outFromPyString);

   Py_DECREF(catcher);
   Py_DECREF(getValuePyFunc);
   Py_DECREF(emptyArgs);
   Py_DECREF(stdOutFromPy);

   return pyFunc;
}

//------------------------------------------------------------------------------
// PyObject* PyExternalFunctionWrapper(const std::string &modName, 
//    const std::string &funcName, const std::vector<void *> &argIn,
//    std::vector<Gmat::ParameterType> paramType, UnsignedInt row, 
//    UnsignedInt col, UnsignedInt argSz)
//------------------------------------------------------------------------------
/**
 * Method that calls the scripted Python function
 *
 * @param modName The name of teh Python file being called
 * @param funcName The Python function in the module
 * @param argIn The input parameters
 * @param paramType The type associated with each input
 * @param row The number of rows in the input (is this used?)
 * @param col The number of columns in the input (is this used?)
 * @param argSz The number of input arguments
 *
 * @return The PyObject containing the returned data
 */
 //------------------------------------------------------------------------------
PyObject* PythonInterface::PyExternalFunctionWrapper(const std::string &modName, const std::string &funcName,
	Real *state, Real now, StringArray stateDescription, Integer order, Integer dimension,
	UnsignedInt argSz)
{
   if (!state || dimension <= 0 || (argSz != 3 && argSz != 4))
      throw InterfaceException("External callback arguments are inconsistent.");
   struct PythonLock
   {
      PyGILState_STATE state = PyGILState_Ensure();
      ~PythonLock() { PyGILState_Release(state); }
   } lock;
   auto release = [](PyObject *object) { Py_XDECREF(object); };
   using PythonObject = std::unique_ptr<PyObject,decltype(release)>;
   auto fail = [&]()
   {
      std::string message;
      PyErrorMsg(nullptr,nullptr,nullptr,message);
      throw InterfaceException("Python Exception: " + message);
   };
   PythonObject module(PyImport_ImportModule(modName.c_str()),release);
   if (!module) fail();
   PythonObject function(PyObject_GetAttrString(module.get(),funcName.c_str()),release);
   if (!function) fail();
   if (!PyCallable_Check(function.get()))
   {
      PyErr_SetString(PyExc_TypeError,"External callback entry point is not callable");
      fail();
   }
   PythonObject arguments(PyTuple_New(argSz),release);
   PythonObject values(PyList_New(dimension),release);
   PythonObject descriptions(PyList_New(stateDescription.size()),release);
   if (!arguments || !values || !descriptions) fail();
   for (Integer index = 0; index < dimension; ++index)
   {
      PyObject *value = PyFloat_FromDouble(state[index]);
      if (!value) fail();
      PyList_SET_ITEM(values.get(),index,value);
   }
   for (size_t index = 0; index < stateDescription.size(); ++index)
   {
      PyObject *value = PyUnicode_FromString(stateDescription[index].c_str());
      if (!value) fail();
      PyList_SET_ITEM(descriptions.get(),index,value);
   }
   PyTuple_SET_ITEM(arguments.get(),0,values.release());
   PyObject *epochValue = PyFloat_FromDouble(now);
   if (!epochValue) fail();
   PyTuple_SET_ITEM(arguments.get(),1,epochValue);
   PyTuple_SET_ITEM(arguments.get(),2,descriptions.release());
   if (argSz == 4)
   {
      PyObject *orderValue = PyFloat_FromDouble(order);
      if (!orderValue) fail();
      PyTuple_SET_ITEM(arguments.get(),3,orderValue);
   }
   PythonObject io(PyImport_ImportModule("io"),release);
   if (!io) fail();
   PythonObject stringIO(PyObject_GetAttrString(io.get(),"StringIO"),release);
   if (!stringIO) fail();
   PythonObject catcher(PyObject_CallObject(stringIO.get(),nullptr),release);
   if (!catcher) fail();
   PyObject *previous = PySys_GetObject("stdout");
   Py_XINCREF(previous);
   PythonObject previousOutput(previous,release);
   if (PySys_SetObject("stdout",catcher.get()) != 0) fail();
   struct RestoreOutput
   {
      PyObject *previous;
      ~RestoreOutput() { PySys_SetObject("stdout",previous); }
   } restore{previousOutput.get()};
   PythonObject result(PyObject_CallObject(function.get(),arguments.get()),release);
   if (!result) fail();
   PythonObject getValue(PyObject_GetAttrString(catcher.get(),"getvalue"),release);
   if (!getValue) fail();
   PythonObject output(PyObject_CallObject(getValue.get(),nullptr),release);
   if (!output) fail();
   const char *text = PyUnicode_AsUTF8(output.get());
   if (!text) fail();
   MessageInterface::ShowMessage("%s",text);
   return result.release();
}

//------------------------------------------------------------------------------
// void PyErrorMsg(PyObject* pType, PyObject* pValue, PyObject* pTraceback, 
//       std::string &msg)
//------------------------------------------------------------------------------
/**
 * Fetches a Python error/exception message
 *
 * @param pType The Python type
 * @param pValue The Python value
 * @param pTraceback The Python traceback object
 * @param msg The message string that is being filled in with the exception 
 *            information
 */
//------------------------------------------------------------------------------
void PythonInterface::PyErrorMsg(PyObject* pType, PyObject* pValue, 
      PyObject* pTraceback, std::string &msg)
{
   // Fetching transfers ownership and clears the interpreter error.  The
   // caller translates it into a GMAT exception, so do not restore it: doing
   // so would leave an unrelated error pending for the next Python command.
   const bool fetched = PyErr_Occurred() != NULL;
   if (fetched)
      PyErr_Fetch(&pType, &pValue, &pTraceback);

   msg = "Python error";
   PyObject *typeName = pType ? PyObject_GetAttrString(pType, "__name__") : NULL;
   if (typeName)
   {
#ifdef IS_PY3K
      const char *text = PyUnicode_AsUTF8(typeName);
#else
      const char *text = PyString_AsString(typeName);
#endif
      if (text)
         msg = text;
   }
   Py_XDECREF(typeName);
   PyErr_Clear();

   PyObject *value = pValue ? PyObject_Str(pValue) : NULL;
   const char *text = NULL;
   if (value)
   {
#ifdef IS_PY3K
      text = PyUnicode_AsUTF8(value);
#else
      text = PyString_AsString(value);
#endif
   }
   msg += ": ";
   msg += text ? text : "<exception message unavailable>";
   Py_XDECREF(value);
   // __str__ or UTF-8 conversion can fail while formatting the original
   // exception.  Keep the original type and fallback text in that case.
   PyErr_Clear();

   if (fetched)
   {
      Py_XDECREF(pType);
      Py_XDECREF(pValue);
      Py_XDECREF(pTraceback);
   }
}


//------------------------------------------------------------------------------
// std::string PythonInterface::PyExternalGetPythonPath(const std::string & modName)
//------------------------------------------------------------------------------
/**
 * Fetches a Python full path based on a file name. Taken from python "__file__" 
 *
 * @param const std::string & modName - the modules name
 */
 //------------------------------------------------------------------------------
std::string PythonInterface::PyExternalGetPythonPath(const std::string & modName)
{

   PyObject* pyModule = NULL;
   PyObject* pyPluginModule = NULL;

   //error messages
   PyObject* pType = NULL;
   PyObject* pValue = NULL;
   PyObject* pTraceback = NULL;

   std::string msg;

#ifdef IS_PY3K
   // create a python Unicode object from an UTF-8 encoded null terminated char buffer
   pyModule = PyUnicode_FromString(modName.c_str());
#else
   pyModule = PyBytes_FromString(modName.c_str());
#endif

   if (!pyModule)
   {
      PyErrorMsg(pType, pValue, pTraceback, msg);
      throw InterfaceException(" Python Exception: " + msg + "\n");
   }

   // import the python module
   pyPluginModule = PyImport_Import(pyModule);
   Py_DECREF(pyModule);

   if (!pyPluginModule)
   {
      PyErrorMsg(pType, pValue, pTraceback, msg);
      throw InterfaceException(" Python Exception: " + msg + "\n");
   }
   PyObject* test = PyObject_GetAttrString(pyPluginModule, "__file__");
   const char * fullpath = PyUnicode_AsUTF8(test);

   return fullpath;
}
