#
#  TOPPERS/HRP Kernel
#      Toyohashi Open Platform for Embedded Real-Time Systems/
#      High Reliable system Profile Kernel
# 
#  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
#                              Toyohashi Univ. of Technology, JAPAN
#  Copyright (C) 2006-2019 by Embedded and Real-Time Systems Laboratory
#              Graduate School of Information Science, Nagoya Univ., JAPAN
# 
#  上記著作権者は，以下の(1)〜(4)の条件を満たす場合に限り，本ソフトウェ
#  ア（本ソフトウェアを改変したものを含む．以下同じ）を使用・複製・改
#  変・再配布（以下，利用と呼ぶ）することを無償で許諾する．
#  (1) 本ソフトウェアをソースコードの形で利用する場合には，上記の著作
#      権表示，この利用条件および下記の無保証規定が，そのままの形でソー
#      スコード中に含まれていること．
#  (2) 本ソフトウェアを，ライブラリ形式など，他のソフトウェア開発に使
#      用できる形で再配布する場合には，再配布に伴うドキュメント（利用
#      者マニュアルなど）に，上記の著作権表示，この利用条件および下記
#      の無保証規定を掲載すること．
#  (3) 本ソフトウェアを，機器に組み込むなど，他のソフトウェア開発に使
#      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
#      と．
#    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
#        作権表示，この利用条件および下記の無保証規定を掲載すること．
#    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
#        報告すること．
#  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
#      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
#      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
#      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを
#      免責すること．
# 
#  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
#  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的
#  に対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェ
#  アの利用により直接的または間接的に生じたいかなる損害に関しても，そ
#  の責任を負わない．
# 
#  $Id: Makefile 650 2019-01-14 07:57:07Z ertl-hiro $
# 

SRCDIR := $(EV3RT_LIB_DIR)
APPLDIR := $(EV3RT_BRICK_DIR)

#
# Functions
#
define print_cmd
  @printf "  %-7s %s\n" $1 $2 1>&2
endef

#
#  ターゲットの指定（Makefile.targetで上書きされるのを防ぐため）
#
all:

#
#  ターゲット略称の定義
#
TARGET = ev3_gcc

#
#  プログラミング言語の定義
#
SRCLANG = c
ifeq ($(SRCLANG),c)
	LIBS = -lc
endif
ifeq ($(SRCLANG),c++)
	USE_CXX = true
	CXXLIBS = -lstdc++ -lm -lc
	CXXRTS = cxxrt.o newlibrt.o
endif

#
#  ソースファイルのディレクトリの定義
#

#
#  オブジェクトファイル名の拡張子の設定
#
OBJEXT = 

#
#  カーネルライブラリ（libkernel.a）のディレクトリ名
#  （カーネルライブラリもmake対象にする時は，空に定義する）
#
KERNEL_LIB = $(APPLDIR)/build

#
#  カーネルを関数単位でコンパイルするかどうかの定義
#
KERNEL_FUNCOBJS = 

#
#  TECSを外すかどうかの定義
#
OMIT_TECS = true

#
#  TECS関係ファイルのディレクトリの定義
#
TECSDIR = $(SRCDIR)/tecsgen

#
#  トレースログを取得するかどうかの定義
#
ENABLE_TRACE = 

#
#  開発ツール（コンパイラ等）のディレクトリの定義
#
DEVTOOLDIR = 

#
#  ユーティリティプログラムの名称
#
CFG = ruby $(SRCDIR)/cfg/cfg.rb
TECSGEN = ruby $(TECSDIR)/tecsgen.rb

#
#  オブジェクトファイル名の定義
#
OBJNAME = hrp
ifdef OBJEXT
	OBJFILE = $(OBJNAME).$(OBJEXT)
	CFG1_OUT = cfg1_out.$(OBJEXT)
	CFG2_OUT = cfg2_out.$(OBJEXT)
	CFG3_OUT = cfg3_out.$(OBJEXT)
else
	OBJFILE = $(OBJNAME)
	CFG1_OUT = cfg1_out
	CFG2_OUT = cfg2_out
	CFG3_OUT = cfg3_out
endif

#
#  中間オブジェクトファイルと依存関係ファイルを置くディレクトリの定義
#
OBJDIR = objs
DEPDIR = objs

#
#  ターゲット依存部のディレクトリの定義
#
TARGETDIR = $(SRCDIR)/target/$(TARGET)

