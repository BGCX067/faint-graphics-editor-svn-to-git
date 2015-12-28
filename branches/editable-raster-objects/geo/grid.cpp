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

#include "geotypes.hh"
#include "grid.hh"

faint::Color default_grid_color(){
  return faint::Color(100,100,255,150);
}

Grid::Grid(bool enabled, bool visible, int spacing, const faint::Color color)
  : m_color(color),
    m_dashed(true),
    m_enabled( enabled ),
    m_spacing(spacing),
    m_visible( visible )
{}

faint::Color Grid::Color() const{
  return m_color;
}

bool Grid::Enabled() const{
  return m_enabled;
}

bool Grid::Dashed() const{
  return m_dashed;
}

void Grid::SetColor( const faint::Color& color ){
  m_color = color;
}

void Grid::SetDashed( bool dashed ){
  m_dashed = dashed;
}

void Grid::SetEnabled( bool enabled ){
  m_enabled = enabled;
}

void Grid::SetSpacing( int spacing ){
  m_spacing = spacing;
}

void Grid::SetVisible( bool visible ){
  m_visible = visible;
}

int Grid::Spacing() const{
  return m_spacing;
}

bool Grid::Visible() const{
  return m_visible;
}

