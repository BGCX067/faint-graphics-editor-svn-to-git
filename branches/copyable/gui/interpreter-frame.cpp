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

#include "python/py-include.hh" // Moved earlier to avoid warning about redefined HAVE_SSIZE_T
#include "gui/interpreter-ctrl.hh"
#include "gui/interpreter-frame.hh"
#include "python/py-interface.hh"
#include "util/convert-wx.hh"
#include "util/gui-util.hh"

namespace faint{

static void run_string_interpreter(const char* str){
  PyObject* module = PyImport_ImportModule("ifaint");
  assert(module != nullptr);
  PyObject* dict = PyModule_GetDict(module); // Borrowed ref
  PyObject* pushFunc = PyDict_GetItemString(dict, "push"); // Borrowed ref
  assert(pushFunc != nullptr);
  assert(PyCallable_Check(pushFunc));
  PyObject* args = Py_BuildValue("(s)",str); // New ref
  PyObject* result = PyObject_CallObject(pushFunc, args); // New ref
  PyObject* err = PyErr_Occurred(); // Borrowed ref
  if (err != nullptr){
    PyErr_PrintEx(0);
  }
  py_xdecref(result);
  py_xdecref(args);
  py_xdecref(module);
}

InterpreterFrame::InterpreterFrame()
  : wxFrame(null_parent(), wxID_ANY, "Faint Python Interpreter")
{
  m_interpreterCtrl = new InterpreterCtrl(this);
  SetMinSize(wxSize(200,200));
  SetSize(600, 400);

  Bind(EVT_FAINT_PYTHON_KEY,
    [](PythonKeyEvent& event){
      const KeyPress& key(event.GetKey());
      wxString cmd = wxString::Format("ifaint.bind2(%d,%d)",
        (int)key.GetKeyCode(),
        key.Modifiers().Raw());
      PyRun_SimpleString(cmd.mb_str());
    });

  Bind(EVT_PYTHON_COMMAND,
    wxCommandEventHandler(InterpreterFrame::OnPyCommand), this);
  Bind(wxEVT_CLOSE_WINDOW, &InterpreterFrame::OnClose, this);
}

void InterpreterFrame::AddNames(const std::vector<std::string>& names){
  m_interpreterCtrl->AddNames(names);
}

bool InterpreterFrame::FaintHasFocus() const{
  return HasFocus() || m_interpreterCtrl->HasFocus();
}

void InterpreterFrame::FaintSetBackgroundColour(const ColRGB& c){
  m_interpreterCtrl->SetBackgroundColor(c);
}

void InterpreterFrame::FaintSetIcons(const wxIcon& icon16, const wxIcon& icon32){
  SetIcons(bundle_icons(icon16, icon32));
}

void InterpreterFrame::GetKey(){
  m_interpreterCtrl->GetKey();
}

void InterpreterFrame::IntFaintPrint(const utf8_string& s){
  m_interpreterCtrl->AppendText(to_wx(s));
}

bool InterpreterFrame::IsHidden() const{
  return !IsShown();
}

void InterpreterFrame::NewContinuation(){
  m_interpreterCtrl->NewContinuation();
}

void InterpreterFrame::NewPrompt(){
  m_interpreterCtrl->NewPrompt();
}

void InterpreterFrame::OnClose(wxCloseEvent& event){
  if (event.CanVeto()){
    // Hide, instead of close the frame if possible
    event.Veto();
    Hide();
  }
  else {
    // otherwise destroy it (e.g. app closed)
    Destroy();
  }
}

void InterpreterFrame::OnPyCommand(wxCommandEvent& event){
  run_string_interpreter(std::string(event.GetString()).c_str());
}

void InterpreterFrame::Print(const utf8_string& s){
  m_interpreterCtrl->AddText(to_wx(s));
}

void InterpreterFrame::SetTextColor(const ColRGB& c){
  m_interpreterCtrl->SetTextColor(c);
}

}