#
#  ターゲット依存の定義のインクルード
#
include $(TARGETDIR)/Makefile.target

#
#  TECS生成ファイルのディレクトリの定義
#
TECSGENDIR = ./gen
ifndef OMIT_TECS
	TECSGEN_TIMESTAMP = $(TECSGENDIR)/tecsgen.timestamp
	INIT_TECS_COBJ = init_tecs.o
endif

#
#  TECSが生成する定義のインクルード
#
ifndef OMIT_TECS
	GEN_DIR = $(TECSGENDIR)
	-include $(TECSGENDIR)/Makefile.tecsgen
endif

#
#  共通コンパイルオプションの定義
#
COPTS := -g  $(COPTS)
ifndef OMIT_WARNING_ALL
	COPTS := -Wall $(COPTS)
endif
ifndef OMIT_OPTIMIZATION
	COPTS := -O2 $(COPTS)
endif
ifdef OMIT_TECS
	CDEFS := -DTOPPERS_OMIT_TECS $(CDEFS)
endif
ifdef USE_CFG_PASS3
	COPTS := -DUSE_CFG_PASS3 $(COPTS)
endif
CDEFS := $(CDEFS) 
INCLUDES := -I. -I$(SRCDIR)/include $(INCLUDES) -I$(SRCDIR) -I$(APPLDIR)
LDFLAGS := $(LDFLAGS) -L$(APPLDIR)
LIBS := $(LIBS) $(CXXLIBS)
CFLAGS = $(COPTS) $(CDEFS) $(INCLUDES)

#
#  アプリケーションプログラムに関する定義
#
APPLNAME = app
APPL_CFG = app.cfg
APPL_CDL = app.cdl

APPL_DIRS := $(APPLDIRS) $(SRCDIR)/library
APPL_ASMOBJS :=
ifdef USE_CXX
	APPL_CXXOBJS := app.o
	APPL_COBJS +=
else
	APPL_COBJS += app.o
endif
APPL_COBJS := $(APPL_COBJS) $(TECS_USER_COBJS) $(TECS_OUTOFDOMAIN_COBJS) \
				log_output.o vasyslog.o t_perror.o strerror.o 
APPL_CFLAGS := $(APPL_CFLAGS)
ifdef APPLDIRS
	INCLUDES := $(INCLUDES) $(foreach dir,$(APPLDIRS),-I$(dir))
endif

#
# Common EV3RT Paths
#
EV3RT_SDK_COM_DIR := $(EV3RT_LIB_DIR)/sdk/common
INCLUDES += -I$(EV3RT_SDK_COM_DIR)

#
# EV3RT C Language API
#
EV3RT_SDK_API_DIR := $(EV3RT_LIB_DIR)/sdk/common/ev3api
APPL_DIRS += $(EV3RT_SDK_API_DIR)/src
INCLUDES += -I$(EV3RT_SDK_API_DIR) -I$(EV3RT_SDK_API_DIR)/include
include $(EV3RT_SDK_API_DIR)/Makefile

#
# Static libraries
#
EV3RT_SDK_LIB_DIR := $(EV3RT_LIB_DIR)/sdk/common/library

#
# Add include/ and src/ under application directory to search path
#
INCLUDES += $(foreach dir,$(shell find $(APPLDIRS) -type d -name include),-I$(dir))
APPL_DIRS += $(foreach dir,$(shell find $(APPLDIRS) -type d -name src),$(dir))


#
# For EV3RT
#
CONFIG_EV3RT_APPLICATION = 0

#
#  システムサービスに関する定義
#
SYSSVC_DIRS := $(TECSGENDIR) $(SRCDIR)/tecs_kernel \
				$(SYSSVC_DIRS) $(SRCDIR)/syssvc \
				$(TECSDIR)/tecs $(TECSDIR)/tecs/rpc
SYSSVC_ASMOBJS := $(SYSSVC_ASMOBJS)
SYSSVC_COBJS := $(SYSSVC_COBJS) banner.o syslog.o serial.o logtask.o $(TECS_KERNEL_COBJS) \
				prb_str.o $(INIT_TECS_COBJ) $(CXXRTS)
SYSSVC_CFLAGS := $(SYSSVC_CFLAGS) -DTOPPERS_SVC_CALL
INCLUDES := $(INCLUDES) -I$(TECSGENDIR) -I$(SRCDIR)/tecs_kernel \
				-I$(TECSDIR)/tecs -I$(TECSDIR)/tecs/rpc

