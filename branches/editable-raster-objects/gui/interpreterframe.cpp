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

#include "python/pythoninclude.hh" // Moved earlier to avoid warning about redefined HAVE_SSIZE_T
#include "gui/interpreterframe.hh"
#include "gui/interpreterctrl.hh"
#include "python/pyinterface.hh"
#include "util/guiutil.hh"

InterpreterFrame::InterpreterFrame()
  : wxFrame( null_parent(), wxID_ANY, "Faint Python Interpreter" )
{
  m_interpreterCtrl = new InterpreterCtrl( this );
  SetMinSize(wxSize(200,200));
  SetSize( 600, 400 );
}

void InterpreterFrame::AddNames( const std::vector<std::string>& names ){
  m_interpreterCtrl->AddNames(names);
}

bool InterpreterFrame::FaintHasFocus() const{
  return HasFocus() || m_interpreterCtrl->HasFocus();
}

void InterpreterFrame::FaintSetBackgroundColour( const faint::Color& c ){
  m_interpreterCtrl->SetBackgroundColour( wxColour( c.r, c.g, c.b ) );
}

void InterpreterFrame::FaintSetIcons(const wxIcon& icon16, const wxIcon& icon32){
  SetIcons(faint::bundle_icons(icon16, icon32));
}

void InterpreterFrame::GetKey(){
  m_interpreterCtrl->GetKey();
}

std::string InterpreterFrame::GetPythonVersion(){
  return Py_GetVersion();
}

void InterpreterFrame::IntFaintPrint( const std::string& s ){
  m_interpreterCtrl->AppendText(wxString(s));
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

void InterpreterFrame::OnClose( wxCloseEvent& event){
  if ( event.CanVeto() ){
    // Hide, instead of close the frame if possible
    event.Veto();
    Hide();
  }
  else {
    // otherwise destroy it (e.g. app closed)
    Destroy();
  }
}

void InterpreterFrame::OnKeyDown( wxKeyEvent& event ){
  if ( event.GetKeyCode() == WXK_ESCAPE ){
    Close();
  }
  else {
    event.Skip();
  }
}

void run_string_interpreter( const char* str ){
  PyObject* obj = PyString_FromString("ifaint");
  PyObject* module = PyImport_Import(obj);
  faint::py_xdecref(obj);
  assert( module != nullptr );
  PyObject* dict = PyModule_GetDict(module); // Borrowed ref
  PyObject* pushFunc = PyDict_GetItemString(dict, "push"); // Borrowed ref
  assert( pushFunc != nullptr );
  assert( PyCallable_Check(pushFunc) );
  PyObject* args = Py_BuildValue("(s)",str); // New ref
  PyObject* result = PyObject_CallObject( pushFunc, args ); // New ref
  PyObject* err = PyErr_Occurred(); // Borrowed ref
  if ( err != nullptr ){
    PyErr_PrintEx(0);
  }
  faint::py_xdecref(result);
  faint::py_xdecref(args);
  faint::py_xdecref(module);
}

void InterpreterFrame::OnPyCommand( wxCommandEvent& event ){
  run_string_interpreter(std::string(event.GetString()).c_str());
}

void InterpreterFrame::OnPyKey( wxCommandEvent& event ){
  int code = event.GetInt();
  int modifiers = event.GetExtraLong();
  wxString cmd = wxString::Format( "ifaint.bind2(%d,%d)", code, modifiers );
  PyRun_SimpleString( cmd.mb_str() );
}

void InterpreterFrame::Print( const std::string& s ){
  m_interpreterCtrl->AddText( wxString(s) );
}

void InterpreterFrame::SetTextColor( const faint::Color& c ){
  wxColour bgColor = m_interpreterCtrl->GetBackgroundColour();
  wxColour textColor( c.r, c.g, c.b );
  wxFont font( 10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
  m_interpreterCtrl->SetDefaultStyle( wxTextAttr(textColor, bgColor, font ) );
}

BEGIN_EVENT_TABLE( InterpreterFrame, wxFrame )
EVT_COMMAND(-1, EVT_PYTHON_COMMAND, InterpreterFrame::OnPyCommand)
EVT_COMMAND(-1, EVT_GOT_KEY, InterpreterFrame::OnPyKey)
EVT_CLOSE( InterpreterFrame::OnClose )
EVT_KEY_DOWN( InterpreterFrame::OnKeyDown )
END_EVENT_TABLE()
