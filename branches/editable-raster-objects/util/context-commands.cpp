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

#include "app/getappcontext.hh" // Fixme: Remove
#include "commands/flip-rotate-cmd.hh"
#include "commands/group-objects-cmd.hh"
#include "util/angle.hh"
#include "util/canvasinterface.hh"
#include "util/commandutil.hh"
#include "util/context-commands.hh"
#include "util/objutil.hh"

ApplyTarget get_apply_target( CanvasInterface& canvas ){
  ToolInterface& tool(canvas.GetTool());
  Layer layer = tool.GetLayerType();
  if ( layer == Layer::RASTER && canvas.GetRasterSelection().Exists() ){
    return APPLY_RASTER_SELECTION;
  }
  else if ( layer == Layer::OBJECT && !canvas.GetObjectSelection().empty() ){
    return APPLY_OBJECT_SELECTION;
  }
  return APPLY_IMAGE;
}

Size get_apply_target_size( CanvasInterface& canvas, ApplyTarget target ){
  const faint::Image& image(canvas.GetImage());
  if ( target == APPLY_OBJECT_SELECTION ){
    return bounding_rect(image.GetObjectSelection()).GetSize();
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    return floated(image.GetRasterSelection().GetSize());
  }
  else{
    assert(target == APPLY_IMAGE);
    return floated(image.GetSize());
  }
}

Command* get_apply_command( CanvasInterface& canvas, const Operation& op ){
  ApplyTarget target = get_apply_target(canvas);
  if ( target == APPLY_OBJECT_SELECTION ){
    return op.DoObjects(canvas.GetImage().GetObjectSelection());
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    return op.DoRasterSelection( canvas.GetImage() );
  }
  else if ( target == APPLY_IMAGE ){
    return op.DoImage();
  }

  assert( false );
  return nullptr;
}

static Command* crop_to_raster_selection_command( const faint::Image& image ){
  const RasterSelection& selection(image.GetRasterSelection());
  if ( !selection.Exists() ){
    return nullptr;
  }
  return get_crop_to_selection_command(selection);
}

Command* context_crop( CanvasInterface& canvas ){
  const faint::Image& active(canvas.GetImage());
  Layer layer = canvas.GetTool().GetLayerType();
  if ( layer == Layer::RASTER ){
    Command* cmd = crop_to_raster_selection_command(active);
    if ( cmd != nullptr ){
      return cmd;
    }
  }
  else {
    // Object layer
    Command* cmd = get_crop_command( active.GetObjectSelection() );
    if ( cmd != nullptr ){
      return cmd;
    }
  }

  // No active raster selections and no croppable objects selected, do
  // an auto-crop
  return get_auto_crop_command(active);
}

Command* context_deselect( CanvasInterface& canvas ){
  ToolInterface& tool(canvas.GetTool());
  if ( tool.Deselect() ){
    return nullptr;
  }

  Layer layer = tool.GetLayerType();
  if ( layer == Layer::RASTER ){
    const RasterSelection& selection(canvas.GetImage().GetRasterSelection());
    return selection.Exists() ?
      get_deselect_raster_command(selection) :
      nullptr;
  }
  else {
    canvas.DeselectObjects();
    canvas.Refresh(); // Fixme: Tricky, required, because no command is used
    return nullptr;
  }
}

Command* context_flatten( CanvasInterface& canvas ){
  const faint::Image& active = canvas.GetImage();
  const objects_t& allObjects = active.GetObjects();
  if ( allObjects.empty() ){
    // No objects to flatten.
    return nullptr;
  }
  const objects_t& selected = active.GetObjectSelection();
  return get_flatten_command( selected.empty() ?
    allObjects : selected,
    active );
}

Command* context_flip_horizontal( CanvasInterface& canvas ){
  return get_apply_command( canvas, OperationFlip(Axis::HORIZONTAL) );
}

Command* context_flip_vertical( CanvasInterface& canvas ){
  return get_apply_command( canvas, OperationFlip(Axis::VERTICAL) );
}

