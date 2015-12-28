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

#ifndef FAINT_COLORPANEL
#define FAINT_COLORPANEL
#include "geo/color.hh"
#include "util/palettecontainer.hh"
class ArtContainer;
class ColorPanelImpl;
class SettingNotifier;
class StatusInterface;
class ZoomLevel;

struct ColorChoice{
  faint::Color fg;
  faint::Color bg;
};

class ColorPanel {
  // Horizontal panel containing a selected color control, a palette
  // control and a zoom control.
public:
  ColorPanel( wxWindow* parent, const Settings&, SettingNotifier&, PaletteContainer&, StatusInterface&, const ArtContainer& );
  ~ColorPanel();
  void AddToPalette( const faint::Color& );
  wxWindow* AsWindow();
  void Freeze();
  void Hide();
  void Show( bool );
  void Thaw();
  void UpdateFrames( size_t numFrames );
  void UpdateGrid();
  void UpdateSelectedColors( const ColorChoice& );
  void UpdateZoom( const ZoomLevel& );
  void SetPalette( const std::vector<faint::Color>& );
  bool Visible() const;
private:
  // Use an impl-method instead of inheriting from wx-classes to
  // minimize the interface
  ColorPanelImpl* m_impl;
};

#endif
