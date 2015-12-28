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

#ifndef FAINT_SELECT_TEXT_HH
#define FAINT_SELECT_TEXT_HH
#include "tasks/task.hh"
#include "util/char.hh"
#include "util/keycode.hh"

class ObjText;
class TextBuffer;

class SelectText : public Task {
public:
  SelectText( ObjText*, bool newTextObject, const Point& clickPos, Settings& );
  bool Draw( FaintDC&, Overlays&, const Point& );
  bool DrawBeforeZoom(Layer::type) const;
  Command* GetCommand();
  Cursor::type GetCursor( const CursorPositionInfo& ) const;
  Task* GetNewTask();
  IntRect GetRefreshRect( const IntRect&, const Point& ) const;
  TaskResult LeftDown( const CursorPositionInfo& );
  TaskResult LeftUp( const CursorPositionInfo& );
  TaskResult Motion( const CursorPositionInfo& );
  TaskResult Preempt( const CursorPositionInfo& );
private:
  SelectText& operator=(const SelectText&); // Prevent assignment
  bool m_newTextObject;
  Point m_origin;
  Settings& m_settings;
  ObjText* m_textObject;
};

#endif
