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
#include "bitmap/bitmap.hh"
#include "bitmap/color-counting.hh"
#include "util/color.hh"

static void insert_color_count( faint::color_counts_t& colors, const faint::Color& c ){
  auto it = colors.find(c);
  if ( it != colors.end() ){
    it->second++;
  }
  else{
    colors.insert( std::make_pair(c, 1) );
  }
}

namespace faint{

void add_color_counts(const Bitmap& bmp, color_counts_t& colors){
  assert(bmp.m_w > 0 && bmp.m_h > 0 );
  for ( int y = 0; y != bmp.m_h; y++ ){
    for ( int x = 0; x != bmp.m_w; ++x ){
      insert_color_count( colors, get_color_raw(bmp, x,y) );
    }
  }
}

static bool color_frequency_cmp( const color_counts_t::value_type& c1, const color_counts_t::value_type& c2 ){
  return c1.second < c2.second;
}

faint::Color most_common( const color_counts_t& colors ){
  assert( !colors.empty() );
  auto it = std::max_element(colors.begin(), colors.end(), color_frequency_cmp);
  assert( it != colors.end() );
  return it->first;
}

} // namespace
