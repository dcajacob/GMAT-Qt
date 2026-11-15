Ground-map overlays and the offline station place picker

Countries use Natural Earth 1:50 million Admin 0 Countries, version 5.1.1.
Cities and towns use Natural Earth 1:10 million Populated Places, version 5.1.2.
US counties and county equivalents use the US Census Bureau 2025
cartographic boundaries at 1:5 million scale.

Natural Earth data are public domain.
https://www.naturalearthdata.com/about/terms-of-use/
US Census Bureau geography files are US government public data.
https://www.census.gov/geographies/mapping-files/time-series/geo/cartographic-boundary.2025.html

Country boundaries follow Natural Earth's de facto representation.
County coverage includes US states and territories. City coverage is a
curated global set of 7,342 populated places, including a selection of towns.
These generalized map layers provide geographic context.

sources.json records archive versions and SHA-256 hashes. Regenerate with
tools/prepare_ground_overlays.py using pyshp 2.3.1 and the source archives.
Coordinates are longitude/latitude degrees. Boundaries have coarse and
fine simplifications, with antimeridian crossings split before drawing.
