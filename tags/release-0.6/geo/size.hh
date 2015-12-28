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

#ifndef FAINT_SIZE
#define FAINT_SIZE
#include "geo/basic.hh"

class Size{
public:
  Size( faint::coord w, faint::coord h );
  bool operator==( const Size& ) const;
  bool operator!=( const Size& ) const;
  Size operator/(const Size& ) const;
  faint::coord w;
  faint::coord h;
};

Size operator-(const Size&, const Size&);

#endif
