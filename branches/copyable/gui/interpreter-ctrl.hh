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

#ifndef FAINT_INTERPRETER_CTRL_HH
#define FAINT_INTERPRETER_CTRL_HH
#include <vector>
#include "wx/window.h"
#include "util/color.hh"
#include "util/keycode.hh"

namespace faint{

class InterpreterImpl;
DECLARE_EVENT_TYPE(EVT_PYTHON_COMMAND, -1)

// Event sent when a key has been specified for a key-press query in
// the interpreter
class PythonKeyEvent : public wxCommandEvent{
public:
  PythonKeyEvent(const KeyPress&);
  wxEvent* Clone() const override;
  const KeyPress& GetKey() const;
private:
  KeyPress m_key;
};

wxDECLARE_EVENT(EVT_FAINT_PYTHON_KEY, PythonKeyEvent);

class InterpreterCtrl{
public:
  InterpreterCtrl(wxWindow* parent);
  void AddNames(const std::vector<std::string>&);
  void AddText(const wxString&);
  void AppendText(const wxString&);

  // Put the interpreter in GetKey-state. When the user has pressed a
  // key, the EVT_GOT_KEY event will be posted
  // Warning: At the time of writing this is used only for key binding, and
  // tailored especially for that. */
  void GetKey();

  bool HasFocus() const;
  void NewContinuation();
  void NewPrompt();
  void SetBackgroundColor(const ColRGB&);
  void SetTextColor(const ColRGB&);
private:
  InterpreterCtrl(const InterpreterCtrl&);
  InterpreterImpl* m_impl;
};

}

#endif
