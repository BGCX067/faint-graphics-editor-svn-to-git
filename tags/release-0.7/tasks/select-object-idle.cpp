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

#include <cassert>
#include "app/getappcontext.hh"
#include "commands/delete-point-cmd.hh"
#include "objects/object.hh"
#include "rendering/overlay.hh"
#include "tasks/edit-text.hh"
#include "tasks/move-object.hh"
#include "tasks/move-point.hh"
#include "tasks/resize-object.hh"
#include "tasks/rotate-object.hh"
#include "tasks/select-object-idle.hh"
#include "tasks/select-object-rectangle.hh"
#include "util/commandutil.hh"
#include "util/flag.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"
#include "util/toolutil.hh"
#include "util/util.hh"

const int dragThreshold = 3; // Pixels required for determining click or drag

class SelectObjectIdleImpl{
public:
  Point clickPos;
  bool mouseDown;
  bool fullRefresh; // Fixme: Necessary?
  PendingTask newTask;
  PendingCommand command;
  // Fixme: Both necessary?
  Object* dragCandidate;
  Object* deselectCandidate;
};

void reset_impl( SelectObjectIdleImpl& impl, bool fullRefresh=false ){
  impl.clickPos = Point(0,0);
  impl.mouseDown = false;
  impl.fullRefresh = fullRefresh;
  impl.newTask.Set(0);
  impl.command.Set(0);
  impl.dragCandidate = 0;
  impl.deselectCandidate = 0;
}

bool corner_handle( int handleIndex ){ // Fixme: Move to utility file for canvas handles
  // The first four handles are at the corners. The following
  // are edge handles, i.e. for resizing up/down, left/right
  return handleIndex < 4;
}

TaskResult clicked_nothing( SelectObjectIdleImpl& impl, const CursorPositionInfo& info ){
  impl.mouseDown = true;
  impl.clickPos = info.pos;
  if ( fl(TOOLMODIFIER1, info.modifiers) || fl(TOOLMODIFIER2, info.modifiers) ){
    // Do not deselect objects if a modifier is held, as this probably
    // means either that the user mis-clicked or intends to draw a
    // selection or deselection-rectangle
    impl.mouseDown = true;
    impl.clickPos = info.pos;
    return TASK_NONE;
  }

  // Clicked outside object - deselect objects
  info.canvas->DeselectObjects();
  // FIXME
  //SettingsFromObjects(info.canvas->GetSelectedObjects() );
  GetAppContext().UpdateShownSettings();
  impl.fullRefresh = true;
  return TASK_DRAW;
}

