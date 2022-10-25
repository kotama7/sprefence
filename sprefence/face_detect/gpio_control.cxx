#include "gpio_control.h"

#include <unistd.h>
#include <arch/board/board.h>
#include <arch/chip/pin.h>

int GPIOInit(){
    board_gpio_config(PIN_SPI4_SCK, 0, false, false, PIN_FLOAT);
    board_gpio_config(PIN_SPI4_MISO, 0, false, false, PIN_FLOAT);
    board_gpio_write(PIN_SPI4_SCK, 0);
    board_gpio_write(PIN_SPI4_MISO, 0);
    return 0;
}

void FenceOn(){
    board_gpio_write(PIN_SPI4_SCK, 1);
    board_gpio_write(PIN_SPI4_MISO, 1);
    usleep(FENCE_ON_DURATION * 1000 * 1000);
    board_gpio_write(PIN_SPI4_MISO, 0);
}

void FenceOff(){
    board_gpio_write(PIN_SPI4_SCK, 0);
}