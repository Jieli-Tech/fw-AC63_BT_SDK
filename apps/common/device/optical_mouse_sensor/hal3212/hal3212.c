#include "OMSensor_config.h"
#include "OMSensor_manage.h"
#include "asm/gpio.h"
#include "app_config.h"
#include "stdlib.h"
#if TCFG_HAL3212_EN
#define LOG_TAG         "[hal3212]"
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ERROR_ENABLE
#include "debug.h"

#define CPI_800     0x15   /* 0x15 * 38 =800    38 per len            */
#define CPI_1200    0x1b   /*27 * 38 = 1026*/
#define CPI_1600    0x2a   /*0x2a * 38 =1600 */



#define CONFIG_CPI		0b111

static OMSENSOR_PLATFORM_DATA *pdata = NULL;
static u8 time_count = 0;
static const u8 timeout = 5;
static u8 configvalue = 0 ;
static void uSecDelay(u8 len);
static void frameDelay();
static bool hal_pixart_init(OMSENSOR_PLATFORM_DATA *priv);
static bool  hal_pixart_readMotion(s16 *deltaX, s16 *deltaY);
static u8 hal_pixart_readRegister(u8 regAddress);
static void hal_pixart_writeRegister(u8 regAddress, u8 innData);
static u8 hal_pixart_read(void);
static void hal_pixart_write(u8 dataOut);
static void resync(void);
/* static u16 hal3205_set_cpi(u16 cpi); */
static bool hal_pixart_resync(void);
static void hal_pixart_led_switch(u8 led_status);
static void get_overflow_status(u8 motion);
static void hal_pixart_info_dump(void);
static void hal_pixart_set_cpi_init(u16 dst_cpi);


const u16 cpi_table[] = {
    CPI_800, CPI_1200, CPI_1600,
};



//功能：超时计数
static void time_counter(void)
{
    time_count++;
}

//功能：毫秒级延时
static void uSecDelay(u8 len)
{
    len += (len >> 1);
    while (len--);
}

// Approximately 2 mSec delay
static void frameDelay()
{
    u8 frameDel = 200;
    while (frameDel--) {
        uSecDelay(98);
    }
}

static void hal_pixart_gpio_init(void)
{
    gpio_set_die(pdata->OMSensor_sclk_io, 1);          //CLK output
    gpio_set_dieh(pdata->OMSensor_sclk_io, 1);
    gpio_set_direction(pdata->OMSensor_sclk_io, 0);

    gpio_set_die(pdata->OMSensor_data_io, 1);         //SDIO output
    gpio_set_dieh(pdata->OMSensor_data_io, 1);
    gpio_set_direction(pdata->OMSensor_data_io, 0);

    gpio_set_die(pdata->OMSensor_int_io, 1);          //int_io input
    gpio_set_dieh(pdata->OMSensor_int_io, 1);
    gpio_set_direction(pdata->OMSensor_int_io, 1);
    gpio_set_pull_up(pdata->OMSensor_int_io, 0);
    gpio_set_pull_down(pdata->OMSensor_int_io, 0);

    gpio_set_output_value(pdata->OMSensor_sclk_io, 1);//SCLK is high

}

//功能：paw3205初始化
static void paw3212_init(void)
{
    u8 reg = 0;

    hal_pixart_writeRegister(pixart_WP_ADDR, 0x5a);	//Unlock WP


    //hal_pixart_writeRegister(0x02, 0x81);

    hal_pixart_writeRegister(0x19, 0x00);   //8bit表示一个数据 还有一种配置12bit
    hal_pixart_writeRegister(0x07, 0x08);   // X Y only
    hal_pixart_writeRegister(0x26, 0x34);  //2_wired mode SPI
    hal_pixart_writeRegister(0x4b, 0x04);   //turn off internal current low power1.7-2.1v
    hal_pixart_writeRegister(0x06, 0x11);
    /* hal_pixart_writeRegister(0x05, 0xb8); */
    /* hal_pixart_writeRegister(0x5b, 0x63);  //LED continue mode */




    hal_pixart_set_cpi_init(CPI_1200);

    hal_pixart_writeRegister(pixart_WP_ADDR, 0x00);	//Lock WP
}

