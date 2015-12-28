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

#ifndef FAINT_CANVASINTERFACE_HH
#define FAINT_CANVASINTERFACE_HH

#include <vector>
#include <string>
#include "bitmap/bitmap.h"
#include "faintimage.hh"
#include "geo/geotypes.hh"
#include "geo/grid.hh"
#include "util/idtypes.hh"

class Object;
class Command;

struct CanvasInfo {
  CanvasInfo( size_t w, size_t h, const faint::Color& bgCol ) :
    backgroundColor( bgCol )
  {
    width = w;
    height = h;
  }
  int width;
  int height;
  faint::Color backgroundColor;
};

enum ApplyTarget{
  APPLY_OBJECT_SELECTION,
  APPLY_RASTER_SELECTION,
  APPLY_IMAGE,
  APPLY_ACTIVE_TOOL
};

class CanvasInterface {
public:
  virtual std::vector<Object*>& GetSelectedObjects() = 0;
  virtual std::vector<Object*>& GetObjects() = 0;
  virtual void CenterView( const Point& ) = 0;
  virtual void SelectObject( Object*, bool deselectOld=true ) = 0;
  virtual void DeselectObject( Object* ) = 0;
  virtual void DeselectObjects() = 0;
  virtual void DeselectRaster() = 0;
  virtual IntSize GetSize() const = 0;
  virtual void Refresh() = 0;
  virtual void Undo() = 0;
  virtual void Redo() = 0;
  virtual void RunCommand( Command* ) = 0;
  virtual void ClearDirty() = 0;
  virtual IntRect GetRasterSelection() = 0;
  virtual void SetRasterSelection( const IntRect& ) = 0;
  virtual void SetFilename( const std::string& ) = 0;
  virtual void ZoomIn() = 0;
  virtual void ZoomOut() = 0;
  virtual void ZoomDefault() = 0;
  virtual faint::coord GetZoom() const = 0;
  virtual faint::Bitmap& GetBitmap() = 0;
  virtual faint::Bitmap GetBitmap( const IntRect& ) = 0;
  virtual faint::Image& GetImage() = 0;
  virtual std::string GetFilename() const = 0;
  virtual CanvasId GetId() const = 0;
  virtual bool Has( const ObjectId& ) const = 0;
  virtual void ContextCrop() = 0;
  virtual void ContextDelete() = 0;
  virtual void ContextDeselect() = 0;
  virtual void ContextFlatten() = 0;
  virtual void ContextFlipHorizontal() = 0;
  virtual void ContextFlipVertical() = 0;
  virtual void ContextRotate90CW() = 0;
  virtual Point GetRelativeMousePos() = 0;
  virtual Grid GetGrid() const = 0;
  virtual void SetGrid( const Grid& ) = 0;
  virtual void ScrollPageUp() = 0;
  virtual void ScrollPageDown() = 0;
  virtual void ScrollPageLeft() = 0;
  virtual void ScrollPageRight() = 0;
  virtual void ScrollMaxLeft() = 0;
  virtual void ScrollMaxRight() = 0;
  virtual void ScrollMaxUp() = 0;
  virtual void ScrollMaxDown() = 0;  
};

#endif
