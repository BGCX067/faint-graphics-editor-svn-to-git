// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#ifndef FAINT_TEST_HH
#define FAINT_TEST_HH

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
extern std::stringstream TEST_OUT;
extern bool TEST_FAILED;

class AbortTestException{};

#define EQUAL(A,B) if ((A) != (B)){ TEST_OUT << "  Error(" << __LINE__ << "): " << #A << " != " << #B << std::endl; TEST_FAILED=true;}

#define VERIFY(C) if ((C)){}else{ TEST_OUT << "  Error(" << __LINE__ << "): " << #C << " failed." << std::endl; TEST_FAILED=true;}

#define ASSERT(C) if ((C)){}else{ TEST_OUT << "  Error(" << __LINE__ << "): " << #C << " failed." << std::endl; TEST_FAILED=true; throw AbortTestException();}

#define FAIL() TEST_OUT << "  FAIL triggered on line " << __LINE__ << std::endl; TEST_FAILED=true; throw AbortTestException();

#endif
