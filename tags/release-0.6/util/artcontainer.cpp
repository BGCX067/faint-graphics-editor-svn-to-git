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

ArtContainer::ArtContainer(){
}

wxBitmap ArtContainer::Get( const std::string& label ) const{
  std::map<std::string, wxBitmap>::const_iterator it = m_bitmaps.find(label);
  assert( it != m_bitmaps.end() );
  return it->second;
}

bool ArtContainer::Load( const std::string& filename, const std::string& label ){
  wxFileName fn_filename( filename );
  if ( fn_filename.IsRelative() ){
    fn_filename.MakeAbsolute( m_rootPath );
  }
  assert( fn_filename.FileExists() );

  wxBitmap bitmap(fn_filename.GetLongPath(), wxBITMAP_TYPE_ANY);
  assert( bitmap.IsOk() );
  m_bitmaps[label] = bitmap;
  return true;
}

void ArtContainer::SetRoot( const std::string& rootPath ){
  m_rootPath = rootPath;
}
