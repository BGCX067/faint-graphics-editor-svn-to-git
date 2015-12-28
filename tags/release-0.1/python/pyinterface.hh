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

void initPython();
void runUserPythonIni();
void runCmd( const std::string& cmd );

// Used for printing debug nonsense to the Python console instead of
// proper debugging
void python_print( const std::string& );

class CanvasInterface;
class Command;
void PythonRunCommand( CanvasInterface*, Command* );
#endif
