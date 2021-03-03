# -----------------Usage --------------------------

# 1.编译apps:		make;
# 2.清理apps:		make clean;
# 3.编译全部libs:	make libs;
# 4.清理全部libs:	make clean_libs;
# 5.编译单独lib:	make lib target=...;
# 6.清理单独lib:	make clean_lib target=...;
# 7.GDB 调试:		make debug;

# --------------basic setting-----------------------
# ifeq ($(LANG),)
ifeq ($(shell uname),Linux)
export HOST_OS = linux
export SLASH=/
export ADDITION_DEP=libs
else
export HOST_OS = windows
export SLASH=\\
export ADDITION_DEP=
endif


# 默认注释编译信息(None/@)
export V=@
export jtag ?=n
export TIDY_CHECK ?= n
export CLANG_TIDY=/opt/utils/llvm11/bin/clang-tidy
export TIDY_FILTER=/opt/utils/tidy-filter


#配置下载目标SoC(br18/br21/br22/br23/br25/br26/bd29)
#export前面不要有空格，会导致文件sync异常
#export SoC?=br18
#export SoC?=br21
#export SoC?=br22
#export SoC?=br23
export SoC?=br25
#export SoC?=br25_r
#export SoC?=br26
#export SoC?=bd29
#export SoC?=br30
#export SoC?=br30c
#export SoC?=br34
#export SoC?=br28
#export SoC?=bd19
#export SoC?=br36


#配置App
#export APP_CASE?=earphone
#export APP_CASE?=qcy_case
#export APP_CASE?=soundbox
#export APP_CASE?=dongle
#export APP_CASE?=spp_and_le
export APP_CASE?=hid
#export APP_CASE?=mesh
#export APP_CASE?=watch
#export APP_CASE?=iot
#export APP_CASE?=bt_udisk
#export APP_CASE?=multi_link


# --------------common var begin-----------------------
export ROOT=$(abspath .)

ifeq ($(cibuild),y)
export OUTPUT_PREFIX=
else
ifeq ($(HOST_OS), windows)
export OUTPUT_PREFIX=
else
export OUTPUT_PREFIX=${HOME}/tmp
endif
endif

export MAKE_RULE = $(ROOT)/rule.mk

export AR_DIR = \
  	$(ROOT)/include_lib/liba/$(CPU)

export AR_TARGET = $(shell find lib -maxdepth 1 -type d)

# --------------common var end-----------------------


# read only , can not modify

OTHER_FILES =

ifeq ($(APP_CASE),dongle)
	OTHER_FILES += \

else
	OTHER_FILES += apps/$(APP_CASE)/include/bt_ble.h \

endif




.PHONY: all clean lib clean_lib libs clean_libs dry_run debug usage cbp winmake app

all: pre_make $(ADDITION_DEP)
	@$(MAKE) -C apps || exit 1

app: pre_make
	@$(MAKE) -C apps || exit 1

libs:
ifneq ($(cibuild),y)
	@if [ ! -d $(OUTPUT_PREFIX)$(ROOT) ]; then \
		find $(ROOT) -type d | while read dir_name; do \
			mkdir -p $(OUTPUT_PREFIX)$$dir_name; \
		done \
	else \
		find $(ROOT) -type d | while read dir_name; do \
			if [ ! -d $(OUTPUT_PREFIX)$$dir_name ]; then \
				mkdir -p $(OUTPUT_PREFIX)$$dir_name; \
			fi \
		done \
	fi
endif
	@$(MAKE) -C lib || exit 1

clean:
	@echo "Clean ..."
	@find . -name "*.d" -delete
	@find . -name "*.o" -delete
	@find . -name "*.z.S" -delete
	@find $(OUTPUT_PREFIX)$(ROOT) -name "*.d" -delete
	@find $(OUTPUT_PREFIX)$(ROOT) -name "*.o" -delete
	@find $(OUTPUT_PREFIX)$(ROOT) -name "*.z.S" -delete

