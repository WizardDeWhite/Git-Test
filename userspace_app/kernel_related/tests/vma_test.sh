#!/bin/bash

pushd tools/testing/vma/
if [[ $? != 0 ]]; then
	echo Error: Not in kernel dir?
	exit
fi
echo === Run vma test
make > /dev/null
./vma
echo === vma test finish
popd
