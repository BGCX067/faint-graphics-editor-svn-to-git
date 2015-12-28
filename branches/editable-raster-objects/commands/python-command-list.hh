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

#ifndef FAINT_PYTHON_COMMAND_LIST_HH
#define FAINT_PYTHON_COMMAND_LIST_HH
class Command;

// Returns a command which merges with all commands, until closed.
Command* open_python_command_list();

// Returns a command which closes an open_python_command_list when
// merged with it.
Command* close_python_command_list();

Command* unwrap_list(Command*);

#endif