#
#  トレースログ記録のサンプルコードに関する定義
#
ifdef ENABLE_TRACE
	COPTS := $(COPTS) -DTOPPERS_ENABLE_TRACE
	SYSSVC_DIRS := $(SYSSVC_DIRS) $(SRCDIR)/arch/tracelog
endif

#
#  ターゲットファイル
#
.PHONY: all
ifndef OMIT_TECS
all: tecs
	@$(MAKE) kernel_cfg.h $(OFFSET_H) check
#	@$(MAKE) kernel_cfg.h $(OFFSET_H) check $(OBJNAME).bin
#	@$(MAKE) kernel_cfg.h $(OFFSET_H) check $(OBJNAME).srec
else
all: kernel_cfg.h $(OFFSET_H) check
#all: kernel_cfg.h $(OFFSET_H) check $(OBJNAME).bin
#all: kernel_cfg.h $(OFFSET_H) check $(OBJNAME).srec
endif

##### 以下は編集しないこと #####

#
#  コンフィギュレータに関する定義
#
CFG_KERNEL := --kernel hrp
CFG_TABS := --api-table $(SRCDIR)/kernel/kernel_api.def \
			--symval-table $(SRCDIR)/kernel/kernel_sym.def $(CFG_TABS)
CFG_ASMOBJS := $(CFG_ASMOBJS)
CFG_COBJS := kernel_cfg.o kernel_mem.o $(CFG_COBJS)
CFG_CFLAGS := -DTOPPERS_CB_TYPE_ONLY $(CFG_CFLAGS)

CFG2_OUT_SRCS := kernel_cfg.h kernel_cfg.c kernel_mem2.c $(CFG2_OUT_SRCS)
CFG2_COBJS := kernel_cfg.o kernel_mem2.o $(CFG2_COBJS)
CFG2_ASMOBJS := $(CFG2_ASMOBJS)

CFG3_OUT_SRCS := kernel_mem3.c $(CFG3_OUT_SRCS)
CFG3_COBJS := kernel_cfg.o kernel_mem3.o $(CFG3_COBJS)
CFG3_ASMOBJS := $(CFG3_ASMOBJS)

#
#  カーネルに関する定義
#
#  KERNEL_ASMOBJS: カーネルライブラリに含める，ソースがアセンブリ言語の
#				   オブジェクトファイル．
#  KERNEL_COBJS: カーネルのライブラリに含める，ソースがC言語で，ソース
#				 ファイルと1対1に対応するオブジェクトファイル．
#  KERNEL_LCSRCS: カーネルのライブラリに含めるC言語のソースファイルで，
#				  1つのソースファイルから複数のオブジェクトファイルを生
#				  成するもの．
#  KERNEL_LCOBJS: 上のソースファイルから生成されるオブジェクトファイル．
#
KERNEL_DIRS := $(KERNEL_DIRS) $(SRCDIR)/kernel
KERNEL_ASMOBJS := $(KERNEL_ASMOBJS)
KERNEL_COBJS := $(KERNEL_COBJS) svc_table.o
KERNEL_CFLAGS := $(KERNEL_CFLAGS) -I$(SRCDIR)/kernel

#
#  カーネルのファイル構成の定義
#
include $(SRCDIR)/kernel/Makefile.kernel
ifdef KERNEL_FUNCOBJS
	KERNEL_LCSRCS := $(KERNEL_FCSRCS)
	KERNEL_LCOBJS := $(foreach file,$(KERNEL_FCSRCS),$($(file:.c=)))
else
	KERNEL_CFLAGS := -DALLFUNC $(KERNEL_CFLAGS)
	KERNEL_COBJS := $(KERNEL_COBJS) \
					$(foreach file,$(KERNEL_FCSRCS),$(file:.c=.o))
endif
ifdef TARGET_OFFSET_TRB
	OFFSET_H = offset.h
endif
ifndef TARGET_KERNEL_TRB
	TARGET_KERNEL_TRB := $(TARGETDIR)/target_kernel.trb
endif
ifndef TARGET_OPT_TRB
	TARGET_OPT_TRB := $(TARGETDIR)/target_opt.trb
endif
ifndef TARGET_MEM_TRB
	TARGET_MEM_TRB := $(TARGETDIR)/target_mem.trb
endif
ifndef TARGET_KERNEL_CFG
	TARGET_KERNEL_CFG := $(TARGETDIR)/target_kernel.cfg
