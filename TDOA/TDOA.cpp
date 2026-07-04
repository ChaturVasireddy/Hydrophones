#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "clock.pio.h"

int main() {
    stdio_init_all();

    PIO pio_clk;
    uint sm_clk;
    uint offset_clk;
    bool success_clk = pio_claim_free_sm_and_add_program_for_gpio_range(&clock_program, &pio_clk, &sm_clk, &offset_clk, 0, 1, true);
    hard_assert(success_clk);

    clock_program_init(pio_clk, sm_clk, offset_clk, 9);

    PIO pio_data0;
    uint sm_data0;
    uint offset_data0;
    bool success_data0 = pio_claim_free_sm_and_add_program_for_gpio_range(&clock_program, &pio_data0, &sm_data0, &offset_data0, 0, 1, true);
    hard_assert(success_data0);

    clock_program_init(pio_data0, sm_data0, offset_data0, 9);
}