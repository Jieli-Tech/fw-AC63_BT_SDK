

  local_irq_enable = ABSOLUTE(0x117d40);
  local_irq_disable = ABSOLUTE(0x117d44);
  p33_buf = ABSOLUTE(0x117d48);
  p33_xor_1byte = ABSOLUTE(0x117d4c);
  p33_and_1byte = ABSOLUTE(0x117d50);
  p33_or_1byte = ABSOLUTE(0x117d54);
  p33_tx_1byte = ABSOLUTE(0x117d58);
  p33_rx_1byte = ABSOLUTE(0x117d5c);
  P33_CON_SET = ABSOLUTE(0x117d60);
  os_init = ABSOLUTE(0x117d64);
  os_start = ABSOLUTE(0x117d68);
  os_get_curr_tcb_var = ABSOLUTE(0x117d6c);
  os_task_create = ABSOLUTE(0x117d70);
  os_current_task_rom = ABSOLUTE(0x117d74);
  os_time_dly = ABSOLUTE(0x117d78);
  os_time_get = ABSOLUTE(0x117d7c);
  os_task_del_req = ABSOLUTE(0x117d80);
  os_task_del_res = ABSOLUTE(0x117d84);
  os_task_del = ABSOLUTE(0x117d88);
  __os_taskq_pend = ABSOLUTE(0x117d8c);
  os_taskq_pend = ABSOLUTE(0x117d90);
  os_task_pend = ABSOLUTE(0x117d94);
  __os_taskq_post_rom = ABSOLUTE(0x117d98);


  os_taskq_flush_rom = ABSOLUTE(0x117da4);



  os_taskq_accept = ABSOLUTE(0x117db4);
  os_taskq_del_rom = ABSOLUTE(0x117db8);
  os_taskq_del_type = ABSOLUTE(0x117dbc);
  os_sem_create = ABSOLUTE(0x117dc0);
  os_sem_del = ABSOLUTE(0x117dc4);
  os_sem_pend = ABSOLUTE(0x117dc8);
  os_sem_post = ABSOLUTE(0x117dcc);
  os_sem_set = ABSOLUTE(0x117dd0);
  os_sem_valid = ABSOLUTE(0x117dd4);
  os_sem_query = ABSOLUTE(0x117dd8);
  os_mutex_create = ABSOLUTE(0x117ddc);
  os_mutex_del = ABSOLUTE(0x117de0);
  os_mutex_pend = ABSOLUTE(0x117de4);
  os_mutex_post = ABSOLUTE(0x117de8);
  os_mutex_valid = ABSOLUTE(0x117dec);
  xTaskCreate = ABSOLUTE(0x117df0);
  xTaskGetCurrentTaskHandle = ABSOLUTE(0x117df4);
  xTaskGetTickCount = ABSOLUTE(0x117df8);
  xTaskGetHandle = ABSOLUTE(0x117dfc);
  vTaskDelete = ABSOLUTE(0x117e00);
  xTaskGetApplicationTaskTag = ABSOLUTE(0x117e04);
  vTaskSetApplicationTaskTag = ABSOLUTE(0x117e08);
  xQueueGenericCreateStatic = ABSOLUTE(0x117e0c);
  xQueueGenericReset = ABSOLUTE(0x117e10);
  xQueueGenericSend = ABSOLUTE(0x117e14);
  xQueueGenericSendFromISR = ABSOLUTE(0x117e18);
  xQueueGenericReceive = ABSOLUTE(0x117e1c);
  xQueueReceiveFromISR = ABSOLUTE(0x117e20);
  vQueueDelete = ABSOLUTE(0x117e24);
  uxQueueMessagesWaiting = ABSOLUTE(0x117e28);
  uxQueueMessagesWaitingFromISR = ABSOLUTE(0x117e2c);
  uxQueueSpacesAvailable = ABSOLUTE(0x117e30);
  pcTaskGetName = ABSOLUTE(0x117e34);
  prvGetExpectedIdleTime = ABSOLUTE(0x117e38);
  eTaskConfirmSleepModeStatus = ABSOLUTE(0x117e3c);
  vTaskStepTick = ABSOLUTE(0x117e40);
  pxPortInitialiseStack = ABSOLUTE(0x117e44);
  xPortStartScheduler = ABSOLUTE(0x117e48);
  vPortEndScheduler = ABSOLUTE(0x117e4c);
  vPortYield = ABSOLUTE(0x117e50);
  vPortExitCritical = ABSOLUTE(0x117e54);
  OS_ClrPending = ABSOLUTE(0x117e58);
  vTickISR = ABSOLUTE(0x117e5c);
  pmalloc_init = ABSOLUTE(0x117e60);
  pmalloc = ABSOLUTE(0x117e64);
  pfree = ABSOLUTE(0x117e68);
  pmalloc_get_unused_pages = ABSOLUTE(0x117e6c);
  get_physic_address = ABSOLUTE(0x117e70);
  pmm_get_var = ABSOLUTE(0x117e74);
  vmm_init_ = ABSOLUTE(0x117e78);
  vmalloc_ = ABSOLUTE(0x117e7c);
  vfree_ = ABSOLUTE(0x117e80);
  vmem_get_phy_adr = ABSOLUTE(0x117e84);
  vmem_start = ABSOLUTE(0x117e88);
  vmem_end = ABSOLUTE(0x117e8c);
  vmm_get_var = ABSOLUTE(0x117e90);
  vmalloc_v2 = ABSOLUTE(0x117e94);
  vfree_v2 = ABSOLUTE(0x117e98);
  vcopy_ = ABSOLUTE(0x117e9c);
  get_cvsd_codec_ops = ABSOLUTE(0x117ea0);
  VecPlus_fix_r_r_r = ABSOLUTE(0x117ea4);
  VecMinus_fix_r_r_r = ABSOLUTE(0x117ea8);
  VecDiv_fix_r_r_r = ABSOLUTE(0x117eac);
  VecEleMul_fix_r_r_r = ABSOLUTE(0x117eb0);
  VecEleMul_fix_c_r_c = ABSOLUTE(0x117eb4);
  VecMagAndDiv_fix = ABSOLUTE(0x117eb8);
  VecCopy_fix_r_r = ABSOLUTE(0x117ebc);
  VecConjEleMulAndDiv_fix = ABSOLUTE(0x117ec0);
  VecRecurAvg_fix = ABSOLUTE(0x117ec4);
  VecConjRecurAvg_fix = ABSOLUTE(0x117ec8);
  VecMagRecurAvg_fix = ABSOLUTE(0x117ecc);
  VecPlusMulScalar_fix = ABSOLUTE(0x117ed0);
  VecGetMag_fix = ABSOLUTE(0x117ed4);
  VecMax_fix_v_s = ABSOLUTE(0x117ed8);
  VecMin_fix_v_s = ABSOLUTE(0x117edc);
  VecOppos_fix = ABSOLUTE(0x117ee0);
  VecMean_fix = ABSOLUTE(0x117ee4);
  VecMeanSq_fix_c = ABSOLUTE(0x117ee8);
  VecRequanByMax_fix_r = ABSOLUTE(0x117eec);
  VecRequanByMax_fix_c = ABSOLUTE(0x117ef0);
  VecEleShift_fix_c = ABSOLUTE(0x117ef4);
  VecOvShift_fix = ABSOLUTE(0x117ef8);
  VecCopy_fix_r_c = ABSOLUTE(0x117efc);
  VecMeanSq_fix_r = ABSOLUTE(0x117f00);
  VecEleShift_fix_r = ABSOLUTE(0x117f04);
  logAprx_fix = ABSOLUTE(0x117f08);
  expAprx_fix = ABSOLUTE(0x117f0c);
  sqrtAprx_fix = ABSOLUTE(0x117f10);
  VecEleMul_s16_s16_s32 = ABSOLUTE(0x117f14);
  VecEleMul_s16_s32_s16 = ABSOLUTE(0x117f18);
  VecEleMul_s32_s16_s32 = ABSOLUTE(0x117f1c);
  VecOvShift_s16_s16 = ABSOLUTE(0x117f20);
  VecOvShift_s16_s32 = ABSOLUTE(0x117f24);
  VecCopy_s16_s32 = ABSOLUTE(0x117f28);
  VecCopy_s32_s16 = ABSOLUTE(0x117f2c);
  VecMinus_s16_s32_s32 = ABSOLUTE(0x117f30);
  VecMeanSq_s16_s32 = ABSOLUTE(0x117f34);
  CircVecArrEwMulAndSum_fix = ABSOLUTE(0x117f38);
  B_fir_cal_s = ABSOLUTE(0x117f3c);
  B_array_vol = ABSOLUTE(0x117f40);
  B_Syn_filt = ABSOLUTE(0x117f44);
  B_Residu = ABSOLUTE(0x117f48);
  B_comput_correlataionS = ABSOLUTE(0x117f4c);
  B_iircal = ABSOLUTE(0x117f50);
  repair_corefun_corr = ABSOLUTE(0x117f54);
  repair_corefun_mixdata = ABSOLUTE(0x117f58);
  get_repair_api = ABSOLUTE(0x117f5c);
  firfft = ABSOLUTE(0x117f60);
  firifft = ABSOLUTE(0x117f64);
  _rfifft_wrap = ABSOLUTE(0x117f68);
  _rififft_wrap = ABSOLUTE(0x117f6c);
  memmem = ABSOLUTE(0x117f70);
  memcpy = ABSOLUTE(0x117f74);
  memmove = ABSOLUTE(0x117f78);
  memcmp = ABSOLUTE(0x117f7c);
  memset = ABSOLUTE(0x117f80);
  strcmp = ABSOLUTE(0x117f84);
  strcpy = ABSOLUTE(0x117f88);
  strlen = ABSOLUTE(0x117f8c);
  strncmp = ABSOLUTE(0x117f90);
  strstr = ABSOLUTE(0x117f94);
  strchr = ABSOLUTE(0x117f98);
  __fixdfsi = ABSOLUTE(0x117f9c);
  __fixsfsi = ABSOLUTE(0x117fa0);
  spi_get_port = ABSOLUTE(0x117fa4);
  spi_set_port = ABSOLUTE(0x117fa8);
  sfc_suspend = ABSOLUTE(0x117fac);
  sfc_resume = ABSOLUTE(0x117fb0);
  sfc_drop_cache = ABSOLUTE(0x117fb4);
  chip_crc16 = ABSOLUTE(0x117fb8);
  CrcDecode = ABSOLUTE(0x117fbc);
  mask_init = ABSOLUTE(0x117fc0);
  wdt_clr = ABSOLUTE(0x117fc4);
  nvram_set_boot_state = ABSOLUTE(0x117fc8);
  chip_reset = ABSOLUTE(0x117fcc);
  the_debug_isr = ABSOLUTE(0x117fd0);
  flash_poweron = ABSOLUTE(0x117fd4);

  print = ABSOLUTE(0x117fdc);
  sprintf = ABSOLUTE(0x117fe0);

  puts = ABSOLUTE(0x117fe8);
  doe = ABSOLUTE(0x117fec);
  LZ4_setStreamDecode = ABSOLUTE(0x117ff0);
  LZ4_decompress_safe_continue = ABSOLUTE(0x117ff4);
  LZ4_decompress_generic = ABSOLUTE(0x117ff8);
  LZ4_decompress_safe_usingDict = ABSOLUTE(0x117ffc);
  bsinc_tab_6 = ABSOLUTE(0x110f60);
  bsinc_tab_6_l = ABSOLUTE(0x1111a4);
  ans_win = ABSOLUTE(0x110c60);
  nlp_win = ABSOLUTE(0x110d60);
  iCos_Tab = ABSOLUTE(0x115f48);
  WOLA_ANA_128 = ABSOLUTE(0x110060);
  WOLA_ANA_64 = ABSOLUTE(0x110460);
  WOLA_SYN_128 = ABSOLUTE(0x110660);
  WOLA_SYN_64 = ABSOLUTE(0x110a60);


  _IRQ_MEM_ADDR = ABSOLUTE(0x31f00);
  _MASK_MEM_BEGIN = ABSOLUTE(0x31b60);
  _MASK_MEM_SIZE = ABSOLUTE(0x318);

