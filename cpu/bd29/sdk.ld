




RAM_LIMIT_L = 0x00000;
RAM_LIMIT_H = 0x0C000;





ISR_SIZE = 0x100;
ISR_BASE = RAM_LIMIT_H - ISR_SIZE;





ROM_RAM_SIZE = 0x400;
ROM_RAM_BEGIN = RAM_LIMIT_H - ROM_RAM_SIZE - ISR_SIZE;




UPDATA_SIZE = 0x80;
UPDATA_BEG = RAM_LIMIT_H - UPDATA_SIZE - ROM_RAM_SIZE - ISR_SIZE;

UPDATA_COPY_BEG = 0x1d000 - UPDATA_SIZE;




RAM_BEGIN = RAM_LIMIT_L;
RAM_END = RAM_LIMIT_H - UPDATA_SIZE - ROM_RAM_SIZE - ISR_SIZE;
RAM_SIZE = RAM_END - RAM_BEGIN;

UPDATA_BREDR_BASE_BEG = 0x1C000;

MEMORY
{




 code0(rx) : ORIGIN = 0x1E000C0, LENGTH = (1024 * 1024)




 ram0(rwx) : ORIGIN = RAM_BEGIN, LENGTH = RAM_SIZE
    irq_table(rw) : ORIGIN = 0xbf00, LENGTH = 0x100
}


  local_irq_enable = ABSOLUTE(0x103e88);
  local_irq_disable = ABSOLUTE(0x103e8c);







  memmem = ABSOLUTE(0x103fa8);
  memcpy = ABSOLUTE(0x103fac);
  memmove = ABSOLUTE(0x103fb0);
  memcmp = ABSOLUTE(0x103fb4);
  memset = ABSOLUTE(0x103fb8);
  strcmp = ABSOLUTE(0x103fbc);
  strcpy = ABSOLUTE(0x103fc0);
  strlen = ABSOLUTE(0x103fc4);
  strncmp = ABSOLUTE(0x103fc8);
  strstr = ABSOLUTE(0x103fcc);
  mask_init = ABSOLUTE(0x103fd0);
  wdt_clr = ABSOLUTE(0x103fd4);
  nvram_set_boot_state = ABSOLUTE(0x103fd8);
  nvram_jumpaddr_set = ABSOLUTE(0x103fdc);
  nvram_signature_set = ABSOLUTE(0x103fe0);
  sfc_suspend = ABSOLUTE(0x103fe4);
  sfc_resume = ABSOLUTE(0x103fe8);
  sfc_drop_cache = ABSOLUTE(0x103fec);
  chip_crc16 = ABSOLUTE(0x103ff0);
  CrcDecode = ABSOLUTE(0x103ff4);
  chip_reset = ABSOLUTE(0x103ff8);
  _IRQ_MEM_ADDR = ABSOLUTE(0xbf00);
  _MASK_MEM_BEGIN = ABSOLUTE(0xbb00);
  _MASK_MEM_SIZE = ABSOLUTE(0x3f0);
  boot_arg_list = ABSOLUTE(0xbee4);
  the_debug_isr = ABSOLUTE(0x100016);
  os_init = ABSOLUTE(0x103eac);
  os_start = ABSOLUTE(0x103eb0);
  os_get_curr_tcb_var = ABSOLUTE(0x103eb4);
  os_task_create = ABSOLUTE(0x103eb8);
  os_current_task_rom = ABSOLUTE(0x103ebc);
  os_time_dly = ABSOLUTE(0x103ec0);
  os_time_get = ABSOLUTE(0x103ec4);
  os_task_del_req = ABSOLUTE(0x103ec8);
  os_task_del_res = ABSOLUTE(0x103ecc);
  os_task_del = ABSOLUTE(0x103ed0);
  __os_taskq_pend = ABSOLUTE(0x103ed4);
  os_taskq_pend = ABSOLUTE(0x103ed8);
  os_task_pend = ABSOLUTE(0x103edc);
  __os_taskq_post = ABSOLUTE(0x103ee0);
  os_taskq_post = ABSOLUTE(0x103ee4);
  os_taskq_post_type = ABSOLUTE(0x103ee8);
  os_taskq_flush = ABSOLUTE(0x103eec);
  task_queue_post_event = ABSOLUTE(0x103ef0);
  os_taskq_post_msg = ABSOLUTE(0x103ef4);
  os_taskq_post_event = ABSOLUTE(0x103ef8);
  os_taskq_accept = ABSOLUTE(0x103efc);
  os_taskq_del = ABSOLUTE(0x103f00);
  os_taskq_del_type = ABSOLUTE(0x103f04);
  os_sem_create = ABSOLUTE(0x103f08);
  os_sem_del = ABSOLUTE(0x103f0c);
  os_sem_pend = ABSOLUTE(0x103f10);
  os_sem_post = ABSOLUTE(0x103f14);
  os_sem_set = ABSOLUTE(0x103f18);
  os_sem_valid = ABSOLUTE(0x103f1c);
  os_sem_query = ABSOLUTE(0x103f20);
  os_mutex_create = ABSOLUTE(0x103f24);
  os_mutex_del = ABSOLUTE(0x103f28);
  os_mutex_pend = ABSOLUTE(0x103f2c);
  os_mutex_post = ABSOLUTE(0x103f30);
  os_mutex_valid = ABSOLUTE(0x103f34);
  xTaskCreate = ABSOLUTE(0x103f38);
  xTaskGetCurrentTaskHandle = ABSOLUTE(0x103f3c);
  xTaskGetTickCount = ABSOLUTE(0x103f40);
  xTaskGetHandle = ABSOLUTE(0x103f44);
  vTaskDelete = ABSOLUTE(0x103f48);
  xTaskGetApplicationTaskTag = ABSOLUTE(0x103f4c);
  vTaskSetApplicationTaskTag = ABSOLUTE(0x103f50);
  xQueueGenericCreateStatic = ABSOLUTE(0x103f54);
  xQueueGenericReset = ABSOLUTE(0x103f58);
  xQueueGenericSend = ABSOLUTE(0x103f5c);
  xQueueGenericSendFromISR = ABSOLUTE(0x103f60);
  xQueueGenericReceive = ABSOLUTE(0x103f64);
  xQueueReceiveFromISR = ABSOLUTE(0x103f68);
  vQueueDelete = ABSOLUTE(0x103f6c);
  uxQueueMessagesWaiting = ABSOLUTE(0x103f70);
  uxQueueMessagesWaitingFromISR = ABSOLUTE(0x103f74);
  uxQueueSpacesAvailable = ABSOLUTE(0x103f78);
  pcTaskGetName = ABSOLUTE(0x103f7c);
  prvGetExpectedIdleTime = ABSOLUTE(0x103f80);
  eTaskConfirmSleepModeStatus = ABSOLUTE(0x103f84);
  vTaskStepTick = ABSOLUTE(0x103f88);
  pxPortInitialiseStack = ABSOLUTE(0x103f8c);
  xPortStartScheduler = ABSOLUTE(0x103f90);
  vPortEndScheduler = ABSOLUTE(0x103f94);
  vPortYield = ABSOLUTE(0x103f98);
  vPortExitCritical = ABSOLUTE(0x103f9c);
  OS_ClrPending = ABSOLUTE(0x103fa0);
  vTickISR = ABSOLUTE(0x103fa4);

