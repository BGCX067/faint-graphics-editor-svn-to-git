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
#ifndef FAINT_RANGE_HH
#define FAINT_RANGE_HH
#include "util/unique.hh"
#include "geo/intsize.hh"

class IntRange;
typedef Unique<int, IntRange, 0> min_t;
typedef Unique<int, IntRange, 1> max_t;

class Interval{
public:
  Interval( const min_t&, const max_t& );
  int Lower() const;
  int Upper() const;
private:
  int m_min;
  int m_max;
};

class IntRange{
  // A range for constraining integers.
  // Can extend to infinity in either direction
public:
  // Creates an unlimited range, Constrain():s nothing,
  // Has() everything.
  IntRange();

  // A range between min and max.
  // Asserts that min <= max.
  IntRange(const min_t&, const max_t&);

  IntRange(const min_t&); // Lower bound only
  IntRange(const max_t&); // Upper bound only

  // Return the value constrained to the range.
  int Constrain( int ) const;

  // True if the value is within the range
  bool Has( int ) const;

  // True if the interval is within the Range
  bool Has( const Interval& ) const;
private:
  Optional<int> m_max;
  Optional<int> m_min;
};

class ClosedIntRange{
  // A range that, unlike IntRange, does not allow infinity
public:
  // A range between min and max.
  // Asserts that min <= max.
  ClosedIntRange(const min_t&, const max_t&);
  // Return the value constrained to the range.
  int Constrain( int ) const;
  double Constrain( double value ) const;
  int Delta() const;
  // True if the value is within the range
  bool Has( int ) const;
  bool Has( double ) const;

  // True if the interval is within the Range
  bool Has( const Interval& ) const;
  int Lower() const;
  int Upper() const;
private:
  int m_max;
  int m_min;
};

IntSize constrained(const IntSize&, const min_t& w, const min_t& h );

// Fixme: Rename as e.g. BoundedInterval
template<int MIN_BOUND, int MAX_BOUND>
class DefinedIntRange{
  // A template for integer ranges with per-type minimum and maximum bounds.
public:
  DefinedIntRange( const Interval& interval )
    : m_interval(interval)
  {
    assert(Valid(interval));
  }

  int GetMax() const{
    return m_interval.Upper();
  }

  int GetMin() const{
    return m_interval.Lower();
  }

  static bool Valid( const Interval& interval ){
    return MIN_BOUND <= interval.Lower() && interval.Upper() <= MAX_BOUND;
  }

  static_assert(MIN_BOUND < MAX_BOUND, "Invalid template range for DefinedIntRange");

  // Using static integers with in-class initialization caused linker
  // error in template function as_closed_range with g++ 4.7.2 when
  // optimization off (flags -g -O0).
  //
  // Using enum-hack instead for now.
  enum {min_allowed=MIN_BOUND};
  enum {max_allowed=MAX_BOUND};
  typedef int value_type;
private:
  Interval m_interval;
};

ClosedIntRange make_closed_range( int v0, int v1 );
Interval make_interval( int v0, int v1 );

template<typename RangeType>
int value_at( double fraction ){
  return static_cast<typename RangeType::value_type>((RangeType::max_allowed - RangeType::min_allowed) * fraction) + RangeType::min_allowed;
}

template<typename RangeType>
Interval fractional_interval( double fraction1, double fraction2 ){
  return make_interval( value_at<RangeType>( fraction1 ), value_at<RangeType>(fraction2) );
}

template<typename RangeType>
ClosedIntRange as_closed_range(){
  return ClosedIntRange(min_t(RangeType::min_allowed), max_t(RangeType::max_allowed));
}

template<typename T>
T constrained( const typename Boundary<T>::Min& minVal, T value, const typename Boundary<T>::Max& maxVal ){
  return std::max(minVal.Get(), std::min(value, maxVal.Get()));
}

#endif
