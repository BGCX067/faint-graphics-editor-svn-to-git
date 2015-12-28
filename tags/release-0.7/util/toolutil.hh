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

#ifndef FAINT_TOOLUTIL_HH
#define FAINT_TOOLUTIL_HH
#include "tools/toolid.hh"
#include "util/commonfwd.hh"
struct CursorPositionInfo;
class ObjComposite;
class ObjText;

struct SearchMode{
  enum Type{exact_object, include_grouped};
};

// Returns the color under the given position, including hit object if
// in the object layer.
// Must not be called with positions outside the Canvas!
faint::Color get_hovered_color( const CursorPositionInfo& );

// Retrieves the hovered selected object as a text object, or 0 if no
// object is hovered, the object isn't selected or is not a text
// object.
ObjText* hovered_selected_text(const CursorPositionInfo&, SearchMode::Type searchMode=SearchMode::exact_object );
ObjComposite* hovered_selected_composite(const CursorPositionInfo&);

bool outside_canvas( const CursorPositionInfo& );

Tool* new_tool(ToolId);

// True if the edge or inside of an object was hit, and the object
// supports ts_FgCol or ts_BgCol
bool object_color_region_hit( const CursorPositionInfo& );

#endif
