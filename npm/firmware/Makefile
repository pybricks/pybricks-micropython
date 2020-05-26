
BUILD_DIR := build
MICROPYTHON_DIR := ../../micropython
BRICKS_DIR := ../../bricks

all:
	$(MAKE) -C $(MICROPYTHON_DIR)/mpy-cross
	mkdir -p $(BUILD_DIR)
	$(MAKE) -C $(BRICKS_DIR)/movehub
	cp $(BRICKS_DIR)/movehub/build/firmware.zip $(BUILD_DIR)/movehub.zip
	$(MAKE) -C $(BRICKS_DIR)/cplushub
	cp $(BRICKS_DIR)/cplushub/build/firmware.zip $(BUILD_DIR)/cplushub.zip
