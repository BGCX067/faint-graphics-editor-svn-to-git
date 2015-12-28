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

SelectObjectRect::SelectObjectRect( const Point& p0, const Point& p1, Settings& settings )
  : m_p0(p0),
    m_p1(p1),
    m_settings(settings)
{}

bool SelectObjectRect::Draw( FaintDC&, Overlays& overlays, const Point& p ){
  m_p1 = p;
  overlays.Rectangle(Rect(m_p0, m_p1));
  return true;
}

bool SelectObjectRect::DrawBeforeZoom(Layer::type) const{
  return false;
}

Cursor::type SelectObjectRect::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::CROSSHAIR;
}

Command* SelectObjectRect::GetCommand(){
  return 0;
}

Task* SelectObjectRect::GetNewTask(){
  return 0;
}

IntRect SelectObjectRect::GetRefreshRect( const IntRect&, const Point& ) const{
  return truncated( inflated(Rect(m_p0, m_p1), 2) );
}

TaskResult SelectObjectRect::LeftDown( const CursorPositionInfo& ){
  // Shan't happen
  return TASK_NONE;
}

bool remove_selection(int modifiers){
  return fl(TOOLMODIFIER1, modifiers);
}

bool add_selection( int modifiers ){
  return !remove_selection(modifiers);
}

TaskResult SelectObjectRect::LeftUp( const CursorPositionInfo& info ){
  const objects_t& objects = info.canvas->GetObjects();
  objects_t inside(get_intersected(objects, Rect(m_p0, m_p1)));

  if ( add_selection(info.modifiers ) ){
    info.canvas->SelectObjects(inside, deselect_old(false));
  }
  else if ( remove_selection(info.modifiers) ){
    info.canvas->DeselectObjects(inside);
  }
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
