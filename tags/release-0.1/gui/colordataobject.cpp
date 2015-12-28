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

#include "wx/window.h"
#include "colordataobject.hh"
#include "util/formatting.hh"
wxDragResult ColorDataObject::m_dragResult = wxDragNone;

struct ColorStruct{
  int r;
  int g;
  int b;
  int a;
};

ColorDataObject::ColorDataObject( const faint::Color& color )
  : m_color( color )
{
  m_dragResult = wxDragNone;
}

faint::Color ColorDataObject::GetColor(){
  return m_color;
}

bool ColorDataObject::IsSupported( const wxDataFormat& format, Direction ) const{
  if ( format == wxDataFormat("FaintColor" ) ){
    return true;
  }
  else if ( format == wxDataFormat( wxDF_TEXT ) ){
    return true;
  }
  return false;
}

void ColorDataObject::SetOperationType( wxDragResult result ){
  m_dragResult = result;
}

wxDragResult ColorDataObject::GetOperationType() const{
  return m_dragResult;
}

wxDataFormat ColorDataObject::GetPreferredFormat( Direction ) const {
  return wxDataFormat("FaintColor");
}

void ColorDataObject::GetAllFormats( wxDataFormat* formats, Direction) const{
  formats[0] = wxDataFormat("FaintColor");
  formats[1] = wxDataFormat(wxDF_TEXT);
}

size_t ColorDataObject::GetFormatCount( Direction ) const{
  return 2;
}

size_t ColorDataObject::GetDataSize( const wxDataFormat& f ) const{
  if ( f == wxDataFormat("FaintColor") ){
    return sizeof( ColorStruct );
  }
  else if ( f == wxDataFormat( wxDF_TEXT ) ){
    wxString dragText( StrSmartRGBA( m_color ) );
    wxTextDataObject textObj( dragText );
    return textObj.GetDataSize();
  }
  return 0;
}

bool ColorDataObject::GetDataHere( const wxDataFormat& format, void *buf ) const{
  if ( format == wxDataFormat("FaintColor") ){
    ColorStruct c = {m_color.r, m_color.g, m_color.b, m_color.a };
    memcpy( buf, &c, sizeof(c) );
    return true;
  }
  else if ( format == wxDataFormat( wxDF_TEXT ) ){
    wxString dragText(StrSmartRGBA(m_color));
    wxTextDataObject textObj( dragText );
    textObj.GetDataHere( format, buf );
    return true;
  }
  return false;
}

bool ColorDataObject::SetData( const wxDataFormat& format, size_t len, const void* buf ){
  if ( format == wxDataFormat("FaintColor") && len == sizeof(ColorStruct) ){
    ColorStruct c;
    memcpy( &c, buf, len );
    m_color = faint::ColorFromInts( c.r, c.g, c.b, c.a );
    return true;
  }
  return false;
}

class ColorDropTargetImpl : public wxDropTarget {
public:
  ColorDropTargetImpl( ColorDropTarget* target );
  bool OnDrop( wxCoord x, wxCoord y );
  virtual wxDragResult OnData( wxCoord x, wxCoord y, wxDragResult def );

private:
  ColorDataObject* colorObj;
  ColorDropTarget* m_target;
};

ColorDropTargetImpl::ColorDropTargetImpl( ColorDropTarget* targetWindow )
{
  m_target = targetWindow;
  colorObj = new ColorDataObject( faint::Color(0,0,0) );
  SetDataObject( colorObj );
}

wxDragResult ColorDropTargetImpl::OnData( wxCoord x, wxCoord y, wxDragResult /*def*/ ){
  // Copy the data into our data object
  GetData();

  wxDragResult result = m_target->OnDropColor( wxPoint(x, y), colorObj->GetColor() );
  colorObj->SetOperationType( result );
  return result;
}

bool ColorDropTargetImpl::OnDrop( wxCoord, wxCoord ){
  return true;
}

ColorDropTarget::ColorDropTarget( wxWindow* targetWindow ){
  ColorDropTargetImpl* impl = new ColorDropTargetImpl( this );
  targetWindow->SetDropTarget( impl );
}
