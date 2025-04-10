
/******************************************************************************/
/***        include files                                                   ***/
/******************************************************************************/

#include "ed047tc1.h"
#include "i2s_data_bus.h"
#include "rmt_pulse.h"

#include <xtensa/core-macros.h>

#include <string.h>

/******************************************************************************/
/***        macro definitions                                               ***/
/******************************************************************************/

/******************************************************************************/
/***        type definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        local function prototypes                                       ***/
/******************************************************************************/

/******************************************************************************/
/***        exported variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        local variables                                                 ***/
/******************************************************************************/

/******************************************************************************/
/***        exported functions                                              ***/
/******************************************************************************/

/*
 * Write bits directly using the registers.
 * Won't work for some pins (>= 32).
 */
void IRAM_ATTR busy_delay(uint32_t cycles)
{
    volatile uint64_t counts = XTHAL_GET_CCOUNT() + cycles;
    while (XTHAL_GET_CCOUNT() < counts) ;
}


void epd_base_init(uint32_t epd_row_width)
{    /* Power Control Output/Off */
    gpio_set_direction(OE, GPIO_MODE_OUTPUT);
    gpio_set_direction(MODE, GPIO_MODE_OUTPUT);
    gpio_set_direction(PWR, GPIO_MODE_OUTPUT);
    gpio_set_direction(STV, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEH, GPIO_MODE_OUTPUT);

    gpio_set_level(OE, 0);
    gpio_set_level(MODE, 0);
    gpio_set_level(PWR, 0);
    gpio_set_level(STV, 1);
    gpio_set_level(LEH, 0);

    // Setup I2S
    i2s_bus_config i2s_config;
    // add an offset off dummy bytes to allow for enough timing headroom
    i2s_config.epd_row_width = epd_row_width + 32;
    i2s_config.clock = CKH;
    i2s_config.start_pulse = STH;
    i2s_config.data_0 = D0;
    i2s_config.data_1 = D1;
    i2s_config.data_2 = D2;
    i2s_config.data_3 = D3;
    i2s_config.data_4 = D4;
    i2s_config.data_5 = D5;
    i2s_config.data_6 = D6;
    i2s_config.data_7 = D7;

    i2s_bus_init(&i2s_config);

    rmt_pulse_init(CKV);
}

void epd_poweron()
{
    gpio_set_level(PWR, 1);
    busy_delay(100 * 240);
    gpio_set_level(STV, 1);
    gpio_set_level(STH, 1);
}

void epd_poweroff()
{
    gpio_set_level(PWR, 0);
    busy_delay(100 * 240);
    gpio_set_level(STV, 0);
}

void epd_poweroff_all()
{
    gpio_set_level(PWR, 0);
    gpio_set_level(STV, 0);
}

void epd_start_frame()
{
    while (i2s_is_busy()) ;
    
    gpio_set_level(MODE, 1);

    pulse_ckv_us(1, 1, true);
    gpio_set_level(STV, 0);
    busy_delay(240);
    pulse_ckv_us(10, 10, false);
    gpio_set_level(STV, 1);
    pulse_ckv_us(0, 10, true);
    gpio_set_level(OE, 1);
    pulse_ckv_us(1, 1, true);
}

static inline void latch_row()
{
    gpio_set_level(LEH, 1);
    gpio_set_level(LEH, 0);
}

void IRAM_ATTR epd_skip()
{
#if defined(CONFIG_EPD_DISPLAY_TYPE_ED097TC2)
    pulse_ckv_ticks(2, 2, false);
#else
    // According to the spec, the OC4 maximum CKV frequency is 200kHz.
    pulse_ckv_ticks(45, 5, false);
#endif
}

void IRAM_ATTR epd_output_row(uint32_t output_time_dus)
{
    while (i2s_is_busy());

    latch_row();

    pulse_ckv_ticks(output_time_dus, 50, false);

    i2s_start_line_output();
    i2s_switch_buffer();
}

void epd_end_frame()
{
    gpio_set_level(OE, 0);
    gpio_set_level(MODE, 0);
    pulse_ckv_us(1, 1, true);
    pulse_ckv_us(1, 1, true);
}

void IRAM_ATTR epd_switch_buffer()
{
    i2s_switch_buffer();
}

uint8_t * IRAM_ATTR epd_get_current_buffer()
{
    return (uint8_t *)i2s_get_current_buffer();
}

/******************************************************************************/
/***        local functions                                                 ***/
/******************************************************************************/

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/