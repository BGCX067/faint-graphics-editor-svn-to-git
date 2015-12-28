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

#include "wx/menu.h" // Fixme: Remove wx-depency somehow
#include "menupredicate.hh"
#include "util/flag.hh"

namespace faint{
  BoundMenuPred::BoundMenuPred( int id, int predicate )
    : m_id(id),
      m_pred(predicate)
  {}

  void BoundMenuPred::Update( wxMenuBar* menu, bool rasterSelection, bool objectSelection, bool hasObjects ) const{
    bool enable = ( rasterSelection && fl(MenuPred::RASTER_SELECTION, m_pred) ) ||
      ( objectSelection && fl(MenuPred::OBJECT_SELECTION, m_pred) ) ||
      ( hasObjects && fl(MenuPred::HAS_OBJECTS, m_pred ) );
    menu->Enable( m_id, enable );
  }

  MenuPred::MenuPred( int predicate )
    : m_predicate(predicate)
  {}

  BoundMenuPred MenuPred::GetBound( int id ) const{
    return BoundMenuPred( id, m_predicate );
  }
}

