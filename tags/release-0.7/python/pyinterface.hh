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

#ifndef FAINT_PYINTERFACE_HH
#define FAINT_PYINTERFACE_HH
#include <string>

class CanvasInterface;
class Command;

namespace faint{
void init_python();

void python_keypress( int keycode, int modifiers );

// Used for printing debug nonsense to the Python console instead of
// proper debugging
void python_print( const std::string& );
void python_run_command( CanvasInterface*, Command* );

// Queues refreshing for the specified canvas,
// which will be refreshed at the end of the read-eval-print.
// Only required for visual changes that don't use commands.
void python_queue_refresh( CanvasInterface* );
void run_python_file( const std::string& filename );
void run_python_str( const std::string& );
void run_python_user_ini();
void resolve_object( const std::string& );

} // namespace
#endif
