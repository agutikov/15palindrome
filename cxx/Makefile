
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))

SEASTAR_PATH := $(abspath $(mkfile_dir)/../../seastar)
SEASTAR_PC := $(abspath $(SEASTAR_PATH)/build/release/seastar.pc)
SEASTAR_CXX_FLAGS := $(shell export SEASTAR_PC="$(SEASTAR_PC)"; pkg-config --cflags --static ${SEASTAR_PC})
SEASTAR_LD_FLAGS := $(shell export SEASTAR_PC="$(SEASTAR_PC)"; pkg-config --libs --static ${SEASTAR_PC})

PROJ_DIR := $(abspath $(mkfile_di)/../)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3 -gdwarf-2 -D_GNU_SOURCE -I${PROJ_DIR} ${SEASTAR_CXX_FLAGS}
LDFLAGS = -lprimesieve ${SEASTAR_LD_FLAGS}

DEPS = Makefile ../15palindrome.h

APPS = 15palindrome

all: ${APPS}

${APPS}: %: %.cpp ${DEPS}
	${CXX} ${CXXFLAGS} $@.cpp -o $@ ${LDFLAGS} 

.PHONY: clean
clean:
	rm -f ${APPS}
