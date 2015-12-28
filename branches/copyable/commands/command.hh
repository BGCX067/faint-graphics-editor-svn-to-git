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

#ifndef FAINT_COMMAND_HH
#define FAINT_COMMAND_HH
#include <string>
#include "util/common-fwd.hh"
#include "util/distinct.hh"
#include "util/id-types.hh"
#include "util/index.hh"
#include "util/pending.hh"

namespace faint{

// The command type informs if a command affects the raster layer,
// objects or both objects and the raster layer.
enum class CommandType{ RASTER, OBJECT, HYBRID, SELECTION, FRAME };
bool fully_reversible(CommandType);
bool has_raster_steps(CommandType);
bool somewhat_reversible(CommandType);

class category_command;
typedef Distinct<bool, category_command, 0> select_added;
typedef Distinct<bool, category_command, 1> deselect_old;

class CommandContext{
  // Interface passed to Commands to let them modify an image.
  // Note: Anything the CommandContext returns by reference
  // (e.g. GetDC, GetFrame) has uncertain life-time and may be
  // invalidated by other CommandContext methods.
public:
  virtual ~CommandContext();
  virtual void Add(Object*, const select_added&, const deselect_old&) = 0;
  virtual void Add(Object*, int z, const select_added&, const deselect_old&) = 0;
  virtual void AddFrame(Image*) = 0;
  virtual void AddFrame(Image*, const index_t&) = 0;
  virtual const Bitmap& GetBitmap() const = 0;
  virtual FaintDC& GetDC() = 0;
  virtual Image& GetFrame() = 0;
  virtual Image& GetFrame(const index_t&) = 0;
  virtual RasterSelection& GetRasterSelection() = 0;
  virtual IntSize GetImageSize() const = 0;
  virtual const objects_t& GetObjects() = 0;
  virtual int GetObjectZ(const Object*) = 0;
  virtual Bitmap& GetRawBitmap() = 0;
  virtual bool HasObjects() const = 0;
  virtual void MoveRasterSelection(const IntPoint& topLeft) = 0;
  virtual void OffsetRasterSelectionOrigin(const IntPoint& delta) = 0;
  virtual void Remove(Object*) = 0;
  virtual void RemoveFrame(const index_t&) = 0;
  virtual void RemoveFrame(Image*) = 0;
  virtual void ReorderFrame(const new_index_t&, const old_index_t&) = 0;
  virtual void SetBitmap(const Bitmap&) = 0;
  virtual void SetBitmap(Bitmap&&) = 0;
  virtual void SetRasterSelection(const SelectionState&) = 0;
  virtual void SetRasterSelectionOptions(const SelectionOptions&) = 0;
  virtual void SetObjectZ(Object*, int z) = 0;
};

class Command{
public:
  Command(CommandType);
  virtual ~Command();
  virtual void Do(CommandContext&) = 0;
  // Do only the Raster part of the command
  // (Calls Do(...) by default, but must be overridden for Hybrid commands)
  virtual void DoRaster(CommandContext&);

  // True if the passed in command could be merged with this command.
  // sameFrame specifies if the command was targetting the same frame
  // as this command.
  virtual bool Merge(Command*, bool sameFrame);

  // "Do What I Mean" - returns an alternate command if available.
  // Should only be called after HasDWIM() returns true
  virtual Command* GetDWIM();
  virtual CommandId GetId() const;
  virtual bool HasDWIM() const;

  // Whether the command modifies the image, which most commands do.
  // Some selection actions do not change the image, and should not
  // flag the image has dirty - but should still support undo/redo.
  virtual bool ModifiesState() const;
  virtual utf8_string Name() const = 0;
  virtual IntPoint SelectionOffset() const;
  // Commands that change the image size (e.g. cropping, scaling) can
  // translate a point, expressed in image coordinates, relative to
  // the transformation.
  virtual Point Translate(const Point&) const;
  CommandType Type() const;
  virtual void Undo(CommandContext&);
  virtual Point UndoTranslate(const Point&) const;
private:
  CommandType m_type;
  CommandId m_id;
};

// Manages memory for a not-yet-performed command. Releases the memory
// for the command if it is not Retrieve():ed before destruction or
// before another command is set.
typedef Pending<Command> PendingCommand;

enum class UndoType{
  // An OldCommand can be an actual command or the start or end of a
  // group of commands.
  OPEN_GROUP, CLOSE_GROUP, NORMAL_COMMAND};

class OldCommand{
  // A previously ran command, with the image it targetted.
public:
  // Creates an OldCommand which signifies the start of a grouped set
  // of commands
  static OldCommand OpenGroup();

  // Creates an OldCommand which signifies the end of a grouped set of
  // commands
  static OldCommand CloseGroup();

  // Creates an OldCommand which maps an executed Command to an Image,
  // so that undo/redo can target the same image.
  OldCommand(Command*, Image*);

  // Attempts to merge the passed in OldCommand with this one,
  // returns true if successful.
  bool Merge(OldCommand&);
  Command* command;
  Image* targetFrame;
  UndoType type;
private:
  explicit OldCommand(UndoType);

};

class Operation{
public:
  virtual ~Operation();
  virtual Command* DoImage() const = 0;
  virtual Command* DoObjects(const objects_t&) const = 0;
  virtual Command* DoRasterSelection(const Image&) const = 0;
};

} // namespace

#endif
