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
  void Draw( FaintDC&, Overlays&, const Point& ) override;
  bool DrawBeforeZoom(Layer) const override;
  Command* GetCommand() override;
  Cursor GetCursor( const CursorPositionInfo& ) const override;
  Task* GetNewTask() override;
  IntRect GetRefreshRect( const IntRect&, const Point& ) const override;
  TaskResult LeftDown( const CursorPositionInfo& ) override;
  TaskResult LeftUp( const CursorPositionInfo& ) override;
  TaskResult Motion( const CursorPositionInfo& ) override;
  TaskResult Preempt( const CursorPositionInfo& ) override;
private:
  SelectText& operator=(const SelectText&); // Prevent assignment
  bool m_newTextObject;
  Point m_origin;
  Settings& m_settings;
  ObjText* m_textObject;
};

#endif
