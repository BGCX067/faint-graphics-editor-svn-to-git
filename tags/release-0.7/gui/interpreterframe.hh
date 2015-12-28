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
  InterpreterFrame();

  // This frame or a child has focus (name avoids risk of clashing
  // with normal wxWidgets functions)
  bool FaintHasFocus() const;
  void FaintSetBackgroundColour( const faint::Color& );
  void FaintSetIcons(const wxIcon& icon16, const wxIcon& icon32); // Fixme: Faint-prefix avoids hiding wxTopLevelWindow::SetIcons. Create non-wx interface instead.
  void GetKey();
  std::string GetPythonVersion(); // Fixme: Not here
  void IntFaintPrint( const std::string& );
  bool IsHidden() const;
  void NewContinuation();
  void NewPrompt();
  void Print( const std::string& );
  void SetTextColor( const faint::Color& );
private:
  void OnClose( wxCloseEvent& ); // Override, hides instead of closing unless forcibly closed.
  void OnKeyDown( wxKeyEvent& ); // Used only for F8 shut down
  void OnPyCommand( wxCommandEvent& );
  void OnPyKey( wxCommandEvent& );
  InterpreterCtrl* m_interpreterCtrl;
  DECLARE_EVENT_TABLE()
};

#endif
