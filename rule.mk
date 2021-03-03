objs:= $(abspath $(objs))
src_objs=$(basename $(objs))
objs_out1=$(addprefix $(OUTPUT_PREFIX),$(objs))
objs_out=$(objs_out1:.o=.c.o)

objs_cxx:= $(abspath $(objs_cxx))
src_objs_cxx=$(basename $(objs_cxx))
objs_cxx_out1=$(addprefix $(OUTPUT_PREFIX),$(objs_cxx))
objs_cxx_out=$(objs_cxx_out1:.o=.cpp.o)

obj_ls:= $(abspath $(obj_ls))
src_obj_ls=$(basename $(obj_ls))
obj_ls_out1=$(addprefix $(OUTPUT_PREFIX),$(obj_ls))
obj_ls_out=$(obj_ls_out1:.o=.s.o)

obj_bs:= $(abspath $(obj_bs))
src_obj_bs=$(basename $(obj_bs))
obj_bs_out1=$(addprefix $(OUTPUT_PREFIX),$(obj_bs))
obj_bs_out=$(obj_bs_out1:.o=.S.o)

objs_ver:=$(abspath $(objs_ver))
src_ver=$(basename $(objs_ver))
objs_ver_out1=$(addprefix $(OUTPUT_PREFIX),$(objs_ver))
objs_ver_out=$(objs_ver_out1:.o=.z.o)

obj_files = $(objs_out) $(objs_cxx_out) $(obj_ls_out) $(obj_bs_out) $(objs_ver_out)
dep_files = $(obj_files:.o=.d)
 
.PHONY: out archive clean dry_run

.SUFFIXES:

out: object
ifeq ($(NEED_USED_LIST),y)
	@$(CC) -MM $(SYS_INCLUDES) $(includes) -D__LD__ $(CC_DEFINE) $(ROOT)/cpu/$(CPU)/sdk_used_list.c -o $(ROOT)/cpu/$(CPU)/sdk_used_list.d
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -E -D__LD__ $(CC_DEFINE) -P $(ROOT)/cpu/$(CPU)/sdk_used_list.c -o $(ROOT)/cpu/$(CPU)/sdk_used_list.used
endif
	@$(CC) -MM $(SYS_INCLUDES) $(includes) -D__LD__ $(CC_DEFINE) $(ROOT)/cpu/$(CPU)/sdk_ld.c -o $(ROOT)/cpu/$(CPU)/sdk_ld.d
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -E -D__LD__ $(CC_DEFINE) -P $(ROOT)/cpu/$(CPU)/sdk_ld.c -o $(ROOT)/cpu/$(CPU)/sdk.ld
ifneq (,$(wildcard $(ROOT)/cpu/$(CPU)/tools/download.c))
	@$(CC) -MM $(SYS_INCLUDES) $(includes) -D__LD__ $(CC_DEFINE) $(ROOT)/cpu/$(CPU)/tools/download.c -o $(ROOT)/cpu/$(CPU)/tools/download.d || true 
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -E -D__LD__ $(CC_DEFINE) -P $(ROOT)/cpu/$(CPU)/tools/download.c -o $(POST_BUILD_SCRIPT) || true 
endif

ifneq (,$(wildcard $(ROOT)/cpu/$(CPU)/tools/isd_config.c))
	@$(CC) -MM $(SYS_INCLUDES) $(includes) -D__LD__ $(CC_DEFINE) $(ROOT)/cpu/$(CPU)/tools/isd_config.c -o $(ROOT)/cpu/$(CPU)/tools/isd_config.d || true 
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -E -D__LD__ $(CC_DEFINE) -P $(ROOT)/cpu/$(CPU)/tools/isd_config.c -o $(ROOT)/cpu/$(CPU)/tools/isd_config.ini || true 
endif

	$(V) $(LD) $(LD_ARGS) -o $(OUTPUT_ELF) $(obj_files) $(SYS_LIBS) $(LIBS) $(LINKER) 
ifneq ($(cibuild),y)
ifneq ($(HOST_OS),windows)
	$(V) /opt/utils/check-mix-diff-cpu $(OUTPUT_ELF).0.5.precodegen.bc || (/opt/utils/view-target-cpu $(LD_ARGS) -o $(obj_files) $(SYS_LIBS) $(LIBS) $(LINKER) && exit 1)
ifeq ($(jtag),n)
	@cd $(DIR_OUTPUT) && bash $(POST_BUILD_SCRIPT) $(ELF)
else
	@cd $(DIR_OUTPUT) && bash $(POST_BUILD_SCRIPT) $(ELF) "_jtag"
endif
endif
endif
ifeq ($(HOST_OS),windows)
	@cd $(DIR_OUTPUT) && $(POST_BUILD_SCRIPT) $(ELF)
endif

archive: object 
	$(V) $(AR) $(AR_ARGS) $(AR_OUT) $(obj_files)
ifeq ($(OVERRIDE),y)
	$(V) $(OVERRIDE_SEG) --input $(AR_OUT) --output $(AR_OUT_NEW) --code_seg ".$(MOUDLE_NAME).$(ORSEG_NAME).text"
endif

_CC = $(CC)

ifeq ($(TIDY_CHECK), y)
_CC = $(TIDY_FILTER) $(CLANG_TIDY)
endif

run: object
	@echo "dry run for YCM server"

object: $(obj_files) rm_lib


$(OUTPUT_PREFIX)%.z.o: %.z
	@echo + VER $<
	$(VER) $< $<.S
	$(V) $(CC) $(SYS_INCLUDES)  $(includes) -D__ASSEMBLY__ $(CC_ARGS) $(CC_DEFINE) -c $<.S -o $@ 


$(OUTPUT_PREFIX)%.s.o: %.s
	@echo + AS $<
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$@" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	$(V) $(CC) $(SYS_INCLUDES)  $(includes) -D__ASSEMBLY__ $(CC_ARGS) $(CC_DEFINE) -c $< -o $@

$(OUTPUT_PREFIX)%.S.o: %.S
	@echo + AS $<
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$@" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	$(V) $(CC) $(SYS_INCLUDES)  $(includes) -D__ASSEMBLY__ $(CC_ARGS) $(CC_DEFINE) -c $< -o $@

$(OUTPUT_PREFIX)%.c.o: %.c
	@echo + CC $<
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$@" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	$(V) $(_CC) $(SYS_INCLUDES)  $(includes) $(CC_ARGS) $(CC_DEFINE) -c $< -o $@

$(OUTPUT_PREFIX)%.cpp.o: %.cpp
	@echo + CXX $<
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$@" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	$(V) $(_CC) $(SYS_INCLUDES)  $(includes) $(CC_ARGS) $(CC_DEFINE) -c $< -o $@



rm_lib:
ifeq ($(GEN_LIB),y)
	@[ -f $(AR_OUT) ] && rm $(AR_OUT) || true
endif
 

 
 
-include $(dep_files)

 
 

