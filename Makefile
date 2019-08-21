
help:
	@echo "Use 'make <BRICK>' to build a brick or 'make doc[-pbio]' to build some docs"

.PHONY: doc

doc:
	@$(MAKE) -C doc html

doc-pbio:
	@$(MAKE) -C lib/pbio/doc

ev3dev-host:
	@$(MAKE) -C bricks/ev3dev CROSS_COMPILE=

clean-ev3dev-host: clean-mpy-cross
	@$(MAKE) -C bricks/ev3dev clean CROSS_COMPILE=

ev3dev-armel:
	@if [ ! -d bricks/ev3dev/build-armel/ports ]; then \
		bricks/ev3dev/docker/setup.sh armel; \
	fi
	@docker exec --tty pybricks-ev3dev_armel make

clean-ev3dev-armel: clean-mpy-cross
	@if [ -d bricks/ev3dev/build-armel/ports ]; then \
		docker exec --tty pybricks-ev3dev_armel make clean; \
	fi

ev3rt:
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

movehub:
	@$(MAKE) -C bricks/movehub

clean-movehub: clean-mpy-cross
	@$(MAKE) -C bricks/movehub clean

cityhub:
	@$(MAKE) -C bricks/cityhub

clean-cityhub: clean-mpy-cross
	@$(MAKE) -C bricks/cityhub clean

cplushub:
	@$(MAKE) -C bricks/cplushub

clean-cplushub: clean-mpy-cross
	@$(MAKE) -C bricks/cplushub clean

clean-mpy-cross:
	@$(MAKE) -C ../../mpy-cross clean
