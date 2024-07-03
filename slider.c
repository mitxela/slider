#include "ch32v003fun.h"

// EN   C7
// DIR  C0
// STEP C1

int main()
{
	SystemInit();

	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;

	GPIOC->CFGLR &= ~((0xf<<(4*7)) | (0xf<<(4*0)) | (0xf<<(4*1)));
	GPIOC->CFGLR |=((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*7))
				 | ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0))
				 | ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*1));

	GPIOC->OUTDR &= ~(1<<7); // enable driver

	GPIOC->OUTDR |= (1<<0); // dir high

	while(1) {
		GPIOC->OUTDR &= ~(1<<1); // step low
		Delay_Us(500);
		GPIOC->OUTDR |= (1<<1); // step high
		Delay_Us(500);
	}
}
