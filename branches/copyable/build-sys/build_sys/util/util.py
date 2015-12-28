# Copyright 2013 Lukas Kemmer
#
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You
# may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

import os

def changed( source, target ):
    return not os.path.exists( target ) or os.path.getmtime( target ) < os.path.getmtime( source )

def _strip_ext(file_name):
    dot = file_name.find('.')
    assert(dot != -1)
    return file_name[:dot]

def create_include_guard(file_path):
    base_name = os.path.basename(file_path)
    return "FAINT_" + _strip_ext(base_name).upper().replace('-','_') + "_HH"

def timed(func, *args, **kwArgs):
    import time
    t1 = time.time()
    res = func(*args, **kwArgs)
    t2 = time.time()
    print('%s took %0.3f ms' % (func.__name__, (t2-t1)*1000.0))
    return res

def print_timing(func):
    def wrapper(*arg, **kwArgs):
        return timed(func, *arg, **kwArgs)
    return wrapper