TaskResult clicked_selected( SelectObjectIdleImpl& impl, const CursorPositionInfo& info ){
  Object* obj(info.object);
  const int hit = info.hitStatus;
  if ( hit == HIT_RESIZE_POINT && resize_handles_enabled(obj) ){
    bool copy = fl( RIGHT_MOUSE, info.modifiers );
    if ( fl( TOOLMODIFIER1, info.modifiers ) && corner_handle(info.handleIndex) ){
      // Rotate the object around the corner handle
      impl.newTask.Set(new RotateObjectTask( copy ? obj->Clone() : obj, info.handleIndex, copy ? MoveMode::COPY : MoveMode::MOVE ));
    }
    else {
      // Resize the object
      impl.newTask.Set(new ResizeObjectTask( copy ? obj->Clone() : obj, info.handleIndex, copy ? MoveMode::COPY : MoveMode::MOVE ));
    }
    return TASK_CHANGE;
  }
  else if ( hit == HIT_MOVABLE_POINT && point_edit_enabled(obj) ){
    bool rightMouse = fl( RIGHT_MOUSE, info.modifiers );
    // Right mouse means remove point for extendable objects
    if ( rightMouse && obj->Extendable() ){
      // Delete a point if possible
      if ( obj->CanRemovePoint() ){
	impl.command.Set( new DeletePointCommand(obj, info.handleIndex ));
	return TASK_COMMIT;
      }
      // Not possible to delete points for some reason (e.g. at
      // minimum number of points) - do nothing.
      return TASK_NONE;
    }
    // Right mouse means clone-object-from-point for non-extendable
    // objects
    const bool copy = rightMouse;
    if ( copy ){
      info.canvas->DeselectObject( obj );
    }
    // Move the point (possibly cloning the object too)
    impl.newTask.Set(new MovePointTask( copy ? obj->Clone() : obj, info.handleIndex, copy ? MovePointMode::CLONE : MovePointMode::MOVE ) );
    return TASK_CHANGE;
  }
  else if ( hit == HIT_EXTENSION_POINT && point_edit_enabled(obj) ){
    impl.newTask.Set(new MovePointTask( obj, info.handleIndex, MovePointMode::INSERT ));
    return TASK_CHANGE;
  }
  else if ( hit == HIT_INSIDE || hit == HIT_NEAR  || hit == HIT_BOUNDARY  ){
    if ( fl(TOOLMODIFIER1, info.modifiers) ){
      // Ctrl-held while clicking a selected object - This can either
      // mean deselect or drag and copy, depending on drag-distance
      // (see Motion(...))
      if (fl( RIGHT_MOUSE, info.modifiers )){
        if ( is_raster_object(info.object) ){
          // Handle ctrl+right-mouse on a raster object (set background)
          ObjRaster* rasterObj = as_ObjRaster(info.object);
          faint::Color color(color_at(rasterObj, info.pos));
          impl.command.Set( get_change_raster_background_command(rasterObj, color) );
          return TASK_COMMIT;
        }
        return TASK_NONE;
      }
      impl.clickPos = info.pos;
      impl.mouseDown = true;
      impl.deselectCandidate = obj;
      impl.dragCandidate = 0;
      return TASK_NONE;
    }
    impl.newTask.Set(new MoveObjectTask( obj,
	info.canvas->GetObjectSelection(),
	info.pos - obj->GetTri().P0(),
	MoveMode::MOVE ));
    impl.deselectCandidate = 0;
    impl.mouseDown = false;
    return TASK_CHANGE;
  }
  assert( false ); // Can't happen
  return TASK_NONE;
}

TaskResult clicked_unselected( SelectObjectIdleImpl& impl, const CursorPositionInfo& info ){
  impl.dragCandidate = info.object;
  impl.clickPos = info.pos;
  impl.mouseDown = true;
  info.canvas->SelectObject( info.object,
    deselect_old(!(fl( TOOLMODIFIER1,info.modifiers ))));

  // FIXME
  // SettingsFromObjects( info.canvas->GetSelectedObjects() );
  GetAppContext().UpdateShownSettings();
  impl.fullRefresh = true;
  return TASK_DRAW;
}

std::string status_for_object( const CursorPositionInfo& info ){
  if ( info.hitStatus == HIT_MOVABLE_POINT ){
    if ( info.object->CanRemovePoint() ){
      return "Left Click=Move, Right Click=Delete";
    }
    else {
      return "Left click to move the point";
    }
  }
  else if ( info.hitStatus == HIT_EXTENSION_POINT ){
    return "Left Click and Drag to add a point";
  }
  else if ( hovered_selected_text(info, SearchMode::include_grouped ) != 0 ){
    return "Double Click to edit the text";
  }
  else if ( info.hitStatus == HIT_RESIZE_POINT && corner_handle(info.handleIndex ) ){
    return fl(TOOLMODIFIER1, info.modifiers) ?
      space_sep("Click to rotate the", info.object->GetType()) :
      space_sep("Click to resize the", info.object->GetType()) + ". Ctrl=Rotate";
  }
  else if ( info.hitStatus == HIT_RESIZE_POINT ){
    return space_sep("Click to resize the", info.object->GetType());
  }
  else if ( info.object != 0 && !info.objSelected ){
    return std::string("Left click to select the ") + info.object->GetType();
  }
  return "";
}

