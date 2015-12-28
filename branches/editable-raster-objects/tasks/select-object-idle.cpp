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
#include "tasks/resize-object-aligned.hh"
#include "tasks/rotate-object.hh"
#include "tasks/select-object-idle.hh"
#include "tasks/select-object-rectangle.hh"
#include "util/commandutil.hh"
#include "util/flag.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"
#include "util/toolutil.hh"
#include "util/util.hh"

static bool is_drag( const Point& p0, const Point& p1 ){
  return distance(p0, p1) >= 3;
}

static bool not_drag( const Point& p0, const Point& p1 ){
  return !is_drag(p0,p1);
}

static bool mod_clone_rotate_toggle( const CursorPositionInfo& info ){
  return fl(TOOLMODIFIER1, info.modifiers);
}

static bool (&mod_clone)(const CursorPositionInfo&) = mod_clone_rotate_toggle;
static bool (&mod_rotate)(const CursorPositionInfo&) = mod_clone_rotate_toggle;
static bool (&mod_toggle_select)(const CursorPositionInfo&) = mod_clone_rotate_toggle;

static bool mod_select_multi_add( const CursorPositionInfo& info ){
  return fl(TOOLMODIFIER2, info.modifiers);
}

static std::string status_for_object( const CursorPositionInfo& info ){
  if ( info.hitStatus == Hit::MOVABLE_POINT ){
    if ( info.object->CanRemovePoint() ){
      return "Left Click=Move, Right Click=Delete";
    }
    else {
      return "Left click to move the point";
    }
  }
  else if ( info.hitStatus == Hit::EXTENSION_POINT ){
    return "Left Click and Drag to add a point";
  }
  else if ( hovered_selected_text(info, SearchMode::include_grouped ) != nullptr ){
    return "Double Click to edit the text";
  }
  else if ( info.hitStatus == Hit::RESIZE_POINT && corner_handle(info.handleIndex ) ){
    return fl(TOOLMODIFIER1, info.modifiers) ?
      space_sep("Click to rotate the", info.object->GetType()) :
      space_sep("Click to resize the", info.object->GetType()) + ". Ctrl=Rotate";
  }
  else if ( info.hitStatus == Hit::RESIZE_POINT ){
    return space_sep("Click to resize the", info.object->GetType());
  }
  else if ( info.object != nullptr && !info.objSelected ){
    return std::string("Left click to select the ") + info.object->GetType();
  }
  return "";
}

struct ClickedObject{
  // Keeps track of whether a clicked object was selected when clicked
  Object* object;
  bool wasSelected;
};

static bool quick_draggable( const ClickedObject& clicked ){
  // An object that wasn't selected can be instantly dragged, saving a
  // click to select it first.
  return clicked.object != nullptr && !clicked.wasSelected;
}

static bool (&selectable)(const ClickedObject&) = quick_draggable;

static bool deselectable( const ClickedObject& clicked ){
  // For the clicked object to be deselectable, it must have been selected
  // already when it was clicked.
  return clicked.object != nullptr && clicked.wasSelected;
}

static bool no_object( const ClickedObject& clicked ){
  return clicked.object == 0;
}

class category_select_object_idle;
typedef Unique<bool, category_select_object_idle, 0> was_selected; // Whether an object was selected when clicked

class SelectObjectIdleImpl{
public:
  SelectObjectIdleImpl(Settings& in_settings)
    : settings(in_settings)
  {
    Reset();
  }
  void Reset(bool in_fullRefresh=false){
    clickPos = Point(0,0);
    mouseDown = false;
    fullRefresh = in_fullRefresh;
    newTask.Set(nullptr);
    command.Set(nullptr);
    clicked.object = nullptr;
    clicked.wasSelected = false;
  }

  void SetClickedObject( Object* obj, const Point& pos, const was_selected& wasSelected ){
    clicked.object = obj;
    clicked.wasSelected = wasSelected.Get();
    clickPos = pos;
    mouseDown = true;
  }

  Settings& settings;
  Point clickPos;
  bool mouseDown;
  bool fullRefresh;
  PendingTask newTask;
  PendingCommand command;
  ClickedObject clicked;
private:
  SelectObjectIdleImpl& operator=(const SelectObjectIdleImpl&);
};

static TaskResult clicked_nothing( SelectObjectIdleImpl& impl, const CursorPositionInfo& info ){
  impl.mouseDown = true;
  impl.clickPos = info.pos;
  if ( mod_clone_rotate_toggle(info) || mod_select_multi_add(info) ){
    // Do not deselect objects if a modifier is held, as this probably
    // means either that the user mis-clicked or intends to draw a
    // selection or deselection-rectangle
    impl.mouseDown = true;
    impl.clickPos = info.pos;
    return TASK_NONE;
  }

  // Clicked outside object - deselect objects
  info.canvas->DeselectObjects();
  GetAppContext().UpdateShownSettings();
  impl.fullRefresh = true;
  return TASK_DRAW;
}

