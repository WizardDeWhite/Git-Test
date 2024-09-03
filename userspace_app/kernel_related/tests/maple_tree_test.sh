#!/bin/bash

pushd tools/testing/radix-tree/
if [[ $? != 0 ]]; then
	echo Error: Not in kernel dir?
	exit
fi
echo === Run maple_tree test
make maple > /dev/null
./maple
echo === maple_tree test finish
popd