UPDATA_SIZE = 0x80;
UPDATA_BEG = _MASK_MEM_BEGIN - UPDATA_SIZE;
UPDATA_BREDR_BASE_BEG = 0xf9000;

RAM_LIMIT_L = 0x10000;
RAM_LIMIT_H = UPDATA_BEG;
PHY_RAM_SIZE = RAM_LIMIT_H - RAM_LIMIT_L;


ISR_BASE = _IRQ_MEM_ADDR;
ROM_RAM_SIZE = _MASK_MEM_SIZE;
ROM_RAM_BEG = _MASK_MEM_BEGIN;


RAM_BEGIN = RAM_LIMIT_L;
RAM_END = RAM_LIMIT_H;
RAM_SIZE = RAM_END - RAM_BEGIN;




EQ_SECTION_NUM = 10;






MEMORY
{
 code0(rx) : ORIGIN = 0x1E00120, LENGTH = (1024 * 1024)
 ram0(rwx) : ORIGIN = RAM_BEGIN, LENGTH = RAM_SIZE
}


ENTRY(_start)

SECTIONS
{
 . = ORIGIN(code0);
    .text ALIGN(4):
    {
        PROVIDE(flash_code_begin = .);

        *startup.o(.text)

        *(.text*)
        bank_stub_start = .;
        *(.bank.stub.*)
        bank_stub_size = . - bank_stub_start;

        *(.LOG_TAG_CONST*)
        *(.rodata*)

        . = ALIGN(4);
        *(.dts_dec_const)
        *(.dts_const)
        *(.dts_dec_code)
        *(.dts_code)
        *(.dts_dec_sparse_code)
        *(.dts_dec_sparse_const)
        *(.dts_dec_ff_const)

        . = ALIGN(4);
        *(.m4a_const)
        *(.m4a_code)
        *(.m4a_dec_const)
        *(.m4a_dec_code)
        *(.m4a_dec_sparse_code)
        *(.m4a_dec_sparse_const)
        *(.m4a_dec_ff_const)

        . = ALIGN(4);
        *(.aac_const)
        *(.aac_code)
        *(.bt_aac_dec_eng_const)
        *(.bt_aac_dec_eng_code)
        *(.bt_aac_dec_core_code)
        *(.bt_aac_dec_core_sparse_code)

  . = ALIGN(4);

        clock_critical_handler_begin = .;
        KEEP(*(.clock_critical_txt))
        clock_critical_handler_end = .;
        . = ALIGN(4);
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

  . = ALIGN(4);
        fm_dev_begin = .;
        KEEP(*(.fm_dev))
  fm_dev_end = .;

  . = ALIGN(4);
        fm_emitter_dev_begin = .;
        KEEP(*(.fm_emitter_dev))
        fm_emitter_dev_end = .;

  . = ALIGN(4);
        storage_device_begin = .;
        KEEP(*(.storage_device))
        storage_device_end = .;

  . = ALIGN(4);
  ui_main_begin = .;
  KEEP(*(.ui_main))
  ui_main_end = .;
  . = ALIGN(4);


        *(.bfilt_code)
        *(.bfilt_table_maskroom)


        . = ALIGN(4);

        *(.tech_lib.aec.text)

  . = ALIGN(4);



        btctler_code_start = .;

        BTCTLER_CONTROLLER_CODE_START = .;
  *(.bt_rf_const)
  *(.bt_rf_code)
  *(.vendor_manager_const)
  *(.vendor_manager_code)
  *(.device_manager_const)
  *(.device_manager_code)
  *(.hci_controller_const)
  *(.hci_controller_code)
  *(.hci_interface_const)
  *(.hci_interface_code)
        BTCTLER_CONTROLLER_CODE_SIZE = ABSOLUTE(. - BTCTLER_CONTROLLER_CODE_START);

        BTCTLER_LE_CONTROLLER_CODE_START = .;
        *(.ble_rf_const)
        *(.ble_rf_code)

     *(.ble_ll_const)
     *(.ble_ll_code)
  *(.ble_hci_const)
  *(.ble_hci_code)
  *(.classic_hci_const)
  *(.classic_hci_code)
        BTCTLER_LE_CONTROLLER_CODE_SIZE = ABSOLUTE(. - BTCTLER_LE_CONTROLLER_CODE_START);

        BTCTLER_CL_CODE_START = .;
        *(.bredr_irq)
        *(.bredr_irq_code)
        *(.bredr_irq_const)

        *(.classic_lmp_const)
        *(.classic_lmp_linkbulk_const)
        *(.classic_lmp_code)
        *(.classic_lmp_linkbulk_code)

        LMP_ENC_CODE_START = .;
        *(.classic_lmp_auth_const)
        *(.classic_lmp_bigint_const)
        *(.classic_lmp_crypt_const)
        *(.classic_lmp_ecdh_const)
        *(.classic_lmp_hmac_const)
        *(.classic_lmp_auth_code)
        *(.classic_lmp_bigint_code)
        *(.classic_lmp_crypt_code)
        *(.classic_lmp_ecdh_code)
        *(.classic_lmp_hmac_code)
        LMP_ENC_CODE_SIZE = ABSOLUTE(. - LMP_ENC_CODE_START);

        *(.classic_rf_const)
        *(.classic_rf_code)
        *(.classic_bb_const)
        *(.classic_bb_code)
        BTCTLER_CL_CODE_SIZE = ABSOLUTE(. - BTCTLER_CL_CODE_START);

  . = ALIGN(4);
        *(.classic_tws_const)
        *(.classic_tws_code)
        *(.classic_tws_code.esco)

        *(.tws_media_sync_code)
        *(.tws_media_sync_const)

        *(.tws_data_forward_code)
        *(.tws_data_forward_const)

  . = ALIGN(4);
        tws_func_stub_begin = .;
        *(.tws_func_stub)
        tws_func_stub_end = .;

        *(.lmp_irq_code)
        *(.link_bulk_code)
        *(.frame_irq_code)

  . = ALIGN(4);
        *(.link_task_const)
        *(.link_task_code)

  . = ALIGN(4);
        *(.classic_irq_const)
        *(.classic_irq_code)

  . = ALIGN(4);
        *(.tws_irq_code)

  . = ALIGN(4);
        tws_sync_call_begin = .;
        *(.tws_sync_call)
        tws_sync_call_end = .;


  . = ALIGN(4);
        tws_sync_channel_begin = .;
        *(.tws_sync_channel.0)
        *(.tws_sync_channel.1)
        *(.tws_sync_channel.2)
        *(.tws_sync_channel.3)
        *(.tws_sync_channel.4)
        *(.tws_sync_channel.5)
        *(.tws_sync_channel.6)
        *(.tws_sync_channel.7)
        *(.tws_sync_channel.8)
        *(.tws_sync_channel.9)
        *(.tws_sync_channel.10)
        *(.tws_sync_channel.11)
        *(.tws_sync_channel.12)
        *(.tws_sync_channel.13)
        tws_sync_channel_end = .;

        btctler_code_end = .;

  . = ALIGN(4);
 . = ALIGN(4);



        btstack_code_start = .;

        . = ALIGN(4);

        a2dp_source_media_codec_begin = .;
        KEEP(*(.a2dp_source_media_codec))
            a2dp_source_media_codec_end = .;
        a2dp_sink_media_probe_begin = .;
        KEEP(*(.a2dp_sink_media_probe))
            a2dp_sink_media_probe_end = .;

        a2dp_sink_media_codec_begin = .;
        KEEP(*(.a2dp_sink_media_codec))
            a2dp_sink_media_codec_end = .;

        a2dp_event_handler_begin = .;
        KEEP(*(.a2dp_event_handler))
            a2dp_event_handler_end = .;

        sdp_record_item_begin = .;
        KEEP(*(.sdp_record_item))
            sdp_record_item_end = .;

        bt_sleep_begin = .;
        KEEP(*(.bt_sleep))
            bt_sleep_end = .;

  *(.bt_stack_const)
  *(.bt_stack_code)
        *(.ble_db_const)
        *(.ble_db_code)
        *(.ble_sm_const)
        *(.ble_sm_code)
        *(.ble_att_const)
        *(.ble_att_code)
        *(.ble_gatt_const)
        *(.ble_gatt_code)


        BTSTACK_LE_HOST_MESH_CODE_START = .;
        *(.ble_mesh_code)
        *(.ble_mesh_tinycrypt_code)

        *(.ble_mesh_const)
        *(.ble_mesh_tinycrypt_const)
        BTSTACK_LE_HOST_MESH_CODE_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_CODE_START);

        btstack_code_end = .;
  . = ALIGN(4);
 . = ALIGN(4);


  . = ALIGN(4);
        system_text_start = .;

   _device_node_begin = .;
     PROVIDE(device_node_begin = .);
          *(.device)
     _device_node_end = .;
     PROVIDE(device_node_end = .);

  config_target_begin = .;
     PROVIDE(config_target_begin = .);
          *(.config_target)
  config_target_end = .;
     PROVIDE(config_target_end = .);

     system_code_begin = .;
         *(.system.*.text)
     system_code_end = .;
  . = ALIGN(4);
  system_code_size = system_code_end - system_code_begin;

  vfs_ops_begin = .;
   *(.vfs_operations)
  vfs_ops_end = .;

     _lib_version_begin = .;
     PROVIDE(lib_version_begin = .);
         *(.lib_version)
     _lib_version_end = .;
     PROVIDE(lib_version_end = .);

  _initcall_begin = .;
  PROVIDE(initcall_begin = .);
   *(.initcall)
  _initcall_end = .;
  PROVIDE(initcall_end = .);

  _early_initcall_begin = .;
  PROVIDE(early_initcall_begin = .);
   *(.early.initcall)
  _early_initcall_end = .;
  PROVIDE(early_initcall_end = .);

  _late_initcall_begin = .;
  PROVIDE(late_initcall_begin = .);
     *(.late.initcall)
  _late_initcall_end = .;
  PROVIDE(late_initcall_end = .);

  _platform_initcall_begin = .;
  PROVIDE(platform_initcall_begin = .);
     *(.platform.initcall)
  _platform_initcall_end = .;
  PROVIDE(platform_initcall_end = .);

  _module_initcall_begin = .;
  PROVIDE(module_initcall_begin = .);
     *(.module.initcall)
  _module_initcall_end = .;
  PROVIDE(module_initcall_end = .);

  _sys_event_handler_begin = .;
  PROVIDE(sys_event_handler_begin = .);
     *(.sys_event.4.handler)
     *(.sys_event.3.handler)
     *(.sys_event.2.handler)
     *(.sys_event.1.handler)
     *(.sys_event.0.handler)
  _sys_event_handler_end = .;
  PROVIDE(sys_event_handler_end = .);

  _syscfg_arg_begin = .;
  PROVIDE(syscfg_arg_begin = .);
     *(.syscfg.arg)
  _syscfg_arg_end = .;
  PROVIDE(syscfg_arg_end = .);

  _syscfg_handler_begin = .;
  PROVIDE(syscfg_handler_begin = .);
     *(.syscfg.handler)
  _syscfg_handler_end = .;
  PROVIDE(syscfg_handler_end = .);

  _syscfg_ops_begin = .;
  PROVIDE(syscfg_ops_begin = .);
     *(.syscfg.2.ops)
     *(.syscfg.1.ops)
     *(.syscfg.0.ops)
  _syscfg_ops_end = .;
  PROVIDE(syscfg_ops_end = .);

  _server_info_begin = .;
  PROVIDE(server_info_begin = .);
     *(.server_info)
  _server_info_end = .;
  PROVIDE(server_info_end = .);

  _bus_device_begin = .;
  PROVIDE(bus_device_begin = .);
     *(.bus_device)
  _bus_device_end = .;
  PROVIDE(bus_device_end = .);

     _sys_power_hal_ops_begin = .;
     PROVIDE(sys_power_hal_ops_begin = .);
         *(.sys_power_hal_ops)
     _sys_power_hal_ops_end = .;
     PROVIDE(sys_power_hal_ops_end = .);


  . = ALIGN(4);
     lp_target_begin = .;
     PROVIDE(lp_target_begin = .);
         *(.lp_target)
     lp_target_end = .;
     PROVIDE(lp_target_end = .);

  . = ALIGN(4);
        deepsleep_target_begin = .;
        PROVIDE(deepsleep_target_begin = .);
            *(.deepsleep_target)
        deepsleep_target_end = .;
        PROVIDE(deepsleep_target_end = .);

     crypto_begin = .;
     *(.crypto_ecdh_code)
     *(.crypto_ecdh_const)

     *(.crypto_bigint_code)
     *(.crypto_bigint_const)

     *(.crypto_code)
     *(.crypto_const)

     *(.ECDH_sample_code)
     *(.ECDH_sample_const)

     *(.uECC_code)
     *(.uECC_const)

     *(.hmac_code)
     *(.hmac_const)

     *(.hash_sample_code)
     *(.hash_sample_const)

     *(.aes_cmac_sample_code)
     *(.aes_cmac_sample_const)
     crypto_end = .;
     crypto_size = . - crypto_begin;

        *(.mem_code)
        *(.mem_const)

        *(.os_port_code)
        *(.os_port_const)

        system_text_end = .;
 . = ALIGN(4);
  *(.mp2_encode_code)
  *(.mp2_encode_const)
  *(.mp2_encode_sparse_code)
  *(.mp2_encode_sparse_code)
  . = ALIGN(4);
  *(.mp3_enc_code)
  *(.mp3_enc_const)
  *(.mp3_enc_sparse_code)
  *(.mp3_enc_sparse_const)
  . = ALIGN(4);
  *(.adpcm_encode_code)
  *(.adpcm_encode_const)
  *(.adpcm_encode_sparse_code)
  *(.adpcm_encode_sparse_const)
  . = ALIGN(4);
        *(.wav_dec_sparse_code)
        *(.wav_dec_sparse_const)
        *(.wav_dec_code)
        *(.wav_dec_const)
        *(.wav_const)
        *(.wav_code)
        . = ALIGN(4);
        *(.amr_const)
        *(.amr_code)
        *(.amr_dec_const)
        *(.amr_dec_code)
        *(.amr_dec_sparse_code)
        *(.amr_dec_sparse_const)
        *(.amr_dec_ff_const)
  . = ALIGN(4);
        *(.ape_dec_sparse_code)
        *(.ape_dec_sparse_const)
        *(.ape_dec_code)
        *(.ape_dec_const)
        *(.ape_bss)
        *(.ape_const)
        *(.ape_code)
        . = ALIGN(4);
        *(.flac_dec_sparse_code)
        *(.flac_dec_sparse_const)
        *(.flac_dec_code)
        *(.flac_dec_const)
        *(.flac_const)
        *(.flac_code)
        . = ALIGN(4);
     *(.alac_dec_code)
        . = ALIGN(4);
        PROVIDE(flash_code_end = flash_code_begin + 1M);

    } > code0

    . = ORIGIN(ram0);

    .mmu_tlb ALIGN(0x4000):
    {
        *(.mmu_tlb_segment);
    } > ram0

 .boot_info ALIGN(32):
 {
  *(.boot_info)
        . = ALIGN(32);
 } > ram0

 .irq_stack ALIGN(32):
    {
        _cpu0_sstack_begin = .;
        PROVIDE(cpu0_sstack_begin = .);
        *(.stack)
        _cpu0_sstack_end = .;
        PROVIDE(cpu0_sstack_end = .);
     _stack_end = . ;
  . = ALIGN(4);

    } > ram0

    .data ALIGN(32):
 {
        . = ALIGN(4);
        *(.data_magic)

        . = ALIGN(32);

        . = ALIGN(4);
        *(.data*)

        . = ALIGN(4);


        btstack_data_start = .;
        *(.bt_stack_data)
        *(.ble_db_data)
        *(.ble_sm_data)
        *(.ble_att_data)
        *(.ble_gatt_data)


        BTSTACK_LE_HOST_MESH_DATA_START = .;
  . = (. +3) & ~ 3;
        _net_buf_pool_list = .;
        *(._net_buf_pool.static.*)

        *(.ble_mesh_data)
        *(.ble_mesh_tinycrypt_data)
        BTSTACK_LE_HOST_MESH_DATA_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_DATA_START);


        btstack_data_end = .;
 . = ALIGN(4);


        btctler_data_start = .;

        BTCTLER_CONTROLLER_DATA_START = .;
        *(.bt_rf_data)
  *(.vendor_manager_data)
  *(.device_manager_data)
  *(.hci_controller_data)
  *(.hci_interface_data)
        BTCTLER_CONTROLLER_DATA_SIZE = ABSOLUTE(. - BTCTLER_CONTROLLER_DATA_START);

        BTCTLER_LE_CONTROLLER_DATA_START = .;
        *(.ble_ll_data)
        *(.ble_hci_data)
        *(.classic_hci_data)
        *(.ble_rf_data)
        BTCTLER_LE_CONTROLLER_DATA_SIZE = ABSOLUTE(. - BTCTLER_LE_CONTROLLER_DATA_START);

        BTCTLER_CL_DATA_START = .;
        *(.classic_lmp_data)
        *(.classic_lmp_auth_data)
        *(.classic_lmp_bigint_data)
        *(.classic_lmp_crypt_data)
        *(.classic_lmp_ecdh_data)
        *(.classic_lmp_linkbulk_data)
        *(.classic_lmp_hmac_data)
        *(.classic_rf_data)
        *(.classic_bb_data)
        BTCTLER_CL_DATA_SIZE = ABSOLUTE(. - BTCTLER_CL_DATA_START);

        btctler_data_end = .;
 . = ALIGN(4);


  . = ALIGN(4);
        system_data_start = .;

  _video_subdev_begin = .;
  PROVIDE(video_subdev_begin = .);
    *(.video_subdev.0)
    *(.video_subdev.1)
    *(.video_subdev.2)
    *(.video_subdev.3)
    *(.video_subdev.4)
    *(.video_subdev.5)
  _video_subdev_end = .;
  PROVIDE(video_subdev_end = .);

  _audio_subdev_begin = .;
  PROVIDE(audio_subdev_begin = .);
    *(.audio_subdev.0)
    *(.audio_subdev.1)
    *(.audio_subdev.2)
    *(.audio_subdev.3)
  _audio_subdev_end = .;
  PROVIDE(audio_subdev_end = .);

  _iic_device_begin = .;
  PROVIDE(iic_device_begin = .);
    *(.iic)
  _iic_device_end = .;
  PROVIDE(iic_device_end = .);

        _avin_spi_device_begin = .;
  PROVIDE(avin_spi_device_begin = .);
   *(.sw_spi)
  _avin_spi_device_end = .;
  PROVIDE(avin_spi_device_end = .);

  _video_dev_begin = .;
  PROVIDE(video_dev_begin = .);
    *(.video_device)
  _video_dev_end = .;
  PROVIDE(video_dev_end = .);

  _key_driver_ops_begin = .;
  PROVIDE(key_driver_ops_begin = .);
     *(.key_driver_ops)
  _key_driver_ops_end = .;
  PROVIDE(key_driver_ops_end = .);

  _touch_driver_begin = .;
  PROVIDE(touch_driver_begin = .);
     *(.touch_driver)
  _touch_driver_end = .;
  PROVIDE(touch_driver_end = .);

  _static_hi_timer_begin = .;
  PROVIDE(static_hi_timer_begin = .);
     *(.hi_timer)
  _static_hi_timer_end = .;
  PROVIDE(static_hi_timer_end = .);

  _sys_cpu_timer_begin = .;
  PROVIDE(sys_cpu_timer_begin = .);
     *(.sys_cpu_timer)
  _sys_cpu_timer_end = .;
  PROVIDE(sys_cpu_timer_end = .);

     _sys_config_begin = .;
     PROVIDE(sys_config_begin = .);
         *(.sys_cfg)
     _sys_config_end = .;
     PROVIDE(sys_config_end = .);

     _sys_fat_begin = .;
     PROVIDE(sys_fat_begin = .);
         *(.fs_fat)
     _sys_fat_end = .;
     PROVIDE(sys_fat_end = .);

  _app_begin = .;
  PROVIDE(app_begin = .);
     *(.app)
  _app_end = .;
  PROVIDE(app_end = .);

  _os_begin = .;
  PROVIDE(os_begin = .);
     *(.os_code)
     *(.os_const)
     *(.os_str)
     *(.os_data)
  _os_end = .;
  PROVIDE(os_end = .);

     *(.crypto_ecdh_data)
     *(.crypto_data)

     *(.mem_data)
        *(.os_port_data)

     *(.uECC_data)
     *(.ECDH_sample_data)

        system_data_end = .;
 . = ALIGN(4);

        . = ALIGN(4);
  EQ_COEFF_BASE = . ;
  . = EQ_COEFF_BASE + 4 * EQ_SECTION_NUM * 14;

   } > ram0

    .bss ALIGN(32):
    {
  . = ALIGN(4);


        btctler_bss_start = .;

        BTCTLER_CONTROLLER_BSS_START = .;

        *(.bd_base)
        *(.bredr_rxtx_bulk)
        acl_tx_pool = .;
        *(.bredr_tx_bulk)

        acl_tx_pool_end = acl_tx_pool + (3 * 1024);



        . = acl_tx_pool_end;

        acl_rx_pool = .;
        *(.bredr_rx_bulk)

        acl_rx_pool_end = acl_rx_pool + (4 * 1024);



        . = acl_rx_pool_end;

        tws_bulk_pool = .;



        tws_bulk_pool_end = tws_bulk_pool;

        . = tws_bulk_pool_end;

        *(.bt_rf_bss)
  *(.hci_controller_bss)
  *(.hci_interface_bss)
  *(.device_manager_bss)
  *(.vendor_manager_bss)
        BTCTLER_CONTROLLER_BSS_SIZE = ABSOLUTE(. - BTCTLER_CONTROLLER_BSS_START);

        BTCTLER_LE_CONTROLLER_BSS_START = .;
        *(.ble_hci_bss)
        *(.ble_ll_bss)
        *(.ble_rf_bss)
        BTCTLER_LE_CONTROLLER_BSS_SIZE = ABSOLUTE(. - BTCTLER_LE_CONTROLLER_BSS_START);

        BTCTLER_CL_BSS_START = .;
        *(.classic_rf_bss)
        *(.classic_lmp_bss)
        *(.classic_lmp_auth_bss)
        *(.classic_lmp_bigint_bss)
        *(.classic_lmp_crypt_bss)
        *(.classic_lmp_ecdh_bss)
        *(.classic_lmp_linkbulk_bss)
        *(.classic_lmp_hmac_bss)
        *(.classic_bb_bss)
        *(.classic_hci_bss)
        BTCTLER_CL_BSS_SIZE = ABSOLUTE(. - BTCTLER_CL_BSS_START);

        btctler_bss_end = .;
 . = ALIGN(4);


        btstack_bss_start = .;
        *(.bt_stack_bss)
        *(.ble_db_bss)
        *(.ble_sm_bss)
        *(.ble_att_bss)
        *(.ble_gatt_bss)
        *(.btstack_pool)


        BTSTACK_LE_HOST_MESH_BSS_START = .;
        *(.ble_mesh_bss)
        *(.ble_mesh_tinycrypt_bss)
        BTSTACK_LE_HOST_MESH_BSS_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_BSS_START);

        btstack_bss_end = .;
 . = ALIGN(4);


        system_bss_start = .;
        . = ALIGN(4);
        *(.os_bss)
        *(.mem_heap)
  *(.memp_memory_x)
        *(.mem_bss)
        *(.os_port_bss)

        *(.uECC_bss)
        *(.ECDH_sample_bss)

        system_bss_end = .;
 . = ALIGN(4);

        *(.bss)
        . = ALIGN(4);
        *(.dts_dec_bss)
        *(.dts_bss)
        . = ALIGN(4);
        *(.m4a_dec_bss)
        *(.m4a_bss)
        . = ALIGN(4);
        *(COMMON)
        *(.volatile_ram)
  *(.audio_play_dma)

        . = ALIGN(4);
        *(.src_filt)
        *(.src_dma)

        . = ALIGN(4);
  *(.non_volatile_ram)
  . = ALIGN(32);
    } > ram0


 data_code_limit_begin = .;
 .data_code ALIGN(32):
 {
     data_code_begin = .;
        *(.common*)
        . = ALIGN(4);

  media_code_begin = .;
        *(.media.*.text)
  media_code_end = .;
        media_code_size = media_code_end - media_code_begin;
        . = ALIGN(4);

  *(.flushinv_icache)
        *(.os_critical_code)
        *(.os_rewrite_code)
        *(.volatile_ram_code)

  . = ALIGN(4);
        _SPI_CODE_START = . ;
        *(.spi_code)
  . = ALIGN(4);
        _SPI_CODE_END = . ;

     data_code_end = .;
 } > ram0

 overlay_begin = .;
 OVERLAY : NOCROSSREFS AT(0x200000) SUBALIGN(4)
    {
  .overlay_aec
  {
   *(.cvsd_data)
   *(.cvsd_const)
   *(.cvsd_code)

   *(.aec_bss_id)
   o_aec_end = .;

            *(.cvsd_codec)
   *(.aec_mem)
            *(.msbc_enc)
   *(.cvsd_bss)
  }
  .overlay_mp3
  {
   *(.mp3_bss_id)
   o_mp3_end = .;

   *(.mp3_mem)
   *(.mp3_ctrl_mem)
   *(.mp3pick_mem)
   *(.mp3pick_ctrl_mem)
  }
  .overlay_wma
  {
   *(.wma_bss_id)
   o_wma_end = .;

   *(.wma_mem)
   *(.wma_ctrl_mem)
   *(.wmapick_mem)
   *(.wmapick_ctrl_mem)
  }
  .overlay_wav
  {

   *(.wav_dec_data)

   *(.wav_data)

   *(.wav_bss_id)

   *(.wav_bss)
   *(.wav_dec_bss)
   *(.wav_mem)
   *(.wav_ctrl_mem)

  }
  .overlay_ape
        {
            *(.ape_mem)
            *(.ape_ctrl_mem)
   *(.ape_dec_data)
   *(.ape_dec_bss)

   *(.ape_bss_id)

   *(.ape_data)
  }
  .overlay_flac
        {
            *(.flac_mem)
            *(.flac_ctrl_mem)
   *(.flac_dec_data)
   *(.flac_dec_bss)

   *(.flac_bss)
   *(.flac_data)
  }
  .overlay_m4a
        {
            *(.m4a_mem)
            *(.m4a_ctrl_mem)
   *(.m4a_dec_data)
   *(.m4a_data)

   *(.aac_ctrl_mem)
   *(.aac_bss)
   *(.aac_data)
  }

  .overlay_amr
        {
            *(.amr_mem)
            *(.amr_ctrl_mem)
  }
  .overlay_dts
        {
            *(.dts_mem)
            *(.dts_ctrl_mem)
   *(.dts_dec_data)
   *(.dts_data)
  }

  .overlay_fm
  {
   *(.fm_code)
   *(.fm_bss_id)
   o_fm_end = .;
   *(.fm_mem)
   *(.linein_pcm_mem)
  }
        .overlay_pc
  {
            *(.usb_audio_play_dma)
            *(.usb_audio_rec_dma)
            *(.uac_rx)
            *(.mass_storage)

            *(.usb_ep0)
            *(.usb_msd_dma)
            *(.usb_hid_dma)
            *(.usb_iso_dma)
            *(.uac_var)
            *(.usb_config_var)
  }

    } > ram0
 data_code_limit_end = .;

 _HEAP_BEGIN = .;
 _HEAP_END = RAM_END;
}





