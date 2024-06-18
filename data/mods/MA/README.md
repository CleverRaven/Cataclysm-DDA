# MA

Overmap overhaul mod for Cataclysm: DDA that uses some features of Massachusetts state when generating overmaps.

# Implemented Features

1. Use cities and towns of Massachusetts. Allow to start in one of them.
2. Use topography of Massachusetts.

# Plannned Features

1. Update topography of Massachusetts.

# Notes

Overmap terrain might be unpolished at times on some of the overmaps (e.g. fields w/o much swamps or forests), so you can use new external options to spice up pregenerated overmap with procgen features:

* OVERMAP_POPULATE_OUTSIDE_CONNECTIONS_FROM_NEIGHBORS - Allows to populate outside connections from neighbors.
* OVERMAP_PLACE_RIVERS - Allows to place procgen rivers during overmap generation.
* OVERMAP_PLACE_LAKES - Allows to place procgen lakes during overmap generation.
* OVERMAP_PLACE_FORESTS - Allows to place procgen forests during overmap generation.
* OVERMAP_PLACE_SWAMPS - Allows to place procgen swamps during overmap generation.
* OVERMAP_PLACE_RAVINES - Allows to place procgen ravines during overmap generation.

# Additional files

* MA_overmap_index.png - visual index of overmap coordinates.
* MA_overmap_cities.xlsx - visual index of city coordinates, original data and json generator.
