BUILD := build

MICROPYTHON_DIR := ../../micropython

SRC = $(addprefix $(MICROPYTHON_DIR)/, \
	py/mpstate.c \
	py/nlr.c \
	py/nlrx86.c \
	py/nlrx64.c \
	py/nlrthumb.c \
	py/nlrpowerpc.c \
	py/nlrxtensa.c \
	py/nlrsetjmp.c \
	py/malloc.c \
	py/gc.c \
	py/pystack.c \
	py/qstr.c \
	py/vstr.c \
	py/mpprint.c \
	py/unicode.c \
	py/mpz.c \
	py/reader.c \
	py/lexer.c \
	py/parse.c \
	py/scope.c \
	py/compile.c \
	py/emitcommon.c \
	py/emitbc.c \
	py/asmbase.c \
	py/asmx64.c \
	py/emitnx64.c \
	py/asmx86.c \
	py/emitnx86.c \
	py/asmthumb.c \
	py/emitnthumb.c \
	py/emitinlinethumb.c \
	py/asmarm.c \
	py/emitnarm.c \
	py/asmxtensa.c \
	py/emitnxtensa.c \
	py/emitinlinextensa.c \
	py/emitnxtensawin.c \
	py/formatfloat.c \
	py/parsenumbase.c \
	py/parsenum.c \
	py/emitglue.c \
	py/persistentcode.c \
	py/runtime.c \
	py/runtime_utils.c \
	py/scheduler.c \
	py/nativeglue.c \
	py/pairheap.c \
	py/ringbuf.c \
	py/stackctrl.c \
	py/argcheck.c \
	py/warning.c \
	py/profile.c \
	py/map.c \
	py/obj.c \
	py/objarray.c \
	py/objattrtuple.c \
	py/objbool.c \
	py/objboundmeth.c \
	py/objcell.c \
	py/objclosure.c \
	py/objcomplex.c \
	py/objdeque.c \
	py/objdict.c \
	py/objenumerate.c \
	py/objexcept.c \
	py/objfilter.c \
	py/objfloat.c \
	py/objfun.c \
	py/objgenerator.c \
	py/objgetitemiter.c \
	py/objint.c \
	py/objint_longlong.c \
	py/objint_mpz.c \
	py/objlist.c \
	py/objmap.c \
	py/objmodule.c \
	py/objobject.c \
	py/objpolyiter.c \
	py/objproperty.c \
	py/objnone.c \
	py/objnamedtuple.c \
	py/objrange.c \
	py/objreversed.c \
	py/objset.c \
	py/objsingleton.c \
	py/objslice.c \
	py/objstr.c \
	py/objstrunicode.c \
	py/objstringio.c \
	py/objtuple.c \
	py/objtype.c \
	py/objzip.c \
	py/opmethods.c \
	py/sequence.c \
	py/stream.c \
	py/binary.c \
	py/builtinimport.c \
	py/builtinevex.c \
	py/builtinhelp.c \
	py/modarray.c \
	py/modbuiltins.c \
	py/modcollections.c \
	py/modgc.c \
	py/modio.c \
	py/modmath.c \
	py/modcmath.c \
	py/modmicropython.c \
	py/modstruct.c \
	py/modsys.c \
	py/moduerrno.c \
	py/modthread.c \
	py/vm.c \
	py/bc.c \
	py/showbc.c \
	py/repl.c \
	py/smallint.c \
	py/frozenmod.c \
	mpy-cross/main.c \
	mpy-cross/gccollect.c \
	lib/utils/gchelper_generic.c \
	)

MPY_CROSS_BUILD := $(MICROPYTHON_DIR)/mpy-cross/build

INC = -I$(MPY_CROSS_BUILD) -I$(MICROPYTHON_DIR)/mpy-cross -I$(MICROPYTHON_DIR)
COPT = -std=gnu99 -m32
ifeq ($(DEBUG),1)
COPT += -g4 -O0 --js-opts 0 --source-map-base http://localhost:8000/mpy-cross/
else
COPT += -Oz
endif
COPT += -s MODULARIZE=1 -s EXPORT_NAME=MpyCross -s EXIT_RUNTIME=1


all: $(BUILD)/mpy-cross.js

# Compile mpy-cross to web assembly using Emscripten
$(BUILD)/mpy-cross.js: $(SRC) mpy-cross.pre.js $(MPY_CROSS_BUILD)
	mkdir -p $(dir $@)
	emcc $(COPT) $(INC) --pre-js mpy-cross.pre.js $(SRC) -o $@

# Build mpy-cross for host OS first - mainly just to get $(MPY_CROSS_BUILD)/genhdr
$(MPY_CROSS_BUILD):
	$(MAKE) -C $(MICROPYTHON_DIR)/mpy-cross

clean:
	rm -rf $(BUILD)