Command* context_offset(CanvasInterface& canvas, const IntPoint& delta){
  const ApplyTarget target = get_apply_target(canvas);
  if ( target == APPLY_IMAGE ){
    canvas.SetScrollPos(canvas.GetScrollPos() + delta);
    return nullptr;
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    const objects_t& objectSelection(canvas.GetImage().GetObjectSelection());
    return get_offset_objects_command(objectSelection,
      floated(delta));
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    return get_offset_raster_selection_command(canvas.GetImage(), delta);
  }
  else{
    assert(false);
    return nullptr;
  }
}

Command* context_rotate90cw( CanvasInterface& canvas ){
  const ApplyTarget target = get_apply_target(canvas);
  if ( target == APPLY_IMAGE ){
    return rotate_image_90cw_command();
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    const objects_t& objectSelection(canvas.GetImage().GetObjectSelection());
    Point origin = bounding_rect(objectSelection).Center();
    return get_rotate_command( objectSelection,
      faint::pi / 2,
      origin );
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    return get_rotate_selection_command(canvas.GetImage());
  }
  assert(false);
  return nullptr;
}

Command* context_select_all( CanvasInterface& canvas ){
  ToolInterface& tool( canvas.GetTool() );
  if ( tool.SelectAll() ){
    // Selection within a tool is not handled by a command
    return nullptr;
  }

  Layer layer = tool.GetLayerType();
  if ( layer == Layer::RASTER ){
    const faint::Image& active(canvas.GetImage());
    GetAppContext().SelectTool(ToolId::RECTANGLE_SELECTION); // Fixme
    return get_select_all_command( active,
      active.GetRasterSelection() );
  }
  else {
    canvas.SelectObjects( canvas.GetObjects(), deselect_old(true) );
    GetAppContext().SelectTool( ToolId::OBJECT_SELECTION );
    // Object selection is not handled with commands
    return nullptr;
  }
}

Command* context_scale_objects( CanvasInterface& canvas, const Size& size ){
  const faint::Image& active = canvas.GetImage();
  const objects_t& objectSelection(active.GetObjectSelection());
  Rect oldRect = bounding_rect(objectSelection);
  Point origin = oldRect.TopLeft();
  Scale scale(New(size), oldRect.GetSize());
  return get_scale_command(objectSelection, scale, origin);
}

Command* group_selected_objects( CanvasInterface& canvas ){
  const faint::Image& active(canvas.GetImage());
  const objects_t& objectSelection(active.GetObjectSelection());

  if ( objectSelection.size() <= 1 ){
    // Need at least two objects to group
    return nullptr;
  }
  cmd_and_group_t cmd =
    group_objects_command(objectSelection, select_added(true));
  return cmd.first;
}

Command* context_objects_backward( CanvasInterface& canvas ){
  const faint::Image& active(canvas.GetImage());
  const objects_t& objectSelection(active.GetObjectSelection());
  if ( objectSelection.empty() ){
    return nullptr;
  }
  return get_objects_backward_command(objectSelection, active);
}

Command* context_objects_forward( CanvasInterface& canvas ){
  const faint::Image& active(canvas.GetImage());
  const objects_t& objectSelection(active.GetObjectSelection());
  if ( objectSelection.empty() ){
    return nullptr;
  }
  return get_objects_forward_command(objectSelection, active);
}

Command* context_objects_to_front( CanvasInterface& canvas ){
  const faint::Image& active(canvas.GetImage());
  const objects_t& objectSelection(active.GetObjectSelection());
  if ( objectSelection.empty() ){
    return nullptr;
  }
  return get_objects_to_front_command(objectSelection, active);
}

Command* context_objects_to_back( CanvasInterface& canvas ){
  const faint::Image& active(canvas.GetImage());
  const objects_t& objectSelection(active.GetObjectSelection());
  if ( objectSelection.empty() ){
    return nullptr;
  }
  return get_objects_to_back_command(objectSelection, active);
}

Command* ungroup_selected_objects( CanvasInterface& canvas ){
  const faint::Image& active(canvas.GetImage());
  objects_t groups = get_groups( active.GetObjectSelection() );
  if ( groups.empty() ){
    return nullptr;
  }
  return ungroup_objects_command(groups, select_added(true));
}