endif

ifndef LDSCRIPT
	LDSCRIPT = ldscript.ld
endif
ifndef CFG2_OUT_LDSCRIPT
	CFG2_OUT_LDSCRIPT = cfg2_out.ld
endif
ifndef CFG3_OUT_LDSCRIPT
	CFG3_OUT_LDSCRIPT = cfg3_out.ld
endif

#
#  Add all 'include' and 'src' by default
#
INCLUDES += $(foreach dir,$(shell find $(TARGETDIR) -type d -name include),-I$(dir))
KERNEL_DIRS += $(foreach dir,$(shell find $(TARGETDIR) -type d -name src),$(dir))

#
#  ソースファイルのあるディレクトリに関する定義
#
vpath %.c $(KERNEL_DIRS) $(SYSSVC_DIRS) $(APPL_DIRS)
vpath %.S $(KERNEL_DIRS) $(SYSSVC_DIRS) $(APPL_DIRS)
vpath %.cfg $(APPL_DIRS)
vpath %.cdl $(APPL_DIRS)

#
#  中間オブジェクトファイルを置くディレクトリの処理
#
LDFLAGS := -L $(OBJDIR) $(LDFLAGS) 
APPL_ASMOBJS   := $(addprefix $(OBJDIR)/, $(APPL_ASMOBJS))
APPL_CXXOBJS   := $(addprefix $(OBJDIR)/, $(APPL_CXXOBJS))
APPL_COBJS     := $(addprefix $(OBJDIR)/, $(APPL_COBJS))
SYSSVC_ASMOBJS := $(addprefix $(OBJDIR)/, $(SYSSVC_ASMOBJS))
SYSSVC_COBJS   := $(addprefix $(OBJDIR)/, $(SYSSVC_COBJS))
KERNEL_ASMOBJS := $(addprefix $(OBJDIR)/, $(KERNEL_ASMOBJS))
KERNEL_COBJS   := $(addprefix $(OBJDIR)/, $(KERNEL_COBJS))
KERNEL_LCOBJS  := $(addprefix $(OBJDIR)/, $(KERNEL_LCOBJS))
CFG_ASMOBJS    := $(addprefix $(OBJDIR)/, $(CFG_ASMOBJS))
CFG_COBJS      := $(addprefix $(OBJDIR)/, $(CFG_COBJS))
CFG2_ASMOBJS   := $(addprefix $(OBJDIR)/, $(CFG2_ASMOBJS))
CFG2_COBJS     := $(addprefix $(OBJDIR)/, $(CFG2_COBJS))
CFG3_ASMOBJS   := $(addprefix $(OBJDIR)/, $(CFG3_ASMOBJS))
CFG3_COBJS     := $(addprefix $(OBJDIR)/, $(CFG3_COBJS))

#
#  コンパイルのための変数の定義
#
APPL_OBJS = $(APPL_ASMOBJS) $(APPL_COBJS) $(APPL_CXXOBJS)
SYSSVC_OBJS = $(SYSSVC_ASMOBJS) $(SYSSVC_COBJS)
KERNEL_LIB_OBJS = $(KERNEL_ASMOBJS) $(KERNEL_COBJS) $(KERNEL_LCOBJS)
CFG_OBJS = $(CFG_ASMOBJS) $(CFG_COBJS)
CFG2_OBJS = $(CFG2_ASMOBJS) $(CFG2_COBJS)
CFG3_OBJS = $(CFG3_ASMOBJS) $(CFG3_COBJS)
ALL2_OBJS = $(START_OBJS) $(APPL_OBJS) $(SYSSVC_OBJS) $(CFG2_OBJS) \
											$(END_OBJS) $(HIDDEN_OBJS)
ALL3_OBJS = $(START_OBJS) $(APPL_OBJS) $(SYSSVC_OBJS) $(CFG3_OBJS) \
											$(END_OBJS) $(HIDDEN_OBJS)
ALL_OBJS = $(START_OBJS) $(APPL_OBJS) $(SYSSVC_OBJS) $(CFG_OBJS) \
											$(END_OBJS) $(HIDDEN_OBJS)
