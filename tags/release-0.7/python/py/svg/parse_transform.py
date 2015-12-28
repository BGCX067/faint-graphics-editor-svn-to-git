# Copyright 2012 Lukas Kemmer
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

import re

def parse_args( s ):
    assert( s.startswith("(") )
    assert( s.endswith(")") )
    
    args = [ float(arg) for arg in s.replace(" ", ",")[1:-1].split(",") ]
    return args

def parse_transform( s ):
    items = re.findall( "\w+\(.*?\)", s )
    transform = []

    for item in items:
        op, args = item.split("(")
        transform.append( (op, parse_args( '(' + args )) )
    return transform

if __name__ == '__main__':
    print parse_transform( 'rotate(20,1,2)')
    print parse_transform( 'rotate(20,1,2) scale(1,2)')
