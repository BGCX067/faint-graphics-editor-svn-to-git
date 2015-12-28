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

#include "app/getappcontext.hh"
#include "objects/object.hh"
#include "rendering/overlay.hh"
#include "tasks/select-object-rectangle.hh"
#include "util/flag.hh"
#include "util/objutil.hh"

static bool add_enclosed( const CursorPositionInfo& info ){
  // Whether the surrounded objects should be added to the current set
  // of objects, or set as the selection, losing any previous
  // selection
  return fl(TOOLMODIFIER2, info.modifiers);
}

static bool remove_enclosed( const CursorPositionInfo& info ){
  // Whether the enclosed objects should be removed from the current
  // selection
  return fl(TOOLMODIFIER1, info.modifiers);
}

static void modify_selection( const CursorPositionInfo& info, const objects_t& enclosed ){
  if ( enclosed.empty() ){
    return;
  }

  // Perform the modification depending on modifiers
  if ( remove_enclosed(info) ){
    info.canvas->DeselectObjects(enclosed);
  }
  else {
    info.canvas->SelectObjects(enclosed, deselect_old(!add_enclosed(info)));
  }
}

SelectObjectRect::SelectObjectRect( const Point& p0, const Point& p1, Settings& settings )
  : m_p0(p0),
    m_p1(p1),
    m_settings(settings)
{}

void SelectObjectRect::Draw( FaintDC&, Overlays& overlays, const Point& p ){
  m_p1 = p;
  overlays.Rectangle(Rect(m_p0, m_p1));
}

bool SelectObjectRect::DrawBeforeZoom(Layer) const{
  return false;
}

Cursor SelectObjectRect::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::CROSSHAIR;
}

Command* SelectObjectRect::GetCommand(){
  return nullptr;
}

Task* SelectObjectRect::GetNewTask(){
  return nullptr;
}

IntRect SelectObjectRect::GetRefreshRect( const IntRect&, const Point& ) const{
  return floored( inflated(Rect(m_p0, m_p1), 2) );
}

TaskResult SelectObjectRect::LeftDown( const CursorPositionInfo& ){
  // Shan't happen
  return TASK_NONE;
}

TaskResult SelectObjectRect::LeftUp( const CursorPositionInfo& info ){
  const objects_t enclosed = get_intersected(info.canvas->GetObjects(), Rect(m_p0, m_p1));
  modify_selection(info, enclosed);
  return TASK_CHANGE;
}

TaskResult SelectObjectRect::Motion( const CursorPositionInfo& ){
  return TASK_DRAW;
}

TaskResult SelectObjectRect::Preempt( const CursorPositionInfo& ){
  return TASK_CHANGE;
}

void SelectObjectRect::SelectionChange(){
  AppContext& app(GetAppContext());
  m_settings = get_object_settings(app.GetActiveCanvas().GetObjectSelection());
  app.UpdateShownSettings();
}
