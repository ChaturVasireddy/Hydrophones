#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/multicore.h"
#include <cmath>
#include <cstdint>
#include "hardware/pio.h"
#include "customcom.pio.h"

#define F0 25000.0f
#define F1 30000.0f
#define F2 35000.0f
#define F3 40000.0f

#define INT_PIN 5
#define SAMPLE_CLK_PIN 9
#define DATA_PIN 8


volatile bool sync_flag = false;
volatile bool ADC_flag = false;

void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == SAMPLE_CLK_PIN)
    {
        ADC_flag = true;
    }
    else if (gpio == INT_PIN)
    {
        sync_flag = true;
    }
}

#define SPI_PORT_ADC spi0

#define PIN_RX_ADC 0
#define PIN_CS_ADC 1
#define PIN_SCK_ADC 2

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

PIO pio;
uint sm;
uint offset;

void send_peak() {
    uint32_t packet = ((uint32_t)peak_time << 2) | (peak_freq & 0x3);

    pio_sm_put_blocking(pio, sm, packet);
}

void core1_entry() {
    while (true) {
        if (binflag[0]) {
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
                else if (goertzel_result[2] > goertzel_result[0] && goertzel_result[2] > goertzel_result[1] && goertzel_result[2] > goertzel_result[3]) {
                    peak_freq = 2;
                }
                else {
                    peak_freq = 3;
                }
                for (int i = 0; i < N; ++i) {
                    if (bin[0][i] > 512 || bin[0][i] < -512) {
                        peak_time = (bin_count * N) + i;
                        send_peak();
                        break;
                    }
                }
            }
            else
                printf("null\n");
            binflag[0] = 0;
            bin_count++;
        }
        if (binflag[1]) {
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
                else if (goertzel_result[2] > goertzel_result[0] && goertzel_result[2] > goertzel_result[1] && goertzel_result[2] > goertzel_result[3]) {
                    peak_freq = 2;
                }
                else {
                    peak_freq = 3;
                }
                for (int i = 0; i < N; ++i) {
                    if (bin[1][i] > 512 || bin[1][i] < -512) {
                        peak_time = (bin_count * N) + i;
                        send_peak();
                        break;
                    }
                }
            }
            else
                printf("null\n");
            binflag[1] = 0;
            bin_count++;
        }
    }
}

int main() {

    stdio_init_all();

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&customcom_program, &pio,
        &sm, &offset, 0, 1, true);
    hard_assert(success);
    customcom_program_init(pio, sm, offset, DATA_PIN, SAMPLE_CLK_PIN);

    spi_init(SPI_PORT_ADC, 8 * 1000 * 1000);
    gpio_set_function(PIN_RX_ADC, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK_ADC, GPIO_FUNC_SPI);
    gpio_init(PIN_CS_ADC);
    gpio_set_dir(PIN_CS_ADC, GPIO_OUT);
    gpio_put(PIN_CS_ADC, 1);

    gpio_init(INT_PIN);
    gpio_set_dir(INT_PIN, GPIO_IN);
    gpio_pull_up(INT_PIN);
    gpio_set_irq_enabled_with_callback(INT_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    gpio_init(SAMPLE_CLK_PIN);
    gpio_set_dir(SAMPLE_CLK_PIN, GPIO_IN);
    gpio_pull_down(SAMPLE_CLK_PIN);
    gpio_set_irq_enabled(SAMPLE_CLK_PIN, GPIO_IRQ_EDGE_RISE, true);

    multicore_launch_core1(core1_entry);

    uint8_t buf[2];

    while (true)
    {
        if (sync_flag == 1) {
            binflag[0] = 0;
            binflag[1] = 0;
            peak_time = -1;
            peak_freq = -1;
            bin_count = 0;
            while (gpio_get(INT_PIN)) {
                tight_loop_contents();
            }
            sync_flag = 0;
        }
        if (!binflag[0] && sync_flag == 0) {
            for (int i = 0; i < N; ++i) {

                while (!ADC_flag)
                    tight_loop_contents();
                ADC_flag = false;

                gpio_put(PIN_CS_ADC, 0);
                spi_read_blocking(SPI_PORT_ADC, 0, buf, 2);
                gpio_put(PIN_CS_ADC, 1);

                bin[0][i] = (((uint16_t(buf[0]) << 8) | buf[1]) & 0x0FFF);
                bin[0][i] -= 2048;
            }
            binflag[0] = 1;
        }
        if (!binflag[1] && sync_flag == 0) {
            for (int i = 0; i < N; ++i) {

                while (!ADC_flag)
                    tight_loop_contents();
                ADC_flag = false;

                gpio_put(PIN_CS_ADC, 0);
                spi_read_blocking(SPI_PORT_ADC, 0, buf, 2);
                gpio_put(PIN_CS_ADC, 1);

                bin[1][i] = (((uint16_t(buf[0]) << 8) | buf[1]) & 0x0FFF);
                bin[1][i] -= 2048;
            }
            binflag[1] = 1;
        }
    }
}