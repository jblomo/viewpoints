#!/usr/bin/perl

#  This is meant to split up a .png file (containing many 64x64 greyscale charcters/symbols) into individual 64x64 tiles.
#  I generated the original file on OSX using: 
#      ttf2png -r 0,10000 -s64 -l1 -c64 -o osaka_64x64.png -v -e /System/Library/Fonts/OsakaMono.dfont
# which claims to have generated 467 glyphs
# we need to make raw files to get rid of the offset, then make a gif so that fluid can deal with it
# (fluid doesn't like 8 bit .png files)
#

# the following code, commented out, is what I orignally used
# for ($i=0; $i<466; $i++) {
#   $n = $i*64;
#   print "converting $i\n";

#   # extract one tile from the big file
#   `convert osaka_64x64.png -crop 64x64+0+$n glyphs/osaka_$i.png`;

#   # convert it to a raw grayscale file, to get rid of the extraneous "offset" information 
#   `convert glyphs/osaka_$i.png -resize 64x64+0+0 gray:tmp.raw`;

#   # convert it to a grayscale gif file, so fltk can deal with it.
#   `convert -depth -size 64x64+0+0 gray:tmp.raw glyphs/osaka_$i.gif`;

# }

# theis code converts all the 64x64 .png (after editing out the ones we dont want,
# and ordering the filename so they appear in a good order in the menu) into 64x64
# gifs, and 16x16 gifs

@names = `ls -1c *.png`;
foreach $name (@names) {
  chop($name);
  $base = $name;
  $base =~ s/.png//;
  print `echo convert $name -resize 64x64+0+0 gray:tmp.raw`;
  `convert $name -resize 64x64+0+0 gray:tmp.raw`;
  print `echo convert -depth 8 -size 64x64 gray:tmp.raw $base.gif`;
  `convert -depth 8 -size 64x64 gray:tmp.raw $base.gif`;
  print `echo convert -depth 8 -size 64x64 gray:tmp.raw -resize 16x16 ../16x16/$base.gif`;
  `convert -depth 8 -size 64x64 gray:tmp.raw -resize 16x16 ../16x16/$base.gif`;
}
