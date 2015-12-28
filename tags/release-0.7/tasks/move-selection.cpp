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
#include "commands/set-raster-selection-cmd.hh"
#include "tasks/move-selection.hh"
#include "util/image.hh"
#include "util/util.hh"

MoveSelection::MoveSelection(const IntPoint& offset, const IntPoint& topLeft, const copy_selected& copy, bool float_selected, CanvasInterface& canvas, Settings& s )
  : RasterSelectionTask(s),
    m_lastOrigin(topLeft),
    m_offset(offset)
{
  // Do all adjustments through a chimera. The final changes to the
  // real selection are done in a command.
  m_selection = new RasterSelection(canvas.GetRasterSelection());
  assert(!m_selection->Empty());
  if ( float_selected ){
    m_selection->Clip(faint::image_rect(canvas.GetImage()));
    if ( !m_selection->Empty() ){
      m_selection->BeginFloat(canvas.GetBitmap(), copy);
    }
  }
  else {
    IntRect r = m_selection->GetRect();
    m_selection->Deselect();
    m_selection->SetRect(r);
  }
  canvas.SetChimera(m_selection);
}

MoveSelection::~MoveSelection(){
  delete m_selection;
  m_selection = 0;
}

bool MoveSelection::Draw( FaintDC&, Overlays&, const Point& ){
  return false;
}

bool MoveSelection::DrawBeforeZoom(Layer::type) const{
  return false;
}

Cursor::type MoveSelection::GetCursor(const CursorPositionInfo& ) const{
  return Cursor::MOVE;
}

Command* MoveSelection::GetCommand(){
  return m_command.Retrieve();
}

Task* MoveSelection::GetNewTask(){
  return 0;
}

IntRect MoveSelection::GetRefreshRect(const IntRect&, const Point&) const{
  return m_lastRect;
}

TaskResult MoveSelection::LeftDown( const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult MoveSelection::LeftUp( const CursorPositionInfo& info ){
  info.canvas->ClearRasterSelectionChimera();
  m_command.Set( new SetRasterSelection( New(m_selection->GetState()),
      Old(info.canvas->GetRasterSelection().GetState()),
      m_selection->Floating() ? "Move Selected Content" : "Move Selection" ) );
  return TASK_COMMIT_AND_CHANGE;
}

TaskResult MoveSelection::Motion( const CursorPositionInfo& info ){
  if ( m_selection->Empty() ){
    return TASK_NONE;
  }
  IntPoint topLeft = truncated(info.pos) - m_offset;
  if ( fl(TOOLMODIFIER2, info.modifiers) ){
    // Fixme: Add required min distance
    // > // Check the effects of constraining orthogonally:
    // > //
    // > // Require a certain distance to change direction
    // > // (horizontal/vertical), to avoid changing direction when close
    // > // to origo, so that the constraint can be used even when
    // > // fine-tuning, without unexpected switch
    constrain_pos(topLeft, m_lastOrigin);
  }

  m_selection->Move( topLeft );
  m_lastRect = m_selection->GetRect();
  info.status->SetMainText("");
  return TASK_DRAW;
}

TaskResult MoveSelection::Preempt( const CursorPositionInfo& info ){
  info.canvas->ClearRasterSelectionChimera();
  return TASK_CHANGE;
}


