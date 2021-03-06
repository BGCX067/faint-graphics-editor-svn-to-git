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
#include "bitmap/histogram.hh"
#include "geo/geotypes.hh"
#include "util/unique.hh"

size_t to_size_t( const bin_t& bin ){
  int v = bin.Get();
  assert( v >= 0 );
  return static_cast<size_t>(v);
}

Histogram::Histogram( const ClosedIntRange& range, const bin_t& numBins )
  : m_range(range),
    m_data(to_size_t(numBins), 0)
{
  assert( numBins.Get() <= range.Delta() + 1 );
}

size_t Histogram::GetBin( int value ) const{
  assert( m_range.Has(value) );
  double perBin = ( m_range.Delta()+ 1) / double(m_data.size());
  return static_cast<size_t>( ( value - m_range.Lower() ) / perBin );
}

int Histogram::Count( const bin_t& bin ) const{
  size_t raw(to_size_t(bin));
  assert(raw < m_data.size() );
  return m_data[raw];
}

void Histogram::Insert( int value ){
  size_t bin = GetBin(value);
  assert( bin < m_data.size() );
  m_data[bin] += 1;
}

void Histogram::Insert( int value, const count_t& count ){
  size_t bin = GetBin(value);
  assert( bin < m_data.size() );
  m_data[bin] += count.Get();
}

int Histogram::Max() const{
  auto it = std::max_element(m_data.begin(), m_data.end());
  assert(it != m_data.end());
  return *it;
}

int Histogram::NumBins() const{
  return resigned(m_data.size());
}
