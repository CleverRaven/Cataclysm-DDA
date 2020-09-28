## Making ASCII art for CDDA
### ASCII art Editors
ASCII art can be made using any text editor, such as Notepad, but it's more convenient to use an editor created for making ASCII art. There are many ASCII art editors on the internet, but this guide focuses on REXPaint because aside from many useful features like drawing ellipses, rectangles, lines and other shapes, it's the only editor with the option to export images to a format readable by CDDA. Note that this is not a comprehensive REXPaint manual, you can find the full manual [here](https://github.com/Lucide/REXPaint-manual/blob/master/manual.md) or in your REXPaint folder.
### Installing REXPaint
1. Download REXPaint [here](https://www.gridsagegames.com/rexpaint/downloads.html) and unzip the folder. Note: in version 1.50 colors yellow and pink are not exported correctly. This can be fixed by downloading the patch [here](https://www.gridsagegames.com/forums/index.php?topic=1463.msg9430#msg9430). This issue will not be present in version 1.51.
2. Download CDDA color palette [here](https://www.gridsagegames.com/rexpaint/resources.html#Palettes) and put in the `palettes` folder.  Change the palette in REXPaint  to "CDDA" palette using `[` and `]` buttons. You can sort the colors nicely by pressing `Ctrl-Shift-o`.
3. Download unifont font (the font used in CDDA) [here](https://www.gridsagegames.com/rexpaint/resources.html#Fonts). Put it into `fonts` folder and paste the following line at the bottom of `_config.xt` file:
`"cp437_8x16_unifont"	cp437_8x16_unifont	16	16	cp437_8x16_unifont	16	16	1`
You can change the font in REXPaint by pressing `Ctrl-wheel`.

### Drawing
Now you can draw your art. It's strongly advised to check the manual for descriptions of all tools. Note that art for CDDA can't be wider than 41 characters, therefore it's advised to set `defaultImageWidth` to 41 in `REXPaint.cfg`. If you use different colors than those found in CDDA palette, they'll be exported as white.
### Exporting 
Once you've finished making your art you can export it by pressing `Ctrl-j`. This will produce a text file ready to be pasted into `ascii_arts.json`, complete with color tags.  Make sure that the canvas isn't wider that 41 characters before exporting. For more information about adding your art to the game read `JSON_INFO.md`.