SECTIONS
{
    .data : ALIGN(4)
    {
        update_data_start = .;

        update_data_end = .;
    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        update_bss_start = .;

        *(.update_bss)
        update_bss_end = .;
    } > ram0

    .text : ALIGN(4)
    {
  update_code_start = .;

  *(.bt_updata_ram_code)
  *(.update_const)
  *(.update_code)

  update_code_end = .;
    } > code0
}


SECTIONS
{
    .data : ALIGN(4)
    {
        media_data_start = .;

        . = ALIGN(4);
  EQ_COEFF_BASE = . ;
  . = EQ_COEFF_BASE + 4 * EQ_SECTION_NUM * 14;

        . = ALIGN(4);
        *(.aec_data)
        . = ALIGN(4);
        *(.res_data)
        . = ALIGN(4);
        *(.ns_data)
        . = ALIGN(4);
        *(.der_data)
        . = ALIGN(4);
        *(.nlp_data)
        . = ALIGN(4);
        *(.qmf_data)
        . = ALIGN(4);
        *(.fft_data)
        . = ALIGN(4);
  *(.bt_audioplc_data)
  *(.sms_data)

        *(.pcm_data)

  *(.g729_data)
  *(.g726_data)
  *(.wtg_dec_data)
  *(.mp3_data)
  *(.mp3_dec_data)
  *(.msbc_data)
  *(.mty_data)
  *(.mp3tsy_dec_data)
  *(.sbc_data)
  *(.sine_data)

  *(.wma_data)
  *(.wma_dec_data)

  *(.amr_data)
  *(.midi_data)

  *(.audio_decoder_data)



        *(.mp3_encode_data)
  *(.media_device_data)
  *(.audio_encoder_data)
  *(.mixer_data)
  *(.dec_server_data)
  *(.rec_server_data)
  *(.auto_mute_data)
  *(.plc_data)
  *(.wireless_sync_data)

  *(.resample_cal_data)
  *(.bt_compressor_data)
  *(.bt_crossOver_data)
  *(.bt_limiter_data)
  *(.bt_compressor_sparse_data)
  *(.bt_limiter_sparse_data)
  *(.bt_crossOver_sparse_data)

        . = ALIGN(4);
  *(.dec2tws_mem)
        media_data_end = .;

    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        media_bss_start = .;
        . = ALIGN(4);
  *(.aec_bss)
  *(.res_bss)
  *(.ns_bss)
  *(.der_bss)
  *(.nlp_bss)
  *(.qmf_bss)
  *(.bt_audioplc_bss)
  *(.fft_bss)
  *(.sms_bss)
        *(.pcm_bss)

  *(.g729_bss)
  *(.g726_bss)
  *(.wtg_dec_bss)
  *(.mp3_bss)
  *(.mp3_dec_bss)
  *(.msbc_bss)
  *(.mty_bss)
  *(.mp3tsy_dec_bss)
  *(.sbc_bss)
  *(.sine_bss)

  *(.wma_bss)
  *(.wma_dec_bss)

  *(.amr_bss)
  *(.midi_bss)

  *(.audio_decoder_bss)



        *(.mp3_encode_bss)
  *(.media_device_bss)
  *(.audio_encoder_bss)
  *(.mixer_bss)
  *(.dec_server_bss)
  *(.rec_server_bss)
  *(.auto_mute_bss)
  *(.plc_bss)
  *(.wireless_sync_bss)

  *(.audio_buf)

  *(.resample_cal_bss)
  *(.bt_crossOver_bss)
  *(.bt_compressor_bss)
  *(.bt_limiter_bss)
  *(.bt_limiter_sparse_bss)
  *(.bt_crossOver_sparse_bss)
  *(.bt_compressor_sparse_bss)
        media_bss_end = .;
    } > ram0


 .data_code ALIGN(32):
 {
        media_data_code_start = .;
        . = ALIGN(4);
  *(.sbc_eng_code)
        . = ALIGN(4);
        media_data_code_end = .;

 } > ram0


    .text : ALIGN(4)
    {
  . = ALIGN(4);
        media_text_start = .;

  *(.aec_code)
  *(.aec_const)
  *(.der_code)
  *(.der_const)
  *(.nlp_code)
  *(.nlp_const)
  *(.qmf_code)
  *(.qmf_const)
  *(.bt_audioplc_sparse_code)
  *(.bt_audioplc_code)
  *(.bt_audioplc_const)

  . = ALIGN(4);
  *(.res_code)
  *(.res_const)

  . = ALIGN(4);
  *(.ns_code)
  *(.ns_const)

  . = ALIGN(4);
        *(.pcm_code)
        *(.pcm_const)


  *(.g729_code)
  *(.g729_const)
  *(.g726_code)
  *(.g726_const)

  *(.wtg_dec_code)
  *(.wtg_dec_const)
  *(.wtg_dec_sparse_code)
  *(.wtg_dec_sparse_const)
        *(.bfilt_code)
        . = ALIGN(4);

  *(.mp3_decstream_const)
   *(.mp3_decstream_code)
   *(.mp3_decstream_sparse_code)
   *(.mp3_decstream_sparse_const)

   *(.mp3_dec_sparse_code)
   *(.mp3_dec_sparse_const)

   *(.mp3_dec_code)
   *(.mp3_dec_const)





   *(.mp3_const)
   *(.mp3_code)


  *(.msbc_code)
  *(.msbc_const)
  *(.mty_code)
  *(.mty_const)
  *(.mp3tsy_dec_code)
  *(.mp3tsy_dec_const)
  *(.mp3tsy_dec_sparse_code)
  *(.mp3tsy_dec_sparse_const)
  *(.sbc_code)
  *(.sbc_const)
  *(.sine_code)
  *(.sine_const)




  *(.wma_code)
  *(.wma_const)
  *(.wma_dec_code)
  *(.wma_dec_const)

  *(.wma_dec_sparse_code)
  *(.wma_dec_sparse_const)
  *(.wma_decstream_const)
  *(.wma_decstream_code)
  *(.wma_decstream_sparse_code)
  *(.wma_decstream_sparse_const)


  *(.amr_code)
  *(.amr_const)
  *(.midi_code)
  *(.midi_const)



  *(.audio_decoder_code)
  *(.audio_decoder_const)
        *(.mp3_encode_code)
        *(.mp3_encode_const)
  *(.media_device_code)
  *(.media_device_const)
  *(.audio_encoder_code)
  *(.audio_encoder_const)
  *(.mixer_code)
  *(.mixer_const)
  *(.dec_server_code)
  *(.dec_server_const)
  *(.rec_server_code)
  *(.rec_server_const)
  *(.auto_mute_code)
  *(.auto_mute_const)
  *(.plc_code)
  *(.plc_const)
  *(.wireless_sync_code)
  *(.wireless_sync_const)
  *(.fft_code)
  *(.fft_const)


  *(.mp3_enc_code)
  *(.mp3_enc_const)
  *(.mp3_enc_sparse_code)
  *(.mp3_enc_sparse_const)
  . = ALIGN(4);
  *(.mp2_encode_code)
  *(.mp2_encode_const)
  *(.mp2_encode_sparse_code)
  *(.mp2_encode_sparse_const)
  . = ALIGN(4);
  *(.adpcm_encode_code)
  *(.adpcm_encode_const)
  *(.adpcm_encode_sparse_code)
  *(.adpcm_encode_sparse_const)
  . = ALIGN(4);


  *(.m4a_dec_sparse_code)
  *(.m4a_dec_sparse_const)

   *(.aac_dec_sparse_code)
  *(.aac_dec_sparse_const)
  *(.bt_aac_dec_core_const)

  *(.amr_dec_const)


  *(.dts_dec_ff_const)
  *(.dts_dec_sparse_code)
  *(.dts_dec_sparse_const)

  *(.res_sparse_code)
  *(.ns_sparse_code )
  *(.aec_sparse_code)
  *(.nlp_sparse_code)
  *(.der_sparse_code)
  *(.qmf_sparse_code)

  *(.sms_const);
  *(.sms_sparse_code);
  *(.sms_code);


  *(.resample_cal_code)
  *(.resample_cal_sparse_code)
  *(.resample_cal_const)

  *(.bt_compressor_sparse_const)
  *(.bt_compressor_sparse_code)
  *(.compressor_sparse_code)
  *(.compressor_sparse_const)

  *(.bt_limiter_sparse_const)
  *(.bt_limiter_sparse_code)
  *(.limiter_sparse_code)
  *(.limiter_sparse_const)

  *(.bt_crossOver_sparse_const)
  *(.bt_crossOver_sparse_code)
  *(.crossOver_sparse_code)
  *(.crossOver_sparse_const)

  *(.audio_codec_code)

  . = ALIGN(4);
     _audio_decoder_begin = .;
     PROVIDE(audio_decoder_begin = .);
         *(.audio_decoder)
     _audio_decoder_end = .;
     PROVIDE(audio_decoder_end = .);

     _audio_encoder_begin = .;
     PROVIDE(audio_encoder_begin = .);
         *(.audio_encoder)
     _audio_encoder_end = .;
     PROVIDE(audio_encoder_end = .);

     _audio_package_begin = .;
     PROVIDE(audio_package_begin = .);
         *(.audio_package)
     _audio_package_end = .;
     PROVIDE(audio_package_end = .);

     _audio_dev_begin = .;
     PROVIDE(audio_dev_begin = .);
         *(.audio_device)
     _audio_dev_end = .;
     PROVIDE(audio_dev_end = .);

     _audio_hwaccel_begin = .;
     PROVIDE(audio_hwaccel_begin = .);
         *(.audio_hwaccel)
     _audio_hwaccel_end = .;
     PROVIDE(audio_hwaccel_end = .);

  . = ALIGN(4);
     media_code_begin = .;
         *(.media.*.text)

  . = ALIGN(4);
  *(.crossOver_code)
  *(.crossOver_const)
  *(.bt_crossOver_const)
  *(.bt_crossOver_code)
  . = ALIGN(4);

  *(.compressor_code)
  *(.compressor_const)
  *(.bt_compressor_const)
  *(.bt_compressor_code)
  . = ALIGN(4);

  *(.limiter_code)
  *(.limiter_const)
  *(.bt_limiter_const)
  *(.bt_limiter_code)
  . = ALIGN(4);
  *(.lib_pitchshift_code)
  *(.lib_pitchshift_const)
  . = ALIGN(4);


     media_code_end = .;
  . = ALIGN(4);
  media_code_size = media_code_end - media_code_begin;

  . = ALIGN(4);
        media_text_end = .;

    } > code0
}
SECTIONS
{
    .data : ALIGN(4)
    {
        driver_data_start = .;


        CLOCK_DATA_START = .;
        *(.clock_data)
        CLOCK_DATA_SIZE = ABSOLUTE(. - CLOCK_DATA_START);

        *(.debug_data)
        *(.power_data)

        *(.uart_data)

        driver_data_end = .;

    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        driver_bss_start = .;

        CLOCK_BSS_START = .;
        *(.clock_bss)
        CLOCK_BSS_SIZE = ABSOLUTE(. - CLOCK_BSS_START);

        *(.debug_bss)
        *(.power_bss)

        *(.uart_bss)

        *(.sd_var)
        driver_bss_end = .;
    } > ram0

    .text : ALIGN(4)
    {
        driver_code_start = .;

        CLOCK_CODE_START = .;
        *(.clock_code)
        *(.clock_const)
        CLOCK_CODE_SIZE = ABSOLUTE(. - CLOCK_CODE_START);

        *(.debug_code)
        *(.debug_const)

        *(.power_code)
        *(.power_const)

        *(.uart_code)
        *(.uart_const)

        driver_code_end = .;
  . = ALIGN(4);
    } > code0


    DRIVER_RAM_TOTAL = (driver_data_end - driver_data_start) + (driver_bss_end - driver_bss_start);
    DRIVER_CODE_TOTAL = (driver_code_end - driver_code_start);
}


