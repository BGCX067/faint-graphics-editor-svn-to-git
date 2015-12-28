#ifndef FAINT_PATHPT_HH
#define FAINT_PATHPT_HH
#include "geotypes.hh"

class PathPt {
public:
  enum Type {
    ArcTo,
    LineTo,
    CurveTo,
    CubicBezier,
    QuadraticBezier,
    MoveTo,
    Close
  };
  PathPt( Type t, faint::coord in_x, faint::coord in_y )
    : type(t), x(in_x), y(in_y), large_arc_flag(0), sweep_flag(0)
  {
    // assert( t == LineTo || t == MoveTo );
    cx = cy = dx = dy = 0;
  }

  PathPt( Type t, faint::coord in_x, faint::coord in_y,
           faint::coord in_cx, faint::coord in_cy,
           faint::coord in_dx, faint::coord in_dy ) :
    type(t),
    x( in_x ),
    y( in_y ),
    cx( in_cx ),
    cy( in_cy ),
    dx( in_dx ),
    dy( in_dy ),
    large_arc_flag(0),
    sweep_flag(0)
  {}
  static PathPt Move( faint::coord in_x, faint::coord in_y ){
    return PathPt( MoveTo, in_x, in_y );
  }
  static PathPt Line( faint::coord in_x, faint::coord in_y ){
    return PathPt( LineTo, in_x, in_y );
  }

  static PathPt Curve( faint::coord in_x, faint::coord in_y, faint::coord in_cx, faint::coord in_cy, faint::coord in_dx, faint::coord in_dy ){
    return PathPt( CubicBezier, in_x, in_y, in_cx, in_cy, in_dx, in_dy );
  }

  static PathPt Arc( faint::coord rx, faint::coord ry, faint::coord axis_rotation, int in_large_arc_flag, int in_sweep_flag, faint::coord in_x, faint::coord in_y ){
    PathPt pt( ArcTo );
    pt.x = in_x;
    pt.y = in_y;
    pt.cx = rx;
    pt.cy = ry;
    pt.dx = axis_rotation;
    pt.large_arc_flag = in_large_arc_flag;
    pt.sweep_flag = in_sweep_flag;
    return pt;
  }
  PathPt( Type t )
    : type(t)
  {
    x = y = cx = cy = dx = dy = 0;
    large_arc_flag = sweep_flag = 0;
  }
  Type type;
  faint::coord x;
  faint::coord y;
  faint::coord cx;
  faint::coord cy;
  faint::coord dx;
  faint::coord dy;
  int large_arc_flag;
  int sweep_flag;

};

#endif