ALL_LIBS = -lkernel $(LIBS)
ifdef KERNEL_LIB
	LIBS_DEP = $(KERNEL_LIB)/libkernel.a $(filter %.a,$(LIBS))
	OBJ_LDFLAGS := $(OBJ_LDFLAGS) -L$(KERNEL_LIB)
	CFG2_OUT_LDFLAGS := $(CFG2_OUT_LDFLAGS) -L$(KERNEL_LIB)
	CFG3_OUT_LDFLAGS := $(CFG3_OUT_LDFLAGS) -L$(KERNEL_LIB)
	REALCLEAN_FILES := libkernel.a $(REALCLEAN_FILES)
else
	LIBS_DEP = libkernel.a $(filter %.a,$(LIBS))
	OBJ_LDFLAGS := $(OBJ_LDFLAGS) -L.
	CFG2_OUT_LDFLAGS := $(CFG2_OUT_LDFLAGS) -L.
	CFG3_OUT_LDFLAGS := $(CFG3_OUT_LDFLAGS) -L.
endif

OBJ_LDFLAGS := $(OBJ_LDFLAGS) -Wl,-T,$(LDSCRIPT)
ifdef CFG1_OUT_LDSCRIPT
	CFG1_OUT_LDFLAGS := $(CFG1_OUT_LDFLAGS) -Wl,-T,$(CFG1_OUT_LDSCRIPT)
endif
CFG2_OUT_LDFLAGS := $(CFG2_OUT_LDFLAGS) -Wl,-T,$(CFG2_OUT_LDSCRIPT)
CFG3_OUT_LDFLAGS := $(CFG3_OUT_LDFLAGS) -Wl,-T,$(CFG3_OUT_LDSCRIPT)

#
#  tecsgenからCプリプロセッサを呼び出す際のオプションの定義
#
TECS_CPP = $(CC) $(CDEFS) $(INCLUDES) $(SYSSVC_CFLAGS) -D TECSGEN -E

#
#  tecsgenの呼出し
#
.PHONY: tecs
tecs $(TECSGEN_SRCS) $(TECS_HEADERS): $(TECSGEN_TIMESTAMP) ;
$(TECSGEN_TIMESTAMP): $(APPL_CDL) $(TECS_IMPORTS)
	$(TECSGEN) $< -R $(INCLUDES) --cpp "$(TECS_CPP)" -g $(TECSGENDIR)

#
#  omit_svc.hの生成規則（ファイルがなければ空のファイルを生成する）
#
omit_svc.h:
	$(call print_cmd, "TOUCH", $@)
	@touch omit_svc.h

#
#  カーネルのコンフィギュレーションファイルの生成
#
cfg1_out.c cfg1_out.db: cfg1_out.timestamp ;
cfg1_out.timestamp: $(APPL_CFG) $(TECSGEN_TIMESTAMP)
	$(call print_cmd, "CFG[1]", $@)
	@$(CFG) --pass 1 $(CFG_KERNEL) $(INCLUDES) $(CFG_TABS) \
						-M $(DEPDIR)/cfg1_out_c.d $(TARGET_KERNEL_CFG) $<

$(CFG1_OUT): $(START_OBJS) $(OBJDIR)/cfg1_out.o $(END_OBJS) $(HIDDEN_OBJS) \
													$(CFG1_OUT_LDSCRIPT)
	$(call print_cmd, "LINK", $@)
	@$(LINK) $(CFLAGS) $(LDFLAGS) $(CFG1_OUT_LDFLAGS) -o $(CFG1_OUT) \
						$(START_OBJS) $(OBJDIR)/cfg1_out.o $(LIBS) $(END_OBJS)

cfg1_out.syms: $(CFG1_OUT)
	$(call print_cmd, "NM", $@)
	@$(NM) -n $(CFG1_OUT) > cfg1_out.syms

cfg1_out.srec: $(CFG1_OUT)
	$(call print_cmd, "OBJCOPY", $@)
	@$(OBJCOPY) -O srec -S $(CFG1_OUT) cfg1_out.srec

$(CFG2_OUT_SRCS) $(CFG2_OUT_LDSCRIPT) cfg2_out.db: kernel_cfg.timestamp ;
kernel_cfg.timestamp: cfg1_out.db cfg1_out.syms cfg1_out.srec
	$(call print_cmd, "CFG[2]", $@)
	@$(CFG) --pass 2 $(CFG_KERNEL) $(INCLUDES) -T $(TARGET_KERNEL_TRB)

