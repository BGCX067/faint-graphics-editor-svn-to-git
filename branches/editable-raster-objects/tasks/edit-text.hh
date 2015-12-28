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

#ifndef FAINT_EDITTEXT_HH
#define FAINT_EDITTEXT_HH
#include "tasks/task.hh"
#include "util/char.hh"
#include "util/keycode.hh"

class ObjText;
class TextBuffer;

class EditText : public Task {
public:
  EditText( const Rect&, const faint::utf8_string&, Settings& );
  EditText( ObjText*, Settings& );
  ~EditText();
  void Activate() override;
  bool AcceptsPastedText() const override;
  TaskResult Char( const KeyInfo& ) override;
  bool CopyText(faint::utf8_string&, bool ) override;
  TaskResult Delete() override;
  TaskResult Deselect() override;
  void Draw( FaintDC&, Overlays&, const Point& ) override;
  bool DrawBeforeZoom(Layer) const override;
  Command* GetCommand() override;
  Cursor GetCursor( const CursorPositionInfo& ) const override;
  Task* GetNewTask() override;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const override;
  bool HasSelection() const override;
  TaskResult LeftDoubleClick( const CursorPositionInfo& ) override;
  TaskResult LeftDown( const CursorPositionInfo& ) override;
  TaskResult LeftUp( const CursorPositionInfo& ) override;
  TaskResult Motion( const CursorPositionInfo& ) override;
  void Paste( const faint::utf8_string& ) override;
  TaskResult Preempt( const CursorPositionInfo& ) override;
  TaskResult SelectAll() override;
  bool UpdateSettings() override;
private:
  bool HandleSpecialKey( key::key_t keycode, int mod, TextBuffer& );
  void EndEntry();
  TaskResult Commit(Layer);
  EditText& operator=(const EditText&); // Prevent assignment
  bool m_active;
  PendingCommand m_command;
  PendingTask m_newTask;
  bool m_newTextObject;
  faint::utf8_string m_oldText;
  Settings& m_settings;
  ObjText* m_textObject;
};

#endif
