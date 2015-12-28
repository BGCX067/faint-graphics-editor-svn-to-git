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

#ifndef FAINT_TEST_SYS_HH
#define FAINT_TEST_SYS_HH
#include "test-sys/test.hh"

void run_test(void (*func)(), const std::string& name, int max_w, int& numFailed){
  // Test title
  std::cout << name << ":" << std::string(static_cast<size_t>(max_w) - name.size(), ' ');

  // Run the test
  try{
    func();
  }
  catch(const AbortTestException&){
  }

  // Append result to title
  std::cout <<(TEST_FAILED ? "FAIL" : "ok") << std::endl;

  if (TEST_FAILED){
    numFailed += 1;
    std::cout << TEST_OUT.str();
    TEST_FAILED = false;
    TEST_OUT.clear();
  }
}

int print_test_summary(const int numFailed){
  if (numFailed > 0){
    std::cout << std::endl;
    std::cout << "Error: " << numFailed << " " <<
      (numFailed == 1 ? "test" : "tests") << " failed!" << std::endl;
    return 1;
  }
  return 0;
}

#endif
