// requires SDCC compiler
// to compile, run:
// sdcc -mstm8 -DSTM8S005 toggle_all.c
// packihx toggle_all.ihx | Out-File -Encoding ascii toggle_all.hex

#include "stm8s.h"

// All outputs toggled together. Skipped: PD1 (SWIM), PD7 (TLI input-only).

typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
} PinDef;

static const PinDef outputs[] = {
    //{ GPIOA, (1 << 3) },
    //{ GPIOA, (1 << 4) },
    //{ GPIOA, (1 << 5) },
    //{ GPIOA, (1 << 6) },
    //{ GPIOB, (1 << 0) },
    //{ GPIOB, (1 << 1) },
    //{ GPIOB, (1 << 2) },
    //{ GPIOB, (1 << 3) },
    //{ GPIOB, (1 << 4) },
    //{ GPIOB, (1 << 5) },
    //{ GPIOB, (1 << 6) },
    //{ GPIOB, (1 << 7) },
    //{ GPIOC, (1 << 1) },
    //{ GPIOC, (1 << 2) },
    //{ GPIOC, (1 << 3) },
    //{ GPIOC, (1 << 4) },
    //{ GPIOC, (1 << 5) },
    //{ GPIOC, (1 << 6) },
    //{ GPIOC, (1 << 7) },
    //{ GPIOD, (1 << 0) },
    //{ GPIOD, (1 << 2) },
    //{ GPIOD, (1 << 3) },
    //{ GPIOD, (1 << 4) },
    //{ GPIOD, (1 << 5) },
    //{ GPIOD, (1 << 6) },
    //{ GPIOE, (1 << 0) },
    { GPIOE, (1 << 1) },
    { GPIOE, (1 << 2) },
    //{ GPIOE, (1 << 3) },
    //{ GPIOE, (1 << 5) },
    //{ GPIOG, (1 << 0) },
    //{ GPIOG, (1 << 1) },
};

#define NUM_OUTPUTS (sizeof(outputs) / sizeof(outputs[0]))

void delay_ms(uint16_t ms) {
    while (ms--) {
        volatile uint16_t i;
        for (i = 0; i < 200; i++) {
        }
    }
}

void main(void) {
    uint8_t i;

    for (i = 0; i < NUM_OUTPUTS; i++) {
        outputs[i].port->DDR |= outputs[i].pin;
        outputs[i].port->CR1 |= outputs[i].pin;
        outputs[i].port->CR2 |= outputs[i].pin;
    }

    while (1) {
        for (i = 0; i < NUM_OUTPUTS; i++)
            outputs[i].port->ODR |=  outputs[i].pin;
        delay_ms(1000);
        for (i = 0; i < NUM_OUTPUTS; i++)
            outputs[i].port->ODR &= ~outputs[i].pin;
        delay_ms(1000);
    }
}
