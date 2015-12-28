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

const int g_defaultLevel=0;

const int positiveZoom[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40 };
int maxZoom = sizeof( positiveZoom ) / sizeof(int); // Fixme: maxZoom implies valid, max is maxZoom -1
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

int find_closest( faint::coord scale ){
  if ( scale < 1.0 ){
    for ( int i = 0; i != 9; i++ ){
      if ( scale > negativeZoom[i] ){
	return(-i - 1);
      }
    }
    return minZoom + 1;
  }

  for ( int i = 0; i != maxZoom; i++ ){
    if ( scale < positiveZoom[i] ){
      return i - 1;
    }
  }
  return maxZoom - 1;
}

ZoomLevel::ZoomLevel(){
  SetLevel(g_defaultLevel);
}

bool ZoomLevel::At100() const{
  return !m_scale.IsSet() && m_zoomLevel == g_defaultLevel;
}

bool ZoomLevel::AtMax() const{
  if ( m_scale.IsSet() ){
    return find_closest(m_scale.Get()) == maxZoom - 1;
  }
  return m_zoomLevel == maxZoom -1;
}

bool ZoomLevel::AtMin() const{
  if ( m_scale.IsSet() ){
    return find_closest(m_scale.Get()) == minZoom + 1;
  }
  return m_zoomLevel == minZoom + 1;
}

bool ZoomLevel::Change( ZoomLevel::ChangeType type ){
  if ( type == PREVIOUS ){
    return Prev();
  }
  else if ( type == NEXT  ){
    return Next();
  }
  else if ( type == DEFAULT ){
    if ( m_zoomLevel == g_defaultLevel && !m_scale.IsSet() ){
      return false;
    }
    SetLevel(g_defaultLevel);
    return true;
  }
  assert( false );
  return false;
}

int ZoomLevel::GetPercentage() const{
  if ( m_scale.IsSet() ){
    return static_cast<int>(m_scale.Get() * 100);
  }
  if ( m_zoomLevel >= 0 ){
    return positiveZoom[ m_zoomLevel ] * 100;
  }
  return negativeZoomPct[-m_zoomLevel - 1];
}

faint::coord ZoomLevel::GetScaleFactor() const{
  if ( m_scale.IsSet() ){
    return m_scale.Get();
  }
  if ( m_zoomLevel >= 0 ){
    return static_cast<faint::coord>(positiveZoom[ m_zoomLevel ] );
  }
  return negativeZoom[ -m_zoomLevel - 1 ];
}

bool ZoomLevel::Next(){
  if ( m_scale.IsSet() ){
    int level = find_closest(m_scale.Get());
    m_zoomLevel = level;
    m_scale.Clear();
  }

  if ( m_zoomLevel == maxZoom -1 ){
    return false;
  }
  m_zoomLevel+= 1;
  return true;
}

bool ZoomLevel::Prev(){
  if ( m_scale.IsSet() ){
    int level = find_closest(m_scale.Get());
    m_zoomLevel = level;
    m_scale.Clear();
  }

  if ( m_zoomLevel == minZoom + 1 ){
    return false;
  }
  m_zoomLevel -= 1;
  return false;
}

void ZoomLevel::SetApproximate( faint::coord scale ){
  if ( scale < 1.0 ){
    m_scale.Set(scale);
    return;
  }
  for ( int i = 0; i != maxZoom; i++ ){
    if ( scale < positiveZoom[i] ){
      SetLevel(i - 1);
      return;
    }
  }
  SetLevel(maxZoom - 1);
}

void ZoomLevel::SetLevel( int level ){
  assert( level > minZoom );
  assert( level < maxZoom );
  m_scale.Clear();
  m_zoomLevel = level;
}
