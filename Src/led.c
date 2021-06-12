#include<stdint.h>
#include "led.h"

void delay(uint32_t count)
{
  for(uint32_t i = 0 ; i < count ; i++);
}

void led_init_all(void)
{

	uint32_t *pRccAhbenr   = (uint32_t*)0x40021014;
	uint32_t *pGpioeModeReg = (uint32_t*)0x48001000;


	*pRccAhbenr |= ( 1 << 21);
	*pGpioeModeReg |= ( 1 << (2 * LED_GREEN));
	*pGpioeModeReg |= ( 1 << (2 * LED_ORANGE));
	*pGpioeModeReg |= ( 1 << (2 * LED_RED));
	*pGpioeModeReg |= ( 1 << (2 * LED_BLUE));

    led_off(LED_GREEN);
    led_off(LED_ORANGE);
    led_off(LED_RED);
    led_off(LED_BLUE);
}

void led_on(uint8_t led_no)
{
  uint32_t *pGpioeDataReg = (uint32_t*)0x48001014;
  *pGpioeDataReg |= ( 1 << led_no);

}

void led_off(uint8_t led_no)
{
	  uint32_t *pGpioeDataReg = (uint32_t*)0x48001014;
	  *pGpioeDataReg &= ~( 1 << led_no);

}
