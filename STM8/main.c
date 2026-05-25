// requires SDCC compiler
// to compile, run:
// sdcc -mstm8 -DSTM8S005 main.c
// packihx main.ihx | Out-File -Encoding ascii main.hex

// peripheral library: 
// https://www.st.com/en/embedded-software/stsw-stm8069.html
// compiler: https://sourceforge.net/projects/sdcc/files/sdcc-win64/
// programmer: https://www.st.com/en/development-tools/stvp-stm8.html

#include "stm8s.h"

// PD4 pulses once after each output is set high, and 4 times when the loop restarts.
// Skipped pins: PD1 (SWIM), PD4 (indicator).
//
//  1=PD5  2=PB6  3=PB7  4=PA5  5=PA4  6=PA3  7=PA2  8=PA1
//  9=PD7 10=PD6 11=PD3 12=PC6 13=PD0 14=PE0 15=PE3 16=PG0
// 17=PG1 18=PE7 19=PB0 20=PE6 21=PC5 22=PC7
// PC1, PE2: always high

#define INDICATOR_PORT GPIOD
#define INDICATOR_PIN  (1 << 4)
#define NUM_OUTPUTS    22

typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
} PinDef;

static const PinDef outputs[NUM_OUTPUTS] = {
    { GPIOD, (1 << 5) },  /*  1: PD5 */
    { GPIOB, (1 << 6) },  /*  2: PB6 */
    { GPIOB, (1 << 7) },  /*  3: PB7 */
    { GPIOA, (1 << 5) },  /*  4: PA5 */
    { GPIOA, (1 << 4) },  /*  5: PA4 */
    { GPIOA, (1 << 3) },  /*  6: PA3 */
    { GPIOA, (1 << 2) },  /*  7: PA2 */
    { GPIOA, (1 << 1) },  /*  8: PA1 */
    { GPIOD, (1 << 7) },  /*  9: PD7 */
    { GPIOD, (1 << 6) },  /* 10: PD6 */
    { GPIOD, (1 << 3) },  /* 11: PD3 */
    { GPIOC, (1 << 6) },  /* 12: PC6 */
    { GPIOD, (1 << 0) },  /* 13: PD0 */
    { GPIOE, (1 << 0) },  /* 14: PE0 */
    { GPIOE, (1 << 3) },  /* 15: PE3 */
    { GPIOG, (1 << 1) },  /* 16: PG1 */
    { GPIOG, (1 << 0) },  /* 17: PG0 */
    { GPIOE, (1 << 7) },  /* 18: PE7 */
    { GPIOB, (1 << 0) },  /* 19: PB0 */
    { GPIOE, (1 << 6) },  /* 20: PE6 */
    { GPIOC, (1 << 5) },  /* 21: PC5 */
    { GPIOC, (1 << 7) },  /* 22: PC7 */
};

void delay_ms(uint16_t ms) {
    while (ms--) {
        volatile uint16_t i;
        for (i = 0; i < 200; i++) {
        }
    }
}

static void pulse_indicator(void) {
    INDICATOR_PORT->ODR |=  INDICATOR_PIN;
    delay_ms(100);
    INDICATOR_PORT->ODR &= ~INDICATOR_PIN;
    delay_ms(100);
}

void main(void) {
    uint8_t i;

    for (i = 0; i < NUM_OUTPUTS; i++) {
        outputs[i].port->DDR |= outputs[i].pin;
        outputs[i].port->CR1 |= outputs[i].pin;
        outputs[i].port->CR2 |= outputs[i].pin;
    }
    // set PC1 high
    GPIOC->DDR |= (1 << 1);
    GPIOC->CR1 |= (1 << 1);
    GPIOC->CR2 |= (1 << 1);
    GPIOC->ODR |= (1 << 1);

    GPIOE->DDR |= (1 << 2);
    GPIOE->CR1 |= (1 << 2);
    GPIOE->CR2 |= (1 << 2);
    GPIOE->ODR |= (1 << 2);

    GPIOD->DDR |= (1 << 2);
    GPIOD->CR1 |= (1 << 2);
    GPIOD->CR2 |= (1 << 2);
    GPIOD->ODR |= (1 << 2);

    // PE1, PE2: inputs with internal pull-ups for I2C
    //GPIOE->DDR &= ~((1 << 1) | (1 << 2));
    //GPIOE->CR1 |=  (1 << 1) | (1 << 2);
    //GPIOE->CR2 &= ~((1 << 1) | (1 << 2));

    INDICATOR_PORT->DDR |= INDICATOR_PIN;
    INDICATOR_PORT->CR1 |= INDICATOR_PIN;
    INDICATOR_PORT->CR2 |= INDICATOR_PIN;

    while (1) {
        for (i = 0; i < NUM_OUTPUTS; i++) {
            outputs[i].port->ODR |=  outputs[i].pin;
            //pulse_indicator();
            delay_ms(250);
            outputs[i].port->ODR &= ~outputs[i].pin;
        }
        //pulse_indicator();
        //pulse_indicator();
        //pulse_indicator();
        //pulse_indicator();
        delay_ms(500);
    }
}