#
#	パス3を使用する場合
#
ifdef USE_CFG_PASS3
$(CFG3_OUT_SRCS) $(CFG3_OUT_LDSCRIPT) cfg3_out.db: kernel_opt.timestamp ;
kernel_opt.timestamp: cfg2_out.db $(ALL2_OBJS) $(LIBS_DEP) $(CFG2_OUT_LDSCRIPT)
	$(LINK) $(CFLAGS) $(LDFLAGS) $(CFG2_OUT_LDFLAGS) -o $(CFG2_OUT)
	$(NM) -n $(CFG2_OUT) > cfg2_out.syms
	$(OBJCOPY) -O srec -S $(CFG2_OUT) cfg2_out.srec
	rm -f $(CFG3_OUT_LDSCRIPT)
	$(CFG) --pass 3 $(CFG_KERNEL) $(INCLUDES) -T $(TARGET_OPT_TRB) \
				--rom-symbol cfg2_out.syms --rom-image cfg2_out.srec

kernel_mem.c $(LDSCRIPT): kernel_mem.timestamp ;
kernel_mem.timestamp: cfg3_out.db $(ALL3_OBJS) $(LIBS_DEP) $(CFG3_OUT_LDSCRIPT)
	$(LINK) $(CFLAGS) $(LDFLAGS) $(CFG3_OUT_LDFLAGS) -o $(CFG3_OUT)
	$(NM) -n $(CFG3_OUT) > cfg3_out.syms
	$(OBJCOPY) -O srec -S $(CFG3_OUT) cfg3_out.srec
	rm -f $(LDSCRIPT)
	$(CFG) --pass 4 $(CFG_KERNEL) $(INCLUDES) -T $(TARGET_MEM_TRB) \
				--rom-symbol cfg3_out.syms --rom-image cfg3_out.srec
endif

#
#	パス3を使用しない場合
#
ifndef USE_CFG_PASS3
kernel_mem.c $(LDSCRIPT): kernel_mem.timestamp ;
kernel_mem.timestamp: cfg2_out.db $(ALL2_OBJS) $(LIBS_DEP) $(CFG2_OUT_LDSCRIPT)
	$(LINK) $(CFLAGS) $(LDFLAGS) $(CFG2_OUT_LDFLAGS) -o $(CFG2_OUT)
	$(NM) -n $(CFG2_OUT) > cfg2_out.syms
	$(OBJCOPY) -O srec -S $(CFG2_OUT) cfg2_out.srec
	cp -p cfg2_out.db cfg3_out.db
	rm -f $(LDSCRIPT)
	$(CFG) --pass 4 $(CFG_KERNEL) $(INCLUDES) -T $(TARGET_MEM_TRB) \
				--rom-symbol cfg2_out.syms --rom-image cfg2_out.srec
endif

#
#  オフセットファイル（offset.h）の生成規則
#
$(OFFSET_H): offset.timestamp ;
offset.timestamp: cfg1_out.db cfg1_out.syms cfg1_out.srec
	$(call print_cmd, "CFG[2]", $@)
	@$(CFG) --pass 2 -O $(CFG_KERNEL) $(INCLUDES) -T $(TARGET_OFFSET_TRB) \
				--rom-symbol cfg1_out.syms --rom-image cfg1_out.srec

#
#  カーネルライブラリファイルの生成
#
libkernel.a: $(OFFSET_H) $(KERNEL_LIB_OBJS)
	$(call print_cmd, "RM", $@)
	@rm -f libkernel.a
	$(call print_cmd, "AR", $@)
	@$(AR) -rcs libkernel.a $(KERNEL_LIB_OBJS)
	$(call print_cmd, "RANLIB", $@)
	@$(RANLIB) libkernel.a

#
#  並列makeのための依存関係の定義
#
$(APPL_OBJS) $(SYSSVC_OBJS): | kernel_cfg.timestamp
$(APPL_ASMOBJS) $(SYSSVC_ASMOBJS) $(KERNEL_ASMOBJS) $(CFG_ASMOBJS): \
														| offset.timestamp

#
#  特別な依存関係の定義
#
$(OBJDIR)/svc_table.o: omit_svc.h
$(OBJDIR)/banner.o: $(filter-out $(OBJDIR)/banner.o,$(ALL_OBJS2)) \
																$(LIBS_DEP)

#
#  全体のリンク
#
$(OBJFILE): $(ALL_OBJS) $(LIBS_DEP) $(LDSCRIPT)
	$(LINK) $(CFLAGS) $(LDFLAGS) $(OBJ_LDFLAGS) -o $(OBJFILE)

