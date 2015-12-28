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

#include "format_wx.hh"
#include "convertwx.hh"
#include "util/imageutil.hh"

FormatWX::FormatWX( const std::string& extension, const std::string& description, wxBitmapType type )
  : Format( extension, description, true, true ),
    m_bmpType(type)
{}

bool FormatWX::Save( const std::string& filename, CanvasInterface& canvas ){
  faint::Image& image(canvas.GetImage());
  faint::Bitmap bmp(faint::Flatten(image));
  wxBitmap wxBmp( to_wx( bmp ) );
  wxBmp.SaveFile( filename, m_bmpType );
  return true;
}

void FormatWX::Load( const std::string& filename, ImageProps& imageProps ){
  // I must load like this (i.e. create a wxImage, init alpha and convert to Bitmap)
  // retain alpha channel. See Ticket #3019
  wxImage wxImg( filename, m_bmpType );
  if ( !wxImg.IsOk() ){
    imageProps.SetError( "Error." ); // Fixme
    return;
  }
  if ( wxImg.HasMask() && !wxImg.HasAlpha() ){
    wxImg.InitAlpha();
  }
  wxBitmap bmp( wxImg );
  imageProps.SetBitmap( to_faint( bmp ) );  
}
