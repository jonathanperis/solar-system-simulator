# Sky overlays

`constellations.lines.json` holds the IAU constellation stick figures
(right ascension wrapped to ±180°, declination, J2000) from
[d3-celestial](https://github.com/ofrohn/d3-celestial) by Olaf Frohn, used
under its BSD 3-Clause licence (`LICENSE-d3-celestial`, reproduced in the
generated `src/render/constellation_lines.inc` and credited on the About
page). `python3 tools/constellations.py` regenerates the C table; `make test`
checks it is current (SPEC A124). Presentation only.
