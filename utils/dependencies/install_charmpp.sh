#!/bin/bash
#####################################################################
# Charm++ Installation Script
#
# Usage:
#   ./install_charmpp.sh
#
# Description:
#   This script clones the Charm++ GitHub repository, builds Charm++
#   with the netlrts-linux-x86_64 backend, and installs it to a 
#   specified local directory.
#####################################################################

# Define installation directory
export CHARMDIR=$PWD/install/charmppNEW

wget https://github.com/charmplusplus/charm/archive/refs/tags/v8.0.0.tar.gz
tar xzf v8.0.0.tar.gz
cd charm-8.0.0
# ./build charm++ netlrts-linux-x86_64 --with-production -j8 --destination=$CHARMDIR

./build charm++ multicore-linux-x86_64 --with-production -j8 --destination=$CHARMDIR
