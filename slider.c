#include "ch32v003fun.h"

// EN   C7
// DIR  C0
// STEP C1
// ADC  D4 (A7)


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


int main()
{
	SystemInit();

	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;

	GPIOC->CFGLR &= ~((0xf<<(4*7)) | (0xf<<(4*0)) | (0xf<<(4*1)));
	GPIOC->CFGLR |=((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*7))
				 | ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0))
				 | ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*1));

	init_adc();


	GPIOC->OUTDR |= (1<<0); // dir

	uint16_t x = 50000;

	while(1) {
		uint32_t y = 200 + ((x*x)  >>5);
		Delay_Us(y); // 200 ... 50000
		if (x<1023-10) {
			GPIOC->OUTDR &= ~(1<<7); // enable driver
			GPIOC->OUTDR |= (1<<1); // step high
		} else {
			GPIOC->OUTDR |= (1<<7); // disable driver
		}
		x = read_adc();
		Delay_Us(1);
		GPIOC->OUTDR &= ~(1<<1); // step low
	}
}