ENTRY(_start)

SECTIONS
{

    . = ORIGIN(code0);
    .text ALIGN(4):
    {
        PROVIDE(text_rodata_begin = .);

        *startup.o(.text)

        *(.text*)
        *(.rodata*)

        *(.LOG_TAG_CONST*)

  . = ALIGN(4);
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

  . = ALIGN(4);

        OMSensor_dev_begin = .;
        KEEP(*(.omsensor_dev))
        OMSensor_dev_end = .;

  . = ALIGN(4);
        storage_device_begin = .;
        KEEP(*(.storage_device))
        storage_device_end = .;

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

    } >code0

    . = ORIGIN(ram0);


    .mmu_tlb ALIGN(0x4):
    {
        *(.mmu_tlb_segment);
    } > ram0


    .data ALIGN(4):
   {
     _data_code_begin = . ;
        . = ALIGN(4);
        *(.data_magic)

        . = ALIGN(4);
        *(.flushinv_icache)
        *(.volatile_ram_code)
        *(.os_critical_code)
  _os_begin = .;
  PROVIDE(os_begin = .);
     *(.os_code)
     *(.os_const)
     *(.os_str)
     *(.os_data)
  _os_end = .;
  PROVIDE(os_end = .);

        *(reboot.rodata)
        *(.non_volatile_ram_code)

     _data_code_end = . ;

     _cpu_store_begin = . ;

        . = ALIGN(4);
        *(.data*)

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

   } > ram0

    .irq_stack ALIGN(32) :
    {
  *(.stack_magic)
     _stack_begin = . ;
  *(.stack)
     _stack_end = . ;
  *(.stack_magic0)

  . = ALIGN(4);
  *(.boot_info)

    } > ram0

    .bss ALIGN(32) :
    {
        *(.bss)
        *(COMMON)
        *(.volatile_ram)
        *(.btstack_pool)
        *(.usb_dma_mem)



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

        acl_rx_pool_end = acl_rx_pool + (3 * 1024);



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

  . = (( . + 31) / 32 * 32);
    } > ram0

    _cpu_store_end = . ;

    _prp_store_begin = . ;
    .prp_bss ALIGN(32) :
    {





  . = (( . + 31) / 32 * 32);
    } > ram0
    _prp_store_end = . ;


 OVERLAY : NOCROSSREFS AT(0x200000) SUBALIGN(4)
    {
  .overlay_aec
  {
   *(.aec_mem)
            *(.msbc_enc)
            *(.cvsd_codec)
  }
  .overlay_mp3
  {
   *(.mp3_mem)
   *(.mp3_ctrl_mem)
  }
  .overlay_wma
  {
   *(.wma_mem)
   *(.wma_ctrl_mem)
  }
  .overlay_wav
  {
   *(.wav_mem)
   *(.wav_ctrl_mem)
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

    .nv_bss ALIGN(32) :
 {
  NVRAM_DATA_START = .;
  *(.non_volatile_ram)
  NVRAM_DATA_SIZE = ABSOLUTE(. - NVRAM_DATA_START);
  . = ALIGN(4);
  NVRAM_END = .;
  _nv_pre_begin = . ;
    } > ram0


    NVRAM_LIMIT = 0xc000 - 0x100 - 0x400;
    _nv_pre_end = 0xc000 - 0x100 - 0x400;
    ASSERT(NVRAM_END <= NVRAM_LIMIT, "NVRAM space overflow!")
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

    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
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
    } > ram0

    .text : ALIGN(4)
    {
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
    } > code0
}

BTSTACK_LE_HOST_MESH_RAM_TOTAL = BTSTACK_LE_HOST_MESH_DATA_SIZE + BTSTACK_LE_HOST_MESH_BSS_SIZE;
BTSTACK_LE_HOST_MESH_FLASH_TOTAL = BTSTACK_LE_HOST_MESH_CODE_SIZE;
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

        driver_bss_end = .;
    } > ram0

    .text : ALIGN(4)
    {
  . = ALIGN(4);
        driver_code_start = .;

        _SPI_CODE_START = . ;
        *(.spi_code)
  . = ALIGN(4);
        _SPI_CODE_END = . ;

        clock_critical_handler_begin = .;
        KEEP(*(.clock_critical_txt))
        clock_critical_handler_end = .;

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

text_begin = ADDR(.text) ;
text_size = SIZEOF(.text) ;
text_end = ADDR(.text) + SIZEOF(.text) ;

bss_begin = ADDR(.bss) ;
bss_size = SIZEOF(.bss);


nvbss_begin = NVRAM_DATA_START;
nvbss_size = NVRAM_LIMIT - nvbss_begin;

data_addr = ADDR(.data) ;
data_begin = text_begin + text_size;
data_size = SIZEOF(.data) ;

_HEAP_BEGIN = NVRAM_END;
PROVIDE(HEAP_BEGIN = NVRAM_END);

_HEAP_END = NVRAM_LIMIT - UPDATA_SIZE - 1;
PROVIDE(HEAP_END = NVRAM_LIMIT - UPDATA_SIZE);

_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);
