
help:
	@echo "Use 'make <BRICK>' to build a brick or 'make doc[-pbio]' to build some docs"

.PHONY: doc

doc:
	@$(MAKE) -C doc html

doc-pbio:
	@$(MAKE) -C lib/pbio/doc

EV3:
	@$(MAKE) -C bricks/EV3 axtls
	@$(MAKE) -C bricks/EV3

MOVEHUB:
	@$(MAKE) -C bricks/MOVEHUB
