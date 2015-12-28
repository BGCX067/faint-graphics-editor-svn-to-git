// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#include <cassert>
#include <algorithm>
#include "geo/range.hh"
#include "geo/int-range.hh"
namespace faint{

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

int IntRange::Constrain(int value) const{
  if (m_min.IsSet()){
    value = std::max(m_min.Get(), value);
  }
  if (m_max.IsSet()){
    value = std::min(m_max.Get(), value);
  }
  return value;
}

bool IntRange::Has(int value) const{
  if (m_min.IsSet() && value < m_min.Get()){
    return false;
  }
  if (m_max.IsSet() && m_max.Get() < value){
    return false;
  }
  return true;
}

ClosedIntRange::ClosedIntRange(const min_t& minValue, const max_t& maxValue)
  : m_max(maxValue.Get()),
    m_min(minValue.Get())
{
  assert(m_min <= m_max);
}

int ClosedIntRange::Constrain(int value) const{
  return std::min(m_max, std::max(m_min, value));
}

double ClosedIntRange::Constrain(double value) const{
  if (value < m_min){
    return static_cast<double>(m_min);
  }
  else if (value > m_max){
    return static_cast<double>(m_max);
  }
  return value;
}

int ClosedIntRange::Delta() const{
  return m_max - m_min;
}

bool ClosedIntRange::Has(int value) const{
  return m_min <= value && value <= m_max;
}

bool ClosedIntRange::Has(double value) const{
  return static_cast<double>(m_min) <= value && value <= static_cast<double>(m_max);
}

int ClosedIntRange::Lower() const{
  return m_min;
}

int ClosedIntRange::Upper() const{
  return m_max;
}

BoundedInt::BoundedInt(const min_t& lower, int v, const max_t& upper)
  : m_range(lower, upper),
    m_value(v)
{
  assert(m_range.Has(m_value));
}

BoundedInt BoundedInt::Mid(const min_t& lower, const max_t& upper){
  return BoundedInt(lower, (lower.Get() + upper.Get()) / 2, upper);
}

BoundedInt BoundedInt::Min(const min_t& lower, const max_t& upper){
  return BoundedInt(lower, lower.Get(), upper);
}

int BoundedInt::GetValue() const{
  return m_value;
}

ClosedIntRange BoundedInt::GetRange() const{
  return m_range;
}

Interval::Interval(const min_t& minValue, const max_t& maxValue)
  : m_min(minValue.Get()),
    m_max(maxValue.Get())
{
  assert(m_min <= m_max);
}
bool Interval::Has(int value) const{
  return m_min <= value && value <= m_max;
}

int Interval::Lower() const{
  return m_min;
}

int Interval::Upper() const{
  return m_max;
}

IntSize constrained(const IntSize& sz, const min_t& w, const min_t& h){
  return IntSize(std::max(w.Get(), sz.w),
    std::max(h.Get(), sz.h));
}

Interval make_interval(int v0, int v1){
  return Interval(min_t(std::min(v0, v1)), max_t(std::max(v0, v1)));
}

ClosedIntRange make_closed_range(int v0, int v1){
  return ClosedIntRange(min_t(std::min(v0, v1)), max_t(std::max(v0, v1)));
}

} // namespace
