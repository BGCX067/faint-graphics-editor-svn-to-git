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
  m_interpreterCtrl->AppendText( wxString(s.c_str(), wxConvUTF8) );
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

void InterpreterFrame::OnPyCommand( wxCommandEvent& event ){
  wxString commandStr = event.GetString();
  commandStr.Replace("\\", "\\\\");
  commandStr.Replace("\"", "\\\"");
  commandStr.Trim();
  const wxChar* cmdCharStr = commandStr.c_str();
  wxString formatted = wxString::Format("push(\"%s\")", cmdCharStr);
  std::string cmdStr( formatted.mb_str() );
  PyRun_SimpleString( cmdStr.c_str() );
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
