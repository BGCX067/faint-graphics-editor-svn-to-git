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

#ifndef FAINT_COMMAND_BUNCH_HH
#define FAINT_COMMAND_BUNCH_HH
#include <deque>
#include <vector>
#include "command.hh"
#include "util/unique.hh"

class CommandBunch;
typedef Unique<std::string, CommandBunch, 0> bunch_name;

// Returns a CommandBunch with all the passed in commands, if more
// than one, using the specified bunch_name for the command name.
// If the container only has one command, that command is returned unchanged.
Command* perhaps_bunch( CommandType, const bunch_name&, const commands_t& );
Command* perhaps_bunch( CommandType, const bunch_name&, const std::deque<Command*>& );
Command* command_bunch( CommandType, const bunch_name&, const commands_t& );
Command* command_bunch( CommandType, const bunch_name&, const std::deque<Command*>& );
Command* command_bunch( CommandType, const bunch_name&, Command*, Command* );

#endif
