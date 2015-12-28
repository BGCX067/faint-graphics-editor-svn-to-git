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

#ifndef FAINT_MENU_BAR_HH
#define FAINT_MENU_BAR_HH
#include <functional>
#include <vector>
#include "app/app-context.hh"
#include "gui/menu-predicate.hh"
#include "util/file-path.hh"

class wxFrame;

namespace faint{
class ArtContainer;
class ZoomLevel;

class RecentFiles {
public:
  virtual ~RecentFiles();
  virtual void Add(const FilePath&) = 0;
  virtual void Clear() = 0;
  virtual FilePath Get(size_t) const = 0;
  virtual void Save() = 0;
  virtual size_t Size() const = 0;
  virtual void UndoClear() = 0;
};

enum class EntryMode{NONE, ALPHA_NUMERIC, NUMERIC};

class MenubarImpl;

class Menubar{
  // The menubar at the top of the main frame. Handles creation,
  // enabling/disabling and so on.
public:
  Menubar(wxFrame&, AppContext&, const ArtContainer&);
  ~Menubar();

  // Disable shortcuts that might interfere with text entry
  void BeginTextEntry(bool numericOnly=false);

  // Re-enable shortcuts after text entry
  void EndTextEntry();
  RecentFiles& GetRecentFiles();
  EntryMode GetTextEntryMode() const;

  // Enable/disable menu items depending on parameters
  void Update(const MenuFlags&);

  // Enable/disable zoom menu items depending on zoom state
  void UpdateZoom(const ZoomLevel&);
private:
  MenubarImpl* m_impl;
};

} // namespace

#endif
