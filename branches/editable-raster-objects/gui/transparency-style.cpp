#include "transparency-style.hh"

TransparencyStyle::TransparencyStyle(){
}

TransparencyStyle::TransparencyStyle( const faint::ColRGB& color )
  : m_color(color)
{}

bool TransparencyStyle::IsCheckered() const{
  return !IsColor();
}

bool TransparencyStyle::IsColor() const{
  return m_color.IsSet();
}

faint::ColRGB TransparencyStyle::GetColor() const{
  return m_color.Get();
}
