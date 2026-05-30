#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/multicore.h"
#include <cmath>
#include <cstdint>

#define SPI_PORT spi0

#define PIN_MISO 0
#define PIN_CS 1
#define PIN_SCK 2

#define ADC_LOOP_US 8

bool binflag[2] = { 0,0 };
uint16_t bin[2][512];

constexpr float SAMPLE_RATE = 125000.0f;
constexpr int N = 256;

struct GoertzelResult {
    float f25k;
    float f30k;
    float f35k;
    float f40k;
};

float goertzel(bool bin_number, float targetFreq) {
    const float omega = 2.0f * M_PI * targetFreq / SAMPLE_RATE;
    const float coeff = 2.0f * cosf(omega);

    float q0 = 0.0f;
    float q1 = 0.0f;
    float q2 = 0.0f;

    for (int i = 0; i < N; i++) {
        q0 = coeff * q1 - q2 + bin[bin_number][i];
        q2 = q1;
        q1 = q0;
    }

    const float real = q1 - q2 * cosf(omega);
    const float imag = q2 * sinf(omega);

    return real * real + imag * imag;
}




void core1_entry() {
    while (true) {
        GoertzelResult result;
        if (binflag[0]) {
            result.f25k = goertzel(0, 25000.0f);
            result.f30k = goertzel(0, 30000.0f);
            result.f35k = goertzel(0, 35000.0f);
            result.f40k = goertzel(0, 40000.0f);
        }
        else if (binflag[1]) {
            result.f25k = goertzel(1, 25000.0f);
            result.f30k = goertzel(1, 30000.0f);
            result.f35k = goertzel(1, 35000.0f);
            result.f40k = goertzel(1, 40000.0f);
        }
        printf("%f\t%f\t%f\t%f\n", result.f25k / 10000000.0, result.f30k / 10000000.0, result.f35k / 10000000.0, result.f40k / 10000000.0);
        sleep_ms(1000);
    }
}

volatile bool ADC_flag = false;
struct repeating_timer ADC_timer;

bool ADC_timer_cb(struct repeating_timer* t)
{
    ADC_flag = true;
    return true;
}

int main() {

    add_repeating_timer_us(
        -ADC_LOOP_US,
        ADC_timer_cb,
        NULL,
        &ADC_timer
    );

    stdio_init_all();

    spi_init(SPI_PORT, 8 * 1000 * 1000);

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    multicore_launch_core1(core1_entry);

    uint8_t buf[2];

    repeating_timer_t timer;

    while (true)
    {
        for (int i = 0; i < N; ++i) {
            if (ADC_flag) {
                ADC_flag = false;

                gpio_put(PIN_CS, 0);
                spi_read_blocking(SPI_PORT, 0, buf, 2);
                gpio_put(PIN_CS, 1);

                bin[0][i] = (uint16_t(buf[0]) << 8) | buf[1];
            }
        }
        binflag[0] = 1;
        binflag[1] = 0;

        for (int i = 0; i < N; ++i) {
            if (ADC_flag) {
                ADC_flag = false;

                gpio_put(PIN_CS, 0);
                spi_read_blocking(SPI_PORT, 0, buf, 2);
                gpio_put(PIN_CS, 1);

                bin[1][i] = (uint16_t(buf[0]) << 8) | buf[1];

            }
        }
        binflag[0] = 0;
        binflag[1] = 1;
    }
}