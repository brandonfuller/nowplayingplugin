#!/bin/sh

cd Src

# See version
agvtool what-version
agvtool what-marketing-version

# Build the project
xcodebuild -configuration Release clean build

# Build the installer
freeze -v NowPlaying.packproj

# App store fail
#productbuild --component 'build/Release/Library/iTunes/iTunes Plug-ins/Now Playing.bundle' 'Library/iTunes/iTunes Plug-ins' --sign "3rd Party Mac Developer Installer: Brandon Fuller" build/installer.pkg

cd ..
