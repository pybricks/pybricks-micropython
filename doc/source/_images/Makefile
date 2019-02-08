
SRC_DIR = .
OUT_DIR = ../images

SVG := $(wildcard $(SRC_DIR)/*.svg)
PNG := $(patsubst $(SRC_DIR)/%.svg,$(OUT_DIR)/%.png,$(SVG))

all: $(PNG)

clean:
	rm -f $(PNG)

$(OUT_DIR)/%.png: $(SRC_DIR)/%.svg
	inkscape --file=$< --export-area-drawing --export-png=$@
