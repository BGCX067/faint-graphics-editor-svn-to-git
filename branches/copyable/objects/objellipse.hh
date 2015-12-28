// -*- coding: us-ascii-unix -*-
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

#ifndef FAINT_OBJELLIPSE_HH
#define FAINT_OBJELLIPSE_HH
#include "geo/arc.hh"
#include "objects/object.hh"
#include "util/template-fwd.hh"

namespace faint{

Object* create_ellipse_object( const Tri&, const Settings& );
bool is_ellipse(Object*);
Optional<AngleSpan> get_angle_span(Object*);
void set_angle_span(Object*, const AngleSpan&);

} // namespace

#endif
