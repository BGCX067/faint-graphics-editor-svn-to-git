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
#include "gui/bitmapdataobject.hh"
#include "gui/objectdataobject.hh"
#include "objects/object.hh"
#include "util/clipboard.hh"
#include "util/color.hh"
#include "util/convertwx.hh"

namespace faint {

Clipboard::Clipboard(){
  m_ok = wxTheClipboard->Open();
}

Clipboard::~Clipboard(){
  if ( m_ok ){
    wxTheClipboard->Close();
  }
}

void Clipboard::Flush(){
  if ( wxTheClipboard->Open() ){
    wxTheClipboard->Flush();
    wxTheClipboard->Close();
  }
}

bool Clipboard::GetBitmap( faint::Bitmap& dest ){
  assert(m_ok);
  wxDataObjectComposite composite;
  FaintBitmapDataObject* bmpObject_faint = new FaintBitmapDataObject;
  wxBitmapDataObject* bmpObject_wx = new wxBitmapDataObject;
  composite.Add( bmpObject_faint, true );
  composite.Add( bmpObject_wx, false );

  if ( wxTheClipboard->GetData( composite ) ){
    wxDataFormat format = composite.GetReceivedFormat();
    if ( format == wxDataFormat("FaintBitmap") ){
      dest = bmpObject_faint->GetBitmap();
      return true;
    }
    else {
      wxBitmap wxBmp = bmpObject_wx->GetBitmap();
      dest = to_faint( clean_bitmap( wxBmp ) );
      return true;
    }
  }
  return false;
}

bool Clipboard::GetObjects( objects_t& objects ){
  assert(m_ok);
  faint::ObjectDataObject objectData;
  if ( !wxTheClipboard->GetData( objectData ) ){
    return false;
  }
  objects = objectData.GetObjects();
  return true;
}

bool Clipboard::GetText( faint::utf8_string& dest ){
  assert(m_ok);
  wxTextDataObject textObject;
  if ( !wxTheClipboard->GetData( textObject ) ){
    return false;
  }
  dest = to_faint(textObject.GetText());
  return true;
}

bool Clipboard::Good() const{
  return m_ok;
}

void Clipboard::SetBitmap( const faint::Bitmap& bmp, const faint::ColRGB& bgCol ){
  assert(m_ok);
  wxDataObjectComposite* composite = new wxDataObjectComposite;
  composite->Add( new faint::FaintBitmapDataObject( bmp ), true );
  composite->Add( new wxBitmapDataObject( to_wx_bmp( faint::alpha_blended( bmp, bgCol ) ) ) );
  wxTheClipboard->SetData( composite );
}

void Clipboard::SetBitmap( const faint::Bitmap& bmp ){
  assert(m_ok);
  SetBitmap(bmp, faint::rgb_black());
}

void Clipboard::SetObjects( const objects_t& objects ){
  assert(m_ok);
  wxTheClipboard->SetData( new faint::ObjectDataObject( objects ) );
}

void Clipboard::SetText( const faint::utf8_string& text ){
  assert(m_ok);
  wxTheClipboard->SetData( new wxTextDataObject(to_wx(text)) );
}

} // Namespace
