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
#include "settings.hh"
#include "settingid.hh"

namespace faint {
  class Image;
}

// The command type informs if a command affects the raster layer,
// objects or both objects and the raster layer.
enum CommandType{ CMD_TYPE_RASTER, CMD_TYPE_OBJECT, CMD_TYPE_HYBRID };

class Object;

class Command{
public:
  Command( CommandType type );
  virtual ~Command();
  virtual void Do( faint::Image& ) = 0;
  // Do only the Raster part of the command
  // (Calls Do(...) by default, but can be overridden for Hybrid commands)
  virtual void DoRaster( faint::Image& );
  virtual void Undo( faint::Image& );
  CommandType GetType();
private:
  CommandType m_type;
};

// Manages memory for a not-yet-performed command. Releases the memory
// for the command if it is not Retrieve():ed before destruction or
// before another command is set.
class PendingCommand{
public:
  PendingCommand();
  ~PendingCommand();
  void Set( Command* );
  Command* Retrieve();
private:
  PendingCommand( const PendingCommand& );
  PendingCommand& operator=( const PendingCommand& );
  Command* m_command;
};

#endif
