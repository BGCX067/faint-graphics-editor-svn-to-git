// -*- coding: us-ascii-unix -*-
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
#include "tools/tool-id.hh"

namespace faint{

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

ToolId to_tool_id(int id){
  assert(valid_tool_id(id));
  return static_cast<ToolId>(id);
}

bool valid_tool_id(int id){
  return (0 <= to_int(ToolId::MIN_ID) && id <= to_int(ToolId::MAX_ID));
}

Tool* brush_tool(const Settings&);
Tool* ellipse_tool(const Settings&);
Tool* fill_tool(const Settings&);
Tool* line_tool(const Settings&);
Tool* object_selection_tool(const ActiveCanvas&);
Tool* path_tool(const Settings&);
Tool* pen_tool(const Settings&);
Tool* picker_tool();
Tool* polygon_tool(const Settings&);
Tool* raster_selection_tool(const Settings&, const ActiveCanvas&);
Tool* rectangle_tool(const Settings&);
Tool* spline_tool(const Settings&);
Tool* text_tool(const Settings&);
Tool* level_tool(const Settings&);

Tool* new_tool(ToolId id, const Settings& settings, const ActiveCanvas& canvas){
  switch (id) {
  case ToolId::BRUSH:
    return brush_tool(settings);
    break;

  case ToolId::ELLIPSE:
    return ellipse_tool(settings);
    break;

  case ToolId::FLOOD_FILL:
    return fill_tool(settings);
    break;

  case ToolId::LEVEL:
    return level_tool(settings);
    break;

  case ToolId::LINE:
    return line_tool(settings);
    break;

  case ToolId::OBJECT_SELECTION:
    return object_selection_tool(canvas);
    break;

  case ToolId::OTHER:
    assert(false); // Can not instantiate unspecific tool
    return nullptr;

  case ToolId::PEN:
    return pen_tool(settings);
    break;

  case ToolId::PATH:
    return path_tool(settings);
    break;
  case ToolId::PICKER:
    return picker_tool();
    break;

  case ToolId::POLYGON:
    return polygon_tool(settings);
    break;

  case ToolId::RECTANGLE:
    return rectangle_tool(settings);
    break;

  case ToolId::RECTANGLE_SELECTION:
    return raster_selection_tool(settings, canvas);
    break;

  case ToolId::SPLINE:
    return spline_tool(settings);
    break;

  case ToolId::TEXT:
    return text_tool(settings);
    break;

  default:
    assert(false);
    return nullptr;
  };
}

} // namespace
