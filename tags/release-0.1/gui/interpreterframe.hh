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

#ifndef FAINT_INTERPRETERFRAME_HH
#define FAINT_INTERPRETERFRAME_HH
#include "wx/frame.h"
#include "wx/event.h"
#include "geo/geotypes.hh"

class InterpreterCtrl;

class InterpreterFrame : public wxFrame{
public:
  InterpreterFrame( wxFrame* parent );
  void AddText( const wxString& );

  // Overrides default closing-behavior: hides instead of closing
  // unless forcibly closed.
  void OnClose( wxCloseEvent& );

  // Used only for F8 shut down
  void OnKeyDown( wxKeyEvent& );

  void OnPyCommand( wxCommandEvent& );
  void OnPyKey( wxCommandEvent& );
  std::string GetPythonVersion();

  void SetBackgroundColour( const faint::Color& );
  void SetTextColor( const faint::Color& );
  // This frame or a child has focus (name avoids risk of clashing
  // with normal wxWidgets functions)
  bool FaintHasFocus() const;
  void NewPrompt();
  void NewContinuation();
  void GetKey();
  void Print( const std::string& );
  void IntFaintPrint( const std::string & );
private:
  InterpreterCtrl* m_interpreterCtrl;

  DECLARE_EVENT_TABLE()
};

#endif
