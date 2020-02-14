Last updated 2020-1-23
Up to date as of Build #10230 (Military Base location (PR #37024))

Installation:
  - Rename 'fonts.json.txt' to 'fonts.json', move into your /config folder and overwrite - unless you have previously made changes to the file, in which case you will need to modify it manually to save your changes. Make sure 'fonts.json' no longer remains in the graphical overmap mod folder.
  - In graphics options make sure the overmap font width/height/size are all set to "16" then restart.
	
Optional:
  - I would advise disabling the 5x5 location map in the sidebar, as it becomes unreadable by using this mod. Press } to access sidebar options and choose either 'labels' (using the Location Alt panel instead of Location) or 'labels-narrow'.
  - For a more muted palette to increase readability move 'base_colors.json' into your /config folder and overwrite. The crossroads with manhole (road_nesw_manhole) actually uses the dark_gray (60,60,60) from this palette, so may look slightly off if you are using a different dark_gray.
	
Issues:
  - MAJOR: Mod doesn't work for people using Cyrillic script or diacritic glyphs.
  - MAJOR: You may have problems (usually color based) when running this on anything but Windows.
  - MINOR: The arbitrary symbol I have used to represent each icon will appear at the top of the sidebar on the overmap screen when the tile is selected.
  - MINOR: Some locations are hidden in forests. However, some of these have the symbol and color of a forest, while their name reveals what they really are (eg. "cabin_aban1"). Not sure whether to make them look like forests or as their name suggests. So far have made them the symbol of their name, but green.
  - MINOR: I know it's possible to change dirt roads, although getting the angles right confused me so I left it for now.
	
Happy cartography,
@Larwick
