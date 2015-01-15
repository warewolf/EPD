#!/usr/bin/env python

import textwrap
import unifont
import shutil
import os.path
import glob
from PIL import Image
from PIL import ImageFont
from PIL import ImageDraw
import qrcode
import csv
from wif import towif, WIF, PAGE, NAME_FONT_SIZE, NORMAL_FONT_SIZE, WIDTH, HEIGHT


dat = open('page02.txt').readlines()
x = 0
y = 0
page = WIF()
for line in dat:
    wrap = 44
    for l in textwrap.wrap(line, wrap, subsequent_indent='  '):
        page.add_7x5_txt(l, x, y)
        y += 9
page.saveas('PAGE02.WIF')
