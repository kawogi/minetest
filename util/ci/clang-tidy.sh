#! /bin/bash -eu

cmake -B build -DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DENABLE_{GETTEXT,SOUND}=FALSE \
cmake --build build --target GenerateVersion

./util/ci/run-clang-tidy.py \
	-clang-tidy-binary=clang-tidy-9 -p build \
	-quiet -config="$(cat .clang-tidy)" \
	'src/.*'
