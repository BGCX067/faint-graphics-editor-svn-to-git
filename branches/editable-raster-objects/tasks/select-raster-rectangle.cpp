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

#include "geo/tri.hh"

#include "rendering/faintdc.hh"
#include "tasks/select-raster-rectangle.hh"
#include "tasks/selection-idle.hh"
#include "tools/tool.hh"
#include "commands/set-raster-selection-cmd.hh"
#include "util/formatting.hh"
#include "util/image.hh"
#include "util/settings.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

SelectRectangle::SelectRectangle( const Point& startPos, Settings& s, bool appendCommand )
  : RasterSelectionTask(s),
    m_p0(startPos),
    m_p1(startPos),
    m_maxDistance(LITCRD(0.0)),
    m_appendCommand(appendCommand)
{}

void SelectRectangle::Draw( FaintDC& dc, Overlays&, const Point& ){
  dc.Rectangle( tri_from_rect(Rect(m_p0, m_p1)), selection_rectangle_settings() );
}

bool SelectRectangle::DrawBeforeZoom(Layer)const {
  return true;
}

Command* SelectRectangle::GetCommand(){
  return m_command.Retrieve();
}

Cursor SelectRectangle::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::CROSSHAIR;
}

Task* SelectRectangle::GetNewTask(){
  return nullptr;
}

IntRect SelectRectangle::GetRefreshRect( const IntRect&, const Point& ) const{
  return IntRect( floored(m_p0), floored(m_p1) );
}

TaskResult SelectRectangle::LeftDown( const CursorPositionInfo& ){
  // Rectangle selection is already active after construction
  return TASK_NONE;
}

TaskResult SelectRectangle::LeftUp( const CursorPositionInfo& info ){
  if ( m_maxDistance < 1.0 ){
    return TASK_CHANGE;
  }
  else {
    IntRect rect( intersection(IntRect(floored(m_p0), floored(m_p1)),
    faint::image_rect(info.canvas->GetImage() )));
    m_command.Set( new SetRasterSelection( New(RasterSelectionState(rect)),
      Old(RasterSelectionState()),
      "Select Rectangle",
      m_appendCommand ) );
    return TASK_COMMIT_AND_CHANGE;
  }
}

TaskResult SelectRectangle::Motion( const CursorPositionInfo& info ){
  m_p1 = info.pos;
  m_maxDistance = std::max( m_maxDistance, distance( m_p0, m_p1 ) );
  info.status->SetText(str_from_to(m_p0, m_p1), 0);
  return TASK_DRAW;
}

TaskResult SelectRectangle::Preempt( const CursorPositionInfo& ){
  return TASK_CHANGE;
}
