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

#include "python/pythoninclude.hh"
#include "interpreterframe.hh"
#include "interpreterctrl.hh"
#include "python/pyinterface.hh"

InterpreterFrame::InterpreterFrame( wxFrame* parent )
  : wxFrame( parent, -1, "Faint Python Interpreter", wxDefaultPosition, wxDefaultSize, ( wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR ) & ~wxMINIMIZE_BOX )
{
  m_interpreterCtrl = new InterpreterCtrl( this );
  SetMinSize(wxSize(200,200));
  SetSize( 600, 400 );
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
  if ( event.GetKeyCode() == WXK_F8 ){
    Close();
  }
  else {
    event.Skip();
  }
}

void InterpreterFrame::OnPyKey( wxCommandEvent& event ){
  int code = event.GetInt();
  int modifiers = event.GetExtraLong();
  wxString cmd = wxString::Format( "ifaint.bind2(%d,%d)", code, modifiers );
  PyRun_SimpleString( cmd.mb_str() );
}

std::string InterpreterFrame::GetPythonVersion(){
  return Py_GetVersion();
}

void InterpreterFrame::SetBackgroundColour( const faint::Color& c ){
  m_interpreterCtrl->SetBackgroundColour( wxColour( c.r, c.g, c.b ) );
}

void InterpreterFrame::SetTextColor( const faint::Color& c ){
  wxColour bgColor = m_interpreterCtrl->GetBackgroundColour();
  wxColour textColor( c.r, c.g, c.b );
  wxFont font( 10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
  m_interpreterCtrl->SetDefaultStyle( wxTextAttr(textColor, bgColor, font ) );
}

bool InterpreterFrame::FaintHasFocus() const{
  return HasFocus() || m_interpreterCtrl->HasFocus();
}

void InterpreterFrame::NewPrompt(){
  m_interpreterCtrl->NewPrompt();
}

void InterpreterFrame::NewContinuation(){
  m_interpreterCtrl->NewContinuation();
}

void InterpreterFrame::GetKey(){
  m_interpreterCtrl->GetKey();
}

void InterpreterFrame::Print( const std::string& s ){
  m_interpreterCtrl->AddText( wxString(s) );
}

void InterpreterFrame::IntFaintPrint( const std::string& s ){  
  m_interpreterCtrl->AppendText( wxString(s.c_str(), wxConvUTF8) );
}

BEGIN_EVENT_TABLE( InterpreterFrame, wxFrame )
EVT_COMMAND(-1, EVT_PYTHON_COMMAND, InterpreterFrame::OnPyCommand)
EVT_COMMAND(-1, EVT_GOT_KEY, InterpreterFrame::OnPyKey)
EVT_CLOSE( InterpreterFrame::OnClose )
EVT_KEY_DOWN( InterpreterFrame::OnKeyDown )
END_EVENT_TABLE()

