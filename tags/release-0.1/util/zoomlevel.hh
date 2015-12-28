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

#ifndef FAINT_ZOOMLEVEL_HH
#define FAINT_ZOOMLEVEL_HH
#include "geo/basic.hh"
class ZoomLevel{
public:
  static const int defaultLevel=0;
  explicit ZoomLevel( int=defaultLevel);
  bool AtMax() const;
  bool AtMin() const;
  bool At100() const;
  enum ChangeType{PREVIOUS, NEXT, DEFAULT};
  bool Change( ChangeType );
  bool Next();
  bool Prev();
  faint::coord GetScaleFactor() const;
  int GetLevel() const;
  int GetPercentage() const;
  void SetLevel( int );
private:
  int m_zoomLevel;
};

#endif
