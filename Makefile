
help:
	@echo "Use 'make <BRICK>' to build a brick or 'make doc[-pbio]' to build some docs"

.PHONY: doc

doc:
	@$(MAKE) -C doc html

doc-pbio:
	@$(MAKE) -C lib/pbio/doc

ev3dev-host:
	@$(MAKE) -C bricks/ev3dev axtls
	@$(MAKE) -C bricks/ev3dev

clean-ev3dev-host:
	@$(MAKE) -C bricks/ev3dev clean

ev3dev-armel:
	@if [ ! -d bricks/ev3dev/build-armel/ports ]; then \
		bricks/ev3dev/docker/setup.sh armel; \
	fi
	@docker exec --tty pybricks-ev3dev_armel make

clean-ev3dev-armel:
	@if [ -d bricks/ev3dev/build-armel/ports ]; then \
		docker exec --tty pybricks-ev3dev_armel make clean; \
	fi

MOVEHUB:
	@$(MAKE) -C bricks/MOVEHUB
