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

#include "bitmaplist.hh"
#include "wx/bitmap.h"
#include "wx/dcclient.h"
#include "wx/dcbuffer.h"
DEFINE_EVENT_TYPE(EVT_BITMAPLIST_SELECTION)

BitmapListCtrl::BitmapListCtrl( wxWindow* parent, const wxSize& bitmapSize, int orientation, int hSpace, int vSpace, bool inheritBitmaps )
: wxPanel( parent ),
  m_imageSize( bitmapSize ),
  m_vSpace(vSpace),
  m_hSpace(hSpace),
  m_orientation( orientation ),
  m_inherit( inheritBitmaps ),
  m_selection(-1),
  m_selectionRect( 0,0, bitmapSize.GetWidth() + 2 * hSpace, bitmapSize.GetHeight() + 2 * vSpace )
{

  SetBackgroundStyle( wxBG_STYLE_PAINT );
  if ( m_orientation == wxVERTICAL ){
    SetSize( bitmapSize.GetWidth() + 2 * m_hSpace, m_vSpace );
  }
  else if ( orientation == wxHORIZONTAL ){
    SetSize( m_hSpace, bitmapSize.GetHeight() + 2 * m_vSpace + 2 );
  }
  else {
    assert( false );
  }
}

BitmapListCtrl::~BitmapListCtrl(){
  if ( !m_inherit ){
    return;
  }

  for ( unsigned int i = 0; i!= m_images.size(); i++ ){
    delete m_images[i];
  }
}

int BitmapListCtrl::Add( wxBitmap* bmp ){
  assert( bmp->GetWidth() == m_imageSize.GetWidth() );
  assert( bmp->GetHeight() == m_imageSize.GetHeight() );

  m_images.push_back( bmp );
  wxSize size = GetSize();
  int width = size.GetWidth();
  int height = size.GetHeight();
  if ( m_orientation == wxVERTICAL ){
    SetSize( width, m_images.size() * ( 2 * m_vSpace + m_imageSize.GetHeight() )
      + 2 // Border
    );
  }
  else{
    SetSize( m_images.size() * ( 2 * m_hSpace + m_imageSize.GetWidth() )
      + 2,  // Border
      height );
  }
  if ( m_selection == -1 ){
    m_selection = 0;
  }
  return m_images.size() - 1;
}

void BitmapListCtrl::OnPaint( wxPaintEvent& ){
  wxAutoBufferedPaintDC paintDC(this);
  paintDC.SetBackground( wxBrush( wxColour(255,255,255)) );
  paintDC.Clear();
  if ( m_orientation == wxVERTICAL ){
    int yOffset = m_vSpace;
    int imageHeight = m_imageSize.GetHeight() + m_vSpace + m_vSpace;

    if ( m_selection != -1 ){
      wxPen tPen( wxColour(0, 0, 0) );
      tPen.SetStyle(wxTRANSPARENT);
      paintDC.SetPen( tPen );
      paintDC.SetBrush( wxBrush(wxColour(112, 154, 209) ) );
      wxRect currentSel( m_selectionRect );
      currentSel.Offset( 0, m_selection * imageHeight );
      paintDC.DrawRectangle( currentSel );
    }

    for ( std::vector<wxBitmap*>::iterator it = m_images.begin();
          it != m_images.end();
          ++it, yOffset += imageHeight ){
      paintDC.DrawBitmap ( **it, m_hSpace, yOffset );
    }
  }
  else {
    int xOffset = m_hSpace;
    int imageWidth = m_imageSize.GetWidth() + 2 * m_hSpace;

    if ( m_selection != -1 ){
      wxPen tPen( wxColour(0, 0, 0) );
      tPen.SetStyle(wxTRANSPARENT);
      paintDC.SetPen( tPen );
      paintDC.SetBrush( wxBrush(wxColour(112, 154, 209) ) );
      wxRect currentSel( m_selectionRect );
      currentSel.Offset( m_selection * imageWidth, 0 );
      paintDC.DrawRectangle( currentSel );
    }

    for ( std::vector<wxBitmap*>::iterator it = m_images.begin();
          it != m_images.end();
          ++it, xOffset += imageWidth ){
      paintDC.DrawBitmap( **it, xOffset, m_vSpace );
    }
  }
}

void BitmapListCtrl::SetSelection( unsigned int selection ){
  assert ( selection < m_images.size() );
  m_selection = selection;
  Refresh();
}

int BitmapListCtrl::GetSelection() const {
  return m_selection;
}

unsigned int BitmapListCtrl::GetClickedIndex( const wxPoint& pos ){
  assert( !m_images.empty() );
  int maxIndex = static_cast<int>(m_images.size()) - 1;
  if ( m_orientation == wxVERTICAL ){
    int imageHeight = m_imageSize.GetHeight() + 2 * m_vSpace;
    return std::min( pos.y / imageHeight, maxIndex );
  }
  else {
    int imageWidth = m_imageSize.GetWidth() + 2 * m_hSpace;
    return std::min( pos.x / imageWidth, maxIndex );
  }
}

void BitmapListCtrl::OnLeftDown( wxMouseEvent& event ){
  int oldSelection = m_selection;
  int selected = GetClickedIndex( event.GetPosition() );

  SetSelection( selected );
  if ( oldSelection != m_selection ){
    wxCommandEvent event( EVT_BITMAPLIST_SELECTION, GetId() );
    event.SetEventObject( this );
    GetEventHandler()->ProcessEvent( event );
  }
}

BEGIN_EVENT_TABLE( BitmapListCtrl, wxPanel )
EVT_PAINT( BitmapListCtrl::OnPaint )
EVT_LEFT_DOWN( BitmapListCtrl::OnLeftDown)
END_EVENT_TABLE()
