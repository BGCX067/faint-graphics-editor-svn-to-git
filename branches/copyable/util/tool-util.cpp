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

#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "geo/adjust.hh"
#include "geo/geo-func.hh"
#include "geo/rotation.hh"
#include "objects/objcomposite.hh"
#include "objects/objtext.hh"
#include "text/formatting.hh"
#include "util/canvas.hh"
#include "util/color.hh"
#include "util/image.hh"
#include "util/paint.hh"
#include "util/pos-info.hh"
#include "util/setting-util.hh"
#include "util/tool-util.hh"

namespace faint{

ColorSetting fg_or_bg(const PosInfo& info){
  return info.modifiers.LeftMouse() ?
    ts_Fg : ts_Bg;
}

static Paint get_hit_object_paint(Hit hit, Object* obj){
  const Settings& s(obj->GetSettings());
  if (hit == Hit::INSIDE && s.Has(ts_FillStyle)){
    return s.Get(setting_used_for_fill(get_fillstyle(s)));
  }
  return s.Get(ts_Fg);
}

static bool transparent_inside_hit(const PosInfo& info){
  return info.hitStatus == Hit::INSIDE && !filled(info.object->GetSettings());
}

Paint get_hovered_paint(const PosInfo& info,
  const include_hidden_fill& includeHiddenFill,
  const include_floating_selection& includeFloatingSelection)
{
  if (includeFloatingSelection.Get() && info.inSelection){
    const RasterSelection& selection = info.canvas.GetRasterSelection();
    if (selection.Floating()){
      const Bitmap& selBmp = selection.GetBitmap();
      return Paint(get_color(selBmp, floored(info.pos) - selection.TopLeft()));
    }
  }
  if (is_object(info.layerType) && object_color_region_hit(info)){
    if (includeHiddenFill.Get() || !transparent_inside_hit(info)){
      return get_hit_object_paint(info.hitStatus, info.object);
    }
  }

  return info.canvas.GetImage().GetBg().Visit(
    [&](const Bitmap& bmp){
      // Fixme: get_color safe?
      return Paint(get_color(bmp, floored(info.pos)));
    },
    [](const ColorSpan& span){
      return Paint(span.color);
    });
}

// Returns the topmost text object in the ObjComposite or 0 if no text
// object is contained by the composite.
static ObjText* get_text_from(Object* group){
  const int numObjects = group->GetObjectCount();
  for (int i = 0; i != numObjects; i++){
    ObjText* objText = dynamic_cast<ObjText*>(group->GetObject(i));
    if (objText != nullptr){
      return objText;
    }
  }
  return nullptr;
}

ObjText* hovered_selected_text(const PosInfo& info, SearchMode mode){
  if (!info.objSelected || info.object == nullptr){
    return nullptr;
  }

  ObjText* objText = dynamic_cast<ObjText*>(info.object);
  if (objText != nullptr || mode == SearchMode::exact_object){
    return objText;
  }
  return get_text_from(info.object);
}

bool has_color_setting(Object* object){
  const Settings& s(object->GetSettings());
  return s.Has(ts_Fg) || s.Has(ts_Bg);
}

bool object_color_region_hit(const PosInfo& info){
  return info.object != 0 && (info.hitStatus == Hit::BOUNDARY || info.hitStatus == Hit::INSIDE) && has_color_setting(info.object);
}

bool outside_canvas(const PosInfo& info){
  return outside_canvas_by(info, 0);
}

bool outside_canvas_by(const PosInfo& info, int pixels){
  if (info.pos.x + pixels < 0 || info.pos.y + pixels < 0){
    return true;
  }
  IntSize sz = info.canvas.GetSize();
  if (info.pos.x - pixels >= sz.w || info.pos.y - pixels >= sz.h){
    return true;
  }
  return false;
}

Point constrain_to_square(const Point& p0, const Point& p1, bool subPixel){
  return subPixel?
    adjust_to(p0, p1, Rotation::Deg(90), Rotation::Deg(45)) :
    adjust_to(floated(floored(p0)), floated(floored(p1)), Rotation::Deg(90), Rotation::Deg(45));
}

}
