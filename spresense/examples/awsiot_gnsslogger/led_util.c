/****************************************************************************
 * examples/awsiot_gnsslogger/led_util.c
 *
 *   Copyright 2021 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>

#include <arch/board/board.h>
#include <arch/chip/pin.h>

#include "led_util.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PIN_LED0 PIN_I2S1_BCK
#define PIN_LED1 PIN_I2S1_LRCK
#define PIN_LED2 PIN_I2S1_DATA_IN
#define PIN_LED3 PIN_I2S1_DATA_OUT

#define SETUP_PIN_OUTPUT(pin) do{ \
  board_gpio_write(pin, -1); \
  board_gpio_config(pin, 0, false, true, PIN_FLOAT); \
}while(0)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: init_leds()
 *
 * Description:
 *   Set up pins of LEDs on the Spresense main board.
 ****************************************************************************/

void init_leds(void)
{
  SETUP_PIN_OUTPUT( PIN_LED0 );
  SETUP_PIN_OUTPUT( PIN_LED1 );
  SETUP_PIN_OUTPUT( PIN_LED2 );
  SETUP_PIN_OUTPUT( PIN_LED3 );
}

/****************************************************************************
 * Name: set_leds()
 *
 * Description:
 *   Control turn on/off LEDs.
 ****************************************************************************/

void set_leds(int ptn)
{
  board_gpio_write(PIN_LED0, (ptn & 0x01) ? 1 : 0);
  board_gpio_write(PIN_LED1, (ptn & 0x02) ? 1 : 0);
  board_gpio_write(PIN_LED2, (ptn & 0x04) ? 1 : 0);
  board_gpio_write(PIN_LED3, (ptn & 0x08) ? 1 : 0);
}
