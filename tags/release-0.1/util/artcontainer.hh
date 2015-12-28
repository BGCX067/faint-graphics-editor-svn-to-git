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

#ifndef FAINT_ARTCONTAINER_HH
#define FAINT_ARTCONTAINER_HH
#include <map>
#include "wx/string.h"
class wxBitmap;

/* Handles bitmap loading for application art etc. (e.g.  icons,
   buttons) and storage. Supports setting a root path for interpreting
   relative paths */
class ArtContainer{
public:
  ArtContainer( const wxString& rootPath=wxEmptyString );
  ~ArtContainer();
  void SetRoot( const wxString& rootPath );
  // Fixme: Why pointers?
  wxBitmap* Get( const wxString& label );
  const wxBitmap* Get( const wxString& label ) const;
  bool Load( const wxString& filename, const wxString& label );
private:
  wxString m_rootPath;
  std::map<wxString, wxBitmap*> bitmaps;
};

#endif
