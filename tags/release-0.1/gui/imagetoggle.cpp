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
ImageToggleCtrl::ImageToggleCtrl( wxWindow* parent, const IntSetting& setting, wxSize bitmapSize, wxOrientation dir, unsigned int hSpace, unsigned int vSpace)
  : IntSettingCtrl(parent, setting)    
{
  bitmapList = new BitmapListCtrl( this, bitmapSize, dir, hSpace, vSpace);  
  SetWindowStyleFlag( wxBORDER_SIMPLE );
}

void ImageToggleCtrl::AddButton( wxBitmap* bmp, int value_){
  int index = bitmapList->Add( bmp );
  indexToValue[ index ] = value_;
  valueToIndex[ value_ ] = index;
  SetClientSize( bitmapList->GetSize() );
  SetMinSize( bitmapList->GetSize() );
}

int ImageToggleCtrl::GetValue() const{
  int index = bitmapList->GetSelection();
  std::map<int,int>::const_iterator it = indexToValue.find( index );
  return it->second;
}

void ImageToggleCtrl::SetValue( int value ){
  int index = valueToIndex[ value ];
  bitmapList->SetSelection(index);
}

void ImageToggleCtrl::OnSelection( wxCommandEvent& ){
  SendChangeEvent();
}

BEGIN_EVENT_TABLE( ImageToggleCtrl, ToolSettingCtrl )
EVT_COMMAND(-1, EVT_BITMAPLIST_SELECTION, ImageToggleCtrl::OnSelection )
END_EVENT_TABLE()

