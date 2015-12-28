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

#ifndef FAINT_SET_RASTER_SELECTION_CMD_HH
#define FAINT_SET_RASTER_SELECTION_CMD_HH
#include "bitmap/bitmap.hh"
#include "commands/command.hh"
#include "util/rasterselection.hh"

// Light-weight command for moving a selection - does not cause
// another copy of the selected content.
Command* move_raster_selection_command(const IntPoint& newPos, const IntPoint& oldPos );
bool is_move_raster_selection_command(Command*);

class SetRasterSelection : public Command{
public:
  SetRasterSelection(const NewRasterSelectionState&, const OldRasterSelectionState&, const std::string& name, bool appendCommand=false);
  void Do( CommandContext& ) override;
  bool ModifiesState() const override;
  std::string Name() const override;
  void Undo( CommandContext& ) override;

  bool ShouldAppend() const;
private:
  SetRasterSelection& operator=(const SetRasterSelection&);
  const std::string m_name;
  RasterSelectionState m_newState;
  RasterSelectionState m_oldState;
  bool m_appendCommand;
};

class SetSelectionOptions : public Command {
public:
  SetSelectionOptions(const NewRasterSelectionOptions&, const OldRasterSelectionOptions&);
  void Do( CommandContext& ) override;
  std::string Name() const override;
  void Undo( CommandContext& ) override;
private:
  RasterSelectionOptions m_newOptions;
  RasterSelectionOptions m_oldOptions;
};

#endif