static bool hal_pixart_init(OMSENSOR_PLATFORM_DATA *priv)
{
    pdata = priv;

    //初始化GPIO
    hal_pixart_gpio_init();

    //初始化延时，确保初始化成功
    frameDelay();

    //初始化定时器，用以超时检查
    sys_s_hi_timer_add(NULL, time_counter, 20);

    //同步通信检查
    if (!hal_pixart_resync()) {
        goto _init_fail_;
    }

#ifdef EN_PAW3212
    paw3212_init(); //paw3212初始化
#endif
    hal_pixart_info_dump();

    return true;

_init_fail_:
    log_error("optical sensor init fail!!!");
    return false;
}


//功能：检测传感器数据是否可读
static u8 hal_pixart_data_ready(void)
{

    return (!gpio_read(pdata->OMSensor_int_io));
}


//功能：光学感应器寄存器信息打印
static void hal_pixart_info_dump(void)
{


    log_info("product  ID_00   = 0x%x.\n", hal_pixart_readRegister(pixart_PID0_ADDR));
    log_info("product  ID_01   = 0x%x.\n", hal_pixart_readRegister(pixart_PID1_ADDR));
    log_info("Motion_Status    = 0x%x.\n", hal_pixart_readRegister(pixart_MOTION_ADDR));
    log_info("Opreation_Mode   = 0x%x.\n", hal_pixart_readRegister(pixart_OPMODE_ADDR));
    log_info("configuration    = 0x%x.\n", hal_pixart_readRegister(pixart_CONFIG_ADDR));
    log_info("Sleep_mode1      = 0x%x.\n", hal_pixart_readRegister(pixart_SLP1_ADDR));
    log_info("IQC              = 0x%x.\n", hal_pixart_readRegister(pixart_IQC_ADDR));
    log_info("Sleep_mode2      = 0x%x.\n", hal_pixart_readRegister(pixart_SLP2_ADDR));
    log_info("Shutter          = 0x%x.\n", hal_pixart_readRegister(pixart_Shutter_ADDR));
    log_info("Sleep_mode3      = 0x%x.\n", hal_pixart_readRegister(pixart_SLP3_ADDR));
    log_info("Frame_Avg        = 0x%x.\n", hal_pixart_readRegister(pixart_Frame_Avg_ADDR));
    log_info("Mouse_Option     = 0x%x.\n", hal_pixart_readRegister(pixart_Mouse_Option_ADDR));
    log_info("POWER_DOWN_CONFIG= 0x%x.\n", hal_pixart_readRegister(pixart_POWER_DOWN_ADDR));


    log_info("\n");
}


//功能：查看传感器的睡眠状态
static u8 hal_pixart_status_dump(void)
{
    u8 reg = 0;
    u8 sleep = 0;


    if ((reg  & 0x07) == 0x06) {
        if (reg & BIT(3)) {
            sleep = 2;
        } else {
            sleep = 1;
        }
    }

    else {
        sleep = 0;
    }

    return sleep;
}

//功能：强制唤醒传感器
static void hal_pixart_force_wakeup(void)
{
    u8 reg = 0;

    reg = hal_pixart_readRegister(pixart_OPMODE_ADDR);
    reg |= BIT(0);

    hal_pixart_writeRegister(pixart_WP_ADDR, 0x5a);	//Lock WP
    hal_pixart_writeRegister(pixart_OPMODE_ADDR, reg);
    hal_pixart_writeRegister(pixart_WP_ADDR, 0x00);	//Lock WP
}

