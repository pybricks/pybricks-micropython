# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2020 The Pybricks Authors

# ensure micropython submodule is checked out for building mpy-cross
ifeq ("$(wildcard micropython/README.md)","")
$(info GIT cloning micropython submodule)
$(info $(shell git submodule update --init micropython))
ifeq ("$(wildcard micropython/README.md)","")
$(error failed)
endif
endif

help:
	@echo "Use 'make <BRICK>' to build a brick."

.PHONY: doc

doc:
	@$(MAKE) -C lib/pbio/doc

clean-doc:
	@$(MAKE) -C lib/pbio/doc clean

all: movehub cityhub cplushub primehub nxt ev3dev-armel doc

clean-all: clean-movehub clean-cityhub clean-cplushub clean-primehub clean-nxt clean-ev3dev-armel clean-doc

ev3dev-host: mpy-cross
	@$(MAKE) -C bricks/ev3dev CROSS_COMPILE=

clean-ev3dev-host: clean-mpy-cross
	@$(MAKE) -C bricks/ev3dev clean CROSS_COMPILE=

ev3dev-armel:
	@if [ ! -d bricks/ev3dev/build-armel/ports ]; then \
		bricks/ev3dev/docker/setup.sh armel; \
	fi
	@docker start pybricks-ev3dev_armel
	@docker exec --tty pybricks-ev3dev_armel make -C ../../micropython/mpy-cross CROSS_COMPILE=
	@docker exec --tty pybricks-ev3dev_armel make

clean-ev3dev-armel:
	@if [ -d bricks/ev3dev/build-armel/ports ]; then \
		@docker start pybricks-ev3dev_armel; \
		docker exec --tty pybricks-ev3dev_armel make -C ../../micropython/mpy-cross clean CROSS_COMPILE=; \
		docker exec --tty pybricks-ev3dev_armel make clean; \
	fi

ev3rt: mpy-cross
	@if [ ! -d bricks/ev3rt/build/ports ]; then \
		bricks/ev3rt/docker/setup.sh; \
	fi
	@docker exec --tty pybricks-ev3rt bash -c 'make && mv libmicropython.a build/'
	@docker exec --tty pybricks-ev3rt bash -c '\
		cd ev3rt-hrp2/sdk/workspace && \
		make img=pybricks && \
		cd ../../../ && \
		cp ev3rt-hrp2/sdk/workspace/uImage build/ \
	'

movehub: mpy-cross
	@$(MAKE) -C bricks/movehub

clean-movehub: clean-mpy-cross
	@$(MAKE) -C bricks/movehub clean

cityhub: mpy-cross
	@$(MAKE) -C bricks/cityhub

clean-cityhub: clean-mpy-cross
	@$(MAKE) -C bricks/cityhub clean

cplushub: mpy-cross
	@$(MAKE) -C bricks/cplushub

clean-cplushub: clean-mpy-cross
	@$(MAKE) -C bricks/cplushub clean

nxt: mpy-cross
	@$(MAKE) -C bricks/nxt

clean-nxt: clean-mpy-cross
	@$(MAKE) -C bricks/nxt clean

primehub: mpy-cross
	@$(MAKE) -C bricks/primehub

clean-primehub: clean-mpy-cross
	@$(MAKE) -C bricks/primehub clean

mpy-cross:
	@$(MAKE) -C micropython/mpy-cross CROSS_COMPILE=

clean-mpy-cross:
	@$(MAKE) -C micropython/mpy-cross clean CROSS_COMPILE=
