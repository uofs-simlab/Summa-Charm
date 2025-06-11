#!/bin/bash
#####################################################################
# Summa-Actors and Summa Build Script
#
# Usage:
#   ./install_summa.sh
#
# Description:
#   This script clones the Summa-Actors project, fetches the SUMMA
#   dependency from a development branch, and builds the code using
#   the provided script.
#####################################################################

export SUMMADIR=$PWD/../../build

cd $SUMMADIR
git clone -b develop https://github.com/ashleymedin/summa.git
