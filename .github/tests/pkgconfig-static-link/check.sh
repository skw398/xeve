#!/bin/sh
# Fork-only install/link regression. No FFmpeg or downstream workarounds.
set -eu
export LC_ALL=C

case "$PROFILE" in
    MAIN) name=xeve ;;
    BASE) name=xeveb ;;
    *) echo "ERROR: invalid profile" >&2; exit 1 ;;
esac

workspace=$GITHUB_WORKSPACE
build=$RUNNER_TEMP/xeve-pkgconfig-$PROFILE-$LIBDIR
test_dir=$workspace/candidate/.github/tests/pkgconfig-static-link
mkdir -p "$build"

for variant in baseline candidate; do
    prefix=$build/$variant-install
    cmake -S "$workspace/$variant" -B "$build/$variant-build" \
        -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release -DSET_PROF="$PROFILE" \
        -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_INSTALL_LIBDIR="$LIBDIR"
    cmake --build "$build/$variant-build" --parallel 4
    cmake --install "$build/$variant-build"
    test -f "$prefix/$LIBDIR/$name/lib$name.a"
    test ! -f "$prefix/$LIBDIR/lib$name.a"
    export PKG_CONFIG_LIBDIR=$prefix/$LIBDIR/pkgconfig
    unset PKG_CONFIG_PATH
    pkg-config --validate "$name"
    cflags=$(pkg-config --cflags "$name")
    normal_flags=$(pkg-config --libs "$name")
    static_flags=$(pkg-config --static --libs "$name")
    printf '%s ordinary flags: %s\n' "$variant" "$normal_flags"
    printf '%s static flags: %s\n' "$variant" "$static_flags"

    case "$normal_flags" in
        *"-L$prefix/$LIBDIR/$name"*)
            echo 'ERROR: private search path leaked into ordinary linking' >&2
            exit 1 ;;
    esac

    # Flag splitting is intentional; CI uses prefixes without whitespace.
    gcc $cflags -c "$test_dir/consumer.c" -o "$build/$variant-consumer.o"
    if [ "$variant" = baseline ]; then
        if gcc -static "$build/$variant-consumer.o" $static_flags \
            -o "$build/baseline-static" > "$build/baseline-link.log" 2>&1; then
            echo 'ERROR: unmodified static consumer unexpectedly linked' >&2
            exit 1
        fi
        cat "$build/baseline-link.log"
        grep -F "cannot find -l$name" "$build/baseline-link.log"
    else
        gcc -static "$build/$variant-consumer.o" $static_flags \
            -o "$build/candidate-static"
        "$build/candidate-static"
        echo "ok: $PROFILE/$LIBDIR static consumer links and runs using pkg-config flags"
    fi

    # Check ordinary linking still selects the installed shared library.
    gcc "$build/$variant-consumer.o" $normal_flags \
        -Wl,-rpath,"$prefix/$LIBDIR" -o "$build/$variant-shared"
    ldd "$build/$variant-shared" > "$build/$variant-shared-libraries.txt"
    grep -F "$prefix/$LIBDIR/lib$name.so" "$build/$variant-shared-libraries.txt"
    "$build/$variant-shared"
    echo "ok: $variant $PROFILE/$LIBDIR ordinary consumer links and runs with the shared library"
done
