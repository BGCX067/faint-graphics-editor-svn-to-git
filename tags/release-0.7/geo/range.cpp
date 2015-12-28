#include <cassert>
#include <algorithm>
#include "geo/range.hh"

IntRange::IntRange(){
}

IntRange::IntRange(const max_t& max)
  :  m_max(max.Get())
{}

IntRange::IntRange(const min_t& min)
  : m_min(min.Get())
{}

IntRange::IntRange(const min_t& min, const max_t& max)
  : m_max(max.Get()),
    m_min(min.Get())
{
  assert(m_min.Get() <= m_max.Get());
}

int IntRange::Constrain( int value ) const{
  if ( m_min.IsSet() ){
    value = std::max(m_min.Get(), value);
  }
  if ( m_max.IsSet() ){
    value = std::min(m_max.Get(), value);
  }
  return value;
}

bool IntRange::Has( int value ) const{
  if ( m_min.IsSet() && value < m_min.Get() ){
    return false;
  }
  if ( m_max.IsSet() && m_max.Get() < value ){
    return false;
  }
  return true;
}

IntSize constrained( const IntSize& sz, const min_t& w, const min_t& h ){
  return IntSize(std::max(w.Get(), sz.w),
    std::max(h.Get(), sz.h));
}
