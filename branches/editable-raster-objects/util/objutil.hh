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

#ifndef FAINT_OBJUTIL_HH
#define FAINT_OBJUTIL_HH
#include "util/commonfwd.hh"
#include "util/unique.hh"
class ObjRaster;

objects_t as_list( Object* );
ObjRaster* as_ObjRaster( Object* );
Rect bounding_rect( const objects_t& );
objects_t clone( const objects_t& );

// Returns the color at imagePos, which is relative to the image (not
// the ObjRaster!). Will return the ts_BgCol of the ObjRaster if the
// imagePos is outside the object.  Ignores masking, so if imagePos
// falls on a region in the object which is transparent due to being
// the masked background color, the color will still be returned.
faint::Color color_at( ObjRaster*, const Point& imagePos );

// true if atleast one object has sub-objects
bool contains_group( const objects_t& );

bool contains( const objects_t&, const Object* );
bool contains( const objects_t&, const these_t<objects_t>& );

// Returns the object index, or the vector size if not found.
size_t find_object_index( Object*, const objects_t& );
std::vector<Point> get_attach_points( const Tri& );
objects_t get_groups( const objects_t& );
objects_t get_intersected(const objects_t&, const Rect& );

// Returns the exact name of a single object (e.g. "Line"), the
// naively-pluralized name for multiple same-type objects
// (e.g. "Lines"), and the generic "Objects" for mixes.
std::string get_collective_name( const objects_t& );
Point get_delta( Object*, const Point& );
Settings get_object_settings( const objects_t& );

// Find where in target to insert to keep the relative sort order of source
size_t get_sorted_insertion_pos( Object* insertee, const objects_t& target, const objects_t& source );

tris_t get_tris( const objects_t& );

// True if the the object or one of its sub-objects match the id
bool is_or_has( const Object*, const ObjectId& );
bool is_raster_object( Object* );
bool is_text_object( Object* );
its_yours_t<Object> its_yours( Object* );
just_a_loan_t<Object> just_a_loan( Object* );
bool lacks( const objects_t&, const Object* );
void move_to( Object*, const Point& );

// Returns the point with the next index, possibly wrapping or doing
// other magic stuff.
Point next_point( Object*, size_t index );
bool object_aligned_resize( const Object* );
void offset_by( Object*, const Point& delta );
void offset_by( Object*, const IntPoint& delta );
void offset_by( const objects_t&, const Point& delta );
void offset_by( const objects_t&, const IntPoint& delta );
bool point_edit_disabled( const Object* );

// True if the objects points should be adjustable. This should be
// checked by tools that care about movable and extension points.
bool point_edit_enabled( const Object* );

// Returns the point with previous index, possibly wrapping or doing
// other magic stuff.
Point prev_point( Object*, size_t index );
bool remove( const Object*, const from_t<objects_t>& );
bool remove( const objects_t&, const from_t<objects_t>& );

bool resize_handles_disabled( const Object* );
bool resize_handles_enabled( const Object* );

extern const faint::coord g_maxSnapDistance;
Point snap( const Point&, const objects_t&, const Grid&, faint::coord maxDistance=g_maxSnapDistance);
Point snap( const Point&, const objects_t&, const Grid&, const std::vector<Point>& extraPoints, faint::coord maxDistance=g_maxSnapDistance);
faint::coord snap_x( faint::coord x, const objects_t&, const Grid&, faint::coord y0, faint::coord y1, faint::coord maxDistance=g_maxSnapDistance );
faint::coord snap_y( faint::coord y, const objects_t&, const Grid&, faint::coord x0, faint::coord x1, faint::coord maxDistance=g_maxSnapDistance );

// Toggles ts_AlignedRsize for the object and returns true, or false
// if the the object does not support aligned resize.
bool toggle_object_aligned_resize( Object* );

// Toggles ts_EditPoints for the object and returns true, or false if
// the object does not support point editing.
bool toggle_edit_points( Object* );
#endif
