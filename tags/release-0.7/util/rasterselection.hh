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
#ifndef FAINT_RASTERSELECTION_HH
#define FAINT_RASTERSELECTION_HH
#include "bitmap/bitmap.hh"
#include "geo/intrect.hh"
#include "util/commonfwd.hh"
#include "util/unique.hh"

class RasterSelection;
typedef Unique<bool,RasterSelection,0> copy_selected;

class RasterSelectionOptions{
public:
  RasterSelectionOptions( bool mask, const faint::Color&, bool alpha );
  bool alpha;
  faint::Color bgCol;
  bool mask;
};

typedef Order<RasterSelectionOptions>::New NewRasterSelectionOptions;
typedef Order<RasterSelectionOptions>::Old OldRasterSelectionOptions;

class RasterSelectionState{
public:
  // No selection
  RasterSelectionState()
    : copy(false),
      floating(false)
  {}
  // A floating pasted bitmap
  RasterSelectionState(const faint::Bitmap&, const IntPoint&);

  // A non-floating rectangle selection
  RasterSelectionState(const IntRect&);

  // Non-pasted float (leaves a hole)
  RasterSelectionState(const faint::Bitmap&, const IntPoint&, const IntRect& oldRect);
private:
  RasterSelectionState& operator=(const RasterSelectionState&);
  friend class RasterSelection;
  bool copy;
  bool floating;
  IntRect rect;
  IntRect oldRect;
  faint::Bitmap floatingBmp;
};

typedef Order<RasterSelectionState>::New NewRasterSelectionState;
typedef Order<RasterSelectionState>::Old OldRasterSelectionState;

class RasterSelection{
  // A selected region in the image or a floating selection.
  // A  selected region indicates a region on the image, while a
  // floating floating selection is a movable overlay graphic,
  // for example pasted raster graphics.
  //
  // A floating selection can be moved in steps without
  // affecting the image (until the selection is committed).
  // A non-floating selection can become floating, which
  // will leave a hole behind
public:
  friend class RasterSelectionState;
  RasterSelection();
  // Changes the current selection to a floating selection.
  // Must not be Empty().
  // Can be used to re-float while already floating to copy the
  // selection by first creating a command with
  // StampFloatingSelection().
  void BeginFloat(const faint::Bitmap& src, const copy_selected& );

  // Clips a non-floating selection to the specified rectangle
  // Does nothing for floating selections.
  void Clip(const IntRect&);

  bool Copying() const;

  // True if the selection exists and contains the point.
  bool Contains(const IntPoint&);

  // Removes any selection, loses any floating image
  void Deselect();

  // Draws the floating image (if any), does not draw
  // any selection indicators.
  void DrawFloating(FaintDC&) const;

  // Draws the selection, possibly including a floating image and
  // any visible overlays
  void Draw(FaintDC&, Overlays&) const;

  void DrawOverlay(Overlays&) const;

  // True if there's no selected region or floating selection
  bool Empty() const;

  // Opposite of Empty()
  bool Exists() const;
  bool Floating() const;

  // Returns the floating bitmap
  // Must be Floating()
  const faint::Bitmap& GetBitmap() const;
  const faint::Color& GetBgCol() const;
  IntRect GetOldRect() const;
  RasterSelectionOptions GetOptions() const;
  IntRect GetRect() const;
  const RasterSelectionState& GetState() const;
  IntSize GetSize() const;
  void Move( const IntPoint& topLeft );

  // Creates a floating selection from "outside"
  void Paste( const faint::Bitmap&, const IntPoint& topLeft );
  void SetAlphaBlending( bool );
  void SetBackground( const faint::Color& );

  // Changes the floating bitmap.
  // Must be Floating(), unlike Paste(...)
  void SetFloatingBitmap( const faint::Bitmap&, const IntPoint& topLeft );
  void SetMask( bool );

  void SetOptions( const RasterSelectionOptions& );

  // Sets a non-floating selection
  void SetRect( const IntRect& );

  void SetState( const RasterSelectionState& );
  // Returns a command which draws the floating image
  // Must be Floating()
  Command* StampFloatingSelection() const;
  IntPoint TopLeft() const;
private:
  RasterSelectionState m_state;
  RasterSelectionOptions m_options;
};

void update_mask( bool enable, const faint::Color& bgCol, bool alpha, RasterSelection& );
RasterSelectionOptions raster_selection_options( const Settings& );

#endif