#
#  シンボルファイルの生成
#
$(OBJNAME).syms: $(OBJFILE)
	$(NM) -n $(OBJFILE) > $(OBJNAME).syms

#
#  バイナリファイルの生成
#
$(OBJNAME).bin: $(OBJFILE)
	$(OBJCOPY) -O binary -S $(OBJFILE) $(OBJNAME).bin

#
#  Sレコードファイルの生成
#
$(OBJNAME).srec: $(OBJFILE)
	$(OBJCOPY) -O srec -S $(OBJFILE) $(OBJNAME).srec

#
#  エラーチェック処理
#
.PHONY: check
check: check.timestamp ;
check.timestamp: $(OBJNAME).syms
	touch check.timestamp
ifdef USE_CFG_PASS3
	diff cfg3_out.syms $(OBJNAME).syms
else
	diff cfg2_out.syms $(OBJNAME).syms
endif
	@echo "configuration check passed"

#
#  コンパイル結果の消去
#
.PHONY: clean
clean:
	rm -f \#* *~ $(OBJDIR)/*.o $(DEPDIR)/*.d $(CLEAN_FILES) check.timestamp
	rm -f $(OBJFILE) $(OBJNAME).syms $(OBJNAME).srec $(OBJNAME).bin
	rm -f kernel_mem.timestamp kernel_mem.c $(LDSCRIPT)
	rm -f kernel_opt.timestamp $(CFG3_OUT_SRCS) $(CFG3_OUT_LDSCRIPT) cfg3_out.db
	rm -f cfg3_out.syms cfg3_out.srec $(CFG3_OUT)
	rm -f kernel_cfg.timestamp $(CFG2_OUT_SRCS) $(CFG2_OUT_LDSCRIPT) cfg2_out.db
	rm -f cfg2_out.syms cfg2_out.srec $(CFG2_OUT)
	rm -f offset.timestamp $(OFFSET_H)
	rm -f cfg1_out.syms cfg1_out.srec $(CFG1_OUT)
	rm -f cfg1_out.timestamp cfg1_out.c cfg1_out.db
	rm -rf $(TECSGENDIR)
ifndef KERNEL_LIB
	rm -f libkernel.a
endif

.PHONY: cleankernel
cleankernel:
	rm -f $(OFFSET_H) $(KERNEL_LIB_OBJS)
	rm -f $(KERNEL_LIB_OBJS:$(OBJDIR)/%.o=$(DEPDIR)/%.d)

.PHONY: realclean
realclean: clean
	rm -f $(REALCLEAN_FILES)

#
#  コンフィギュレータが生成したファイルのコンパイルルールの定義
#
#  コンフィギュレータが生成したファイルは，共通のコンパイルオプション
#  のみを付けてコンパイルする．
#
ALL_CFG_COBJS = $(sort $(CFG_COBJS) $(CFG2_COBJS) $(CFG3_COBJS) \
													$(OBJDIR)/cfg1_out.o)
ALL_CFG_ASMOBJS = $(sort $(CFG_ASMOBJS) $(CFG2_ASMOBJS) $(CFG3_ASMOBJS))

$(ALL_CFG_COBJS): $(OBJDIR)/%.o: %.c
	$(call print_cmd, "CC", $<)
	@$(CC) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $(CFG_CFLAGS) $<

$(ALL_CFG_COBJS:$(OBJDIR)/%.o=%.s): %.s: %.c
	$(CC) -S -o $@ $(CFLAGS) $(CFG_CFLAGS) $<

$(ALL_CFG_ASMOBJS): $(OBJDIR)/%.o: %.S
	$(call print_cmd, "CC", $<)
	@$(CC) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $(CFG_CFLAGS) $<

