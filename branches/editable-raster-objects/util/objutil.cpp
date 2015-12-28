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

#include <algorithm>
#include <cassert>
#include "geo/grid.hh"
#include "objects/objcomposite.hh"
#include "objects/objrectangle.hh"
#include "objects/objraster.hh"
#include "objects/objtext.hh"
#include "util/objutil.hh"
#include "rendering/faintdc.hh"
#include "util/iter.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

objects_t as_list( Object* obj ){
  objects_t objects;
  objects.push_back(obj);
  return objects;
}

ObjRaster* as_ObjRaster( Object* obj ){
  ObjRaster* raster = dynamic_cast<ObjRaster*>(obj);
  assert( raster != nullptr );
  return raster;
}

Rect bounding_rect( const objects_t& objects ){
  assert( !objects.empty() );
  Tri t(objects[0]->GetTri());
  Point minPt = min_coords(t.P0(), t.P1(), t.P2(), t.P3());
  Point maxPt = max_coords(t.P0(), t.P1(), t.P2(), t.P3());
  for ( const Object* obj : but_first(objects) ){
    t = obj->GetTri();
    minPt = min_coords(minPt, min_coords(t.P0(), t.P1(), t.P2(), t.P3()));
    maxPt = max_coords(maxPt, max_coords(t.P0(), t.P1(), t.P2(), t.P3()));
  }
  return Rect(minPt, maxPt);
}

objects_t clone(const objects_t& oldObjects ){
  objects_t objects;
  for ( const Object* obj : oldObjects ){
    objects.push_back( obj->Clone() );
  }
  return objects;
}

faint::Color color_at(ObjRaster* obj, const Point& imagePos){
  faint::Bitmap bmp(IntSize(1,1), get_color_default(obj->GetSettings().Get(ts_BgCol), faint::color_white()));
  FaintDC dc(bmp);
  dc.SetOrigin(-imagePos);
  obj->Draw(dc);
  return get_color(bmp, IntPoint(0,0));
}

bool contains( const objects_t& objects, const Object* obj ){
  return std::find(objects.begin(), objects.end(), obj ) != objects.end();
}

bool contains( const objects_t& objects, const these_t<objects_t>& in_contained ){
  const objects_t& contained(in_contained.Get());
  for ( const Object* obj : contained ){
    if ( !contains(objects, obj) ){
      return false;
    }
  }
  return true;
}

bool contains_group( const objects_t& objects ){
  for ( Object* obj : objects ){
    if ( obj->GetObjectCount() > 0 ){
      return true;
    }
  }
  return false;
}

size_t find_object_index( Object* obj, const objects_t& objects ){
  // Fixme: Review, looks fishy
  assert( obj != nullptr );
  size_t i = 0;
  for ( ; i != objects.size(); i++ ){
    if ( objects[i] == obj ){
      break;
    }
  }
  return i;
}

void get_attach_points( const Tri& tri, std::vector<Point>& v ){
  v.push_back( tri.P0() );
  v.push_back( tri.P1() );
  v.push_back( tri.P2() );
  v.push_back( tri.P3() );
  v.push_back( mid_P0_P1( tri ) );
  v.push_back( mid_P0_P2( tri ) );
  v.push_back( mid_P1_P3( tri ) );
  v.push_back( mid_P2_P3( tri ) );
  v.push_back( center_point( tri ) );
}

std::vector<Point> get_attach_points( const Tri& tri ){
  std::vector<Point> v;
  get_attach_points( tri, v );
  return v;
}

std::string get_collective_name( const objects_t& objects ){
  assert(!objects.empty());
  std::string first = objects.front()->GetType();
  if ( objects.size() == 1 ){
    return first;
  }

  for ( const Object* obj : but_first(objects) ){
    if ( obj->GetType() != first ){
      return "Objects";
    }
  }
  return first + "s"; // Naive plural
}

Point get_delta( Object* obj, const Point& p ){
  return p - obj->GetTri().P0();
}

objects_t get_groups( const objects_t& objects ){
  objects_t groups;
  for ( Object* obj : objects ){
    if ( obj->GetObjectCount() != 0 ){
      groups.push_back(obj);
    }
  }
  return groups;
}

objects_t get_intersected(const objects_t& objects, const Rect& r){
  objects_t intersected;
  for ( Object* obj : objects ){
    if ( intersects( obj->GetRect(), r) ){
      intersected.push_back(obj);
    }
  }
  return intersected;
}

Settings get_object_settings( const objects_t& objects ){
  Settings s;
  for ( Object* obj : objects ){
    s.UpdateAll(obj->GetSettings());
  }
  return s;
}

