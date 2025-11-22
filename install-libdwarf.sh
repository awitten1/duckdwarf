#!/bin/bash

set -eux

basedir=$(dirname $(realpath $0))

pushd "$basedir"

libdwarf_version=2.1.0
dir=libdwarf-${libdwarf_version}
url="https://github.com/davea42/libdwarf-code/releases/download/v${libdwarf_version}/${dir}.tar.xz"

if [ ! -d $dir ]; then
  wget $url
fi

tar -xJvf ${dir}.tar.xz

install_dir=${1:-$basedir/libdwarf-install}
mkdir -p "$install_dir"
install_dir=$(realpath "$install_dir")
echo "installing libdwarf into $install_dir"
(cd $dir && ./configure --prefix="$install_dir" && make && make install)

popd

