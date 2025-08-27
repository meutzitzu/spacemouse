#!/bin/bash
python ./code_listing.py
pandoc code_listing.md -o code_listing.pdf --pdf-engine=xelatex -V geometry:margin=1in -H listing.tex --highlight-style=pygments
