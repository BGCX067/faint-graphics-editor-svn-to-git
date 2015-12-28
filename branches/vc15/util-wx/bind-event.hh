// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#ifndef FAINT_BIND_EVENT_HH
#define FAINT_BIND_EVENT_HH
#include "wx/event.h"

namespace faint{

// Allows binding two events to the same functor
template<typename EventTag, typename FUNC>
void bind(const EventTag& e1,
  const EventTag& e2,
  wxEvtHandler* eventHandler,
  const FUNC& onEvent)
{
  eventHandler->Bind(e1, onEvent);
  eventHandler->Bind(e2, onEvent);
}

// Allows binding an event to the same functor for multiple controls
template<typename EventTag, typename FUNC>
void bind(const EventTag& e,
  wxEvtHandler* handler1,
  wxEvtHandler* handler2,
  const FUNC& onEvent)
{
  handler1->Bind(e, onEvent);
  handler2->Bind(e, onEvent);
}

} // namespace

#endif
