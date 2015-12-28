#ifndef FAINT_NAN_CHECK_HH
#define FAINT_NAN_CHECK_HH
#ifdef FAINT_MSW
#include <cfloat>
inline bool faint_isnan( double v ){
  return _isnan(v) == 1;
}
#endif // FAINT_MSW
#ifndef FAINT_MSW
#include <cmath>
inline bool faint_isnan( double v ){
  return std::isnan(v);
}
#endif
#endif
