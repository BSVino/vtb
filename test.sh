#!/bin/bash

set -o nounset
set -e

ProjectDir=`pwd`
OutputDir="Debug/"
ProjectOutputDir="${ProjectDir}/${OutputDir}"
CommonInclude=""
CommonCPPFlags="-Werror -std=c++11 -g -lc++abi"
CommonCFlags="-Werror -std=c99 -g"
CommonExtDepsFlags=""
CommonLinkerFlags="-L${ProjectOutputDir}"

CommonDebugCPPFlags="${CommonCPPFlags} -D_DEBUG"
CommonReleaseCPPFlags="${CommonCPPFlags} -O2"
CommonDebugCFlags="${CommonCFlags} -D_DEBUG"
CommonReleaseCFlags="${CommonCFlags} -O2"

# TEST VTB_ALLOC_RING
echo "testing vtb_alloc_ring..."
mkdir -p $ProjectOutputDir/o/vtb_alloc_ring

pushd $ProjectOutputDir/o/vtb_alloc_ring > /dev/null

clang $CommonInclude $CommonDebugCFlags $ProjectDir/tests/vtb_alloc_ring.c -o $ProjectOutputDir/o/vtb_alloc_ring_c $CommonLinkerFlags
clang $CommonInclude $CommonDebugCPPFlags $ProjectDir/tests/vtb_alloc_ring.cpp -o $ProjectOutputDir/o/vtb_alloc_ring_cpp $CommonLinkerFlags
clang $CommonInclude $CommonDebugCPPFlags -DVTBAR_NO_MALLOC $ProjectDir/tests/vtb_alloc_ring.cpp -o $ProjectOutputDir/o/vtb_alloc_ring_cpp_nomalloc $CommonLinkerFlags

echo "vtb_alloc_ring_c..."
$ProjectOutputDir/o/vtb_alloc_ring_c || exit

echo "vtb_alloc_ring_cpp..."
$ProjectOutputDir/o/vtb_alloc_ring_cpp || exit

echo "vtb_alloc_ring_cpp_nomalloc..."
$ProjectOutputDir/o/vtb_alloc_ring_cpp_nomalloc || exit

popd > /dev/null

echo "ALL TESTS PASS"

