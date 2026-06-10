#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/multicore.h"
#include <cmath>
#include <cstdint>

#define F0 25000.0f
#define F1 30000.0f
#define F2 35000.0f
#define F3 40000.0f


#define SPI_PORT spi0

#define PIN_MISO 0
#define PIN_CS 1
#define PIN_SCK 2

#define ADC_LOOP_US 8

constexpr float SAMPLE_RATE = 125000.0f;
constexpr int N = 2048;

volatile bool binflag[2] = { 0,0 };
int16_t bin[2][N];

int peak_freq = -1;
int peak_time = -1;
int bin_count = 0;

float goertzel_result[4] = { 0,0,0,0 };

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
        if (binflag[0]) {
            bin_count++;
            goertzel_result[0] = goertzel(0, F0);
            goertzel_result[1] = goertzel(0, F1);
            goertzel_result[2] = goertzel(0, F2);
            goertzel_result[3] = goertzel(0, F3);
            if (goertzel_result[0] > 100000000.0 || goertzel_result[1] > 100000000.0 || goertzel_result[2] > 100000000.0 || goertzel_result[3] > 100000000.0) {
                if (goertzel_result[0] > goertzel_result[1] && goertzel_result[0] > goertzel_result[2] && goertzel_result[0] > goertzel_result[3]) {
                    peak_freq = 0;
                }
                else if (goertzel_result[1] > goertzel_result[0] && goertzel_result[1] > goertzel_result[2] && goertzel_result[1] > goertzel_result[3]) {
                    peak_freq = 1;
                }
                else if (goertzel_result[2] > goertzel_result[0] && goertzel_result[2] > goertzel_result[30] && goertzel_result[2] > goertzel_result[3]) {
                    peak_freq = 2;
                }
                else {
                    peak_freq = 3;
                }
                for (int i = 0; i < N; ++i) {
                    if (bin[0][i] > 512 || bin[0][i] < -512) {
                        peak_time = (bin_count * N) + i;
                        break;
                    }
                }
            }
            else
                printf("null\n");
            binflag[0] = 0;
        }
        if (binflag[1]) {
            bin_count++;
            goertzel_result[0] = goertzel(1, F0);
            goertzel_result[1] = goertzel(1, F1);
            goertzel_result[2] = goertzel(1, F2);
            goertzel_result[3] = goertzel(1, F3);
            if (goertzel_result[0] > 100000000.0 || goertzel_result[1] > 100000000.0 || goertzel_result[2] > 100000000.0 || goertzel_result[3] > 100000000.0) {
                if (goertzel_result[0] > goertzel_result[1] && goertzel_result[0] > goertzel_result[2] && goertzel_result[0] > goertzel_result[3]) {
                    peak_freq = 0;
                }
                else if (goertzel_result[1] > goertzel_result[0] && goertzel_result[1] > goertzel_result[2] && goertzel_result[1] > goertzel_result[3]) {
                    peak_freq = 1;
                }
                else if (goertzel_result[2] > goertzel_result[0] && goertzel_result[2] > goertzel_result[30] && goertzel_result[2] > goertzel_result[3]) {
                    peak_freq = 2;
                }
                else {
                    peak_freq = 3;
                }
                for (int i = 0; i < N; ++i) {
                    if (bin[0][i] > 512 || bin[0][i] < -512) {
                        peak_time = (bin_count * N) + i;
                        break;
                    }
                }
            }
            else
                printf("null\n");
            binflag[1] = 0;
        }
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
        if (!binflag[0]) {
            for (int i = 0; i < N; ++i) {

                while (!ADC_flag)
                    tight_loop_contents();
                ADC_flag = false;

                gpio_put(PIN_CS, 0);
                spi_read_blocking(SPI_PORT, 0, buf, 2);
                gpio_put(PIN_CS, 1);

                bin[0][i] = (((uint16_t(buf[0]) << 8) | buf[1]) & 0x0FFF);
                bin[0][i] -= 2048;
            }
            binflag[0] = 1;
        }
        if (!binflag[1]) {
            for (int i = 0; i < N; ++i) {

                while (!ADC_flag)
                    tight_loop_contents();
                ADC_flag = false;

                gpio_put(PIN_CS, 0);
                spi_read_blocking(SPI_PORT, 0, buf, 2);
                gpio_put(PIN_CS, 1);

                bin[1][i] = (((uint16_t(buf[0]) << 8) | buf[1]) & 0x0FFF);
                bin[1][i] -= 2048;
            }
            binflag[1] = 1;
        }
    }
}