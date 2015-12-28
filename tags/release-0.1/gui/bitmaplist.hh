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

#ifndef FAINT_BITMAPLIST_HH
#define FAINT_BITMAPLIST_HH
#include <vector>
#include "wx/panel.h"
#include "wx/event.h"

DECLARE_EVENT_TYPE(EVT_BITMAPLIST_SELECTION, -1)

// A graphical list box choice control
class BitmapListCtrl : public wxPanel {
public:
  BitmapListCtrl( wxWindow* parent, const wxSize& bitmapSize, int orientation=wxVERTICAL, int hSpace=5, int vSpace=5, bool inheritBitmaps=false );
  ~BitmapListCtrl();
  int Add( wxBitmap* );
  void SetSelection( unsigned int );
  int GetSelection() const;
  void OnPaint( wxPaintEvent& );
  void OnLeftDown( wxMouseEvent& );
private:
  unsigned int GetClickedIndex( const wxPoint& );
  const wxSize m_imageSize;
  const int m_vSpace;
  const int m_hSpace;
  const int m_orientation;
  const bool m_inherit;
  int m_selection;
  std::vector<wxBitmap*> m_images;
  wxRect m_selectionRect;
  DECLARE_EVENT_TABLE()
};

#endif
