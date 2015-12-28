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

#include <cassert>
#include "zoomlevel.hh"
const int positiveZoom[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40 };
int maxZoom = sizeof( positiveZoom ) / sizeof(int);
const faint::coord negativeZoom[] = {
  LITCRD(0.9),
  LITCRD(0.8),
  LITCRD(0.7),
  LITCRD(0.6),
  LITCRD(0.5),
  LITCRD(0.4),
  LITCRD(0.3),
  LITCRD(0.2),
  LITCRD(0.1)
};
const int negativeZoomPct[] = {90, 80, 70, 60, 50, 40, 30, 20, 10 };
int minZoom = - (int(( sizeof( negativeZoom ) / sizeof(faint::coord) ))) - 1;

ZoomLevel::ZoomLevel( int level ){
  SetLevel( level );
}

void ZoomLevel::SetLevel( int level ){
  assert( level > minZoom );
  assert( level < maxZoom );
  m_zoomLevel = level;
}

int ZoomLevel::GetLevel() const {
  return m_zoomLevel;
}

bool ZoomLevel::Change( ZoomLevel::ChangeType type ){
  if ( type == PREVIOUS ){
    return Prev();
  }
  else if ( type == NEXT  ){
    return Next();
  }
  else if ( type == DEFAULT ){
    if ( m_zoomLevel == defaultLevel ){
      return false;
    }
    SetLevel(defaultLevel);
    return true;
  }
  assert( false );
  return false;
}

bool ZoomLevel::Next(){
  if ( m_zoomLevel == maxZoom -1 ){
    return false;
  }
  m_zoomLevel+= 1;
  return true;
}

bool ZoomLevel::Prev(){
  if ( m_zoomLevel == minZoom + 1 ){
    return false;
  }
  m_zoomLevel -= 1;
  return false;
}

bool ZoomLevel::AtMax() const{
  return m_zoomLevel == maxZoom -1;
}

bool ZoomLevel::AtMin() const{
  return m_zoomLevel == minZoom + 1;
}

bool ZoomLevel::At100() const{
  return m_zoomLevel == defaultLevel;
}

faint::coord ZoomLevel::GetScaleFactor() const{
  if ( m_zoomLevel >= 0 ){
    return static_cast<faint::coord>(positiveZoom[ m_zoomLevel ] );
  }
  return negativeZoom[ -m_zoomLevel - 1 ];
}

int ZoomLevel::GetPercentage() const{
  if ( m_zoomLevel >= 0 ){
    return positiveZoom[ m_zoomLevel ] * 100;
  }
  return negativeZoomPct[-m_zoomLevel - 1];
}
