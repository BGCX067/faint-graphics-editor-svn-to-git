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

#include <fstream>
#include <sstream>
#include "app/getappcontext.hh"
#include "python/py_functions.hh"
#include "python/pyinterface.hh"
#include "python/pythoninclude.hh"
#include "util/pathutil.hh"

namespace faint{
void write_user_ini( const std::string& );

void init_python(){
  Py_Initialize();
  init_faint_module();

  // Prepare the critical Python functions
  std::string dataDir = get_data_dir();
  std::string addPath = dataDir + "/python/";
  std::string str_sys_path = std::string("import sys; sys.path.append('") + addPath + "')";
  PyRun_SimpleString( str_sys_path.c_str() );
  #ifndef __WXMSW__
  const std::string envPath = dataDir + "/python/py/envsetup.py";
  const std::string envCmd = std::string("execfile('") + envPath + "')";
  PyRun_SimpleString( envCmd.c_str() );
  #else
  PyRun_SimpleString("from python.py.envsetup import *");
  #endif

  // The above ran in __main__ but is required in __console__ namespace
  PyRun_SimpleString("push_silent( 'import __main__; from __main__ import * ')");
  PyRun_SimpleString("push_silent( 'from ifaint import *')");
}

void python_keypress( int keycode, int modifiers ){
  std::stringstream ss;
  ss << "keypress(" << keycode << "," << modifiers << ")";
  run_python_str(ss.str());
}

void python_print( const std::string& text ){
  GetAppContext().PythonPrint(text);
}

void python_queue_refresh( CanvasInterface* canvas ){
  GetAppContext().PythonQueueRefresh(canvas);
}

void python_run_command( CanvasInterface* canvas, Command* cmd ){
  GetAppContext().PythonRunCommand( canvas, cmd );
}

void run_python_file( const std::string& filename ){
  const std::string cmd = "execfile('" + filename + "')";
  faint::run_python_str(cmd);
}

void run_python_str( const std::string& cmd ){
  std::string cmd_silent( std::string("push_silent(\"") + cmd + std::string("\")") );
  PyRun_SimpleString( cmd_silent.c_str() );
  // runCmd is for invoking Python from the C++-code, not the interpreter.
  // This case also requires calling PythonDone (like read_eval_print_done(...))
  GetAppContext().PythonDone();
}

void run_python_user_ini(){
  AppContext& app( GetAppContext() );

  // Execute the user's ini-script
  const std::string iniPath = join_path(get_home_dir(), ".faint.py");
  if ( !file_exists(iniPath) ){
    // Recreate the ini file from the default
    write_user_ini(iniPath);
  }

  if ( file_exists(iniPath) ){
    const std::string iniCmd = "execfile('" + iniPath + "')";
    faint::run_python_str(iniCmd);
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

void write_user_ini( const std::string& dstPath ){
  // Rewriting the ini file instead of copying it should give
  // the os-correct eol markers
  AppContext& app( GetAppContext() );
  std::string srcFilename = get_data_dir() + "/python/py/default_ini.py";
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

void resolve_object( const std::string& identifier ){
  std::string command = "faint_resolve(\""+ identifier + "\")";
  PyRun_SimpleString( command.c_str() );
}

} // namespace