SelectObjectIdle::SelectObjectIdle( Settings& settings )
  : m_settings(settings)
{
  m_impl = new SelectObjectIdleImpl;
  reset_impl(*m_impl);
}

SelectObjectIdle::~SelectObjectIdle(){
  delete m_impl;
}

void SelectObjectIdle::Activate(){
  reset_impl(*m_impl);
  m_impl->fullRefresh = true;
}

bool SelectObjectIdle::Draw( FaintDC&, Overlays& overlays, const Point& ){
  const objects_t& objects = GetAppContext().GetActiveCanvas().GetObjectSelection();
  for ( size_t i = 0; i != objects.size(); i++ ){
    const Object* obj(objects[i]);
    if ( point_edit_disabled(obj) ){
      // Skip drawing points for this object
      continue;
    }

    // Draw circles for each of the selected objects' movable points.
    std::vector<Point> movablePoints = obj->GetMovablePoints();
    for ( std::vector<Point>::const_iterator it = movablePoints.begin(); it != movablePoints.end(); ++it ){
      overlays.MovablePoint( *it );
    }
    const std::vector<Point> extensionPoints = obj->GetExtensionPoints();
    for ( std::vector<Point>::const_iterator it = extensionPoints.begin(); it != extensionPoints.end(); ++it ){
      overlays.ExtensionPoint( *it );
    }
  }
  return true;
}

bool SelectObjectIdle::DrawBeforeZoom(Layer::type) const{
  return false;
}

Command* SelectObjectIdle::GetCommand(){
  return m_impl->command.Retrieve();
}

Cursor::type SelectObjectIdle::GetCursor( const CursorPositionInfo& info ) const{
  if ( info.objSelected ){
    if ( info.hitStatus == HIT_RESIZE_POINT ){
      if ( fl( TOOLMODIFIER1, info.modifiers ) && corner_handle(info.handleIndex) ){
        return Cursor::ROTATE_RIGHT;
      }
      if ( info.handleIndex == 0 || info.handleIndex == 3 ){
        return Cursor::RESIZE_NE;
      }
      else if ( info.handleIndex == 1 || info.handleIndex == 2 ) {
        return Cursor::RESIZE_NW;
      }
      else if ( info.handleIndex == 4 || info.handleIndex == 5 ){
        return Cursor::RESIZE_WE;
      }
      else if ( info.handleIndex == 6 || info.handleIndex == 7 ){
        return Cursor::RESIZE_NS;
      }
    }
    else if ( info.hitStatus == HIT_INSIDE || info.hitStatus == HIT_NEAR || info.hitStatus == HIT_BOUNDARY ){
      return fl( TOOLMODIFIER1, info.modifiers ) ? Cursor::CLONE : Cursor::MOVE;
    }
    else if ( point_edit_enabled(info.object) ) {
      if ( info.hitStatus == HIT_MOVABLE_POINT ) {
	return Cursor::MOVE_POINT;
      }
      else if ( info.hitStatus == HIT_EXTENSION_POINT ){
	return Cursor::ADD_POINT;
      }
    }
  }
  return Cursor::ARROW;
}

Task* SelectObjectIdle::GetNewTask(){
  return m_impl->newTask.Retrieve();
}

IntRect SelectObjectIdle::GetRefreshRect( const IntRect& visible, const Point& ) const{
  if ( m_impl->fullRefresh ){
    m_impl->fullRefresh = false;
    return visible;
  }
  return IntRect(IntPoint(0, 0), IntSize(0, 0));
}

TaskResult SelectObjectIdle::LeftDoubleClick( const CursorPositionInfo& info ){
  if ( !info.objSelected ) {
    return TASK_NONE;
  }

  bool toggledPoints = toggle_edit_points(info.object);
  if ( toggledPoints ){
    SelectionChange();
    return TASK_DRAW;
  }

  // Retrieve clicked text object or grouped text object, if any
  ObjText* objText = hovered_selected_text(info, SearchMode::include_grouped);
  if ( objText != 0 ){
    // Start editing the double-clicked text object
    m_impl->newTask.Set(new EditText(objText, m_settings));
    return TASK_CHANGE;
  }
  return TASK_NONE;
}