dry_run:
	@for i in $(AR_TARGET); do \
		if [ -d $$i ] && [ -s $$i/Makefile ]; then \
			echo "********"$$i; \
			$(MAKE) clean -C $$i; \
			$(MAKE) dry_run -C || exit; \
			$(MAKE) clean -C $$i; \
		fi \
	done
	$(MAKE) clean -C apps
	$(MAKE) dry_run -C apps || exit
	$(MAKE) clean -C apps

uboot_bt:
	@$(MAKE) clean -C lib/btctrler -f Makefile_uboot || exit
	@$(MAKE) archive -C lib/btctrler -f Makefile_uboot || exit 1

TERMINAL 	:=gnome-terminal

debug:
	$(SHELL) $(DIR_OUTPUT)/run_jtag.sh $(OUTPUT_ELF)
#	$(TERMINAL) --geometry=100x57+10+10 -e "/opt/gdb -q $(OUTPUT_ELF) -ex \"target remote proxy:$(NICKNAME_DEBUG):9872\""

cbp:
	$(V) python3 /opt/utils/gen_cbp.py --cc_path $(CC) --ar_path $(AR) \
		--ld_path $(LD) --make_log make_log.txt \
		--prefix `realpath .` \
		--ignore_dirs `realpath lib` \
		--omit_files $(OMIT_FILES) \
		--gen_ld gen_sdk_ld.bat \
		--gen_ld_cc C:\\JL\\pi32\\bin\\clang.exe \
		--gen_ld_incs C:\\JL\\pi32\\pi32v2-include \
		--outdir `realpath ../cbp_out` \
		--cbp_title $(CBP_TITLE) \
		--cbp_compiler $(CBP_COMPILER) \
		$(CBP_ADD_OPT) \
		--cbp_position $(CBP_POSITION) \
		--post_build $(DIR_OUTPUT)/download.bat \
		--other_files $(DIR_OUTPUT) $(OTHER_FILES)
	$(V) python3 /opt/utils/gen_winmake.py --cc_path $(CC) --ar_path $(AR) \
		--ld_path $(LD) --make_log make_log.txt \
		--prefix `realpath .` \
		--ignore_dirs `realpath lib` \
		--omit_files $(OMIT_FILES) \
		--outdir `realpath ../cbp_out` \
		--other_files $(DIR_OUTPUT)/uboot.bin \
			$(DIR_OUTPUT)/download.bat	\
			make_prompt.bat \
			$(OTHER_FILES) \
		--other_dirs tools/utils

winmake:
	$(V) python3 /opt/utils/gen_winmake.py --cc_path $(CC) --ar_path $(AR) \
		--ld_path $(LD) --make_log make_log.txt \
		--prefix `realpath .` \
		--outdir `realpath ../winmake_out` \
		--other_files $(DIR_OUTPUT)/uboot.bin \
			$(DIR_OUTPUT) 	\
			make_prompt.bat \
			README.md *.pdf \
			$(OTHER_FILES) 	\
		--other_dirs tools/utils

winrelease: cbp winmake
	$(V) python3 /opt/utils/gen_merge_dir.py --indirs \
		`realpath ../winmake_out`  \
		`realpath ../cbp_out`	   \
		--outdir `realpath ../merge_out`

branch_jenkins:
	$(V) python3 /opt/utils/jenkins-gen-build-config.py --config jenkins/jenkins-librepo.json --project $(project) > jenkins/jenkins-build-branch.json


usage:
	@echo Complie apps : make
	@echo Clean apps   : make clean
	@echo Complie libs : make libs
	@echo Clean libs   : make clean_libs
	@echo Complie lib  : make lib TARGET=...
	@echo Clean lib    : make clean_lib TARGET=...
	@echo GDB 		   : make debug
	@echo CBP          : make cbp



-include $(ROOT)/tools/platform/Makefile.$(SoC)
-include $(ROOT)/include_lib/Makefile.include
