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

#ifndef FAINT_DRAW_SOURCE_MAP_HH
#define FAINT_DRAW_SOURCE_MAP_HH
#include "geo/intpoint.hh"
#include "util/unique.hh"

namespace faint{
class DrawSource;
class Color;
class DrawSourceMapImpl;

class category_drawsource_map;
typedef Unique<IntSize, category_drawsource_map, 0> CellSize;
typedef Unique<IntSize, category_drawsource_map, 1> CellSpacing;
typedef Unique<IntPoint, category_drawsource_map, 2> CellPos;
typedef Order<CellPos>::New NewPos;
typedef Order<CellPos>::Old OldPos;

class DrawSourceMap{
public:
  DrawSourceMap();
  DrawSourceMap( const DrawSourceMap& );
  ~DrawSourceMap();
  void Append( const DrawSource& );
  void Copy( const OldPos&, const NewPos& );
  Bitmap CreateBitmap( const CellSize&, const CellSpacing&,
    const Color& ) const;
  void Erase( const CellPos& );
  const DrawSource& Get( const CellPos& ) const;
  IntSize GetSize() const;
  bool Has( const CellPos& ) const;
  void Insert( const CellPos&, const DrawSource& );
  void Move( const OldPos&, const NewPos& );
  void Replace( const CellPos&, const DrawSource& );
  DrawSourceMap& operator=( const DrawSourceMap& );
private:
  DrawSourceMapImpl* m_impl;
};

// Adds a border around a cell for a bitmap created with CreateBitmap
void add_cell_border( Bitmap& bmp, const CellPos& pos, const CellSize& in_cellSize, const CellSpacing& in_cellSpacing );

// Converts a point in view-coordinates relative to the top-left of a
// bitmap created with CreateBitmap into positions in a DrawSourceMap.
//
CellPos view_to_cell_pos(const IntPoint&, const CellSize& in_cellSize, const CellSpacing& in_cellSpacing);

} // namespace faint

#endif
