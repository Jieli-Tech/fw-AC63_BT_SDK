

SECTIONS
{
    .data : ALIGN(4)
    {
        update_data_start = .;

        update_data_end = .;
		. = ALIGN(4);
    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        update_bss_start = .;

        *(.update_bss)

        update_bss_end = .;
		. = ALIGN(4);
    } > ram0

    .text : ALIGN(4)
    {
		update_code_start = .;

		*(.bt_updata_ram_code)
		*(.update_const)
		*(.update_code)

		update_code_end = .;
		. = ALIGN(4);
    } > code0


    UPDATE_CODE_TOTAL_SIZE = update_code_end - update_code_start;
}


