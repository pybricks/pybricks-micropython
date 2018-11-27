
help:
	@echo "Use 'make <BRICK>' to build a brick or 'make doc[-pbio]' to build some docs"

.PHONY: doc

doc:
	@$(MAKE) -C doc html

doc-pbio:
	@$(MAKE) -C lib/pbio/doc

ev3dev:
	@$(MAKE) -C bricks/ev3dev axtls
	@$(MAKE) -C bricks/ev3dev

clean-ev3dev:
	@$(MAKE) -C bricks/ev3dev clean

MOVEHUB:
	@$(MAKE) -C bricks/MOVEHUB