static void hal_pixart_led_switch(u8 led_status)
{
    u8 reg = 0;
    u8 ret = 0;

    reg = hal_pixart_readRegister(pixart_POWER_DOWN_ADDR);
    ret = hal_pixart_readRegister(pixart_CONFIG_ADDR);


    if (led_status) {
        ret &= ~BIT(3);
        ret |= BIT(5);
        reg = 0x1b;

    } else {
        ret |= BIT(3);
        reg = 0x13;
    }

    hal_pixart_writeRegister(pixart_WP_ADDR, 0x5a);	//Lock WP
    hal_pixart_writeRegister(pixart_CONFIG_ADDR, ret);
    hal_pixart_writeRegister(pixart_POWER_DOWN_ADDR, reg);
    hal_pixart_writeRegister(pixart_WP_ADDR, 0x00);	//Lock WP
    hal_pixart_readRegister(pixart_CONFIG_ADDR);

}


//功能：读数据
static bool hal_pixart_readMotion(s16 *deltaX, s16 *deltaY)
{
    u8 pid = 0, motion = 0, i = 0, tmp_X = 0, tmp_Y = 0;
    s16 deltaX_l = 0, deltaY_l = 0, deltaX_h = 0, deltaY_h = 0;

    /* if (!gpio_read(pdata->OMSensor_int_io)) */
    /* { */

    //同步通信检查
    if (!hal_pixart_resync()) {
        goto _resync_fail_;
    }

    for (i = 0; i < 2; i++) {
        motion = hal_pixart_readRegister(pixart_MOTION_ADDR);

        uSecDelay(1);

        get_overflow_status(motion);

        if (motion & 0x80) {

            tmp_X = hal_pixart_readRegister(pixart_DELTAX_ADDR);
            uSecDelay(1);

            tmp_Y = hal_pixart_readRegister(pixart_DELTAY_ADDR);
            uSecDelay(1);

            if (tmp_X & 0x80) {
                deltaX_h -= (int8_t)(~tmp_X + 1);
            } else {
                deltaX_h += tmp_X;
            }

            if (tmp_Y & 0x80) {
                deltaY_h -= (int8_t)(~tmp_Y + 1);
            } else {
                deltaY_h += tmp_Y;
            }
        }
    }


    *deltaX = deltaX_h;
    *deltaY = deltaY_h;


    return true;

_resync_fail_:
    log_error("optical sensor resync fail!!!");
    return false;
}


static u8 hal_pixart_readRegister(u8 regAddress)
{
    u8 returnVal = 0;

    hal_pixart_write(regAddress);			//Read address
    returnVal = hal_pixart_read();		//Read data

    return (returnVal);
}


static void hal_pixart_writeRegister(u8 regAddress, u8 innData)
{
    u8 time = 0;

    time = time_count;


    do {	//Ensure write setting correct
        hal_pixart_write(pixart_WRITE | regAddress);  //Write address
        hal_pixart_write(innData);

        if (abs(time - time_count) >= timeout) { //写操作超时
            log_error("optical sensor writting timeout!!!");
            break;
        }
    } while (hal_pixart_readRegister(regAddress) != innData);

}
/* static void hal_pixart_writeRegister(u8 regAddress, u8 innData) */
/* { */
/*   do	//Ensure write setting correct */
/*   { */
/*   	hal_pixart_write(pixart_WRITE | regAddress);  //Write address */
/*   	hal_pixart_write(innData); */
/*   }while( hal_pixart_readRegister(regAddress)!= innData ); */
/* } */


static bool  hal_pixart_resync(void)
{
    u8 time = 0;

    time = time_count;

    while (hal_pixart_readRegister(pixart_PID0_ADDR) != 0x30 && hal_pixart_readRegister(pixart_PID0_ADDR) != 0x31) {	//Make sure SPI is sync
        log_debug("sensor Id :%x", hal_pixart_readRegister(pixart_PID0_ADDR));
        uSecDelay(10);
        resync();

        if (abs(time - time_count) >= timeout) { //信号同步超时
            log_error("optical sensor resync fail!!!");
            return false;
        }
    }
    return true;
}


