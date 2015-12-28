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

#ifndef FAINT_TOOLID_HH
#define FAINT_TOOLID_HH

enum ToolId{
  T_MIN_ID = 0,
  T_RECT_SEL = T_MIN_ID,
  T_OBJ_SEL = T_MIN_ID + 1,
  T_PEN = T_MIN_ID + 2,
  T_BRUSH = T_MIN_ID + 3,
  T_PICKER = T_MIN_ID + 4,
  T_LINE = T_MIN_ID + 5,
  T_SPLINE = T_MIN_ID + 6,
  T_RECTANGLE = T_MIN_ID + 7,
  T_ELLIPSE = T_MIN_ID + 8,
  T_POLYGON = T_MIN_ID + 9,
  T_TEXT = T_MIN_ID + 10,
  T_FLOODFILL = T_MIN_ID + 11,
  T_OTHER = T_MIN_ID + 12,
  T_MAX_ID = T_OTHER
};

#endif
