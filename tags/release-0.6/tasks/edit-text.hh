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
  void Activate();
  bool AcceptsPastedText() const;
  TaskResult Char( const KeyInfo& );
  bool CopyText(faint::utf8_string&, bool );
  TaskResult Delete();
  TaskResult Deselect();
  bool Draw( FaintDC&, Overlays&, const Point& );
  bool DrawBeforeZoom(Layer::type) const;
  Command* GetCommand();
  Cursor::type GetCursor( const CursorPositionInfo& ) const;
  Task* GetNewTask();
  IntRect GetRefreshRect( const IntRect&, const Point& ) const;
  bool HasSelection() const;
  TaskResult LeftDown( const CursorPositionInfo& );
  TaskResult LeftUp( const CursorPositionInfo& );
  TaskResult Motion( const CursorPositionInfo& );
  void Paste( const faint::utf8_string& );
  TaskResult Preempt( const CursorPositionInfo& );
  TaskResult SelectAll();
  bool UpdateSettings();
private:
  bool HandleSpecialKey( key::key_t keycode, int mod, TextBuffer& );
  void EndEntry();
  Command* CreateCommand(Layer::type);
  EditText& operator=(const EditText&); // Prevent assignment
  bool m_active;
  PendingCommand m_command;
  bool m_newTextObject;
  faint::utf8_string m_oldText;
  Settings& m_settings;
  ObjText* m_textObject;
};

#endif
