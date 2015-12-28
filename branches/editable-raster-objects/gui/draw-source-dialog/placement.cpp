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

#include "wx/wx.h"
#include "gui/draw-source-dialog/placement.hh"

wxPoint below( wxWindow* window){
  return wxPoint(window->GetPosition().x, window->GetPosition().y + window->GetSize().y + 5);
}

int bottom( wxWindow* window ){
  return window->GetPosition().y + window->GetSize().y;
}

int right_side( wxWindow* window ){
  return window->GetPosition().x + window->GetSize().x;
}

wxPoint to_the_right_of( wxWindow* window ){
  return wxPoint(window->GetPosition().x + window->GetSize().x + 5, window->GetPosition().y );
}
