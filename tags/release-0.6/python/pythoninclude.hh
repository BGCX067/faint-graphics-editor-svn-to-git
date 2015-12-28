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

#ifdef _WIN32
// On MSW, Microsofts math.h will redefine
// Pythons _hypot in pyconfig.h, issuing a warning: "warning C4211:
// nonstandard extension used : redefined extern to static".
// If <cmath> is included first, this doesn't happen. See:
// http://connect.microsoft.com/VisualStudio/feedback/details/633988/warning-in-math-h-line-162-re-nonstandard-extensions-used
// (Python 2.7, Visual Studio 2010)
#include <cmath>
#endif
#include <Python.h>
#define GETTER(func)(getter)((void*)(func)) // Silence C4191 in vc
#define SETTER(func) (setter)(func) // for symmetry
