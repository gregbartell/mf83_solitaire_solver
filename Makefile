CMAKE_BUILD_TYPE ?= Debug
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
CMAKE_EXTRA_FLAGS ?=

SRC_DIR := .
BUILD_DIR := build

GEN_CMD := cmake -S${SRC_DIR} -B${BUILD_DIR} -GNinja ${CMAKE_FLAGS} ${CMAKE_EXTRA_FLAGS}
BUILD_TARGET_CMD := cmake --build ${BUILD_DIR} --target

all: mf83

build/.ran-cmake:
	mkdir -p build
	${GEN_CMD}
	touch $@

mf83: build/.ran-cmake
	${BUILD_TARGET_CMD} install

test: mf83
	${BUILD_TARGET_CMD} test

clean: | build/.ran-cmake
	${BUILD_TARGET_CMD} clean

distclean:
	rm -rf build
