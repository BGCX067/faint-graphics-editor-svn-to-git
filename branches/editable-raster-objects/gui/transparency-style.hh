#ifndef FAINT_TRANSPARENCY_STYLE_HH
#define FAINT_TRANSPARENCY_STYLE_HH
#include "util/color.hh"
#include "util/unique.hh"

class TransparencyStyle{
  // The background style for indicating image regions with alpha.
public:
  // Default (Blend towards checkered pattern)
  TransparencyStyle();
  // Blend towards solid color
  TransparencyStyle(const faint::ColRGB&);

  bool IsCheckered() const;
  bool IsColor() const;

  // Note: IsColor must be true.
  faint::ColRGB GetColor() const;
private:
  Optional<faint::ColRGB> m_color;
};

#endif
