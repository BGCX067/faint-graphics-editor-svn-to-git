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

#ifndef FAINT_RECTSELDRAGIMPL_HH
#define FAINT_RECTSELDRAGIMPL_HH

FaintSettings SettingsEraseRect( const faint::Color& );

// Status flags for the RectSelDragBehavior
enum DragStatus {
  IS_DRAGGING,
  PENDING_FURTHER_DRAG, // The dragging is not active but also not
                        // commited. The selection might either be
                        // moved more, or comitted in the current
                        // position

  CURRENT_DRAG_DONE,    // One drag is completed and should be
                        // commited, but the DragBehavior is not
                        // done. (i.e. a clone operation)

  DONE_DRAGGING         // The drag is complete and ready to be
                        // committed
};

// Helper for RectSelBehavior while dragging. Note that it does not
// inherit from ToolBehavior.
class RectSelDragBehavior {
public:
  DragStatus LeftDown( const Point&, int modifiers );
  DragStatus LeftUp( const Point&, int modifiers );
  ToolRefresh Motion( const Point&, int modifiers );
  // Returns true if the drag behavior wishes to commit.
  bool Preempt();
  bool Draw( FaintDC&, Overlays&, const Point& currPos );

  IntRect GetRefreshRect();
  const Rect GetRect();
  Command* GetCommand();
  DragStatus GetStatus();
  void UpdateMask( bool transparent, const faint::Color& bgColor );
  bool CopyData( faint::Bitmap& );
  Command* Delete();
  faint::Bitmap* GetBitmap();
  void UpdateBitmap();

private:
  friend class RectangleSelectBehavior;
  RectSelDragBehavior( const IntRect&, const faint::Bitmap&, const IntPoint& offset, bool dragging=true );
  DragStatus m_status;
  faint::Bitmap m_bitmap;
  IntRect m_rect;
  IntRect m_originalRect;
  IntPoint m_lastOrigin;

  // Offset for the click-position
  IntPoint m_offset;

  IntPoint m_lastPos;
  bool m_transparent;
  faint::Color m_bgColor;
  bool m_copying;
  ConstrainDir m_constrainDir;
  unsigned int m_maxConstrainDist;
  PendingCommand m_command;
};

#endif
