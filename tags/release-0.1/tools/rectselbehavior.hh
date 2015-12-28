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

#ifndef FAINT_RECTANGLESELECT_HH
#define FAINT_RECTANGLESELECT_HH
#include "toolbehavior.hh"
#include "settingid.hh"
#include "bitmap/bitmap.h"
#include "rectseldragimpl.hh"
FaintSettings SettingsEraseRect( const faint::Color& );
class RectSelDragBehavior;

// The behavior for selecting and dragging rectangular raster areas
class RectangleSelectBehavior : public ToolBehavior {
public:
  RectangleSelectBehavior();
  ~RectangleSelectBehavior();
  bool DrawBeforeZoom(Layer) const;

  void StartDrag( const faint::Bitmap&, const IntPoint& topLeft );
  void StartDrag();
  ToolRefresh LeftDown( const CursorPositionInfo&, int modifiers );
  ToolRefresh LeftUp( const CursorPositionInfo&, int modifiers );
  ToolRefresh Motion( const CursorPositionInfo&, int modifiers );
  ToolRefresh Preempt();
  bool Draw( FaintDC&, Overlays&, const Point& );
  IntRect GetRefreshRect( const IntRect& visible, const Point& currPos );
  Command* GetCommand();
  void InitializeFromPaste( const faint::Bitmap& bitmap, const IntPoint& topLeft );
  int GetCursor( const CursorPositionInfo& );
  bool CopyData( faint::Bitmap& );
  ToolRefresh Delete();
  ToolRefresh Deselect();
  bool HasSelection() const;
  bool HasBitmap() const;
  faint::Bitmap* GetBitmap();
  void UpdateBitmap();
  unsigned int GetStatusFieldCount();
  template< typename T >
  void NotifySetting( const T&, const typename T::ValueType& ){
    if ( m_dragBehavior != 0 ){
      bool transparent = m_settings.Get( ts_Transparency ) == TRANSPARENT_BG;
      m_dragBehavior->UpdateMask( transparent, m_settings.Get( ts_BgCol ) );
    }
  }
private:
  enum Status { RSEL_NONE, RSEL_SELECTING, RSEL_DRAGGING, RSEL_SELECTING_DRAG_COMMIT_PENDING };
  ToolRefresh InitDragBehavior( const CursorPositionInfo&, int );
  ToolRefresh InitSelecting( const CursorPositionInfo& );
  ToolRefresh UpdateDragStatus( const CursorPositionInfo&, int );
  ToolRefresh UpdateTransparency( const Point& );
  ToolRefresh HandleRightClick( const CursorPositionInfo&, int );
  ToolRefresh PreemptedDrag();    
  RectSelDragBehavior* m_dragBehavior;
  Status m_status;
  // Two points defining the selection rectangle
  Point m_p1;
  Point m_p2;  
  faint::coord m_maxDistance;
  PendingCommand m_command;
  bool m_transparent;
  TargettedNotifier< RectangleSelectBehavior > m_notifier;
};

#endif
