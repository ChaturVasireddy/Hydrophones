#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/time.h"

#define SPI_PORT spi0

#define PIN_MISO 0
#define PIN_CS   1
#define PIN_SCK  2

int main()
{
    stdio_init_all();

    spi_init(SPI_PORT, 4000 * 1000);   //4mhz spi max od mcu
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    uint8_t buf[2];

    printf("started");

    absolute_time_t next = get_absolute_time();

    while (true)
    {
        next = delayed_by_us(next, 8);   //125khz

        gpio_put(PIN_CS, 0);
        spi_read_blocking(SPI_PORT, 0, buf, 2);
        gpio_put(PIN_CS, 1);

        uint16_t raw = (buf[0] << 8) | buf[1];
        uint16_t adc = (raw >> 1) & 0x0FFF;

        if (adc == 0)
        {
            printf("peak detected\n");
            sleep_ms(500);
            next = get_absolute_time();
        }

        sleep_until(next);
    }
}