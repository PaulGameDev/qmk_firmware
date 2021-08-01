#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ch.h"
#include "hal.h"

#include "color.h"
#include "wait.h"
#include "util.h"
#include "matrix.h"
#include "debounce.h"
#include "quantum.h"

#define I2C_SCL B6
#define I2C_SDA B7

#define I2C_SCL_IN readPin(I2C_SCL)
#define I2C_SDA_IN readPin(I2C_SDA)

#define I2C_SCL_HI setPinInputHigh(I2C_SCL)
#define I2C_SCL_LO setPinOutput(I2C_SCL)
#define I2C_SCL_HIZ setPinInputHigh(I2C_SCL)

#define I2C_SDA_HI setPinInputHigh(I2C_SDA)
#define I2C_SDA_LO setPinOutput(I2C_SDA)
#define I2C_SDA_HIZ setPinInputHigh(I2C_SDA)

static uint8_t  i2c_byte_ct = 0;
static uint8_t  i2c_addr_rw = 0;
static uint8_t* i2c_data_ptr;
static uint8_t  i2c_data_byte[2] = {0};
static uint8_t  i2c_tx_byte = 0;

void i2c_init(void)
{
    writePinLow(I2C_SCL);
    writePinLow(I2C_SDA);

    I2C_SCL_HIZ;
    I2C_SDA_HIZ;
}

static inline void i2c_process_bit(void)
{
    if(i2c_tx_byte & 0x80)
    {
        I2C_SDA_HIZ;
    }
    else
    {
        I2C_SDA_LO;
    }

    i2c_tx_byte = i2c_tx_byte << 1;

    I2C_SCL_HIZ;
    I2C_SCL_LO;
}

static inline void i2c_transaction(void)
{
//I2C_STATE_START_COND:
    I2C_SDA_LO;
    I2C_SCL_LO;

    i2c_tx_byte = i2c_addr_rw;

//I2C_STATE_SEND_ADDR_RW:
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();

    goto I2C_STATE_READ_ACK;

I2C_STATE_WRITE_BYTE:
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();

    i2c_byte_ct = i2c_byte_ct - 1;
    i2c_data_ptr++;

I2C_STATE_READ_ACK:
    I2C_SDA_HIZ;
    I2C_SCL_HIZ;

    if(!I2C_SDA_IN)
    {
        // Slave did ACK, move on to data
        if(i2c_byte_ct > 0)
        {
            i2c_tx_byte = *i2c_data_ptr;

            I2C_SDA_LO;
            I2C_SCL_LO;

            goto I2C_STATE_WRITE_BYTE;
        }
    }

    I2C_SDA_LO;
    I2C_SCL_LO;

//I2C_STATE_STOP_COND:
    I2C_SCL_HIZ;
    I2C_SDA_HIZ;
}

static inline void i2c_write_buf(uint8_t devid, uint8_t* data, uint8_t len)
{
    i2c_addr_rw  = devid;

    i2c_data_ptr = data;
    i2c_byte_ct  = len;

    i2c_transaction();
}

static inline void i2c_write_reg(uint8_t devid, uint8_t reg, uint8_t data)
{
    i2c_addr_rw      = devid;
    i2c_data_byte[0] = reg;
    i2c_data_byte[1] = data;

    i2c_data_ptr     = i2c_data_byte;
    i2c_byte_ct      = 2;

    i2c_transaction();
}

enum
{
    REG_CONFIGURE_COMMAND       = 0xFD,
    REG_FUNC_CONFIGURATION      = 0x00,
    REG_FUNC_PICTURE_DISPLAY    = 0x01,
    REG_FUNC_DISPLAY_OPTION     = 0x05,
    REG_FUNC_AUDIO_SYNC         = 0x06,
    REG_FUNC_BREATH_CONTROL_1   = 0x08,
    REG_FUNC_BREATH_CONTROL_2   = 0x09,
    REG_FUNC_SHUTDOWN           = 0x0A,
    REG_FUNC_AUDIO_GAIN_CONTROL = 0x0B,
    REG_FUNC_STAGGERED_DELAY    = 0x0D,
    REG_FUNC_SLEW_RATE_CONTROL  = 0x0E,
    REG_FUNC_CURRENT_CONTROL    = 0x0F,
    REG_FUNC_OPEN_SHORT_TEST_1  = 0x10,
    REG_FUNC_OPEN_SHORT_TEST_2  = 0x11,
    REG_FUNC_ADC_OUTPUT         = 0x13,
    REG_FUNC_VAF_1              = 0x14,
    REG_FUNC_VAF_2              = 0x15,
    REG_FUNC_THERMAL_DETECTION  = 0x17,
};

