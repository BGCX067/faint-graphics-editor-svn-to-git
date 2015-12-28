# Copyright 2012 Lukas Kemmer
#
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You
# may obtain a copy of the License at
#
# http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

import math
from math import atan2, cos, sin
deg_per_rad = 360 / ( 2 * math.pi )
rad_per_deg = 1 / deg_per_rad

def rad2deg( angle ):
    return angle * deg_per_rad

def deg2rad( angle ):
    return angle * rad_per_deg

def parse_value( v ):
    for item in ["px","pt","mm", "cm"]:
        if v.endswith(item):
            return float( v[:-len(item)] )
    return float(v)

def actual_line_end( lineEndX, lineEndY, angle, lineWidth ):
    x = lineEndX - cos( angle ) * 15 * (lineWidth / 2.0 )
    y = lineEndY - sin( angle ) * 15 * (lineWidth / 2.0 )
    return x, y

def arrow_line_end( arrowTipX, arrowTipY, angle, lineWidth ):
    x = arrowTipX + cos( angle ) * 15 * ( lineWidth / 2.0 )
    y = arrowTipY + sin( angle ) * 15 * ( lineWidth / 2.0 )
    return x, y

def rad_angle( x0, y0, x1, y1 ):
    return atan2( y1 - y0, x1 - x0 )


