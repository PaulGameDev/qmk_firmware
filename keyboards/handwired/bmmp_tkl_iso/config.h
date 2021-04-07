/*
Copyright 2015 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/* USB Device descriptor parameter */
#define VENDOR_ID       0xDEAD
#define PRODUCT_ID      0xBEEF
#define DEVICE_VER      0x0001
#define MANUFACTURER    PauL_GD
#define PRODUCT         BMMP TKL ISO

/* key matrix size */
#define MATRIX_ROWS 7
#define MATRIX_COLS 13

#define DIODE_DIRECTION ROW2COL

/* define if matrix has ghost */
//#define MATRIX_HAS_GHOST

/* RGB Matrix */
#define DRIVER_ADDR_1   0b11101110
#define DRIVER_ADDR_2   0b11101000

#define DRIVER_COUNT    2
#define DRIVER_1_LED_TOTAL  63
#define DRIVER_2_LED_TOTAL  25
#define DRIVER_LED_TOTAL    (DRIVER_1_LED_TOTAL + DRIVER_2_LED_TOTAL)

/* Set 0 if debouncing isn't needed */
#define DEBOUNCE    5

#define TAPPING_TERM    500

/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
//#define NO_DEBUG

/* disable print */
//#define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT
//#define NO_ACTION_MACRO
//#define NO_ACTION_FUNCTION
