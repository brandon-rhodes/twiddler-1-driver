#!/bin/bash

set -ex

sudo apt install libxext-dev libxmu-dev libxmu-headers libxtst-dev xutils-dev

cd ~/src/twiddler
tar xvf a2x.tar.gz
pushd pub/R6untarred/contrib/programs/a2x/
xmkmf
make a2x
cp a2x ~/bin
popd
if ! grep -q sys/ioctl.h twidder.h
then
    sed -i '/sys.types/i#include <sys/ioctl.h>' twiddler.h
fi
sed -i 's/default:$/default: break;/' twid.c
gcc -DGDB=1 twid.c -o twiddler
#echo 'Try: TWID_DEFAULTS=~/src/twiddler/twid_defaults.ini ./twiddler'
sudo TWID_DEFAULTS=~/src/twiddler/twid_defaults.ini PATH=$PATH:/home/brhodes/bin strace ./twiddler -p ttyS0
