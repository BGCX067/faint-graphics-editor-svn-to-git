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

#include <algorithm> // For std::unique
#include <map>
#include <numeric> // For std::accumulate
#include "bitmap/bitmap.hh"
#include "util/colorutil.hh"
#include "util/drawsource.hh"
#include "util/draw-source-map.hh"

namespace faint{
typedef std::vector<faint::DrawSource> drawsource_vec_t;
typedef std::vector<drawsource_vec_t> color_map_t;

IntPoint unpack( const NewPos& newPos ){
  return newPos.Get().Get();
}

IntPoint unpack( const OldPos& oldPos ){
  return oldPos.Get().Get();
}

static int get_max_row( const color_map_t& colors ){
  assert(!colors.empty());
  return resigned(colors.size()) - 1;
}

static int get_max_column( const color_map_t& colors ){
  assert(!colors.empty());
  int maxCol = resigned(colors[0].size());
  for ( size_t i = 1; i != colors.size(); i++ ){
    maxCol = std::max(resigned(colors[i].size()), maxCol);
  }
  return maxCol;
}

struct container_size_less{
  bool operator()( const drawsource_vec_t& lhs, const drawsource_vec_t& rhs ) const{
    return lhs.size() < rhs.size();
  }
};

static drawsource_vec_t& get_shortest_row( color_map_t& colors ){
  assert(!colors.empty());
  color_map_t::iterator it = std::min_element(colors.begin(), colors.end(), container_size_less());
  return *it;
}

struct max_coords_vec{
  IntPoint operator()( const IntPoint& lhs, const IntPoint& rhs ) const {
    return max_coords(lhs, rhs);
  }
};

struct min_coords_vec{
  IntPoint operator()( const IntPoint& lhs, const IntPoint& rhs ) const {
    return min_coords(lhs, rhs);
  }
};

static IntPoint get_max_coords( const std::vector<IntPoint>& v ){
  assert(!v.empty());
  return std::accumulate(v.begin(), v.end(), v.front(), max_coords_vec());
}

static IntPoint get_min_coords( const std::vector<IntPoint>& v ){
  assert(!v.empty());
  return std::accumulate(v.begin(), v.end(), v.front(), min_coords_vec());
}

static bool has_index( const color_map_t& colors, const IntPoint& pt ){
  if ( pt.x < 0 || pt.y < 0 || colors.size() <= static_cast<size_t>(pt.y) ){
    return false;
  }
  return static_cast<size_t>(pt.x) < colors[pt.y].size();
}

static void erase_index( color_map_t& colors, const IntPoint& index ){
  assert(has_index(colors, index));
  drawsource_vec_t& row = colors[index.y];
  row.erase(row.begin() + index.x);
}

static const faint::DrawSource& get_color( const color_map_t& colors, const IntPoint& pt ){
  assert( pt.x >= 0 );
  assert( pt.y >= 0 );
  assert(colors.size() > static_cast<size_t>(pt.y));
  const drawsource_vec_t& row = colors[pt.y];
  assert( row.size() > static_cast<size_t>(pt.x) );
  return row[pt.x];
}

static bool lacks( const std::vector<IntPoint>& v, const IntPoint& p ){
  return std::find(v.begin(), v.end(), p) == v.end();
}

static bool contains( const std::vector<IntPoint>& v, const IntPoint& p ){
  return !lacks(v, p);
}

static std::vector<IntPoint> flood_fill_search( const color_map_t& colors, const IntPoint& pos ){
  std::vector<IntPoint> consider;
  std::vector<IntPoint> filled;
  assert(has_index(colors, pos));
  const faint::DrawSource& src = get_color(colors, pos);
  consider.push_back(pos);
  for ( const IntPoint& currPt : consider ){
    if ( !has_index(colors, currPt) || get_color(colors, currPt) != src ){
      continue;
    }
    {
      IntPoint w = currPt;
      for (;;){
        filled.push_back(w);
        w -= delta_x(1);

        if ( !has_index(colors, w) || get_color(colors, w) != src || contains(filled, w) ){
          break;
        }
        const IntPoint above(w - delta_y(1));
        if ( has_index(colors, above) ){
          const faint::DrawSource& tempSrc(get_color(colors, above));
          if ( tempSrc == src && lacks(filled, above) ){
            consider.push_back(above);
          }
        }

        const IntPoint below(w + delta_y(1));
        if ( has_index(colors, below) ){
          const faint::DrawSource& tempSrc(get_color(colors, below));
          if ( tempSrc == src && lacks(filled, below) ){
            consider.push_back(below);
          }
        }
      }
    } // Extra scope w
    {
      IntPoint e = currPt;
      for (;;){
        filled.push_back(e);
        const IntPoint above = e - delta_y(1);
        if ( has_index(colors, above) ){
          const faint::DrawSource& tempSrc(get_color(colors, above));
          if ( tempSrc == src && lacks(filled, above) ){
            consider.push_back(above);
          }
        }

        const IntPoint below(e + delta_y(1));
        if ( has_index(colors, below) ){
          const faint::DrawSource& tempSrc(get_color(colors, below));
          if ( tempSrc == src && lacks(filled, below) ){
            consider.push_back(below);
          }
        }
        e += delta_x(1);
        if ( !has_index(colors, e) || get_color(colors, e) != src ){
          break;
        }
      }
    }
  }
  filled.erase(std::unique(filled.begin(), filled.end()), filled.end());
  return filled;
}

static void copy_drawsource( color_map_t& colors, const IntPoint& oldPos, const IntPoint& newPos, bool eraseOld )
{
  assert(has_index(colors, oldPos));
  assert( fully_positive(newPos) );
  if ( oldPos == newPos ){
    return;
  }
  const faint::DrawSource& src(get_color(colors, oldPos));
  assert( static_cast<size_t>(newPos.y) < colors.size() );
  drawsource_vec_t& row = colors[newPos.y];
  if ( row.size() <= static_cast<size_t>(newPos.x) ){
    row.push_back(src);
    if ( eraseOld ){
      erase_index(colors, oldPos);
    }
  }
  else {
    bool moveLeft = oldPos.x >= newPos.x;
    bool sameRow = newPos.y == oldPos.y;
    bool moveRight = !moveLeft;
    int insertOffset = moveRight && sameRow && eraseOld ? 1 : 0;
    int removeOffset = moveLeft && sameRow ? 1 : 0;
    row.insert(row.begin() + newPos.x + insertOffset, src);
    if ( eraseOld ){
      erase_index(colors, oldPos + delta_x(removeOffset));
    }
  }
}

static faint::Bitmap palette_bitmap( const color_map_t& colors, const CellSize& in_cellSize, const CellSpacing& in_cellSpacing, const faint::Color& bgColor ){
  if ( colors.empty() ){
    return Bitmap(in_cellSize.Get(), bgColor);
  }

  IntSize cellSize(in_cellSize.Get());
  IntSize cellSpacing(in_cellSpacing.Get());

  int maxRow = get_max_row(colors);
  IntPoint maxPos( get_max_column(colors),
    maxRow );
  IntPoint offset( point_from_size(cellSize + cellSpacing) );
  IntSize bitmapSize( (maxPos.x + 1) * (cellSize.w + cellSpacing.w),
    (maxPos.y + 1) * (cellSize.h + cellSpacing.h) );
  faint::Bitmap bmp(bitmapSize, bgColor);
  std::set<IntPoint> ignore;

  for ( int rowNum = 0; rowNum <= maxRow; rowNum++ ){
    const drawsource_vec_t& row(colors[rowNum]);
    const int colSize = resigned(row.size());
    for ( int colNum = 0; colNum != colSize; colNum++ ){
      const IntPoint pos(colNum, rowNum);
      if ( ignore.find(pos) != ignore.end() ){
        continue;
      }
      assert(has_index(colors, pos));
      const faint::DrawSource& src(get_color(colors, pos));
      std::vector<IntPoint> identical = flood_fill_search(colors, pos);
      assert( !identical.empty() );
      ignore.insert(identical.begin(), identical.end());
      IntPoint maxPos(get_max_coords(identical));
      IntPoint minPos(get_min_coords(identical));
      IntSize deltaCells(size_from_point(maxPos - minPos));
      IntSize size(cellSize * (deltaCells + IntSize(1,1)) + deltaCells * cellSpacing);

      faint::Bitmap cellBitmap = draw_source_bitmap(src, size, cellSize);
      for ( int x = minPos.x; x <= maxPos.x; x++ ){
        for ( int y = minPos.y; y <= maxPos.y; y++ ){
          if (lacks(identical, IntPoint(x,y))){

            fill_rect_color(cellBitmap,
              IntRect(IntPoint((x - minPos.x)  * ( cellSize.w + cellSpacing.w) - cellSpacing.w,
                (y - minPos.y) * (cellSize.h + cellSpacing.h) - cellSpacing.h),
                cellSize + 2 * cellSpacing),
              faint::color_transparent_white());
          }
        }
      }
      IntPoint topLeft(minPos * offset);
      blend( cellBitmap, onto(bmp), minPos * offset );
    }
  }
  return bmp;
}

class DrawSourceMapImpl{
public:
  color_map_t colors;
};

DrawSourceMap::DrawSourceMap(){
  m_impl = new DrawSourceMapImpl;
  m_impl->colors.push_back(drawsource_vec_t());
  m_impl->colors.push_back(drawsource_vec_t());
}

DrawSourceMap::DrawSourceMap( const DrawSourceMap& other ){
  m_impl = new DrawSourceMapImpl;
  m_impl->colors = other.m_impl->colors;
}

DrawSourceMap::~DrawSourceMap(){
  delete m_impl;
}

void DrawSourceMap::Append( const DrawSource& src ){
  color_map_t& colors = m_impl->colors;
  assert(!colors.empty());
  drawsource_vec_t& row = get_shortest_row(colors);
  row.push_back(src);
  return;
}

void DrawSourceMap::Copy( const OldPos& oldPos, const NewPos& newPos ){
  return copy_drawsource( m_impl->colors, unpack(oldPos), unpack(newPos), false );
}

Bitmap DrawSourceMap::CreateBitmap( const CellSize& size, const CellSpacing& spacing, const Color& bgColor ) const{
  return palette_bitmap( m_impl->colors, size, spacing, bgColor );
}

void DrawSourceMap::Erase( const CellPos& removePos ){
  erase_index(m_impl->colors, removePos.Get());
}

const DrawSource& DrawSourceMap::Get( const CellPos& in_pos ) const{
  IntPoint pos(in_pos.Get());
  assert(has_index(m_impl->colors, pos));
  return get_color(m_impl->colors, pos);
}

IntSize DrawSourceMap::GetSize() const{
  return IntSize( get_max_column(m_impl->colors),
    get_max_row(m_impl->colors) );
}

bool DrawSourceMap::Has( const CellPos& pos ) const{
  return has_index(m_impl->colors, pos.Get());
}

void DrawSourceMap::Move( const OldPos& in_oldPos, const NewPos& in_newPos){
  copy_drawsource(m_impl->colors, unpack(in_oldPos), unpack(in_newPos), true);
}

void DrawSourceMap::Replace( const CellPos& in_pos, const faint::DrawSource& src ){
  IntPoint pos(in_pos.Get());
  assert(has_index(m_impl->colors,pos));
  m_impl->colors[pos.y][pos.x] = src;
}

DrawSourceMap& DrawSourceMap::operator=( const DrawSourceMap& other ){
  if ( this == &other ){
    return *this;
  }
  delete m_impl;
  m_impl = new DrawSourceMapImpl;
  m_impl->colors = other.m_impl->colors;
  return *this;
}

void add_cell_border( faint::Bitmap& bmp, const CellPos& pos, const CellSize& in_cellSize, const CellSpacing& in_cellSpacing ){
  IntSize cellSize(in_cellSize.Get());
  IntSize cellSpacing(in_cellSpacing.Get());
  IntPoint offset(point_from_size( cellSize + cellSpacing ));
  IntPoint topLeft( pos.Get() * offset );

  faint::draw_rect_color(bmp, IntRect(topLeft, cellSize),
    faint::color_black(), 1, false);
}

CellPos view_to_cell_pos(const IntPoint& viewPos, const CellSize& in_cellSize, const CellSpacing& in_cellSpacing){
  IntPoint offset(point_from_size(in_cellSize.Get() + in_cellSpacing.Get()));
  assert( offset.x > 0 && offset.y > 0 );
  return CellPos(viewPos / point_from_size(in_cellSize.Get() + in_cellSpacing.Get()));
}

} // namespace faint