size_t get_sorted_insertion_pos( Object* obj, const objects_t& v_trg, const objects_t& v_src ){
  size_t i_src = 0;
  size_t i_trg = 0;
  for ( ; i_trg != v_trg.size(); i_trg++ ){
    const Object* o_trg = v_trg[i_trg];

    for ( ; i_src!= v_src.size(); i_src++ ){
      const Object* o_src = v_src[i_src];
      if ( o_src == obj ){
        // Object to be inserted found before reference object ->
        // this is the insertion point.
        return i_trg;
      }
      if ( o_src == o_trg ){
        // Reference object found in destination vector -
        // inserted object must be later in the source vector.
        break;
      }
    }
  }
  return i_trg;
}

tris_t get_tris( const objects_t& objects ){
  tris_t tris;
  for ( Object* obj : objects ){
    tris.push_back(obj->GetTri());
  }
  return tris;
}

its_yours_t<Object> its_yours( Object* obj ){
  return its_yours_t<Object>(obj);
}

just_a_loan_t<Object> just_a_loan( Object* obj ){
  return just_a_loan_t<Object>(obj);
}

bool is_or_has( const Object* object, const ObjectId& id ){
  if ( object->GetId() == id ){
    return true;
  }
  for ( size_t i = 0; i != object->GetObjectCount(); i++ ){
    if ( is_or_has( object->GetObject( i ), id ) ){
      return true;
    }
  }
  return false;
}

bool is_raster_object( Object* obj ){
  return obj != nullptr && dynamic_cast<ObjRaster*>(obj) != nullptr;
}

bool is_text_object( Object* obj ){
  return obj != nullptr && dynamic_cast<ObjText*>(obj) != nullptr;
}

bool lacks( const objects_t& objects, const Object* obj ){
  return std::find(objects.begin(), objects.end(), obj ) == objects.end();
}

void move_to( Object* obj, const Point& p ){
  offset_by( obj, get_delta( obj, p ) );
}

Point next_point( Object* obj, size_t index ){
  const size_t end = obj->NumPoints();
  assert(index < end);
  if ( index != end - 1 ){
    return obj->GetPoint(index + 1);
  }
  return obj->CyclicPoints() ?
    obj->GetPoint(0) :
    obj->GetPoint(end - 2);
}

bool object_aligned_resize( const Object* obj ){
  return obj->GetSettings().GetDefault(ts_AlignedResize, false);
}

void offset_by( Object* obj, const Point& d ){
  obj->SetTri( translated( obj->GetTri(), d.x, d.y ) );
}

void offset_by( Object* obj, const IntPoint& d ){
  offset_by( obj, floated(d) );
}

void offset_by( const objects_t& objects, const Point& d ){
  for ( Object* obj : objects ){
    offset_by(obj, d);
  }
}

void offset_by( const objects_t& objects, const IntPoint& d ){
  offset_by(objects, floated(d));
}

bool point_edit_disabled( const Object* obj ){
  return !point_edit_enabled(obj);
}

bool point_edit_enabled( const Object* obj ){
  return obj->GetSettings().GetDefault(ts_EditPoints, false);
}

Point prev_point( Object* obj, size_t index ){
  const size_t end = obj->NumPoints();
  assert(index < end);
  if ( index != 0 ){
    return obj->GetPoint(index - 1);
  }
  return obj->CyclicPoints() ?
    obj->GetPoint(end - 1) :
    obj->GetPoint(index + 1);
}

bool remove( const Object* obj, const from_t<objects_t>& from ){
  objects_t& objects(from.Get());
  auto obj_eq = [&obj](Object* x){return x == obj;};
  objects_t::iterator newEnd = std::remove_if( objects.begin(), objects.end(), obj_eq);
  if ( newEnd == objects.end() ){
    return false;
  }
  objects.erase(newEnd, objects.end());
  return true;
}

bool remove_objects_from( objects_t& objects, const objects_t& remove ){
  auto should_remove = [&remove](const Object* obj){return contains(remove, obj);};
  objects_t::iterator newEnd = std::remove_if( objects.begin(), objects.end(), should_remove);
  if ( newEnd == objects.end() ){
    return false;
  }
  objects.erase(newEnd, objects.end());
  return true;
}

bool remove( const objects_t& remove, const from_t<objects_t>& objects ){
  return remove_objects_from(objects.Get(), remove);
}

bool resize_handles_disabled( const Object* obj ){
  return !resize_handles_enabled(obj);
}

bool resize_handles_enabled( const Object* obj ){
  return point_edit_disabled(obj);
}

ColorSetting setting_used_for_fill( const Object* obj ){
  const Settings& s(obj->GetSettings());
  assert( s.Has(ts_FillStyle) );
  return s.Get(ts_FillStyle) == FillStyle::FILL ?
    ts_FgCol : ts_BgCol;
}

