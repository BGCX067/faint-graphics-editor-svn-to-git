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

#ifndef FAINT_TOOL_ID_HH
#define FAINT_TOOL_ID_HH

namespace faint{
class ActiveCanvas;
class Settings;

enum class ToolId{
  MIN_ID = 0,
  RECTANGLE_SELECTION = MIN_ID,
  OBJECT_SELECTION,
  PEN,
  BRUSH,
  PICKER,
  LINE,
  SPLINE,
  RECTANGLE,
  ELLIPSE,
  PATH,
  POLYGON,
  TEXT,
  FLOOD_FILL,
  LEVEL,
  MAX_VALID_ID = LEVEL,
  OTHER,
  MAX_ID = OTHER
};

bool object_layer_only(ToolId);
bool raster_layer_only(ToolId);
int to_int(ToolId);
ToolId to_tool_id(int);
bool valid_tool_id(int);

class Tool;
Tool* new_tool(ToolId, const Settings&, const ActiveCanvas&);

} // namespace

#endif
