
help:
	@echo "Use 'make <BRICK>' to build a brick or 'make doc-<name>' to build some docs"

doc: doc-pybricks doc-pbio

doc-pybricks:
	$(MAKE) -C doc/micropython html

doc-pbio:
	$(MAKE) -C doc/pbio

EV3:
	$(MAKE) -C bricks/MOVEHUB

MOVEHUB:
	$(MAKE) -C bricks/MOVEHUB
