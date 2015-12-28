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

#include "imagetoggle.hh"
#include "bitmaplist.hh"
#include "util/formatting.hh"
#include "util/guiutil.hh"
#include "util/statusinterface.hh"
#include "util/settingid.hh"

ImageToggleCtrl::ImageToggleCtrl( wxWindow* parent, const IntSetting& setting, const wxSize& bitmapSize, StatusInterface& status, wxOrientation dir, unsigned int hSpace, unsigned int vSpace)
  : IntSettingCtrl(parent, setting)
{
  m_bitmapList = new BitmapListCtrl( this, bitmapSize, status, dir, hSpace, vSpace);
  SetWindowStyleFlag( wxBORDER_SIMPLE );
}

void ImageToggleCtrl::AddButton( const wxBitmap& bmp, int value, const std::string& statusText){
  int index = m_bitmapList->Add( bmp, statusText );
  m_indexToValue[ index ] = value;
  m_valueToIndex[ value ] = index;
  SetClientSize( m_bitmapList->GetSize() );
  SetMinSize( m_bitmapList->GetSize() );
}

void ImageToggleCtrl::DoSetToolTip( wxToolTip* toolTip){
  m_bitmapList->SetToolTip(toolTip);
}

int ImageToggleCtrl::GetValue() const{
  int index = m_bitmapList->GetSelection();
  std::map<int,int>::const_iterator it = m_indexToValue.find( index );
  return it->second;
}

void ImageToggleCtrl::OnSelection( wxCommandEvent& ){
  SendChangeEvent();
}

void ImageToggleCtrl::SetValue( int value ){
  int index = m_valueToIndex[ value ];
  m_bitmapList->SetSelection(index);
}

BEGIN_EVENT_TABLE( ImageToggleCtrl, IntSettingCtrl )
EVT_COMMAND(-1, EVT_BITMAPLIST_SELECTION, ImageToggleCtrl::OnSelection )
END_EVENT_TABLE()


void update_status( StatusInterface& status, const BoolSetting& s, bool value){
  status.SetMainText(space_sep( value ? "Disable" : "Enable",
      setting_name(untyped(s))));
}

BoolImageToggle::BoolImageToggle( wxWindow* parent, const BoolSetting& s, const wxBitmap& bmp, StatusInterface& status )
: BoolSettingControl(parent, s),
  m_bitmap(bmp),
  m_value(false),
  m_status(status)
{
  SetInitialSize(wxSize(50,28));
  SetWindowStyleFlag( wxBORDER_SIMPLE );
}

bool BoolImageToggle::GetValue() const{
  return m_value;
}

void BoolImageToggle::OnLeaveWindow( wxMouseEvent& event ){
  m_status.SetMainText("");
  event.Skip();
}

void BoolImageToggle::OnLeftDown( wxMouseEvent& ){
  m_value = !m_value;
  update_status(m_status, GetSetting(), m_value);
  SendChangeEvent();
  Refresh();
}

void BoolImageToggle::OnMotion( wxMouseEvent& event ){ 
  update_status(m_status, GetSetting(), m_value);  
  event.Skip();
}

void BoolImageToggle::OnPaint( wxPaintEvent& ){
  wxPaintDC dc(this);
  wxColour deselectedColor(faint::get_gui_deselected_color());
  wxColour selectedColor(faint::get_gui_selected_color());
  if ( m_value ){
    dc.SetPen(wxPen(selectedColor));
    dc.SetBrush(wxBrush(selectedColor));
  }
  else {
    dc.SetPen(wxPen(deselectedColor));
    dc.SetBrush(wxBrush(deselectedColor));
  }
  dc.DrawRectangle(0,0, 50, 28 );
  dc.DrawBitmap( m_bitmap, 12, 2);
}

void BoolImageToggle::SetValue(bool value){
  m_value = value;
}

BEGIN_EVENT_TABLE(BoolImageToggle,BoolSettingControl)
EVT_LEFT_DOWN(BoolImageToggle::OnLeftDown)
EVT_LEFT_DCLICK(BoolImageToggle::OnLeftDown)
EVT_LEAVE_WINDOW(BoolImageToggle::OnLeaveWindow)
EVT_MOTION(BoolImageToggle::OnMotion)
EVT_PAINT(BoolImageToggle::OnPaint)
END_EVENT_TABLE()
