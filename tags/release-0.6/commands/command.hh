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

#ifndef FAINT_COMMAND_HH
#define FAINT_COMMAND_HH
#include <cstddef> // Fixme: For size_t on gcc
#include <string>
#include "util/commonfwd.hh"
#include "util/pending.hh"
#include "util/unique.hh"

typedef std::pair<Command*, faint::Image*> old_command_t;
class RasterSelectionState; // Fixme: Just include the raster selection or add these to commonfwd
class RasterSelectionOptions;
// The command type informs if a command affects the raster layer,
// objects or both objects and the raster layer.
enum CommandType{ CMD_TYPE_RASTER, CMD_TYPE_OBJECT, CMD_TYPE_HYBRID };

class CommandContext;
typedef Unique<bool, CommandContext,0> select_added;
typedef Unique<bool, CommandContext, 1> deselect_old;

class CommandContext{
public:
  virtual ~CommandContext();
  virtual void Add( Object*, const select_added&, const deselect_old& ) = 0;
  virtual void Add( Object*, size_t z, const select_added&, const deselect_old& ) = 0;
  virtual void AddFrame( faint::Image* ) = 0;
  virtual void AddFrame( faint::Image*, size_t index ) = 0;
  virtual const faint::Bitmap& GetBitmap() const = 0;
  virtual FaintDC& GetDC() = 0;
  virtual faint::Image& GetFrame(size_t) = 0;
  virtual IntSize GetImageSize() const = 0;
  virtual const objects_t& GetObjects() = 0;
  virtual size_t GetObjectZ( Object* ) = 0;
  virtual faint::Bitmap& GetRawBitmap() = 0;
  virtual bool HasObjects() const = 0;
  virtual void Remove( Object* ) = 0;
  virtual void RemoveFrame( size_t index ) = 0;
  virtual void RemoveFrame( faint::Image* ) = 0;
  virtual void ReorderFrame(size_t newIndex, size_t oldIndex) = 0;
  virtual void SetBitmap( const faint::Bitmap& ) = 0;
  virtual void SetFrameDelay( size_t num, const delay_t& ) = 0;
  virtual void SetRasterSelection( const RasterSelectionState& ) = 0;
  virtual void SetRasterSelectionOptions( const RasterSelectionOptions& ) = 0;
  virtual void SetObjectZ( Object*, size_t z ) = 0;
};

class Command{
public:
  Command( CommandType );
  virtual ~Command();
  virtual void Do( CommandContext& ) = 0;
  // Do only the Raster part of the command
  // (Calls Do(...) by default, but can be overridden for Hybrid commands)
  virtual void DoRaster( CommandContext& );

  // "Do What I Mean" - returns an alternate command if available.
  // Should only be called after HasDWIM() returns true
  virtual Command* GetDWIM();
  virtual bool HasDWIM() const;
  virtual std::string Name() const = 0;
  virtual IntPoint SelectionOffset() const;
  // Commands that change the image size (e.g. cropping, scaling) can
  // translate a point, expressed in image coordinates, relative to
  // the transformation.
  virtual Point Translate(const Point&) const;
  CommandType Type() const;
  virtual void Undo( CommandContext& );
private:
  CommandType m_type;
};

// Manages memory for a not-yet-performed command. Releases the memory
// for the command if it is not Retrieve():ed before destruction or
// before another command is set.
typedef Pending<Command> PendingCommand;

class Operation{
public:
  virtual ~Operation();
  virtual Command* DoImage() const = 0;
  virtual Command* DoObjects( const objects_t& ) const = 0;
  virtual Command* DoRasterSelection( const faint::Image& ) const = 0;
};

#endif
