#!/bin/bash

set -eux

basedir=$(dirname $(realpath $0))

pushd "$basedir"

libdwarf_version=2.2.0
dir=libdwarf-${libdwarf_version}
url="https://github.com/davea42/libdwarf-code/releases/download/v${libdwarf_version}/${dir}.tar.xz"

if [ ! -d $dir ]; then
  wget $url
fi

tar -xJvf ${dir}.tar.xz

pushd $dir

install_dir=$basedir/libdwarf-install
mkdir -p "$install_dir"
./configure --prefix="$install_dir" && make && make install

popd

