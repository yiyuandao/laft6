#!/bin/bash - 
#===============================================================================
#
#          FILE: compile.sh
# 
#         USAGE: ./compile.sh 
# 
#   DESCRIPTION: 
# 
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: dbgong(kevin), dbgong@biigroup.cn
#  ORGANIZATION: 
#       CREATED: 06/21/2012 10:18:37 AM CST
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

cd tmp
mv modules.order Module.symvers test_entry.mod.* test_entry.o built-in.o ../
mv *.o ../src/
mv `echo .*cmd` ../src/
cd ..

make
mv modules.order Module.symvers test_entry.mod.* test_entry.o built-in.o tmp/
mv src/*.o tmp/
mv `echo src/.*.cmd` tmp/
