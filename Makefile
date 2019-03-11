
help:
	@echo "Use 'make <BRICK>' to build a brick or 'make doc[-pbio]' to build some docs"

.PHONY: doc

doc:
	@$(MAKE) -C doc html

doc-pbio:
	@$(MAKE) -C lib/pbio/doc

ev3dev-host:
	@$(MAKE) -C bricks/ev3dev

clean-ev3dev-host: clean-mpy-cross
	@$(MAKE) -C bricks/ev3dev clean

ev3dev-armel:
	@if [ ! -d bricks/ev3dev/build-armel/ports ]; then \
		bricks/ev3dev/docker/setup.sh armel; \
	fi
	@docker exec --tty pybricks-ev3dev_armel make

clean-ev3dev-armel: clean-mpy-cross
	@if [ -d bricks/ev3dev/build-armel/ports ]; then \
		docker exec --tty pybricks-ev3dev_armel make clean; \
	fi

movehub:
	@$(MAKE) -C bricks/MOVEHUB

clean-movehub: clean-mpy-cross
	@$(MAKE) -C bricks/MOVEHUB clean

hub4:
	@$(MAKE) -C bricks/HUB4

clean-hub4: clean-mpy-cross
	@$(MAKE) -C bricks/HUB4 clean	

clean-mpy-cross:
	@$(MAKE) -C ../../mpy-cross clean
