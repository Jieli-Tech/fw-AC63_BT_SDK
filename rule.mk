objs:= $(abspath $(objs))
objs_cxx:= $(abspath $(objs_cxx))
obj_ls:= $(abspath $(obj_ls))
obj_bs:= $(abspath $(obj_bs))
deps = $(objs:.o=.d)
deps += $(objs_cxx:.o=.d)
deps += $(obj_ls:.o=.d)
deps += $(obj_bs:.o=.d)
 
.PHONY: out archive clean dry_run

.SUFFIXES:


out: object 
	@$(CC) -MM $(SYS_INCLUDES) $(includes) -D__LD__ $(CC_DEFINE) $(ROOT)/cpu/$(CPU)/sdk_ld.c > $(ROOT)/cpu/$(CPU)/sdk_ld.d
	@[ -f $(ROOT)/cpu/$(CPU)/tools/download.c ] && \
	$(CC) -MM $(SYS_INCLUDES) $(includes) -D__LD__ $(CC_DEFINE) $(ROOT)/cpu/$(CPU)/tools/download.c > $(ROOT)/cpu/$(CPU)/tools/download.d || true
ifeq ($(NEED_USED_LIST),y)
	@$(CC) -MM $(SYS_INCLUDES) $(includes) -D__LD__ $(CC_DEFINE) $(ROOT)/cpu/$(CPU)/sdk_used_list.c > $(ROOT)/cpu/$(CPU)/sdk_used_list.d
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -E -D__LD__ $(CC_DEFINE) -P $(ROOT)/cpu/$(CPU)/sdk_used_list.c -o $(ROOT)/cpu/$(CPU)/sdk_used_list.used
endif
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -E -D__LD__ $(CC_DEFINE) -P $(ROOT)/cpu/$(CPU)/sdk_ld.c -o $(ROOT)/cpu/$(CPU)/sdk.ld
ifneq (,$(wildcard $(ROOT)/cpu/$(CPU)/tools/download.c))
	$(V) $(CC) $(SYS_INCLUDES) $(includes) -E -D__LD__ $(CC_DEFINE) -P $(ROOT)/cpu/$(CPU)/tools/download.c -o $(POST_BUILD_SCRIPT) || true 
endif
	$(V) $(LD) $(LD_ARGS) -o $(OUTPUT_ELF) $(objs) $(obj_ls) $(obj_bs) $(objs_ver) $(SYS_LIBS) $(LIBS) $(LINKER) 
ifneq ($(cibuild),y)
ifneq ($(HOST_OS),windows)
	$(V) /opt/utils/check-mix-diff-cpu $(OUTPUT_ELF).0.5.precodegen.bc || (/opt/utils/view-target-cpu $(LD_ARGS) -o $(objs) $(obj_ls) $(obj_bs) $(SYS_LIBS) $(LIBS) $(LINKER) && exit 1)
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
	$(V) $(AR) $(AR_ARGS) $(AR_OUT) $(objs) $(obj_ls) $(obj_bs) $(objs_cxx) $(objs_ver)
ifeq ($(OVERRIDE),y)
	$(V) $(OVERRIDE_SEG) --input $(AR_OUT) --output $(AR_OUT_NEW) --code_seg ".$(MOUDLE_NAME).$(ORSEG_NAME).text"
endif


run: object
	@echo "dry run for YCM server"

object: $(objs_ver) $(obj_ls) $(obj_bs) $(objs) $(objs_cxx) rm_lib


$(objs_ver):%.o:%.z
	@echo + VER $<
	$(V) $(VER) $< $<.S
	$(V) $(CC) $(SYS_INCLUDES)  $(includes) -D__ASSEMBLY__ $(CC_ARGS) $(CC_DEFINE) -c $<.S -o $@

$(obj_ls):%.o:%.s
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$(<:.s=.o)" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	@echo + AS $<
	$(V) $(CC) $(SYS_INCLUDES)  $(includes) -D__ASSEMBLY__ $(CC_ARGS) $(CC_DEFINE) -c $< -o $@
  
 
$(obj_bs):%.o:%.S
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$(<:.S=.o)" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	@echo + AS $<
	$(V) $(CC) $(SYS_INCLUDES)  $(includes) -D__ASSEMBLY__ $(CC_ARGS) $(CC_DEFINE) -c $< -o $@
  
 
$(objs):%.o:%.c
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$(<:.c=.o)" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	@echo + CC $<
ifeq ($(TIDY_CHECK), y)
	@$(TIDY_FILTER) $(CLANG_TIDY) $(SYS_INCLUDES)  $(includes) $(CC_ARGS) $(CC_DEFINE) -c $< -o $@
endif
ifeq ($(GEN_LIB),y)
	$(V) -$(CC) $(SYS_INCLUDES)  $(includes) $(CC_ARGS) $(CC_DEFINE) -c $< -o $@
else
	$(V) $(CC) $(SYS_INCLUDES)  $(includes) $(CC_ARGS) $(CC_DEFINE) -c $< -o $@
endif

$(objs_cxx):%.o:%.cpp
	@$(CC) $(CC_ARGS) $(CC_DEFINE) -MM -MT "$(<:.cpp=.o)" $(SYS_INCLUDES) $(includes) $< > $(@:.o=.d)
	@echo + CXX $<
ifeq ($(GEN_LIB),y)
	$(V) -$(CC) $(SYS_INCLUDES)  $(includes) $(CC_ARGS) $(CC_DEFINE) -c $< -o $@
else
	$(V) $(CC) $(SYS_INCLUDES)  $(includes) $(CC_ARGS) $(CC_DEFINE) -c $< -o $@
endif



rm_lib:
ifeq ($(GEN_LIB),y)
	@[ -f $(AR_OUT) ] && rm $(AR_OUT) || true
endif
 

 
 
-include $(deps)

 
 

