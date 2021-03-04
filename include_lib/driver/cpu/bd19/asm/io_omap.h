
//===============================================================================//
//
//      output function define
//
//===============================================================================//
#define FO_GP_OCH0          0
#define FO_GP_OCH1          1
#define FO_GP_OCH2          2
#define FO_GP_OCH3          3
#define FO_GP_OCH4          4
#define FO_GP_OCH5          5
#define FO_GP_OCH6          6
#define FO_GP_OCH7          7
#define FO_GP_OCH8          8
#define FO_GP_OCH9          9
#define FO_GP_OCH10         10
#define FO_GP_OCH11         11
#define FO_TMR0_PWM         12
#define FO_TMR1_PWM         13
#define FO_TMR2_PWM         14
#define FO_TMR3_PWM         15
#define FO_SPI0_CLK         16
#define FO_SPI0_DA0         17
#define FO_SPI0_DA1         18
#define FO_SPI0_DA2         19
#define FO_SPI0_DA3         20
#define FO_SPI1_CLK         21
#define FO_SPI1_DA0         22
#define FO_SPI1_DA1         23
#define FO_SPI1_DA2         24
#define FO_SPI1_DA3         25
#define FO_SPI2_CLK         26
#define FO_SPI2_DA0         27
#define FO_SPI2_DA1         28
#define FO_SPI2_DA2         29
#define FO_SPI2_DA3         30
#define FO_MCPWM_CH0L     31
#define FO_MCPWM_CH0H     32
#define FO_MCPWM_CH1L     33
#define FO_MCPWM_CH1H     34
#define FO_MCPWM_CH2L     35
#define FO_MCPWM_CH2H     36
#define FO_MCPWM_CH3L     37
#define FO_MCPWM_CH3H     38
#define FO_IIC_SCL        39
#define FO_IIC_SDA        40
#define FO_LEDC0_DO       41
#define FO_LEDC1_DO       42
#define FO_UART0_TX       43
#define FO_UART1_TX       44
#define FO_UART1_RTS      45
#define FO_UART2_TX       46
#define FO_CHAIN_OUT0     47
#define FO_CHAIN_OUT1       48
#define FO_CHAIN_OUT2       49
#define FO_CHAIN_OUT3       50

//===============================================================================//
//
//      IO output select sfr
//
//===============================================================================//
typedef struct {
    __RW __u8 PA0_OUT;
    __RW __u8 PA1_OUT;
    __RW __u8 PA2_OUT;
    __RW __u8 PA3_OUT;
    __RW __u8 PA4_OUT;
    __RW __u8 PA5_OUT;
    __RW __u8 PA6_OUT;
    __RW __u8 PA7_OUT;
    __RW __u8 PA8_OUT;
    __RW __u8 PA9_OUT;
    __RW __u8 PB0_OUT;
    __RW __u8 PB1_OUT;
    __RW __u8 PB2_OUT;
    __RW __u8 PB3_OUT;
    __RW __u8 PB4_OUT;
    __RW __u8 PB5_OUT;
    __RW __u8 PB6_OUT;
    __RW __u8 PB7_OUT;
    __RW __u8 PB8_OUT;
    __RW __u8 PB9_OUT;
    __RW __u8 PD0_OUT;
    __RW __u8 PD1_OUT;
    __RW __u8 PD2_OUT;
    __RW __u8 PD3_OUT;
    __RW __u8 PD4_OUT;
    __RW __u8 USBDP_OUT;
    __RW __u8 USBDM_OUT;
    __RW __u8 USB1DP_OUT;
    __RW __u8 USB1DM_OUT;
    __RW __u8 PP0_OUT;
} JL_OMAP_TypeDef;

#define JL_OMAP_BASE      (ls_base + map_adr(0x56, 0x00))
#define JL_OMAP           ((JL_OMAP_TypeDef   *)JL_OMAP_BASE)