static u8 hal_pixart_read(void)
{
    u8 returnVal = 0;
    u8 bitCnt = 0;

    /*SDIO在SCLK处于下降沿时被sensor修改，在SCLK处于上升沿时被controller读取*/
    OS_ENTER_CRITICAL();

    gpio_set_direction(pdata->OMSensor_data_io, 1);  //SDIO, as input
    gpio_set_pull_up(pdata->OMSensor_data_io, 1);
    gpio_set_pull_down(pdata->OMSensor_data_io, 0);

    for (bitCnt = 8; bitCnt != 0; bitCnt--) { /* Read 8 bits MSB...LSB */
        //new spi ~760khz(21 cycles)
        gpio_set_output_value(pdata->OMSensor_sclk_io, 0);    //SCLK is low, 3 cycles
        /* gpio_set_output_value(pdata->OMSensor_sclk_io, 0); //3 cycles */

        returnVal <<= 1;	                                  //Shift read data, 5 cycles

        delay(1);                                             //3 cycles

        gpio_set_output_value(pdata->OMSensor_sclk_io, 1);    //SCLK is high, 3 cycles

        if (gpio_read(pdata->OMSensor_data_io)) {             //JNB 4 cycles
            returnVal |= 0x01;	                              //DJNZ 3 cycles; if SDIO, plus 4 cycles
        }
        delay(1);
    }

    OS_EXIT_CRITICAL();
    return (returnVal);
}


static void hal_pixart_write(u8 dataOut)
{
    u8 bitCnt = 0;

    OS_ENTER_CRITICAL();
    /*SDIO在SCLK处于下降沿时被controller修改，在SCLK处于上升沿时被sensor读取*/
    gpio_set_direction(pdata->OMSensor_data_io, 0);              //SDIO, as output

    for (bitCnt = 8; bitCnt != 0; bitCnt--) {               /* Read 8 bits MSB...LSB */
        //new spi ~730khz(22 cycles)
        gpio_set_output_value(pdata->OMSensor_sclk_io, 0);		//SCLK is low, 3 cycles
        delay(1);

        if (dataOut & 0x80) {	//JNB 4 cycles
            gpio_set_output_value(pdata->OMSensor_data_io, 1);    //SDIO is high, 3 cycles; plus SJMP 4 cycles
        } else {
            gpio_set_output_value(pdata->OMSensor_data_io, 0);    //SDIO is low, 3 cycles
        }

        gpio_set_output_value(pdata->OMSensor_sclk_io, 1);		//SCLK is high, 3 cycles

        delay(1);

        dataOut <<= 1;		                                    //Shift write data, 5 cycles; plus DJNZ 3 cycles
    }

    gpio_set_direction(pdata->OMSensor_data_io, 1);		        //SDIO, as input

    OS_EXIT_CRITICAL();

}

static void resync(void)
{
    gpio_set_output_value(pdata->OMSensor_sclk_io, 0);    //SCLK is low
    gpio_set_output_value(pdata->OMSensor_sclk_io, 0);
    gpio_set_output_value(pdata->OMSensor_sclk_io, 0);
    gpio_set_output_value(pdata->OMSensor_sclk_io, 0);
    gpio_set_output_value(pdata->OMSensor_sclk_io, 0);

    gpio_set_output_value(pdata->OMSensor_sclk_io, 1);    //SCLK is high
    gpio_set_output_value(pdata->OMSensor_sclk_io, 1);
    gpio_set_output_value(pdata->OMSensor_sclk_io, 1);
    gpio_set_output_value(pdata->OMSensor_sclk_io, 1);
    gpio_set_output_value(pdata->OMSensor_sclk_io, 1);
}

