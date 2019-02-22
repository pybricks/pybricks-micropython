
SRC_DIR = .
OUT_DIR = ../images

SVG := $(wildcard $(SRC_DIR)/*.svg)
PNG := $(patsubst $(SRC_DIR)/%.svg,$(OUT_DIR)/%.png,$(SVG))
DIR := ${CURDIR}

all: $(PNG)

clean:
	rm -f $(PNG)

$(OUT_DIR)/%.png: $(SRC_DIR)/%.svg
	inkscape --file=$(DIR)/$< --export-area-drawing --export-png=$(DIR)/$@
