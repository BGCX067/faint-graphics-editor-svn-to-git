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

#include "toolutil.hh"
#include "bitmap/bitmap.hh"
#include "objects/objcomposite.hh"
#include "objects/objtext.hh"
#include "tools/brush-tool.hh"
#include "tools/ellipse-tool.hh"
#include "tools/fill-tool.hh"
#include "tools/line-tool.hh"
#include "tools/object-selection-tool.hh"
#include "tools/pen-tool.hh"
#include "tools/picker-tool.hh"
#include "tools/polygon-tool.hh"
#include "tools/raster-selection-tool.hh"
#include "tools/rectangle-tool.hh"
#include "tools/spline-tool.hh"
#include "tools/text-tool.hh"
#include "util/settingutil.hh"

faint::Color get_hovered_color( const CursorPositionInfo& info ){
  if ( is_object(info.layerType) && object_color_region_hit(info) ){
    return info.object->GetSettings().Get( info.hitStatus == HIT_BOUNDARY ? ts_FgCol : ts_BgCol );
  }
  else {
    const faint::Bitmap& bmp( info.canvas->GetBitmap() );
    return get_color( bmp, truncated(info.pos) );
  }
}

// Returns the topmost text object in the ObjComposite or 0 if no text
// object is contained by the composite.
ObjText* get_text_from(ObjComposite* group){
  size_t numObjects = group->GetObjectCount();
  for ( size_t i = 0; i != numObjects; i++ ){
    ObjText* objText = dynamic_cast<ObjText*>(group->GetObject(i));
    if ( objText != 0 ){
      return objText;
    }
  }
  return 0;
}
ObjText* hovered_selected_text( const CursorPositionInfo& info, SearchMode::Type mode ){
  if ( !info.objSelected || info.object == 0 ){
    return 0;
  }

  ObjText* objText = dynamic_cast<ObjText*>( info.object );
  if ( objText != 0 || mode == SearchMode::exact_object ){
    return objText;
  }
  ObjComposite* objComposite = dynamic_cast<ObjComposite*>(info.object);
  return objComposite == 0 ? 0 :
    get_text_from(objComposite);
}

ObjComposite* hovered_selected_composite( const CursorPositionInfo& info ){
  if ( !info.objSelected || info.object == 0 ){
    return 0;
  }
  return dynamic_cast<ObjComposite*>( info.object );
}

bool has_color_setting(Object* object){
  const Settings& s(object->GetSettings());
  return s.Has(ts_FgCol) || s.Has(ts_BgCol);
}

bool object_color_region_hit( const CursorPositionInfo& info ){
  return info.object != 0 && (info.hitStatus == HIT_BOUNDARY || info.hitStatus == HIT_INSIDE ) && has_color_setting(info.object);
}

bool outside_canvas( const CursorPositionInfo& info ){
  if ( info.pos.x < 0 || info.pos.y < 0 ){
    return true;
  }
  IntSize sz = info.canvas->GetSize();
  if ( info.pos.x >= sz.w || info.pos.y >= sz.h ){
    return true;
  }
  return false;
}

Tool* new_tool(ToolId id){
  switch (id) {
  case T_RECT_SEL:
    return new RectangleSelectTool();
    break;
  case T_OBJ_SEL:
    return new ObjSelectTool();
    break;
  case T_PEN:
    return new PenTool();
    break;
  case T_BRUSH:
    return new BrushTool();
    break;
  case T_PICKER:
    return new PickerTool();
    break;
  case T_LINE:
    return new LineTool();
    break;
  case T_SPLINE:
    return new SplineTool();
    break;
  case T_RECTANGLE:
    return new RectangleTool();
    break;
  case T_ELLIPSE:
    return new EllipseTool();
    break;
  case T_POLYGON:
    return new PolygonTool();
    break;
  case T_TEXT:
    return new TextTool();
    break;
  case T_FLOODFILL:
    return new FillTool();
    break;
  case T_OTHER:
    assert(false);
    return 0;
  default:
    assert( false );
    return 0;
  };
}