static const u16 cpi_value[3] = {800, 1200, 1600};
static void hal_pixart_set_cpi_init(u16 dst_cpi)
{
    u8 reg, i;

    for (i = 0; i < ARRAY_SIZE(cpi_table); i++) {
        if (dst_cpi == cpi_value[i]) {
            reg = cpi_table[i];
            break;
        }
    }

    reg = hal_pixart_readRegister(pixart_CPI_X_ADDR);
    reg = hal_pixart_readRegister(pixart_CPI_Y_ADDR);
    hal_pixart_writeRegister(pixart_WP_ADDR, 0x5a);
    hal_pixart_writeRegister(pixart_CPI_X_ADDR, reg);
    hal_pixart_writeRegister(pixart_CPI_Y_ADDR, reg);
    hal_pixart_writeRegister(pixart_WP_ADDR, 0x00);
    log_info("cpi init default = %d", cpi_table[i]);

}

//功能：设置CPI
static u16 hal_pixart_set_cpi(u16 dst_cpi)
{
    u8 reg = 0;
    u8 cpi_idx = -1;
    u8 i;

    for (i = 0; i < ARRAY_SIZE(cpi_table); i++) {
        if (dst_cpi == cpi_value[i]) {
            cpi_idx = i;
            break;
        }
    }

    if (cpi_idx > ARRAY_SIZE(cpi_table)) {
        log_debug("cpi set fail");
        return 0;
    }

    reg = hal_pixart_readRegister(pixart_CPI_X_ADDR);
    reg = hal_pixart_readRegister(pixart_CPI_Y_ADDR);
    reg = cpi_table[i];
    hal_pixart_writeRegister(pixart_WP_ADDR, 0x5a);     //Unlock WP
    hal_pixart_writeRegister(pixart_CPI_X_ADDR, reg);  //set cpi
    hal_pixart_writeRegister(pixart_CPI_Y_ADDR, reg);  //set cpi
    hal_pixart_writeRegister(pixart_WP_ADDR, 0x00);	   //Lock WP
    log_debug("CONFIGURATION:CPI[2:0] = %d. idx = %d\n", reg, cpi_idx);
    reg = hal_pixart_readRegister(pixart_CONFIG_ADDR) & 7;
    cpi_idx = (cpi_idx + 1) % ARRAY_SIZE(cpi_table);
    return cpi_table[reg & CONFIG_CPI];
}


REGISTER_OMSENSOR(optical_mouse_sensor) = {
    .OMSensor_id          = "hal3212",
    .OMSensor_init        = hal_pixart_init,
    .OMSensor_read_motion = hal_pixart_readMotion,
    .OMSensor_data_ready  = hal_pixart_data_ready,
    .OMSensor_status_dump = hal_pixart_status_dump,
    .OMSensor_wakeup = hal_pixart_force_wakeup,
    .OMSensor_led_switch  = hal_pixart_led_switch,
    .OMSensor_set_cpi     = hal_pixart_set_cpi,
};

static u8 hal3212_idle_query(void)
{
    if (pdata) {
        return (gpio_read(pdata->OMSensor_int_io));
    }
    return 1;
}

REGISTER_LP_TARGET(hal3205_lp_target) = {
    .name = "hal3212",
    .is_idle = hal3212_idle_query,
};


static void get_overflow_status(u8 motion)
{
    u8 reg = 0, dxovf = 0, dyovf = 0;

    dxovf = (motion & BIT(3)) >> 3;
    dyovf = (motion & BIT(4)) >> 4;

    if (dxovf) {
        printf(">>>>>>>>>>>>>>>>>>>DXOVF>>>>>>>>>>>>>>>>>>>>>>>>\n");
    }

    if (dyovf) {
        printf(">>>>>>>>>>>>>>>>>>>DYOVF>>>>>>>>>>>>>>>>>>>>>>>>\n");
    }
}




#endif  /*  hal3212 enable */



