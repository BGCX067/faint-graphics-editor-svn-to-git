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

#include "tasks/edit-text.hh"
#include "tasks/make-text-box.hh"
#include "rendering/overlay.hh"
#include "util/formatting.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

MakeTextBox::MakeTextBox( const Point& startPos, Settings& s )
  : m_maxDistance(LITCRD(0.0)),
    m_p0(startPos),
    m_p1(startPos),
    m_settings(s)
{}

void MakeTextBox::Draw( FaintDC&, Overlays& overlays, const Point& ){
  overlays.Rectangle( Rect(m_p0, m_p1) );
}

bool MakeTextBox::DrawBeforeZoom( Layer ) const{
  return false;
}

TaskResult MakeTextBox::Motion( const CursorPositionInfo& info ){
  m_p1 = info.pos;
  info.status->SetMainText("Release to start editing text");
  info.status->SetText(str_from_to(m_p0, m_p1));
  m_maxDistance = std::max( m_maxDistance, distance( m_p0, m_p1 ) );
  return TASK_DRAW;
}

Command* MakeTextBox::GetCommand(){
  return nullptr;
}

Cursor MakeTextBox::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::CROSSHAIR;
}

Task* MakeTextBox::GetNewTask(){
  return m_newTask.Retrieve();
}

IntRect MakeTextBox::GetRefreshRect( const IntRect&, const Point& ) const{
  return IntRect( floored(m_p0), floored(m_p1) );
}

TaskResult MakeTextBox::LeftDown( const CursorPositionInfo& ){
  // Rectangle selection is already active after construction
  return TASK_NONE;
}

TaskResult MakeTextBox::LeftUp( const CursorPositionInfo& ){
  if ( m_maxDistance < 1.0 ){
    // To prevent selecting a single pixel by mistake, require some
    // distance at some point during this selection
    m_newTask.Set(DefaultTask());
  }
  else {
    m_newTask.Set(new EditText( Rect(m_p0, m_p1), faint::utf8_string(""), m_settings));
  }
  return TASK_CHANGE;
}

TaskResult MakeTextBox::Preempt( const CursorPositionInfo& ){
  m_newTask.Set(DefaultTask());
  return TASK_CHANGE;
}
