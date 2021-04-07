/* Copyright 2017 Jason Williams
 * Copyright 2018 Jack Humbert
 * Copyright 2021 Paul_GD
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sled1734x.h"
#include "i2c_master.h"
#include "wait.h"

#define SN_ADDR_DEFAULT 0x74

#define SN_REG_CONFIG 0x00
#define SN_REG_CONFIG_PICTUREMODE 0x00
#define SN_REG_CONFIG_AUTOPLAYMODE 0x08
#define SN_REG_CONFIG_AUDIOPLAYMODE 0x18

#define SN_CONF_PICTUREMODE 0x00
#define SN_CONF_AUTOFRAMEMODE 0x04
#define SN_CONF_AUDIOMODE 0x08

#define SN_REG_PICTUREFRAME 0x01

#define SN_REG_SHUTDOWN 0x0A
#define SN_REG_AUDIOSYNC 0x06

#define SN_COMMANDREGISTER 0xFD
#define SN_BANK_FUNCTIONREG 0x0B

#ifndef SN_TIMEOUT
    #define SN_TIMEOUT 100
#endif

#ifndef SN_PERSISTENCE
    #define SN_PERSISTENCE 0
#endif

uint8_t g_twi_transfer_buffer[20];

uint8_t g_pwm_buffer[DRIVER_COUNT][144];
bool    g_pwm_buffer_update_required[DRIVER_COUNT] = {false};

uint8_t g_led_control_registers[DRIVER_COUNT][18]             = {{0}};
bool    g_led_control_registers_update_required[DRIVER_COUNT] = {false};

void SLED1734X_write_registers(uint8_t addr, uint8_t reg, uint8_t data) {
    g_twi_transfer_buffer[0] = reg;
    g_twi_transfer_buffer[1] = data;

    #if SN_PERSISTENCE > 0
        for(uint8_t i = 0; i < SN_PERSISTENCE; i++) {
            if(i2c_transmit(addr << 1, g_twi_transfer_buffer, 2, SN_TIMEOUT) == 0) break;
        }
    #else
        i2c_transmit(addr << 1, g_twi_transfer_buffer, 2, SN_TIMEOUT);
    #endif
}

void SLED1734X_write_pwm_buffer(uint8_t addr, uint8_t *pwm_buffer) {
    for(int i = 0; i < 144; i += 16) {
        g_twi_transfer_buffer[0] = 0x24 + i;
        for(int j = 0; j < 16; j++) {
            g_twi_transfer_buffer[1 + j] = pwm_buffer[i + j];
        }

        #if SN_PERSISTENCE > 0
            for(uint8_t i = 0; i < SN_PERSISTENCE; i++) {
                if(i2c_transmit(addr << 1, g_twi_transfer_buffer, 17, SN_TIMEOUT) == 0) break;
            }
        #else
            i2c_transmit(addr << 1, g_twi_transfer_buffer, 17, SN_TIMEOUT);
        #endif
    }
}

void SLED1734X_init(uint8_t addr) {
    SLED1734X_write_registers(addr, SN_COMMANDREGISTER, SN_BANK_FUNCTIONREG);

    SLED1734X_write_registers(addr, SN_REG_SHUTDOWN, 0x00);

    wait_ms(10);

    SLED1734X_write_registers(addr, SN_REG_CONFIG, SN_REG_CONFIG_PICTUREMODE);
    SLED1734X_write_registers(addr, SN_REG_PICTUREFRAME, 0x00);
    SLED1734X_write_registers(addr, SN_REG_AUDIOSYNC, 0x00);

    for(int i = 0x00; i <= 0x11; i++) {
        SLED1734X_write_registers(addr, i, 0x00);
    }

    for(int i = 0x12; i <= 0x23; i++) {
        SLED1734X_write_registers(addr, i, 0x00);
    }

    for(int i = 0x24; i <= 0xB3; i++) {
        SLED1734X_write_registers(addr, i, 0x00);
    }

    SLED1734X_write_registers(addr, SN_COMMANDREGISTER, SN_BANK_FUNCTIONREG);

    SLED1734X_write_registers(addr, SN_REG_SHUTDOWN, 0x01);

    SLED1734X_write_registers(addr, SN_COMMANDREGISTER, 0);
}

void SLED1734X_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    if(index >= 0 && index < DRIVER_LED_TOTAL) {
        sn34x_led led = g_sn34x_leds[index];

        g_pwm_buffer[led.driver][led.r - 0x24]   = red;
        g_pwm_buffer[led.driver][led.g - 0x24]   = green;
        g_pwm_buffer[led.driver][led.b - 0x24]   = blue;
        g_pwm_buffer_update_required[led.driver] = true;
    }
}

void SLED1734X_set_color_all(uint8_t red, uint8_t green, uint8_t blue) {
    for(int i = 0; i < DRIVER_LED_TOTAL; i++) {
        SLED1734X_set_color(i, red, green, blue);
    }
}

void SLED1734X_set_led_control_registers(uint8_t index, bool red, bool green, bool blue) {
    sn34x_led led = g_sn34x_leds[index];

    uint8_t control_register_r = (led.r - 0x24) / 8;
    uint8_t control_register_g = (led.g - 0x24) / 8;
    uint8_t control_register_b = (led.b - 0x24) / 8;
    uint8_t bit_r              = (led.r - 0x24) % 8;
    uint8_t bit_g              = (led.g - 0x24) % 8;
    uint8_t bit_b              = (led.b - 0x24) % 8;

    if (red) {
        g_led_control_registers[led.driver][control_register_r] |= (1 << bit_r);
    } else {
        g_led_control_registers[led.driver][control_register_r] &= ~(1 << bit_r);
    }
    if (green) {
        g_led_control_registers[led.driver][control_register_g] |= (1 << bit_g);
    } else {
        g_led_control_registers[led.driver][control_register_g] &= ~(1 << bit_g);
    }
    if (blue) {
        g_led_control_registers[led.driver][control_register_b] |= (1 << bit_b);
    } else {
        g_led_control_registers[led.driver][control_register_b] &= ~(1 << bit_b);
    }

    g_led_control_registers_update_required[led.driver] = true;
}

void SLED1734X_update_pwm_buffers(uint8_t addr, uint8_t index) {
    if (g_pwm_buffer_update_required[index]) {
        SLED1734X_write_pwm_buffer(addr, g_pwm_buffer[index]);
    }
    g_pwm_buffer_update_required[index] = false;
}

void SLED1734X_update_led_control_registers(uint8_t addr, uint8_t index) {
    if (g_led_control_registers_update_required[index]) {
        for (int i = 0; i < 18; i++) {
            SLED1734X_write_registers(addr, i, g_led_control_registers[index][i]);
        }
    }
    g_led_control_registers_update_required[index] = false;
}
