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
#include "bitmap/bitmap.hh"
#include "commands/move-content-cmd.hh"
#include "geo/tri.hh"
#include "rendering/cairocontext.hh"
#include "rendering/faintdc.hh"
#include "rendering/overlay.hh"
#include "util/rasterselection.hh"
#include "util/settingutil.hh"

RasterSelectionOptions::RasterSelectionOptions( bool in_mask, const faint::Color& in_bgCol, bool in_alpha )
  : alpha(in_alpha),
    bgCol(in_bgCol),
    mask(in_mask)
{}

// A floating pasted bitmap
RasterSelectionState::RasterSelectionState(const faint::Bitmap& bmp, const IntPoint& topLeft )
  : copy(true),
    floating(true),
    floatingBmp(bmp),
    oldRect(IntRect()),
    rect(IntRect(topLeft, bmp.GetSize()))
{}

RasterSelectionState::RasterSelectionState(const IntRect& r )
  : copy(false),
    floating(false),
    oldRect(IntRect()),
    rect(r)
{}

RasterSelectionState::RasterSelectionState( const faint::Bitmap& bmp, const IntPoint& topLeft, const IntRect& in_oldRect )
  : copy(false),
    floating(true),
    floatingBmp(bmp),
    oldRect(in_oldRect),
    rect(IntRect(topLeft, bmp.GetSize()))
{}

RasterSelectionState& RasterSelectionState::operator=(const RasterSelectionState& state){
  copy = state.copy;
  floating = state.floating;
  floatingBmp = state.floatingBmp;
  oldRect = state.oldRect;
  rect = state.rect;
  return *this;
}

RasterSelection::RasterSelection()
  : m_options(false, faint::Color(255,255,255), false)
{}

void RasterSelection::BeginFloat(const faint::Bitmap& src, const copy_selected& copy ){
  assert(!Empty());
  if ( m_state.floating ){
    if ( copy.Get() ){
      m_state.copy = true;
    }
    return;
  }
  assert(faint::inside(m_state.rect, src));
  m_state.copy = copy.Get();
  m_state.floating = true;
  m_state.oldRect = m_state.rect;
  SetFloatingBitmap(cairo_compatible_sub_bitmap(src, m_state.rect), m_state.rect.TopLeft());
}

void RasterSelection::Clip(const IntRect& clipRegion){
  if ( m_state.floating || Empty() ){
    return;
  }
  m_state.rect = intersection(m_state.rect, clipRegion);
}

bool RasterSelection::Copying() const{
  return m_state.copy;
}

bool RasterSelection::Contains( const IntPoint& pos ){
  return Exists() ? m_state.rect.Contains(pos) : false;
}

void RasterSelection::Deselect(){
  m_state.copy = false;
  m_state.floating = false;
  m_state.floatingBmp = faint::Bitmap();
  m_state.oldRect = IntRect();
  m_state.rect = IntRect();
}

void RasterSelection::DrawFloating(FaintDC& dc) const{
  if ( Empty() || !m_state.floating ){
    return;
  }
  if ( !m_state.copy ){
    // Clear the source region with the background color
    dc.Rectangle( tri_from_rect(floated(m_state.oldRect)),
      eraser_rectangle_settings(m_options.bgCol));
  }

  dc.Bitmap( m_state.floatingBmp, floated(m_state.rect.TopLeft()), bitmap_mask_settings(m_options.mask, m_options.bgCol, m_options.alpha) );
}

void RasterSelection::Draw(FaintDC& dc, Overlays& overlays) const{
  if ( Empty() ){
    return;
  }
  DrawFloating(dc);
  overlays.Rectangle(floated(m_state.rect));
}

void RasterSelection::DrawOverlay( Overlays& overlays ) const{
  if ( Empty() ){
    return;
  }
  overlays.Rectangle(floated(m_state.rect));
}

bool RasterSelection::Empty() const{
  return empty(m_state.rect);
}

bool RasterSelection::Exists() const{
  return !empty(m_state.rect);
}

bool RasterSelection::Floating() const{
  return m_state.floating;
}

const faint::Bitmap& RasterSelection::GetBitmap() const{
  assert(Floating());
  return m_state.floatingBmp;
}

const faint::Color& RasterSelection::GetBgCol() const{
  return m_options.bgCol;
}

IntRect RasterSelection::GetRect() const{
  //assert(!Empty()); // Fixme: Enable
  return m_state.rect;
}

IntRect RasterSelection::GetOldRect() const{
  assert(!m_state.copy);
  return m_state.oldRect;
}

RasterSelectionOptions RasterSelection::GetOptions() const{
  return m_options;
}

IntSize RasterSelection::GetSize() const{
  return m_state.rect.GetSize();
}

const RasterSelectionState& RasterSelection::GetState() const{
  return m_state;
}

void RasterSelection::Move( const IntPoint& topLeft ){
  m_state.rect.x = topLeft.x;
  m_state.rect.y = topLeft.y;
}

void RasterSelection::Paste(const faint::Bitmap& bmp, const IntPoint& topLeft){
  m_state.copy = true; // Don't erase anything
  m_state.floating = true;
  m_state.floatingBmp = bmp;
  m_state.oldRect = IntRect();
  m_state.rect = IntRect(topLeft, bmp.GetSize());
}

void RasterSelection::SetAlphaBlending( bool alpha ){
  m_options.alpha = alpha;
}

void RasterSelection::SetBackground( const faint::Color& color ){
  m_options.bgCol = color;
}

void RasterSelection::SetFloatingBitmap( const faint::Bitmap& bmp, const IntPoint& topLeft ){
  assert(m_state.floating);
  m_state.floatingBmp = bmp;
  m_state.rect = IntRect(topLeft, bmp.GetSize());
}

void RasterSelection::SetMask( bool enable ){
  m_options.mask = enable;
}

void RasterSelection::SetOptions( const RasterSelectionOptions& options ){
  m_options = options;
}

void RasterSelection::SetRect(const IntRect& rect){
  if ( m_state.floating ){
    return; // Fixme
  }
  m_state.copy = false;
  m_state.floating = false;
  m_state.floatingBmp = faint::Bitmap();
  m_state.oldRect = IntRect();
  m_state.rect = rect;
}

void RasterSelection::SetState( const RasterSelectionState& state ){
  m_state = state;
}

Command* RasterSelection::StampFloatingSelection() const{
  assert(m_state.floating);

  return  new CmdMoveContent( m_state.floatingBmp,
    New(m_state.rect),
    Old(m_state.oldRect),
    copy_content(m_state.copy),
    bitmap_mask_settings(m_options.mask, m_options.bgCol, m_options.alpha),
    "CommitFloatingSelection" ); // Fixme: Needs a name
}

IntPoint RasterSelection::TopLeft() const{
  assert(!Empty());
  return m_state.rect.TopLeft();
}

void update_mask( bool enable, const faint::Color& bgCol, bool alpha, RasterSelection& selection ){
  selection.SetAlphaBlending(alpha);
  selection.SetMask(enable);
  selection.SetBackground(bgCol);
}

RasterSelectionOptions raster_selection_options( const Settings& s ){
  faint::Color bgCol = s.Get(ts_BgCol);
  bool mask = s.Get(ts_BackgroundStyle) == BackgroundStyle::MASKED;
  bool alpha = s.Get(ts_AlphaBlending);
  return RasterSelectionOptions(mask, bgCol, alpha);
}
