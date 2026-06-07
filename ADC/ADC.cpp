#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT spi0

#define PIN_MISO 0
#define PIN_CS   1
#define PIN_SCK  2

int main()
{
    stdio_init_all();

    spi_init(SPI_PORT, 8 * 1000 * 1000);

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    uint8_t buf[2];
    int16_t samples[512];
    absolute_time_t next_sample_time = get_absolute_time();

    while (true)
    {
        sleep_ms(2000);
        for (int i = 0; i < 512; ++i)
        {
            gpio_put(PIN_CS, 0);

            spi_read_blocking(SPI_PORT, 0, buf, 2);

            gpio_put(PIN_CS, 1);

            samples[i] = (((uint16_t(buf[0]) << 8) | buf[1]) & 0x0FFF);
            samples[i] -= 2048;
            sleep_us(5);
        }

        for (int i = 0; i < 512; ++i)
        {
            printf("%d\n", samples[i]);
        }

    }
}