#
#  依存関係ファイルのインクルード
#
-include $(DEPDIR)/*.d

#
#  開発ツールのコマンド名の定義
#
ifeq ($(TOOL),gcc)
	#
	#  GNU開発環境用
	#
	ifdef GCC_TARGET
		GCC_TARGET_PREFIX = $(GCC_TARGET)-
	else
		GCC_TARGET_PREFIX =
	endif
	CC := $(GCC_TARGET_PREFIX)gcc
	CXX := $(GCC_TARGET_PREFIX)g++
	AS := $(GCC_TARGET_PREFIX)as
	LD := $(GCC_TARGET_PREFIX)ld
	AR := $(GCC_TARGET_PREFIX)ar
	NM := $(GCC_TARGET_PREFIX)nm
	RANLIB := $(GCC_TARGET_PREFIX)ranlib
	OBJCOPY := $(GCC_TARGET_PREFIX)objcopy
	OBJDUMP := $(GCC_TARGET_PREFIX)objdump
endif

ifdef DEVTOOLDIR
	CC := $(DEVTOOLDIR)/$(CC)
	CXX := $(DEVTOOLDIR)/$(CXX)
	AS := $(DEVTOOLDIR)/$(AS)
	LD := $(DEVTOOLDIR)/$(LD)
	AR := $(DEVTOOLDIR)/$(AR)
	NM := $(DEVTOOLDIR)/$(NM)
	RANLIB := $(DEVTOOLDIR)/$(RANLIB)
	OBJCOPY := $(DEVTOOLDIR)/$(OBJCOPY)
	OBJDUMP := $(DEVTOOLDIR)/$(OBJDUMP)
endif

ifdef USE_CXX
	LINK = $(CXX)
else
	LINK = $(CC)
endif

#
#  コンパイルルールの定義
#
$(KERNEL_COBJS): $(OBJDIR)/%.o: %.c
	$(call print_cmd, "CC[K]", $<)
	@$(CC) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $(KERNEL_CFLAGS) $<

$(KERNEL_COBJS:$(OBJDIR)/%.o=%.s): %.s: %.c
	$(CC) -S -o $@ $(CFLAGS) $(KERNEL_CFLAGS) $<

$(KERNEL_LCOBJS): $(OBJDIR)/%.o:
	$(CC) -c -o $@ -DTOPPERS_$(*F) -MD -MP -MF $(DEPDIR)/$*.d \
									$(CFLAGS) $(KERNEL_CFLAGS) $<

$(KERNEL_LCOBJS:$(OBJDIR)/%.o=%.s): %.s:
	$(CC) -S -o $@ -DTOPPERS_$(*F) $(CFLAGS) $(KERNEL_CFLAGS) $<

$(KERNEL_ASMOBJS): $(OBJDIR)/%.o: %.S
	$(call print_cmd, "CC[K]", $<)
	@$(CC) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $(KERNEL_CFLAGS) $<

$(SYSSVC_COBJS): $(OBJDIR)/%.o: %.c
	$(CC) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $(SYSSVC_CFLAGS) $<

$(SYSSVC_COBJS:$(OBJDIR)/%.o=%.s): %.s: %.c
	$(CC) -S -o $@ $(CFLAGS) $(SYSSVC_CFLAGS) $<

$(SYSSVC_ASMOBJS): $(OBJDIR)/%.o: %.S
	$(call print_cmd, "CC", $<)
	@$(CC) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $(SYSSVC_CFLAGS) $<

$(APPL_COBJS): $(OBJDIR)/%.o: %.c
	$(call print_cmd, "CC", $<)
	@$(CC) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $(APPL_CFLAGS) $<

$(APPL_COBJS:$(OBJDIR)/%.o=%.s): %.s: %.c
	$(CC) -S -o $@ $(CFLAGS) $(APPL_CFLAGS) $<

$(APPL_CXXOBJS): $(OBJDIR)/%.o: %.cpp
	$(CXX) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $(APPL_CFLAGS) $<

$(APPL_CXXOBJS:$(OBJDIR)/%.o=%.s): %.s: %.cpp
	$(CXX) -S -o $@ $(CFLAGS) $(APPL_CFLAGS) $<

$(APPL_ASMOBJS): $(OBJDIR)/%.o: %.S
	$(call print_cmd, "CC", $<)
	@$(CC) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $(APPL_CFLAGS) $<

#
#  デフォルトコンパイルルールを上書き
#
$(OBJDIR)/%.o: %.c
	@echo "*** Default compile rules should not be used."
	$(CC) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $<

%.s: %.c
	@echo "*** Default compile rules should not be used."
	$(CC) -S -o $@ $(CFLAGS) $<

$(OBJDIR)/%.o: %.cpp
	@echo "*** Default compile rules should not be used."
	$(CXX) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $<

%.s: %.cpp
	@echo "*** Default compile rules should not be used."
	$(CXX) -S -o $@ $(CFLAGS) $<

$(OBJDIR)/%.o: %.S
	@echo "*** Default compile rules should not be used."
	$(CC) -c -o $@ -MD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) $<
