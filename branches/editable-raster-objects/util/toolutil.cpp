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
#include "util/drawsource.hh"
#include "util/formatting.hh"
#include "util/settingutil.hh"

ColorSetting fg_or_bg( const CursorPositionInfo& info ){
  return mbtn( info.modifiers ) == LEFT_MOUSE ?
    ts_FgCol : ts_BgCol;
}

static faint::DrawSource get_hit_object_draw_source( Hit hit, Object* obj ){
  const Settings& s( obj->GetSettings() );
  if ( hit == Hit::INSIDE && s.Has(ts_FillStyle)){
    return s.Get(setting_used_for_fill(get_fillstyle(s)));
  }
  return s.Get(ts_FgCol);
}

static bool transparent_inside_hit( const CursorPositionInfo& info ){
  return info.hitStatus == Hit::INSIDE && !filled(info.object->GetSettings());
}

faint::DrawSource get_hovered_draw_source( const CursorPositionInfo& info, const hidden_fill& hiddenFill ){
  if ( is_object(info.layerType) && object_color_region_hit(info) ){
    if ( hiddenFill.Get() || !transparent_inside_hit(info) ){
      return get_hit_object_draw_source( info.hitStatus, info.object );
    }
  }

  const faint::Bitmap& bmp( info.canvas->GetBitmap() );
  return faint::DrawSource(get_color( bmp, floored(info.pos) ));
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
  return nullptr;
}

ObjText* hovered_selected_text( const CursorPositionInfo& info, SearchMode mode ){
  if ( !info.objSelected || info.object == nullptr ){
    return nullptr;
  }

  ObjText* objText = dynamic_cast<ObjText*>( info.object );
  if ( objText != nullptr || mode == SearchMode::exact_object ){
    return objText;
  }
  ObjComposite* objComposite = dynamic_cast<ObjComposite*>(info.object);
  return objComposite == nullptr ? nullptr :
    get_text_from(objComposite);
}

ObjComposite* hovered_selected_composite( const CursorPositionInfo& info ){
  if ( !info.objSelected || info.object == nullptr ){
    return nullptr;
  }
  return dynamic_cast<ObjComposite*>( info.object );
}

bool has_color_setting(Object* object){
  const Settings& s(object->GetSettings());
  return s.Has(ts_FgCol) || s.Has(ts_BgCol);
}

bool object_color_region_hit( const CursorPositionInfo& info ){
  return info.object != 0 && (info.hitStatus == Hit::BOUNDARY || info.hitStatus == Hit::INSIDE ) && has_color_setting(info.object);
}

bool outside_canvas( const CursorPositionInfo& info ){
  return outside_canvas_by(info, 0);
}

bool outside_canvas_by( const CursorPositionInfo& info, int pixels ){
  if ( info.pos.x + pixels < 0 || info.pos.y + pixels < 0 ){
    return true;
  }
  IntSize sz = info.canvas->GetSize();
  if ( info.pos.x - pixels >= sz.w || info.pos.y - pixels >= sz.h ){
    return true;
  }
  return false;
}

Tool* new_tool(ToolId id){
  switch (id) {
  case ToolId::RECTANGLE_SELECTION:
    return new RectangleSelectTool();
    break;
  case ToolId::OBJECT_SELECTION:
    return new ObjSelectTool();
    break;
  case ToolId::PEN:
    return new PenTool();
    break;
  case ToolId::BRUSH:
    return new BrushTool();
    break;
  case ToolId::PICKER:
    return new PickerTool();
    break;
  case ToolId::LINE:
    return new LineTool();
    break;
  case ToolId::SPLINE:
    return new SplineTool();
    break;
  case ToolId::RECTANGLE:
    return new RectangleTool();
    break;
  case ToolId::ELLIPSE:
    return new EllipseTool();
    break;
  case ToolId::POLYGON:
    return new PolygonTool();
    break;
  case ToolId::TEXT:
    return new TextTool();
    break;
  case ToolId::FLOOD_FILL:
    return new FillTool();
    break;
  case ToolId::OTHER:
    assert(false); // Can not instantiate unspecific tool
    return nullptr;
  default:
    assert( false );
    return nullptr;
  };
}
