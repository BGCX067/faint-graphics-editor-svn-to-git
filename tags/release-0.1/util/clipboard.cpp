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

#include "clipboard.hh"
#include "gui/bitmapdataobject.hh"
#include "gui/objectdataobject.hh"
#include "objects/object.hh"
#include "util/convertwx.hh"
#include "bitmap.h"
namespace faint {
bool GetBitmap( wxClipboard* clipboard, faint::Bitmap& dest ){
  wxDataObjectComposite composite;

  FaintBitmapDataObject* bmpObject_faint = new FaintBitmapDataObject;
  wxBitmapDataObject* bmpObject_wx = new wxBitmapDataObject;
  composite.Add( bmpObject_faint, true );
  composite.Add( bmpObject_wx, false );

  if ( clipboard->GetData( composite ) ){
    wxDataFormat format = composite.GetReceivedFormat();
    if ( format == wxDataFormat("FaintBitmap") ){
      dest = bmpObject_faint->GetBitmap();
      clipboard->Close();
      return true;
    }
    else {
      wxBitmap wxBmp = bmpObject_wx->GetBitmap();
      dest = to_faint( CleanBitmap( wxBmp ) );
      clipboard->Close();
      return true;
    }
  }
  return false;
}

bool GetObjects( wxClipboard* clipboard, std::vector<Object*>& objects ){
  faint::ObjectDataObject objectData;
  if ( !clipboard->GetData( objectData ) ){
    return false;
  }
  objects = objectData.GetObjects();
  clipboard->Close();
  return true;
}

bool GetText( wxClipboard* clipboard, std::string& dest ){
  wxTextDataObject textObject;
  if ( !clipboard->GetData( textObject ) ){
    return false;
  }

  dest = std::string( textObject.GetText() );
  clipboard->Close();
  return true;
}


void SetBitmap( wxClipboard* clipboard, const faint::Bitmap& bmp, const faint::Color& bgCol ){
  wxDataObjectComposite* composite = new wxDataObjectComposite;
  composite->Add( new faint::FaintBitmapDataObject( bmp ), true );
  composite->Add( new wxBitmapDataObject( to_wx( faint::AlphaBlended( bmp, bgCol ) ) ) );
  clipboard->SetData( composite );
  clipboard->Close();
}

void SetObjects( wxClipboard* clipboard, const std::vector<Object*>& objects ){
  clipboard->SetData( new faint::ObjectDataObject( objects ) );
  clipboard->Close();
}

void SetText( wxClipboard* clipboard, const std::string& text ){
  clipboard->SetData( new wxTextDataObject( wxString(text) ) );
  clipboard->Close();
}

} // Namespace
