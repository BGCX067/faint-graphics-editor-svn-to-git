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

Grid::Grid(bool enabled, bool visible, int spacing, const faint::Color color)
  : m_enabled( enabled ),
    m_visible( visible ),
    m_spacing(spacing),
    m_color(color)
{}

bool Grid::Visible() const{
  return m_visible;
}

bool Grid::Enabled() const{
  return m_enabled;
}

int Grid::Spacing() const{
  return m_spacing;
}

void Grid::SetVisible( bool visible ){
  m_visible = visible;
}

void Grid::SetEnabled( bool enabled ){
  m_enabled = enabled;
}

void Grid::SetSpacing( int spacing ){
  m_spacing = spacing;
}

const faint::Color& Grid::Color() const{
  return m_color;
}

faint::Color DefaultGridColor(){
  return faint::Color(217, 217, 240);
}
