import re
import os
import sys
sys.path.append(os.path.join(os.getcwd(), "mock"))
sys.path.append(os.path.join(os.getcwd(), ".."))

import faint.svg.parse_svg_etree as parse_svg_etree
from faint.svg.parse_svg_etree import svg_re

def regex_test():
    print(re.match(svg_re.number_attr, "10.1").group(0))
    assert re.match(svg_re.number_attr, "10.1").group(0) == '10.1'
    assert re.match(svg_re.number_attr, "10").group(0) == '10'
    assert re.match(svg_re.number_attr, "10.10").group(0) == '10.10'

    assert re.match(svg_re.length_attr, "-640.0E10").groups() == ('-640.0E10','')
    assert re.match(svg_re.length_attr, "-640pt").groups() == ('-640','pt')
    assert re.match(svg_re.length_attr, "640pt").groups() == ('640','pt')
    print(re.match(svg_re.length_attr, "1.37071896000000004pt").groups())
    assert re.match(svg_re.length_attr, "640pu").groups() == None

def gradient_test():
    assert(parse_svg_etree.parse_gradient_offset("1.0") == 1.0)
    print(parse_svg_etree.parse_gradient_offset("1"))
    #assert(parse_svg_etree.parse_gradient_offset("1") == 1)

if __name__ == '__main__':
    gradient_test()
    regex_test()
