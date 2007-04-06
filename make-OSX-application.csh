#!/bin/csh -x
#
# This script creates a bare-bones MacOS X application package from a raw executable.
# 
# To turn a raw executable a.out into a MacOS X application foo.app use:
#  make-OSX-application.csh a.out foo
#
# foo.app can be invoked by opening it from the finder or by using the shell command:
#  open foo.app
#
# To invoke the raw executable (say, with command line arguments) use:
#   ./foo.app/Contents/MacOS/foo -arg1 ...
#
# or use the hard link to the executable that this script creates (it has to be a hard link)
#
# you can also create shell alias such as (for tcsh):
#   alias a.out /Applications/foo.app/Contents/MacOS/foo
#
# todo: add icons, create CFBundleVersion from svn:keyword $Rev$
#
if -e $2.app rm -rf $2.app
mkdir $2.app
mkdir $2.app/Contents
mkdir $2.app/Contents/Resources
mkdir $2.app/Contents/MacOS
echo APPLnone > $2.app/Contents/PkgInfo
mv $1 $2.app/Contents/MacOS/$2
# make a hard so we can run the executable from the command line using its original name
# (note: this must be a hard link, or a shell alias)
ln $2.app/Contents/MacOS/$2 $1
chmod 755 $2.app/Contents/MacOS/$2
cat << EOF > $2.app/Contents/info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
        <key>CFBundleName</key>
        <string>$2</string>
        <key>CFBundlePackageType</key>
        <string>APPL</string>
        <key>CFBundleVersion</key>
        <string>1.4</string>
        <key>CFBundleShortVersionString</key>
        <string>1.4</string>
        <key>LSPrefersCarbon</key>
        <true/>
        <key>CFBundleSignature</key>
        <string>none</string>
</dict>
</plist>
EOF
