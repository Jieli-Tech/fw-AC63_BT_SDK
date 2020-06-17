
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

