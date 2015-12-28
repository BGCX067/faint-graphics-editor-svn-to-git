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

#ifndef FAINT_FORMAT_HH
#define FAINT_FORMAT_HH
#include <string>
#include "faintimage.hh"
#include "canvasinterface.hh"
class CanvasScroller;

// Base class for file formats
class Format{
public:
  Format( const std::string& format, const std::string& label, bool canSave, bool canLoad );
  bool CanSave();
  bool CanLoad();

  virtual bool Save( const std::string& filename, CanvasInterface& ) = 0;
  virtual void Load( const std::string& filename, ImageProps& ) = 0;
  const std::string& GetFormat() const;
  const std::string& GetLabel() const;
private:
  std::string format;
  std::string label;
  bool canSave;
  bool canLoad;
};

#endif
