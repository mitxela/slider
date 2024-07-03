#include "ch32v003fun.h"

// EN   C7
// DIR  C0
// STEP C1
// ADC  D4 (A7)
// Joystick on A1 and A2

#define dir_left()   GPIOC->OUTDR |=  (1<<0)
#define dir_right()  GPIOC->OUTDR &= ~(1<<0)
#define driver_on()  GPIOC->OUTDR &= ~(1<<7)
#define driver_off() GPIOC->OUTDR |=  (1<<7)
#define step_high()  GPIOC->OUTDR |=  (1<<1)
#define step_low()   GPIOC->OUTDR &= ~(1<<1)
#define joystick_left()  (!(GPIOA->INDR & (1<<1)))
#define joystick_right() (!(GPIOA->INDR & (1<<2)))

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
	FAST_MOVE,
	JOYSTICK_TAP,
	JOYSTICK_HELD
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
//	GPIOA->BSHR = GPIO_BSHR_BS1 | GPIO_BSHR_BS2;

	init_adc();
	dir_right();



	uint16_t adc = 0;

	while(1) {
		if (joystick_left()) dir_left();
		else if (joystick_right()) dir_right();

		uint32_t y = 200 + ((adc*adc)  >>5);
		Delay_Us(y); // 200 ... 50000
		if (adc<1023-10) {
			driver_on();
			step_high();
		} else {
			driver_off();
		}
		adc = read_adc();
		Delay_Us(1);
		step_low();
	}
}
