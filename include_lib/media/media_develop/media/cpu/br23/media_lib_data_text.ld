        media_data_code_start = .;
        . = ALIGN(4);
#if (SOUNDCARD_ENABLE)
		*(.dvol_ram_code)
		*(.reverb_cal_const)
		*(.reverb_cal_code)
		*(.mp3_code)

		*(.stream_code_copy_run)		// 数据流分支输出处理
		*(.mixer_code_out_run)			// mixer数据流叠加后输出

#endif

#if (SOUNDCARD_ENABLE && !TCFG_CODE_RUN_RAM_FM_MODE)
		*(.stream_code_nor_run)			// 数据流普通输出处理
		*(.mixer_code_nor_run)			// mixer数据流直通输出
#endif /*(SOUNDCARD_ENABLE && !TCFG_CODE_RUN_RAM_FM_MODE)*/

		*(.sbc_eng_code)
#if (TCFG_CODE_RUN_RAM_FM_MODE)
        *(.pcm_code)
        *(.pcm_const)
		*(.audio_dec_pcm_code)
		*(.audio_dec_pcm_const)
		*(.stream_code_nor_run)			// 数据流普通输出处理
		*(.mixer_code_nor_run)			// mixer数据流直通输出
		*(.audio_codec_code)			// 解码任务流程
		*(.audio_codec_code_read_file)	// 解码读文件
		*(.audio_src_code_nor_run)		// audio_src运行处理
		*(.src_base_code_nor_run)		// src_base运行处理
		*(.audio_dac_code_nor_run)		// audio_dac普通输出运行处理
		*(.audio_cfifo_code_nor_run)	// audio_cfifo普通运行处理
		*(.audio_track_code)
		*(.audio_track_const)
#endif /*(TCFG_CODE_RUN_RAM_FM_MODE)*/
        . = ALIGN(4);
        media_data_code_end = .;
