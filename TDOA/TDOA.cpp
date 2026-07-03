#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "clock.pio.h"

int main() {
    stdio_init_all();

    PIO pio;
    uint sm;
    uint offset;
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&clock_program, &pio,
        &sm, &offset, 0, 1, true);
    hard_assert(success);

    clock_program_init(pio, sm, offset, 0);
}