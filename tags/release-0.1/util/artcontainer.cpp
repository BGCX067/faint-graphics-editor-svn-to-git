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

#include "artcontainer.hh"
#include "wx/filename.h"
#include "wx/msgdlg.h"
#include "wx/bitmap.h"

ArtContainer::ArtContainer( const wxString& rootPath )
  : m_rootPath( rootPath )
{
  if ( rootPath == wxEmptyString ){
    m_rootPath = wxFileName::GetCwd();
  }
}

void ArtContainer::SetRoot( const wxString& rootPath ){
  m_rootPath = rootPath;
}

ArtContainer::~ArtContainer(){
  for ( std::map<wxString, wxBitmap*>::iterator it = bitmaps.begin(); it != bitmaps.end(); ++it ){
    delete it->second;
  }
}

wxBitmap* ArtContainer::Get( const wxString& label ){
  assert( bitmaps.find(label) != bitmaps.end() );
  return bitmaps[label];
}

const wxBitmap* ArtContainer::Get( const wxString& label ) const{
  std::map<wxString, wxBitmap*>::const_iterator it = bitmaps.find(label);
  assert( it != bitmaps.end() );
  return it->second;
}

bool ArtContainer::Load( const wxString& filename, const wxString& label ){
  wxFileName fn_filename( filename );
  if ( fn_filename.IsRelative() ){
    fn_filename.MakeAbsolute( m_rootPath );
  }
  assert( fn_filename.FileExists() );

  wxBitmap bitmap;
  bitmap.LoadFile( fn_filename.GetLongPath(), wxBITMAP_TYPE_ANY );
  assert( bitmap.IsOk() );
  bitmaps[label] = new wxBitmap( bitmap );
  return true;
}