TaskResult SelectObjectIdle::LeftDown( const CursorPositionInfo& info ){
  info.status->SetMainText("");
  if ( info.object == 0 ){
    return clicked_nothing(*m_impl, info);
  }
  else if ( info.objSelected ){
    return clicked_selected(*m_impl, info );
  }
  return clicked_unselected(*m_impl, info);
}

TaskResult SelectObjectIdle::LeftUp( const CursorPositionInfo& info ){
  if ( m_impl->mouseDown && fl( TOOLMODIFIER1, info.modifiers ) && distance( m_impl->clickPos, info.pos ) < dragThreshold ) {
    // Mouse click without movement with ctrl held means deselect
    // clicked object
    info.canvas->DeselectObject( m_impl->deselectCandidate );
    reset_impl(*m_impl, true);
    return TASK_DRAW;
  }
  reset_impl(*m_impl);
  return TASK_NONE;
}

TaskResult SelectObjectIdle::Motion( const CursorPositionInfo& info ){
  info.status->SetText(str(info.pos));

  if ( !m_impl->mouseDown ){
    // Mouse not held, update status text
    info.status->SetMainText(status_for_object(info));
    return TASK_NONE;
  }

  info.status->SetMainText( "" );
  bool cloneBtn = fl( TOOLMODIFIER1, info.modifiers );
  if ( m_impl->deselectCandidate != 0 && cloneBtn && distance( m_impl->clickPos, info.pos ) >= dragThreshold ){ // Fixme: Simplify
    // Clone the selected object(s)
    m_impl->mouseDown = false;
    objects_t objects = info.canvas->GetObjectSelection();
    info.canvas->DeselectObjects();
    size_t mainIndex = find_object_index( m_impl->deselectCandidate, objects );
    assert(mainIndex != objects.size() );
    objects_t clones(clone(objects));
    Object* mainObject = clones[mainIndex];
    m_settings = get_object_settings(clones);
    m_impl->newTask.Set(new MoveObjectTask( mainObject,
	clones,
	info.pos - mainObject->GetTri().P0(),
	MoveMode::COPY ));
    m_impl->deselectCandidate = 0;
    return TASK_CHANGE;
  }
  else if ( m_impl->dragCandidate != 0 && !cloneBtn && distance(m_impl->clickPos, info.pos ) >= dragThreshold ){ // Fixme: Simplify
    // Move the selected object(s)
    m_impl->newTask.Set( new MoveObjectTask( m_impl->dragCandidate,
	info.canvas->GetObjectSelection(),
	info.pos - m_impl->dragCandidate->GetTri().P0(),
	MoveMode::MOVE ));
    return TASK_CHANGE;
  }
  else if ( m_impl->dragCandidate == 0 && m_impl->deselectCandidate == 0 ){
    faint::coord dist = distance( m_impl->clickPos, info.pos );
    if ( abs(dist >= dragThreshold ) ){
      m_impl->newTask.Set( new SelectObjectRect(m_impl->clickPos, info.pos, m_settings) );
      if ( m_impl->dragCandidate != 0 ){
        // Deselect the ctrl-clicked object
        info.canvas->DeselectObject( m_impl->dragCandidate );
	reset_impl(*m_impl, true);
      }
      return TASK_CHANGE;
    }
  }
  return TASK_NONE;
}

TaskResult SelectObjectIdle::Preempt( const CursorPositionInfo& ){
  return TASK_NONE;
}

void SelectObjectIdle::SelectionChange(){
  AppContext& app(GetAppContext());
  m_settings = get_object_settings(app.GetActiveCanvas().GetObjectSelection());
  app.UpdateShownSettings();
}
