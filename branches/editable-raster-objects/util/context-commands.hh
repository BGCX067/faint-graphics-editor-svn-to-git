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

#ifndef FAINT_CONTEXT_COMMANDS_HH
#define FAINT_CONTEXT_COMMANDS_HH
#include "util/commonfwd.hh"

enum ApplyTarget{
  APPLY_OBJECT_SELECTION,
  APPLY_RASTER_SELECTION,
  APPLY_IMAGE
};

// If the layer type is the raster layer and there's a raster
// selection the image will be cropped to the selection.
//
// If the layer type is the object layer, any selected
// croppable objects will be cropped.
//
// If neither objects or raster cropping happened, auto-crop is
// attempted.
Command* context_crop( CanvasInterface& );

// Tool, object or raster deselect
Command* context_deselect( CanvasInterface& );

// Flattens the selected objects, or all objects if none are selected.
// Does nothing if there are no objects.
Command* context_flatten( CanvasInterface& );

Command* context_flip_horizontal( CanvasInterface& );

Command* context_flip_vertical( CanvasInterface& );

Command* context_objects_backward( CanvasInterface& );

Command* context_objects_forward( CanvasInterface& );

Command* context_objects_to_back( CanvasInterface& );

Command* context_objects_to_front( CanvasInterface& );

Command* context_offset( CanvasInterface&, const IntPoint& delta );

Command* context_rotate90cw( CanvasInterface& );

Command* context_scale_objects( CanvasInterface&, const Size& );

Command* context_select_all( CanvasInterface& );

ApplyTarget get_apply_target( CanvasInterface& );
Size get_apply_target_size( CanvasInterface&, ApplyTarget );
Command* get_apply_command( CanvasInterface&, const Operation& );

// Groups the selected objects. Does nothing if no objects are
// selected.
Command* group_selected_objects( CanvasInterface& );

// Ungroups the selected groups. Does nothing if no groups are
// selected.
Command* ungroup_selected_objects( CanvasInterface& );

#endif
