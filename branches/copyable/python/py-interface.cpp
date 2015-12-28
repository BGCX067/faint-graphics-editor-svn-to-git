// -*- coding: us-ascii-unix -*-
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

#include <algorithm>
#include <fstream>
#include <sstream>
#include "app/get-app-context.hh"
#include "python/py-functions.hh"
#include "python/py-interface.hh"
#include "python/py-include.hh"
#include "python/py-util.hh"
#include "text/char-constants.hh"
#include "text/formatting.hh"
#include "util/file-path-util.hh"
#include "util/frame.hh"

namespace faint{
std::string get_python_version(){
  return Py_GetVersion();
}

static void display_error_info(const FaintPyExc& info, AppContext& app){
  app.PythonIntFaintPrint(no_sep(info.stackTrace));
  if ( info.syntaxErrorInfo.IsSet() ){
    FaintPySyntaxError syntaxError = info.syntaxErrorInfo.Get();
    app.PythonIntFaintPrint("  File " + quoted(syntaxError.file) + ", line " + str_int(syntaxError.line) + "\n");
    app.PythonIntFaintPrint("    " + syntaxError.code);
    if ( syntaxError.col > 0 ){
      if ( syntaxError.code.str().back() != '\n'){
        app.PythonIntFaintPrint("\n");
      }

      // Put a '^'-under the start of the syntax error
      app.PythonIntFaintPrint("    " + faint::utf8_string(to_size_t(syntaxError.col - 1), faint::space) + "^\n");
    }
  }
  app.PythonIntFaintPrint(info.type + ": " + info.message + "\n");
  app.PythonIntFaintPrint("\n");
}


Optional<FaintPyExc> run_file_in_python_main(const faint::FilePath& path){
  std::ifstream f(iostream_friendly(path));
    if (!f.good()){
      FaintPyExc err;
      err.type = "OSError";
      err.message = "Failed opening " + path.Str();
      return option(err);
    }

    std::string text;
    std::string line;
    while ( std::getline(f, line) ){
      text += line + "\n";
    }

    ScopedRef module(PyImport_ImportModule("__main__"));
    assert(*module != nullptr);
    PyObject* dict = PyModule_GetDict(*module); // Borrowed
    assert(dict != nullptr);
    ScopedRef obj(PyRun_String(text.c_str(), Py_file_input, dict, dict));
    if ( *obj == nullptr ){
      return option(faint::py_error_info());
    }
    return no_option();
}

static void run_envsetup(const faint::FilePath& path){
  assert(exists(path));
  auto err = run_file_in_python_main(path);
  assert(err.NotSet());
}

void add_to_python_path(const DirPath& dirPath){
  ScopedRef sys(PyImport_ImportModule("sys"));
  assert(*sys != nullptr);
  PyObject* dict(PyModule_GetDict(*sys)); // Borrowed
  assert(dict != nullptr);
  PyObject* path = PyDict_GetItemString(dict, "path"); // Borrowed
  assert(path != nullptr);
  int result = PyList_Append(path, build_unicode(dirPath.Str()));
  assert(result == 0);
}

bool init_python(){
  PyImport_AppendInittab("ifaint", PyInit_ifaint);
  Py_Initialize();

  faint::DirPath dataDir = get_data_dir();
  add_to_python_path(dataDir.SubDir("py"));
  run_envsetup(dataDir.SubDir("py").SubDir("core").File("envsetup.py"));
  return true;
}

void python_print(const faint::utf8_string& text){
  get_app_context().PythonPrint(text);
}

void python_queue_refresh( Canvas& canvas ){
  get_app_context().PythonQueueRefresh(&canvas);
}

void python_run_command( Canvas& canvas, Command* cmd ){
  if ( cmd != nullptr ){
    get_app_context().PythonRunCommand( &canvas, cmd );
  }
}

void python_run_command( const BoundObject& obj, Command* cmd ){
  if ( cmd != nullptr ){
    get_app_context().PythonRunCommand( obj.canvas, cmd, obj.frameId );
  }
}

void python_run_command( const Frame& frame, Command* cmd ){
  if ( cmd != nullptr ){
    get_app_context().PythonRunCommand( frame.canvas, cmd, frame.frameId );
  }
}

bool run_python_file(const faint::FilePath& path, AppContext& app){
  auto err = run_file_in_python_main(path);
  if ( err.IsSet() ){
    app.PythonIntFaintPrint("\n");
    app.PythonIntFaintPrint(space_sep("Error in script", quoted(path.Str()), "specified with --run:\n"));
    display_error_info(err.Get(), app);
  }
  return err.NotSet();
}

void run_python_str( const std::string& cmd ){
  std::string cmd_silent( std::string("push_silent(\"") + cmd + std::string("\")") );
  PyRun_SimpleString( cmd_silent.c_str() );
  // runCmd is for invoking Python from the C++-code, not the interpreter.
  // This case also requires calling PythonDone (like read_eval_print_done(...))
  get_app_context().PythonDone();
}

static void write_python_user_config( const faint::FilePath& dstPath ){
  // Rewriting the ini file instead of copying it should give
  // the os-correct eol markers
  AppContext& app( get_app_context() );

  Optional<FilePath> srcPath = make_absolute_file_path(
    get_data_dir().Str() + "/py/core/default_ini.py");
  assert(srcPath.IsSet());

  std::ifstream defaultIni(iostream_friendly(srcPath.Get()));
  if ( !defaultIni.good() ){
    app.PythonIntFaintPrint( "Failed opening standard ini" );
    return;
  }
  std::ofstream userIni( iostream_friendly(dstPath) );

  std::string line;
  while ( std::getline( defaultIni, line ) ){
    userIni << line << std::endl;
  }
}

bool run_python_user_config(AppContext& app){
  // Execute the user's ini-script
  faint::FilePath configPath(get_user_config_file_path());
  if ( !exists(configPath) ){
    // Recreate the config file from the default
    write_python_user_config(configPath);
  }

  if ( exists(configPath) ){
    Optional<FaintPyExc> err = run_file_in_python_main(configPath);
    if ( err.IsSet() ){
      app.PythonIntFaintPrint("\n");
      app.PythonIntFaintPrint(space_sep("Error in personal config file",
          bracketed(quoted(configPath.Str()))) + ":\n");

      display_error_info(err.Get(), app);
      return false;
    }
    else{
      app.PythonIntFaintPrint(space_sep("Executed personal config file at",
        quoted(configPath.Str())) + "\n");
      return true;
    }
  }
  else {
    faint::utf8_string userIniInfo(
      space_sep("Personal config file not found at",
        quoted(configPath.Str())));
    app.PythonIntFaintPrint( userIniInfo + "\n" );
    return false;
  }
  return true;
}

void resolve_object( const std::string& identifier ){
  std::string command = "faint_resolve(\""+ identifier + "\")";
  PyRun_SimpleString( command.c_str() );
}

std::vector<std::string> list_ifaint_names(){
  PyObject* module = PyImport_ImportModule("ifaint");
  assert( module != nullptr );
  PyObject* dict = PyModule_GetDict(module); // Borrowed ref
  PyObject* keys = PyDict_Keys(dict); // New reference
  const int n = PySequence_Length(keys);

  std::vector<std::string> names;
  for ( int i = 0; i != n; i++ ){
    PyObject* name = PySequence_GetItem(keys, i);
    PyObject* utf8 = PyUnicode_AsUTF8String(name);
    assert(utf8 != nullptr);
    char* str = PyBytes_AsString(utf8);
    names.push_back(std::string(str)); // Fixme: use utf8_string
    faint::py_xdecref(name);
  }
  faint::py_xdecref(keys);
  faint::py_xdecref(module);
  return names;
}


} // namespace