static TaskResult clicked_selected( SelectObjectIdleImpl& impl, const CursorPositionInfo& info ){
  Object* obj(info.object);
  const Hit hit = info.hitStatus;
  if ( hit == Hit::RESIZE_POINT && resize_handles_enabled(obj) ){
    bool copy = fl( RIGHT_MOUSE, info.modifiers );
    if ( mod_rotate(info) && corner_handle(info.handleIndex) ){
      // Rotate the object around the corner handle
      impl.newTask.Set(new RotateObjectTask( copy ? obj->Clone() : obj, info.handleIndex, copy ? MoveMode::COPY : MoveMode::MOVE ));
    }
    else {
      // Resize the object
      if ( object_aligned_resize(obj) ){
        impl.newTask.Set(new ResizeObjectAlignedTask( copy ? obj->Clone() : obj, info.handleIndex, copy ? MoveMode::COPY : MoveMode::MOVE ));
      }
      else{
        impl.newTask.Set(new ResizeObjectTask( copy ? obj->Clone() : obj, info.handleIndex, copy ? MoveMode::COPY : MoveMode::MOVE ));
      }
    }
    return TASK_CHANGE;
  }
  else if ( hit == Hit::MOVABLE_POINT && point_edit_enabled(obj) ){
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
  else if ( hit == Hit::EXTENSION_POINT && point_edit_enabled(obj) ){
    if ( fl( LEFT_MOUSE, info.modifiers ) ){
      impl.newTask.Set(new MovePointTask( obj, info.handleIndex, MovePointMode::INSERT ));
      return TASK_CHANGE;
    }
    return TASK_NONE;
  }
  else if ( hit == Hit::INSIDE || hit == Hit::NEARBY  || hit == Hit::BOUNDARY  ){
    if ( mod_clone_rotate_toggle(info) ){
      // Clicked inside object with clone/deselect modifier. Cloning
      // or deselecting depends on drag-distance, see Motion(..).
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

      impl.SetClickedObject(obj, info.pos, was_selected(true));
      return TASK_NONE;
    }
    else if (fl( RIGHT_MOUSE, info.modifiers ) ){
        bool toggledAlign = toggle_object_aligned_resize(info.object);
        if ( toggledAlign ){
          return TASK_DRAW;
        }
        return TASK_NONE;
    }
    impl.Reset();
    impl.newTask.Set(new MoveObjectTask( obj,
    info.canvas->GetObjectSelection(),
    info.pos - obj->GetTri().P0(),
    MoveMode::MOVE ));
    return TASK_CHANGE;

  }
  assert( false ); // Can't happen
  return TASK_NONE;
}

static TaskResult clicked_unselected( SelectObjectIdleImpl& impl, const CursorPositionInfo& info ){
  impl.SetClickedObject(info.object, info.pos, was_selected(false));
  info.canvas->SelectObject( info.object, deselect_old(!mod_toggle_select(info)));
  GetAppContext().UpdateShownSettings();
  impl.fullRefresh = true;
  return TASK_DRAW;
}

static TaskResult clone_selected_objects( SelectObjectIdleImpl& impl, CanvasInterface& canvas, const Point& mousePos ){
  impl.mouseDown = false;
  objects_t objects = canvas.GetObjectSelection();
  canvas.DeselectObjects();
  size_t mainIndex = find_object_index( impl.clicked.object, objects );
  assert(mainIndex != objects.size());
  objects_t clones(clone(objects));
  Object* mainObject = clones[mainIndex];
  impl.Reset();
  impl.settings = get_object_settings(clones);
  impl.newTask.Set(new MoveObjectTask( mainObject,
      clones,
      mousePos - mainObject->GetTri().P0(),
      MoveMode::COPY ) );
  return TASK_CHANGE;
}

static TaskResult move_selected_objects( SelectObjectIdleImpl& impl, const objects_t& objects, const Point& mousePos ){
  Object* mainObject = impl.clicked.object;
  impl.newTask.Set(new MoveObjectTask( mainObject,
      objects,
      mousePos - mainObject->GetTri().P0(),
      MoveMode::MOVE ) );
  return TASK_CHANGE;
}

static TaskResult select_object_rectangle( SelectObjectIdleImpl& impl, const Point& mousePos  ){
  impl.newTask.Set( new SelectObjectRect(impl.clickPos, mousePos, impl.settings) );
  return TASK_CHANGE;
}

SelectObjectIdle::SelectObjectIdle( Settings& settings ){
  m_impl = new SelectObjectIdleImpl(settings);
}

SelectObjectIdle::~SelectObjectIdle(){
  delete m_impl;
}

void SelectObjectIdle::Activate(){
  m_impl->Reset(true);
}

void SelectObjectIdle::Draw( FaintDC&, Overlays& overlays, const Point& ){
  const objects_t& objects = GetAppContext().GetActiveCanvas().GetObjectSelection();
  for ( const Object* obj : objects ){
    if ( point_edit_disabled(obj) ){
      // Skip showing point-editing overlays for this object
      continue;
    }
    for ( const Point& pt : obj->GetMovablePoints() ){
      overlays.MovablePoint( pt );
    }
    for ( const Point& pt : obj->GetExtensionPoints() ){
      overlays.ExtensionPoint( pt );
    }
  }
}

bool SelectObjectIdle::DrawBeforeZoom(Layer) const{
  return false;
}

Command* SelectObjectIdle::GetCommand(){
  return m_impl->command.Retrieve();
}

Cursor SelectObjectIdle::GetCursor( const CursorPositionInfo& info ) const{
  if ( info.objSelected ){
    if ( info.hitStatus == Hit::RESIZE_POINT ){
      if ( mod_rotate(info) && corner_handle(info.handleIndex) ){
        return Cursor::ROTATE;
      }
      if ( info.handleIndex == 0 || info.handleIndex == 3 ){
        return Cursor::RESIZE_NW;
      }
      else if ( info.handleIndex == 1 || info.handleIndex == 2 ) {
        return Cursor::RESIZE_NE;
      }
      else if ( info.handleIndex == 4 || info.handleIndex == 5 ){
        return Cursor::RESIZE_WE;
      }
      else if ( info.handleIndex == 6 || info.handleIndex == 7 ){
        return Cursor::RESIZE_NS;
      }
    }
    else if ( info.hitStatus == Hit::INSIDE || info.hitStatus == Hit::NEARBY || info.hitStatus == Hit::BOUNDARY ){
      return mod_clone(info) ? Cursor::CLONE : Cursor::MOVE;
    }
    else if ( point_edit_enabled(info.object) ) {
      if ( info.hitStatus == Hit::MOVABLE_POINT ) {
        return Cursor::MOVE_POINT;
      }
      else if ( info.hitStatus == Hit::EXTENSION_POINT ){
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
  if ( then_false(m_impl->fullRefresh) ){
    // Refresh the entire visible area (e.g. after selection changes)
    return visible;
  }
  // The idle selection for the most time requires no refresh.
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
  if ( objText != nullptr ){
    // Start editing the double-clicked text object
    m_impl->newTask.Set(new EditText(objText, m_impl->settings));
    return TASK_CHANGE;
  }
  return TASK_NONE;
}

TaskResult SelectObjectIdle::LeftDown( const CursorPositionInfo& info ){
  info.status->SetMainText("");
  if ( info.object == nullptr ){
    return clicked_nothing(*m_impl, info);
  }
  else if ( info.objSelected ){
    return clicked_selected(*m_impl, info );
  }
  return clicked_unselected(*m_impl, info);
}

TaskResult SelectObjectIdle::LeftUp( const CursorPositionInfo& info ){
  if ( m_impl->mouseDown && mod_toggle_select(info) && not_drag(m_impl->clickPos, info.pos) ){
    if ( selectable(m_impl->clicked) ){
      info.canvas->SelectObject( m_impl->clicked.object, deselect_old(false));
    }
    else if ( deselectable(m_impl->clicked) ){
      info.canvas->DeselectObject( m_impl->clicked.object );
    }
    m_impl->Reset(true);
    return TASK_DRAW;
  }
  m_impl->Reset();
  return TASK_NONE;
}

TaskResult SelectObjectIdle::Motion( const CursorPositionInfo& info ){
  info.status->SetText(str(info.pos));

  if ( !m_impl->mouseDown || !is_drag(m_impl->clickPos, info.pos) ){
    // There's no action for motion when not dragging with mouse held.
    info.status->SetMainText(status_for_object(info));
    return TASK_NONE;
  }

  info.status->SetMainText( "" );
  const bool cloneBtn = fl( TOOLMODIFIER1, info.modifiers );
  if ( deselectable(m_impl->clicked) && cloneBtn ){
    return clone_selected_objects( *m_impl, *info.canvas, info.pos );
  }
  else if ( quick_draggable(m_impl->clicked) && !cloneBtn ){
    return move_selected_objects(*m_impl, info.canvas->GetObjectSelection(), info.pos);
  }
  else if ( no_object(m_impl->clicked) ){
    return select_object_rectangle(*m_impl, info.pos);
  }
  return TASK_NONE;
}

TaskResult SelectObjectIdle::Preempt( const CursorPositionInfo& ){
  return TASK_NONE;
}

void SelectObjectIdle::SelectionChange(){
  AppContext& app(GetAppContext());
  m_impl->settings = get_object_settings(app.GetActiveCanvas().GetObjectSelection());
  app.UpdateShownSettings();
}
