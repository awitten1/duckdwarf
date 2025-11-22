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

install_dir=$(realpath ${1:-$basedir/libdwarf-install})
mkdir -p $install_dir 
echo "installing libdwarf into $install_dir"
(cd $dir && ./configure --prefix="$install_dir" && make && make install)

popd