text_begin = ADDR(.text);
text_size = SIZEOF(.text);
text_end = ADDR(.text) + SIZEOF(.text);

bss_begin = ADDR(.bss);
bss_size = SIZEOF(.bss);
bss_end = bss_begin + bss_size;

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size = SIZEOF(.data);

data_code_addr = ADDR(.data_code);
data_code_begin = data_begin + data_size;
data_code_size = SIZEOF(.data_code);




aec_addr = ADDR(.overlay_aec);
aec_begin = data_code_begin + data_code_size;
aec_size = o_aec_end - aec_addr;


wav_addr = ADDR(.overlay_wav);
wav_begin = aec_begin + aec_size;
wav_size = SIZEOF(.overlay_wav);

ape_addr = ADDR(.overlay_ape);
ape_begin = wav_begin + wav_size;
ape_size = SIZEOF(.overlay_ape);

flac_addr = ADDR(.overlay_flac);
flac_begin = ape_begin + ape_size;
flac_size = SIZEOF(.overlay_flac);

m4a_addr = ADDR(.overlay_m4a);
m4a_begin = flac_begin + flac_size;
m4a_size = SIZEOF(.overlay_m4a);

amr_addr = ADDR(.overlay_amr);
amr_begin = m4a_begin + m4a_size;
amr_size = SIZEOF(.overlay_amr);

dts_addr = ADDR(.overlay_dts);
dts_begin = amr_begin + amr_size;
dts_size = SIZEOF(.overlay_dts);

fm_addr = ADDR(.overlay_fm);
fm_begin = dts_begin + dts_size;
fm_size = o_fm_end - fm_addr;
overlay_code_end_addr = fm_begin + fm_size;




bank_code_load_addr = overlay_code_end_addr;


PROVIDE(HEAP_BEGIN = _HEAP_BEGIN);
PROVIDE(HEAP_END = _HEAP_END);
_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);
