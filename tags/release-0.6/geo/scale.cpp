#include "scale.hh"

Scale::Scale(faint::coord xy)
  : x(xy),
    y(xy)
{}

Scale::Scale(faint::coord in_x, faint::coord in_y)
  : x(in_x),
    y(in_y)
{}

Scale::Scale(const NewSize& inNew, const Size& oldSz){
  const Size& newSz(inNew.Get());
  x = newSz.w / oldSz.w;
  y = newSz.h / oldSz.h;
}

Scale invert_x_scale(){
  return Scale(LITCRD(-1.0), LITCRD(1.0));
}

Scale invert_y_scale(){
  return Scale(LITCRD(1.0), LITCRD(-1.0));
}
