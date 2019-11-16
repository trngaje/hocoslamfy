#!/bin/sh

mkdir -p ipkg/home/retrofw/games/hocoslamfy
mkdir -p ipkg/home/retrofw/apps/gmenu2x/sections/games
cp data/* ipkg/home/retrofw/games/hocoslamfy/
cp hocoslamfy-od ipkg/home/retrofw/games/hocoslamfy/hocoslamfy
cp COPYRIGHT ipkg/home/retrofw/games/hocoslamfy/
mv ipkg/home/retrofw/games/hocoslamfy/manual-en.txt ipkg/home/retrofw/games/hocoslamfy/hocoslamfy.man.txt
rm ipkg/home/retrofw/games/hocoslamfy/default.gcw0.desktop

cd ipkg

# create control
cat > temp <<EOF
Package: hocoslamfy
Version: 
Description: Avoid the bamboo while flying with your bee
Section: games
Priority: optional
Maintainer: scooterpsu
Architecture: mipsel
Homepage: https://github.com/scooterpsu/hocoslamfy
Depends:
Source: https://github.com/scooterpsu/hocoslamfy
EOF
sed "s/^Version:.*/Version: $(date +%Y%m%d)/" temp > control

# create debian-binary
echo '2.0' > debian-binary

# create gmenu2x links
cat > home/retrofw/apps/gmenu2x/sections/games/hocoslamfy.lnk <<EOF
title=hocoslamfy
description=Avoid the bamboo while flying with your bee
exec=/home/retrofw/games/hocoslamfy/hocoslamfy
clock=600
EOF

#build ipk
tar -czvf control.tar.gz control --owner=0 --group=0
tar -czvf data.tar.gz home --owner=0 --group=0
ar rv ../hocoslamfy.ipk control.tar.gz data.tar.gz debian-binary

cd ..
rm -r ipkg
