#!/bin/bash
# Makes a built DesktopStreamer.app self-contained, so it runs on a Mac
# without Homebrew or Qt, and packs it into a .dmg to hand out:
#
#   streamer/package-macos.sh [build-streamer/DesktopStreamer.app]
#
# macdeployqt alone isn't enough with Homebrew's Qt: it resolves a copied
# plugin's @rpath from its new place in the bundle, and skips some frameworks
# (QtDBus, which QtGui needs). The app then picks up Homebrew's Qt as well as
# its own on the build Mac, and fails to start anywhere else. So this also
# copies in what it missed, points the app's rpath at its own Frameworks, and
# checks that nothing in the bundle refers outside it.
#
# The app is signed ad hoc, not with a developer ID, so on another Mac the
# first open has to be right-click > Open (or System Settings > Privacy &
# Security > Open Anyway). It runs on the kind of Mac it was built on; an
# Intel build also runs on Apple silicon, under Rosetta.

set -e

app=${1:-build-streamer/DesktopStreamer.app}
app=$(cd "$(dirname "$app")" && pwd)/$(basename "$app")
exe="$app/Contents/MacOS/DesktopStreamer"
frameworks="$app/Contents/Frameworks"
qtlib=${QT_LIB:-$(brew --prefix qt)/lib}
# where to find a library named only by @rpath: Qt's, then Homebrew's
search="$qtlib $(brew --prefix 2> /dev/null)/lib"

[ -x "$exe" ] || { echo "no app at $app; build it first" >&2; exit 1; }
[ -d "$qtlib/QtCore.framework" ] || { echo "no Qt in $qtlib; set QT_LIB" >&2; exit 1; }

# macdeployqt looks for a copied plugin's Qt in a lib folder beside the app;
# give it one for the duration
link="$(dirname "$app")/lib"
[ -e "$link" ] && { echo "$link is in the way" >&2; exit 1; }
ln -s "$qtlib" "$link"
trap 'rm -f "$link"' EXIT

macdeployqt "$app" -libpath="$qtlib" 2>&1 | grep -v '^$' || true

# every binary in the bundle; a framework's isn't marked executable, so go
# by what's in the file
machos() {
    find "$app" -type f ! -path '*/Headers/*' ! -path '*/Resources/*' -print0 \
        | xargs -0 file | awk -F: '/Mach-O/ { print $1 }'
}

# what a binary loads, leaving out a library's own name, which otool -L also
# lists
deps() {
    otool -L "$1" | awk 'NR > 1 { print $1 }' | grep -v -x -F "$(otool -D "$1" | awk 'NR > 1')"
}

# copy in what macdeployqt missed, until nothing is: Qt frameworks named by
# @rpath, and any other library named by an absolute path outside the system
changed=1
while [ $changed = 1 ]; do
    changed=0
    for f in $(machos); do
        # a library copied from outside keeps its old name; name it as ours
        id=$(otool -D "$f" | awk 'NR > 1')
        case "$id" in
            /usr/lib/* | /System/* | @* | "") ;;
            /*) install_name_tool -id "@rpath/$(basename "$id")" "$f" 2> /dev/null ;;
        esac

        for dep in $(deps "$f"); do
            case "$dep" in
                @rpath/*.framework/*)
                    name=${dep#@rpath/}
                    name=${name%%.framework/*}
                    if [ ! -d "$frameworks/$name.framework" ]; then
                        echo "adding $name.framework"
                        ditto "$qtlib/$name.framework" "$frameworks/$name.framework"
                        changed=1
                    fi
                    ;;
                @rpath/*)
                    lib=${dep#@rpath/}
                    if [ ! -f "$frameworks/$lib" ]; then
                        src=$(for d in $search; do [ -f "$d/$lib" ] && echo "$d/$lib" && break; done)
                        [ -n "$src" ] || { echo "can't find $lib (needed by $f)" >&2; exit 1; }
                        echo "adding $lib"
                        cp -L "$src" "$frameworks/$lib"
                        chmod u+w "$frameworks/$lib"
                        install_name_tool -id "@rpath/$lib" "$frameworks/$lib" 2> /dev/null
                        changed=1
                    fi
                    ;;
                /usr/lib/* | /System/*)
                    ;;
                /*)
                    lib=$(basename "$dep")
                    if [ ! -f "$frameworks/$lib" ]; then
                        echo "adding $lib"
                        cp "$dep" "$frameworks/$lib"
                        chmod u+w "$frameworks/$lib"
                        install_name_tool -id "@rpath/$lib" "$frameworks/$lib" 2> /dev/null
                        changed=1
                    fi
                    install_name_tool -change "$dep" "@rpath/$lib" "$f" 2> /dev/null
                    ;;
            esac
        done
    done
done

# only the bundle's own Frameworks, not the Qt it was built against
for r in $(otool -l "$exe" | awk '/LC_RPATH/ { getline; getline; print $2 }'); do
    case "$r" in @*) ;; *) install_name_tool -delete_rpath "$r" "$exe" 2> /dev/null ;; esac
done
otool -l "$exe" | grep -q '@executable_path/../Frameworks' \
    || install_name_tool -add_rpath @executable_path/../Frameworks "$exe" 2> /dev/null

codesign --force --deep --sign - "$app"

# everything must come from the bundle or the system
all=$(for f in $(machos); do deps "$f"; done | sort -u)
outside=$(echo "$all" | grep -v -E '^(@|/usr/lib/|/System/)' || true)
missing=$(echo "$all" | sed -n 's|^@rpath/||p' | while read -r lib; do
              [ -e "$frameworks/$lib" ] || echo "$lib"
          done)
if [ -n "$outside$missing" ]; then
    [ -n "$outside" ] && { echo "still refers outside the app:" >&2; echo "$outside" >&2; }
    [ -n "$missing" ] && { echo "missing from the app:" >&2; echo "$missing" >&2; }
    exit 1
fi

dmg="$(dirname "$app")/DesktopStreamer.dmg"
rm -f "$dmg"
hdiutil create -quiet -volname DesktopStreamer -srcfolder "$app" -format UDZO "$dmg"
echo "self-contained: $app"
echo "to hand out: $dmg"
