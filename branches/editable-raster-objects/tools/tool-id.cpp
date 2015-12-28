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
#include "tool-id.hh"

bool doesnt_handle_selection(ToolId id){
  return id != ToolId::RECTANGLE_SELECTION &&
    id != ToolId::OBJECT_SELECTION;
}

bool object_layer_only(ToolId id){
  return id == ToolId::OBJECT_SELECTION ||
    id == ToolId::OTHER; // Fixme: What is other?
}

bool raster_layer_only(ToolId id){
  return id == ToolId::RECTANGLE_SELECTION ||
    id == ToolId::PEN ||
    id == ToolId::BRUSH;
}

int to_int(ToolId id){
  return static_cast<int>(id);
}

ToolId to_tool_id( int id ){
  assert(valid_tool_id(id));
  return static_cast<ToolId>(id);
}

bool valid_tool_id( int id ){
  return (0 <= to_int(ToolId::MIN_ID) && id <= to_int(ToolId::MAX_ID));
}
