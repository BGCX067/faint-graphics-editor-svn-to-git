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

#ifndef FAINT_PY_INTERFACE_HH
#define FAINT_PY_INTERFACE_HH
#include <vector>
#include <string>
#include "util/common-fwd.hh"
#include "util/id-types.hh"

namespace faint{
class BoundObject;
class Command;
class Frame;

std::string get_python_version();

bool init_python();

// Used for printing debug nonsense to the Python console instead of
// proper debugging
void python_print( const faint::utf8_string& );

void python_run_command( Canvas&, Command* );
void python_run_command( const BoundObject&, Command* );
void python_run_command( const Frame&, Command* );

// Queues refreshing for the specified canvas,
// which will be refreshed at the end of the read-eval-print.
// Only required for visual changes that don't use commands.
void python_queue_refresh( Canvas& );

bool run_python_file(const faint::FilePath&, AppContext&);
void run_python_str(const std::string&);

// Runs the user configuration. Returns true if loaded without error
bool run_python_user_config(AppContext&);
void resolve_object( const std::string& );
std::vector<std::string> list_ifaint_names();

} // namespace
#endif
