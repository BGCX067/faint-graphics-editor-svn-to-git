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

#ifndef FAINT_PATTERN_HH
#define FAINT_PATTERN_HH
#include <map> // Fixme: For pattern_status test function
#include "util/commonfwd.hh"
#include "util/unique.hh"
#include "geo/intpoint.hh"

class category_pattern;
typedef Unique<bool, category_pattern, 0> object_aligned_t;
namespace faint{

class PatternRef;
class Pattern{
public:
  // Creates a non-object aligned Pattern, anchored at 0,0
  explicit Pattern( const Bitmap& );
  Pattern( const Bitmap&, const IntPoint& anchor, const object_aligned_t& );
  Pattern( const Pattern& );
  ~Pattern();
  const Bitmap& GetBitmap() const;
  IntPoint GetAnchor() const;
  bool GetObjectAligned() const;
  IntSize GetSize() const;
  void SetAnchor( const IntPoint& );
  void SetBitmap( const Bitmap& );
  void SetObjectAligned( bool );
  Pattern& operator=( const Pattern& );
  bool operator==( const Pattern& ) const;
  bool operator<( const Pattern& ) const;
private:
  IntPoint m_anchor;
  bool m_objectAligned;
  PatternRef* m_ref;
};

std::map<int, int> pattern_status();
}
#endif
