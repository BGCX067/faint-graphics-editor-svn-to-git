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

#ifndef FAINT_INTSIZE
#define FAINT_INTSIZE
#include "util/commonfwd.hh"
class IntSize{
public:
  IntSize();
  IntSize( int w, int h );
  bool operator==( const IntSize& ) const;
  bool operator!=( const IntSize& ) const;
  int w;
  int h;
private:
  // Undefined constructors to prevent conversion from float
  // Use truncated(Size(w,h)) instead.
  IntSize(float, float);
  IntSize(double, double);
};

int area(const IntSize&);
IntSize operator+( const IntSize&, const IntSize& );
IntSize operator-( const IntSize&, const IntSize& );
IntSize operator*( int, const IntSize& );
IntSize transposed(const IntSize&);
#endif