const faint::coord g_maxSnapDistance = LITCRD(20.0);
Point snap( const Point& sourcePt, const objects_t& objects, const Grid& grid, faint::coord maxSnapDistance ){
  std::vector<Point> noExtraPoints;
  return snap( sourcePt, objects, grid, noExtraPoints, maxSnapDistance );
}

Point snap( const Point& sourcePt, const objects_t& objects, const Grid& grid, const std::vector<Point>& extraPoints, faint::coord maxSnapDistance ){
  faint::coord lastSnapDistance = maxSnapDistance;
  Point currentPt( sourcePt );
  for ( const Object* obj : objects ){
    Point snapPt(0,0);
    for ( const Point& pt : obj->GetAttachPoints() ){
      snapPt = pt;
      faint::coord snapDistance = distance(sourcePt, snapPt);
      if ( snapDistance < lastSnapDistance ){
        // Snap to this closer point instead
        lastSnapDistance = snapDistance;
        currentPt = snapPt;
      }
    }
    if ( currentPt == snapPt ){
      // Fixme: I'm not sure what this is about? Is it to prefer snapping to the topmost object?
      break;
    }
  }

  if ( grid.Enabled() ){
    Point gridPoint = snap_to_grid( grid, sourcePt );
    faint::coord snapDistance = distance(sourcePt, gridPoint);
    if ( snapDistance < lastSnapDistance ){
      lastSnapDistance = snapDistance;
      currentPt = gridPoint;
    }
  }

  for ( const Point& snapPt : extraPoints ){
    faint::coord snapDistance = distance(sourcePt, snapPt);
    if ( snapDistance < lastSnapDistance ){
      // Snap to this closer point instead
      lastSnapDistance = snapDistance;
      currentPt = snapPt;
    }
  }
  return currentPt;
}

faint::coord snap_x( faint::coord sourceX, const objects_t& objects, const Grid& grid, faint::coord y0, faint::coord y1, faint::coord maxSnapDistance ){
  faint::coord lastSnapDistance = maxSnapDistance;
  faint::coord current( sourceX );
  for ( const Object* obj : objects ){
    Point snapPt(0,0);
    for ( const Point& pt : obj->GetAttachPoints() ){
      snapPt = pt;
      if ( y0 <= snapPt.y && snapPt.y <= y1 ){
        faint::coord snapDistance = std::fabs(snapPt.x - sourceX );
        if ( lastSnapDistance > snapDistance ){
          // Snap to this closer point instead
          lastSnapDistance = snapDistance;
          current = snapPt.x;
        }
      }
    }
    if ( current == snapPt.x ){
      // Fixme: I'm not sure what this is about? Is it to prefer snapping to the topmost object?
      break;
    }
  }

  if ( grid.Enabled() ){
    Point gridPoint = snap_to_grid( grid, Point( sourceX, 0 ) );
    faint::coord snapDistance = std::fabs( sourceX - gridPoint.x );
    if ( snapDistance < lastSnapDistance ){
      current = gridPoint.x;
    }
  }

  return current;
}

faint::coord snap_y( faint::coord sourceY, const objects_t& objects, const Grid& grid, faint::coord x0, faint::coord x1, faint::coord maxSnapDistance ){
  faint::coord lastSnapDistance = maxSnapDistance;
  faint::coord current( sourceY );
  for ( Object* obj : objects ){
    Point snapPt(0,0);
    for ( const Point& pt : obj->GetAttachPoints() ){
      snapPt = pt;
      if ( x0 <= snapPt.x && snapPt.x <= x1 ){
        faint::coord snapDistance = std::fabs(snapPt.y - sourceY );
        if ( lastSnapDistance > snapDistance ){
          // Snap to this closer point instead
          lastSnapDistance = snapDistance;
          current = snapPt.y;
        }
      }
    }
    if ( current == snapPt.y ){
      // Fixme: I'm not sure what this is about? Is it to prefer snapping to the topmost object?
      break;
    }
  }
  if ( grid.Enabled() ){
    Point gridPoint = snap_to_grid( grid, Point( 0, sourceY ) );
    faint::coord snapDistance = fabs( sourceY - gridPoint.y );
    if ( snapDistance < lastSnapDistance ){
      current = gridPoint.y;
    }
  }
  return current;
}

bool toggle_object_aligned_resize(Object* object){
  const Settings& settings = object->GetSettings();
  if ( !settings.Has(ts_AlignedResize) ){
    return false;
  }
  object->Set( ts_AlignedResize, !settings.Get(ts_AlignedResize));
  return true;
}

bool toggle_edit_points(Object* object){
  const Settings& settings = object->GetSettings();
  if ( !settings.Has(ts_EditPoints) ){
    return false;
  }
  object->Set( ts_EditPoints, !settings.Get(ts_EditPoints));
  return true;
}
