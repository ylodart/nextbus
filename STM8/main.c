// requires SDCC compiler
// to compile, run:
// sdcc -mstm8 -DSTM8S005 main.c
// packihx main.ihx | Out-File -Encoding ascii main.hex
//
// PC1: global enable output, driven high at startup
// PC2: clock input from Raspberry Pi (sampled on rising edge)
// PC3: data input from Raspberry Pi (MSB first: bit 21 = Rp ... bit 0 = L0)
// PD4: debug indicator, pulses once per 22-bit update

#include "stm8s.h"

#define ENABLE_PORT    GPIOC
#define ENABLE_PIN     (1 << 1)

#define CLK_PORT       GPIOC
#define CLK_PIN        (1 << 2)

#define DATA_PORT      GPIOC
#define DATA_PIN       (1 << 3)

#define INDICATOR_PORT GPIOD
#define INDICATOR_PIN  (1 << 4)

#define NUM_OUTPUTS    22

/* All 22 bits set — impossible as a real display state, used as a command. */
#define CMD_CATHODE_EXERCISE  0x3FFFFFUL

typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
} PinDef;

static const PinDef outputs[NUM_OUTPUTS] = {
    { GPIOD, (1 << 5) },  /*  0: L0  = PD5 */
    { GPIOB, (1 << 6) },  /*  1: L1  = PB6 */
    { GPIOB, (1 << 7) },  /*  2: L2  = PB7 */
    { GPIOA, (1 << 5) },  /*  3: L3  = PA5 */
    { GPIOA, (1 << 4) },  /*  4: L4  = PA4 */
    { GPIOA, (1 << 3) },  /*  5: L5  = PA3 */
    { GPIOA, (1 << 2) },  /*  6: L6  = PA2 */
    { GPIOA, (1 << 1) },  /*  7: L7  = PA1 */
    { GPIOD, (1 << 7) },  /*  8: L8  = PD7 */
    { GPIOD, (1 << 6) },  /*  9: L9  = PD6 */
    { GPIOD, (1 << 3) },  /* 10: Lp  = PD3 */
    { GPIOC, (1 << 6) },  /* 11: R0  = PC6 */
    { GPIOD, (1 << 0) },  /* 12: R1  = PD0 */
    { GPIOE, (1 << 0) },  /* 13: R2  = PE0 */
    { GPIOE, (1 << 3) },  /* 14: R3  = PE3 */
    { GPIOG, (1 << 1) },  /* 15: R4  = PG1 */
    { GPIOG, (1 << 0) },  /* 16: R5  = PG0 */
    { GPIOE, (1 << 7) },  /* 17: R6  = PE7 */
    { GPIOB, (1 << 0) },  /* 18: R7  = PB0 */
    { GPIOE, (1 << 6) },  /* 19: R8  = PE6 */
    { GPIOC, (1 << 5) },  /* 20: R9  = PC5 */
    { GPIOC, (1 << 7) },  /* 21: Rp  = PC7 */
};

void delay_ms(uint16_t ms) {
    while (ms--) {
        volatile uint16_t i;
        for (i = 0; i < 200; i++) {}
    }
}

static void pulse_indicator(void) {
    INDICATOR_PORT->ODR |=  INDICATOR_PIN;
    delay_ms(5);
    INDICATOR_PORT->ODR &= ~INDICATOR_PIN;
    delay_ms(5);
}

static void set_outputs(uint32_t bits) {
    uint8_t i;
    for (i = 0; i < NUM_OUTPUTS; i++) {
        if (bits & (1UL << i))
            outputs[i].port->ODR |=  outputs[i].pin;
        else
            outputs[i].port->ODR &= ~outputs[i].pin;
    }
}

/* Cycles every digit (0-9) and the decimal point on both tubes briefly to
   exercise all cathodes and prevent cathode poisoning on first boot. */
static void cathode_exercise(void) {
    uint8_t d;
    for (d = 0; d <= 9; d++) {
        set_outputs((1UL << d) | (1UL << (11 + d)));  /* Ld + Rd */
        delay_ms(150);
    }
    set_outputs((1UL << 10) | (1UL << 21));  /* Lp + Rp */
    delay_ms(150);
    set_outputs(0);
}

/* Waits for clock idle (low) before sampling, so frames self-sync after any
   partial receive at startup or after a communication gap. */
static uint32_t receive_bits(void) {
    uint32_t bits = 0;
    uint8_t i;

    while (CLK_PORT->IDR & CLK_PIN) {}

    for (i = 0; i < NUM_OUTPUTS; i++) {
        while (!(CLK_PORT->IDR & CLK_PIN)) {}  /* rising edge */
        bits <<= 1;
        if (DATA_PORT->IDR & DATA_PIN)
            bits |= 1;
        while (CLK_PORT->IDR & CLK_PIN) {}     /* falling edge */
    }
    return bits;
}

void main(void) {
    uint8_t i;
    uint32_t bits;
    uint32_t last_display = 0;

    for (i = 0; i < NUM_OUTPUTS; i++) {
        outputs[i].port->DDR |= outputs[i].pin;
        outputs[i].port->CR1 |= outputs[i].pin;
        outputs[i].port->CR2 |= outputs[i].pin;
        outputs[i].port->ODR &= ~outputs[i].pin;
    }

    ENABLE_PORT->DDR |= ENABLE_PIN;
    ENABLE_PORT->CR1 |= ENABLE_PIN;
    ENABLE_PORT->CR2 |= ENABLE_PIN;
    ENABLE_PORT->ODR |= ENABLE_PIN;

    CLK_PORT->DDR &= ~CLK_PIN;
    CLK_PORT->CR1 |=  CLK_PIN;
    CLK_PORT->CR2 &= ~CLK_PIN;

    DATA_PORT->DDR &= ~DATA_PIN;
    DATA_PORT->CR1 |=  DATA_PIN;
    DATA_PORT->CR2 &= ~DATA_PIN;

    INDICATOR_PORT->DDR |= INDICATOR_PIN;
    INDICATOR_PORT->CR1 |= INDICATOR_PIN;
    INDICATOR_PORT->CR2 |= INDICATOR_PIN;
    INDICATOR_PORT->ODR &= ~INDICATOR_PIN;

    cathode_exercise();

    while (1) {
        bits = receive_bits();
        if (bits == CMD_CATHODE_EXERCISE) {
            cathode_exercise();
            set_outputs(last_display);
        } else {
            last_display = bits;
            set_outputs(bits);
        }
        //pulse_indicator();
    }
}