enum
{
    PAGE_FRAME_1                = 0x00,
    PAGE_FRAME_2                = 0x01,
    PAGE_FUNCTION               = 0x0B,
    PAGE_DETECTION              = 0x0C,
    PAGE_VAF                    = 0x0D,
};

void process_backlight(uint8_t devid, volatile LED_TYPE* states)
{
    static unsigned char led_val[49];
    static unsigned short state = 0;

    switch(state)
    {
        case 0:
            i2c_init();

            i2c_write_reg(devid, REG_CONFIGURE_COMMAND,       PAGE_FUNCTION);
            i2c_write_reg(devid, REG_FUNC_SHUTDOWN,           0x00);
            i2c_write_reg(devid, REG_FUNC_CONFIGURATION,      0x00);
            i2c_write_reg(devid, REG_FUNC_PICTURE_DISPLAY,    0x10);
            i2c_write_reg(devid, REG_FUNC_DISPLAY_OPTION,     0x00);
            i2c_write_reg(devid, REG_FUNC_AUDIO_SYNC,         0x00);
            i2c_write_reg(devid, REG_FUNC_BREATH_CONTROL_1,   0x00);
            i2c_write_reg(devid, REG_FUNC_BREATH_CONTROL_2,   0x00);
            i2c_write_reg(devid, REG_FUNC_AUDIO_GAIN_CONTROL, 0x00);
            i2c_write_reg(devid, REG_FUNC_STAGGERED_DELAY,    0x0F);
            i2c_write_reg(devid, REG_FUNC_SLEW_RATE_CONTROL,  0x01);
            i2c_write_reg(devid, REG_FUNC_VAF_1,              68);
            i2c_write_reg(devid, REG_FUNC_VAF_2,              128);
            i2c_write_reg(devid, REG_FUNC_CURRENT_CONTROL,    153);

            state = 1;
            break;

        case 1:
            i2c_write_reg(devid, REG_CONFIGURE_COMMAND,       PAGE_FRAME_1);

            led_val[0] = 0x00;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = 0x00;
                led_val[led_id + 1 + 0x10] = 0x00;
            }

            //Enable Columns 0-2 for the first 13 LEDs
            led_val[0x01] = 0b11111000;
            led_val[0x02] = 0b11111111;

            led_val[0x03] = 0b11111000;
            led_val[0x04] = 0b11111111;

            led_val[0x05] = 0b11111000;
            led_val[0x06] = 0b11111111;

            //Enable Columns 3-5 for the second 13 LEDs
            led_val[0x07] = 0b11000111;
            led_val[0x08] = 0b11111111;

            led_val[0x09] = 0b11000111;
            led_val[0x0A] = 0b11111111;

            led_val[0x0B] = 0b11000111;
            led_val[0x0C] = 0b11111111;

            //Enable Column 6 for the third 13 LEDs (Red and Blue channels)
            led_val[0x0D] = 0b00111111;
            led_val[0x0E] = 0b11111110;

            led_val[0x0F] = 0b00111111;
            led_val[0x10] = 0b11111110;

            i2c_write_buf(devid, led_val, 33);

            state = 2;
            break;

        case 2:
            i2c_write_reg(devid, REG_CONFIGURE_COMMAND,       PAGE_FRAME_2);
            led_val[0] = 0x00;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = 0x00;
                led_val[led_id + 1 + 0x10] = 0x00;
            }

            //Enable Column 6 for the third 13 LEDs (Green channel)
            led_val[0x01] = 0b00111111;
            led_val[0x02] = 0b11111110;

            //Enable Columns 7-9 for the fourth LEDs
            led_val[0x03] = 0b10000000;
            led_val[0x04] = 0b11111001;

            led_val[0x05] = 0b10000000;
            led_val[0x06] = 0b11111001;

            led_val[0x07] = 0b10000000;
            led_val[0x08] = 0b11111001;

            //Enable Columns 10-12 for the fifth LEDs
            led_val[0x09] = 0b00000000;
            led_val[0x0A] = 0b11001000;

            led_val[0x0B] = 0b00000000;
            led_val[0x0C] = 0b11001000;

            led_val[0x0D] = 0b00000000;
            led_val[0x0E] = 0b11001000;

            i2c_write_buf(devid, led_val, 33);

            i2c_write_reg(devid, REG_CONFIGURE_COMMAND,       PAGE_FUNCTION);
            i2c_write_reg(devid, REG_FUNC_SHUTDOWN,           0x01);

            state = 3;
            break;

        case 3:
            i2c_write_reg(devid, REG_CONFIGURE_COMMAND,       PAGE_FRAME_1);

            // Column 0-2 LEDs starting at address 0x20, RBG order
            led_val[0] = 0x20;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = states[led_id].r;
                led_val[led_id + 1 + 0x10] = states[led_id].b;
                led_val[led_id + 1 + 0x20] = states[led_id].g;
            }
            i2c_write_buf(devid, led_val, 49);

            // Column 3-5 LEDs starting at address 0x50, RBG order
            led_val[0] = 0x50;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = states[led_id + 16].r;
                led_val[led_id + 1 + 0x10] = states[led_id + 16].b;
                led_val[led_id + 1 + 0x20] = states[led_id + 16].g;
            }
            i2c_write_buf(devid, led_val, 49);

            state = 4;
            break;

        case 4:
            // Column 6-8 LEDs (Red and Blue channels) starting at address 0x80, RBG order
            led_val[0] = 0x80;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = states[led_id + 32].r;
                led_val[led_id + 1 + 0x10] = states[led_id + 32].b;
            }
            i2c_write_buf(devid, led_val, 33);

            i2c_write_reg(devid, REG_CONFIGURE_COMMAND,       PAGE_FRAME_2);

            // Column 6-8 LEDs (Green channel) starting at address 0x20, RBG order
            led_val[0] = 0x20;
            for(unsigned int led_id = 0; led_id < 16; led_id++)
            {
                led_val[led_id + 1 + 0x00] = states[led_id + 32].g;
            }
            i2c_write_buf(devid, led_val, 17);

            // Column 7-9 LEDs starting at address 0x30, RBG order
            led_val[0] = 0x37;
            for(unsigned int led_id = 7; led_id < 16; led_id++)
            {
                led_val[led_id - 6 + 0x00] = states[led_id + 48].r;
            }
            i2c_write_buf(devid, led_val, 11);

            led_val[0] = 0x47;
            for(unsigned int led_id = 7; led_id < 16; led_id++)
            {
                led_val[led_id - 6 + 0x00] = states[led_id + 48].b;
            }
            i2c_write_buf(devid, led_val, 11);

            led_val[0] = 0x57;
            for(unsigned int led_id = 7; led_id < 16; led_id++)
            {
                led_val[led_id - 6 + 0x00] = states[led_id + 48].g;
            }
            i2c_write_buf(devid, led_val, 11);

            // Column 10-12 LEDs starting at address 0x60, RBG order
            led_val[0] = 0x6B;
            for(unsigned int led_id = 11; led_id < 16; led_id++)
            {
                led_val[led_id - 10 + 0x00] = states[led_id + 64].r;
            }
            i2c_write_buf(devid, led_val, 6);

            led_val[0] = 0x7B;
            for(unsigned int led_id = 11; led_id < 16; led_id++)
            {
                led_val[led_id - 10 + 0x00] = states[led_id + 64].b;
            }
            i2c_write_buf(devid, led_val, 6);

            led_val[0] = 0x8B;
            for(unsigned int led_id = 11; led_id < 16; led_id++)
            {
                led_val[led_id - 10 + 0x00] = states[led_id + 64].g;
            }
            i2c_write_buf(devid, led_val, 6);

            state = 3;
            break;
    }
}
