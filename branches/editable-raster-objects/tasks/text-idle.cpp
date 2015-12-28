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

#include "objects/objtext.hh"
#include "tasks/edit-text.hh"
#include "tasks/make-text-box.hh"
#include "tasks/text-idle.hh"
#include "util/formatting.hh"
#include "util/toolutil.hh"

TextIdle::TextIdle(Settings& s)
 : m_settings(s)
{}

void TextIdle::Draw( FaintDC&, Overlays&, const Point& ){
}

bool TextIdle::DrawBeforeZoom( Layer layer ) const{
  return layer == Layer::RASTER;
}

Command* TextIdle::GetCommand(){
  return nullptr;
}

Cursor TextIdle::GetCursor( const CursorPositionInfo& info ) const{
  return (hovered_selected_text(info) == nullptr) ?
    Cursor::TEXT_CROSS :
    Cursor::CARET;
}

IntRect TextIdle::GetRefreshRect( const IntRect&, const Point& ) const{
  return IntRect();
}

Task* TextIdle::GetNewTask(){
  return m_newTask.Retrieve();
}

TaskResult TextIdle::LeftDown( const CursorPositionInfo& info ){
  ObjText* objText = hovered_selected_text(info);
  if ( objText == nullptr ){
    m_newTask.Set(new MakeTextBox(info.pos, m_settings));
  }
  else {
    m_newTask.Set(new EditText(objText, m_settings));
  }
  return TASK_CHANGE;
}

TaskResult TextIdle::LeftUp( const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult TextIdle::Motion( const CursorPositionInfo& info ){
  if ( hovered_selected_text(info) ){
    info.status->SetMainText("Click to edit the text object");
  }
  else {
    info.status->SetMainText("Click to start drawing a text-box");
  }
  info.status->SetText(str(info.pos));
  return TASK_NONE;
}

TaskResult TextIdle::Preempt( const CursorPositionInfo& ){
  return TASK_NONE;
}
