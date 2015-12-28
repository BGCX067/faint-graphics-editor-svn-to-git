// Copyright 2012 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#include "pythoninclude.hh"
#include "pyinterface.hh"
#include "cpp_to_py.hh"
#include "util/pathutil.hh"
#include "getappcontext.hh"
#include <fstream>
void PythonRunCommand( CanvasInterface* canvas, Command* cmd ){
  GetAppContext().PythonRunCommand( canvas, cmd );
}

void python_print( const std::string& text ){
  GetAppContext().PythonPrint(text);
}

void initPython(){
  Py_Initialize();
  InitializeFaintModule();

  // Prepare the critical Python functions
  std::string dataDir = GetDataDir();
  std::string addPath = dataDir + "/python/";
  std::string str_sys_path = std::string("import sys; sys.path.append('") + addPath + "')";
  PyRun_SimpleString( str_sys_path.c_str() );
  #ifndef __WXMSW__
  const std::string envPath = dataDir + "/python/envsetup.py";
  const std::string envCmd = std::string("execfile('") + envPath + "')";
  PyRun_SimpleString( envCmd.c_str() );
  #else
  PyRun_SimpleString("from python.envsetup import *");
  PyRun_SimpleString("push_silent(\"from ifaint import *\")" );
  #endif

  // The above ran in __main__ but is required in __console__ namespace
  PyRun_SimpleString("push_silent( 'import __main__; from __main__ import * ')");
  PyRun_SimpleString("push_silent( 'from ifaint import *')");
}

void writeUserIni( const std::string& dstPath ){
  // Rewriting the ini file instead of copying it should give
  // the os-correct eol markers
  AppContext& app( GetAppContext() );
  std::string srcFilename = GetDataDir() + "/python/.faint.py";
  std::ifstream defaultIni(srcFilename.c_str());
  if ( !defaultIni.good() ){
    app.PythonIntFaintPrint( "Failed opening standard ini" );
  }
  std::ofstream userIni( dstPath.c_str() );
  
  std::string line;
  while ( std::getline( defaultIni, line ) ){
    userIni << line << std::endl;
  }    
}

void runUserPythonIni(){
  AppContext& app( GetAppContext() );

  // Execute the user's ini-script
  const std::string iniPath = JoinPath(GetHomeDir(), ".faint.py");
  if ( !FileExists(iniPath) ){
    // Recreate the ini file from the default
    writeUserIni(iniPath);
  }

  if ( FileExists(iniPath) ){
    const std::string iniCmd = "execfile('" + iniPath + "')";
    runCmd(iniCmd);
    std::string userIniInfo = "Executed personal ini file at \"";
    userIniInfo += iniPath + "\"\n";
    app.PythonIntFaintPrint( userIniInfo );
  }
  else {
    std::string userIniInfo = "Personal ini file not found at \"";
    userIniInfo += iniPath + "\"\n";
    app.PythonIntFaintPrint( userIniInfo );
  }
  app.PythonNewPrompt();
}

void runCmd( const std::string& cmd ){
  std::string cmd_silent( std::string("push_silent(\"") + cmd + std::string("\")") );
  PyRun_SimpleString( cmd_silent.c_str() );
  // runCmd is for invoking Python from the C++-code, not the interpreter.
  // This case also requires calling PythonDone (like read_eval_print_done(...))
  GetAppContext().PythonDone();
}
