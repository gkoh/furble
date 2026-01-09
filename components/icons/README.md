# Icons

Icons are sourced from [Material Symbols](https://fonts.google.com/icons).

Icons were configured for download as:
- No fill
- Weight 300
- Grade 0
- Size 24px

They are further converted to the correct sized PNG by:
```
inkscape -w 64 -h 64 icon.svg -o icon.png
```

Then converted to a compressed in-memory variable by:
```
LVGLImage.py --ofmt C --cf RGB565A8 --compress LZ4 icon.png
```
