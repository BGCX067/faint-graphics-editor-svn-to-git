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

#include "wx/filename.h"
#include "wx/msgdlg.h"
#include "wx/bitmap.h"
#include "wx/cursor.h"
#include "formats/msw_icon.hh"
#include "util/artcontainer.hh"
#include "util/convertwx.hh"
#include "util/guiutil.hh"
#include "util/image.hh"
#include "util/imageprops.hh"

static wxCursor cur_from_bmp( const wxBitmap& bmp, const IntPoint& hotspot ){
  wxImage img( bmp.ConvertToImage() );
  img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspot.x );
  img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspot.y );
  img.SetMaskColour( 255, 0, 255 );
  return wxCursor(img);
}

ArtContainer::ArtContainer(){
}

wxBitmap ArtContainer::Get( Icon iconId ) const{
  std::map<Icon, wxBitmap>::const_iterator it = m_icons.find(iconId);
  assert( it != m_icons.end() );
  return it->second;
}

const wxCursor& ArtContainer::Get( Cursor cursorId ) const{
  std::map<Cursor, wxCursor>::const_iterator it = m_cursors.find(cursorId);
  assert( it != m_cursors.end() );
  return it->second;
}

void ArtContainer::Add( const wxCursor& cursor, Cursor cursorId ){
  m_cursors[cursorId] = cursor;
}

void ArtContainer::Load( const wxString& filename, Icon iconId ){
  wxFileName fn_filename( filename );
  if ( fn_filename.IsRelative() ){
    fn_filename.MakeAbsolute( m_rootPath );
  }
  assert( fn_filename.FileExists() );

  wxBitmap bitmap(fn_filename.GetLongPath(), wxBITMAP_TYPE_ANY);
  assert( bitmap.IsOk() );
  m_icons[iconId] = bitmap;
}

void ArtContainer::Load( const wxString& filename, Cursor cursorId ){
  wxFileName fn_filename( filename );
  if ( fn_filename.IsRelative() ){
    fn_filename.MakeAbsolute( m_rootPath );
  }
  assert( fn_filename.FileExists() );
  assert( fn_filename.GetExt() == "cur" );

  ImageProps props;
  faint::load_cursor(faint::FilePath::FromAbsoluteWx(fn_filename), props);
  assert(props.IsOk()); // Fixme: Load error of resource may be too likely for assert
  assert(props.GetNumFrames() == 1);
  faint::Image image(props.GetFrame(0));
  m_cursors[cursorId] = cur_from_bmp(to_wx_bmp(image.GetBitmap()), image.GetHotSpot());
}

void ArtContainer::SetRoot( const wxString& rootPath ){
  m_rootPath = rootPath;
}
