#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright 2014 Lukas Kemmer
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
import build_sys.util.util as util
from build_sys.cpp_writer import Code

"""Generates the simple constant text-expressions and
documentation."""

COMMANDS = [
    # Command, character, docs
    ('angle', 'utf8_char(0x2220)', 'Angle-symbol'),
    ('deg', 'degree_sign', 'Degree sign'),
    ('delta', 'greek_capital_letter_delta', 'Greek capital delta'),
    ('dog', 'utf8_char(0x1f415)', 'Dog symbol'),
    ('dprime', 'double_prime', 'Double prime'),
    ('ellipsis', 'utf8_char(0x22ef)', 'Ellipsis symbol'),
    ('ge', 'utf8_char(0x2265)', 'Greater or equal sign'),
    ('in', 'double_prime', 'Inch symbol (double prime)'),
    ('inf', 'utf8_char(0x221e)', 'Infinity symbol'),
    ('interrobang', 'utf8_char(0x203d)', 'Surprised exclamation'),
    ('larr', 'utf8_char(0x2190)', 'Left arrow'),
    ('le', 'utf8_char(0x2264)', 'Less or equal sign'),
    ('li', 'bullet', 'Bullet point'),
    ('lrarr', 'utf8_char(0x2194)', 'Left and right arrow'),
    ('ne', 'utf8_char(0x2260)', 'Not equal sign'),
    ('notparallel', 'utf8_char(0x2226)', 'Not parallel symbol'),
    ('parallel', 'utf8_char(0x2225)', 'Parallel symbol'),
    ('perfect', 'utf8_char(0x1f44c)', 'Perfect hand gesture'),
    ('pi', 'greek_small_letter_pi', 'Greek small pi'),
    ('poop', 'utf8_char(0x1f4a9)', 'Pile of poop symbol'),
    ('prime', 'prime', 'Prime symbol'),
    ('rarr', 'utf8_char(0x2192)', 'Right arrow'),
    ('scissors', 'utf8_char(0x2702)', 'A pair of scissors'),
    ('sq', 'superscript_two', 'Superscript two'),
    ('sqrt', 'utf8_char(0x221a)', 'Square-root symbol'),
    ('times', 'utf8_char(0xD7)', 'Multiplication symbol'),
    ('tm', 'utf8_char(0x2122)', 'Trademark symbol'),
    ('tprime', 'triple_prime', 'Triple prime'),
    ('whoa', 'utf8_char(0x1f450)', 'Raised hands')]


def get_header_code():
    hh = Code()
    hh.line('// Generated by %s\n' % os.path.basename(__file__))
    hh.line('namespace faint{')
    hh.line('const std::map<utf8_string, utf8_string>& constant_exprs(){')
    hh.line('static std::map<utf8_string, utf8_string> constants =')
    hh.line('{')

    for c in COMMANDS[:-1]:
        hh.line('{"%s", utf8_string(%s)},' % (c[0], c[1]))
    c = COMMANDS[-1]
    hh.line('{"%s", utf8_string(%s)}' % (c[0], c[1]))

    hh.line('};')
    hh.line('return constants;')
    hh.line('}')
    hh.line('} // namespace')
    return hh


def get_help():
    help = '# Generated by %s\n' % os.path.basename(__file__)
    help += '||*Command*||*Symbol*||*Description*||\n'
    for c in COMMANDS:
        help += ('||\\%s||\\image(symbol-%s.png)||%s||\n' % (c[0], c[0], c[2]))
    return help


def generate_header(file_path):
    with open(file_path, 'w', newline='\n') as f:
        f.write(str(get_header_code()));


def generate_help(file_path):
    """Writes a help-source-file documenting the commands"""
    with open(file_path, 'w', newline='\n') as f:
        f.write(get_help())


def generate(hh_path, help_path):
    if util.changed(__file__, hh_path):
        generate_header(hh_path)

    if util.changed(__file__, help_path):
        generate_help(help_path)