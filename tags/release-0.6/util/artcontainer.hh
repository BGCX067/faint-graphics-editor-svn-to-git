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
#include <string>
class wxBitmap;

class ArtContainer{
  // Handles bitmap loading and storage for application art (e.g.  icons,
  // buttons).
  // Supports setting a root path for interpreting relative paths.
public:
  ArtContainer();
  wxBitmap Get( const std::string& label ) const;
  bool Load( const std::string& filename, const std::string& label );
  void SetRoot( const std::string& );
private:
  ArtContainer(const ArtContainer&);
  ArtContainer& operator=(const ArtContainer&);
  std::map<std::string, wxBitmap> m_bitmaps;
  std::string m_rootPath;
};

#endif
