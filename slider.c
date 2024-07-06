#include "ch32v003fun.h"

// EN   C7
// DIR  C0
// STEP C1
// ADC  D4 (A7)
// Joystick on A1 and A2

#define dir_left()   GPIOC->BSHR = GPIO_BSHR_BS0
#define dir_right()  GPIOC->BSHR = GPIO_BSHR_BR0
#define driver_on()  GPIOC->BSHR = GPIO_BSHR_BR7
#define driver_off() GPIOC->BSHR = GPIO_BSHR_BS7
#define step_high()  GPIOC->BSHR = GPIO_BSHR_BS1
#define step_low()   GPIOC->BSHR = GPIO_BSHR_BR1
#define joystick_left()  (!(GPIOA->INDR & (1<<1)))
#define joystick_right() (!(GPIOA->INDR & (1<<2)))

#define debounce() Delay_Ms(200)

#define pulse_step() \
	step_high(); \
	Delay_Us(2); \
	step_low();

void init_adc(){

	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_ADC1;

	// PD4 is analog input chl 7
	GPIOD->CFGLR &= ~(0xf<<(4*4));	// CNF = 00: Analog, MODE = 00: Input

	RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;

	RCC->CFGR0 &= ~(0x1F<<11); //RCC_ADCPRE = 0
	ADC1->RSQR1 = 0;
	ADC1->RSQR2 = 0;
	ADC1->RSQR3 = 7;
	// sampling time
	ADC1->SAMPTR2 &= ~(ADC_SMP0<<(3*7));
	ADC1->SAMPTR2 |= 7<<(3*7);

	// turn on, set sw trig
	ADC1->CTLR2 |= ADC_ADON | ADC_EXTSEL;
	// reset calibration
	ADC1->CTLR2 |= ADC_RSTCAL;
	while(ADC1->CTLR2 & ADC_RSTCAL);
	// calibrate
	ADC1->CTLR2 |= ADC_CAL;
	while(ADC1->CTLR2 & ADC_CAL);

}


uint16_t read_adc(void){
	ADC1->CTLR2 |= ADC_SWSTART;
	while(!(ADC1->STATR & ADC_EOC));
	return ADC1->RDATAR;
}

enum {
	IDLE = 0,
	MOVE,
	FAST_MOVE
};
int state = IDLE;

int main()
{
	SystemInit();

	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC;

	GPIOC->CFGLR &= ~((0xf<<(4*7)) | (0xf<<(4*0)) | (0xf<<(4*1)));
	GPIOC->CFGLR |=((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*7))
				 | ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0))
				 | ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*1));

	GPIOA->CFGLR &= ~((0xf<<(4*1)) | (0xf<<(4*2)));
	GPIOA->CFGLR |= (GPIO_CNF_IN_PUPD)<<(4*1) | (GPIO_CNF_IN_PUPD)<<(4*2);
	GPIOA->OUTDR |= (1<<1)|(1<<2);

	init_adc();
	#define RAMP_MAX 350
	int ramp = RAMP_MAX;

	while(1) switch (state) {
	case IDLE:
		driver_off();
		if (joystick_left()) {
			dir_left();
			driver_on();
			debounce();
			if (joystick_left()) state = FAST_MOVE;
			else state = MOVE;
		} else if (joystick_right()) {
			dir_right();
			driver_on();
			debounce();
			if (joystick_right()) state = FAST_MOVE;
			else state = MOVE;
		}
	break;

	case FAST_MOVE:
		if (joystick_left() || joystick_right()) {
			pulse_step();
			Delay_Us(200 + ramp);
			if (ramp > 0) ramp -= 1;
		} else {
			while (ramp < RAMP_MAX) {
				pulse_step();
				Delay_Us(200 + ramp);
				ramp += 1;
			}
			state = IDLE;
		}
	break;

	case MOVE:
		uint16_t adc = 1023 - read_adc();
		if (joystick_left() || joystick_right()) {
			state = IDLE;
			debounce();
		} else {
			pulse_step();
			Delay_Us( 200 + ((adc*adc)>>5) );
		}
	break;

	}
}
