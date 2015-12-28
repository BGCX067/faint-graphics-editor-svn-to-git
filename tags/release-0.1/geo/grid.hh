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

#ifndef FAINT_GRID
#define FAINT_GRID

faint::Color DefaultGridColor();
class Grid{
public:
  Grid(bool enabled=false, bool visible=false, int spacing=10, const faint::Color=DefaultGridColor());
  const faint::Color& Color() const;
  bool Enabled() const;
  int Spacing() const;
  bool Visible() const;
  void SetEnabled( bool );
  void SetSpacing( int );
  void SetVisible( bool );
private:
  bool m_enabled;
  bool m_visible;
  int m_spacing;
  faint::Color m_color;
};

#endif
