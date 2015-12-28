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

#ifndef FAINT_INTERPRETERPANEL_HH
#define FAINT_INTERPRETERPANEL_HH
#include "wx/textctrl.h"
#include <vector>

DECLARE_EVENT_TYPE(EVT_PYTHON_COMMAND, -1)
DECLARE_EVENT_TYPE(EVT_GOT_KEY, -1)

class AutoCompleteState;
class FileAutoComplete;
class InterpreterCtrl : public wxTextCtrl {
public:
  InterpreterCtrl( wxWindow* parent );
  ~InterpreterCtrl();
  void AddText(const wxString& );

  // Put the interpreter in GetKey-state. When the user has pressed a
  // key, the EVT_GOT_KEY event will be posted
  // Warning: At the time of writing this is used only for key binding, and
  // tailored especially for that. */
  void GetKey();
  void NewContinuation();
  void NewPrompt();
private:
  void BackwardWord();
  long BeginningOfLine() const;
  void CommandCompletion( bool );
  void DeleteSelection();
  bool Editable( long ) const;
  long EndOfLine() const;
  void ForwardWord();
  void GotKey( int, int );
  bool HasSelection() const;
  void OnChar(wxKeyEvent&);
  void OnKeyDown(wxKeyEvent&);
  void PathCompletion( bool );
  void PushLine();
  bool ReadOnly( long ) const;
  bool SelectionEditable() const;
  void ToBeginningOfLine();
  void ToEndOfLine();
  int m_inputStart;
  int m_currRowStart;
  bool m_getKey;
  AutoCompleteState* m_completion;
  FileAutoComplete* m_fileCompletion;
  std::vector<wxString> m_history;
  size_t m_historyIndex;
  DECLARE_EVENT_TABLE()
};

#endif
