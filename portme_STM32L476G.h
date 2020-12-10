#include <stdio.h> // sprintf
#include "stm32l476xx.h"

// Sets up any platform-specific facilities.
// In this case, initializes USART 2.
void init( void )
{
	// Initialize USART 2.

	// To correctly read data from FLASH memory, the number of wait states (LATENCY)
	// must be correctly programmed according to the frequency of the CPU clock
	// (HCLK) and the supply voltage of the device.		
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	FLASH->ACR |=  FLASH_ACR_LATENCY_2WS;
		
	// Enable the Internal High Speed oscillator.
	RCC->CR |= RCC_CR_HSION;
	while((RCC->CR & RCC_CR_HSIRDY) == 0);
	// Adjust the Internal High Speed oscillator (HSI) calibration value.
	// RC oscillator frequencies are factory calibrated by ST for 1 % accuracy at 25oC
	// After reset, the factory calibration value is loaded in HSICAL[7:0] of RCC_ICSCR	
	RCC->ICSCR &= ~RCC_ICSCR_HSITRIM;
	RCC->ICSCR |= 16 << 24;
	
	RCC->CR    &= ~RCC_CR_PLLON; 
	while((RCC->CR & RCC_CR_PLLRDY) == RCC_CR_PLLRDY);
	
	// Select clock source to PLL
	RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLSRC;
	RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSI; // 00 = No clock, 01 = MSI, 10 = HSI, 11 = HSE
	
	// Make PLL as 80 MHz
	// f(VCO clock) = f(PLL clock input) * (PLLN / PLLM) = 16MHz * 20/2 = 160 MHz
	// f(PLL_R) = f(VCO clock) / PLLR = 160MHz/2 = 80MHz
	RCC->PLLCFGR = (RCC->PLLCFGR & ~RCC_PLLCFGR_PLLN) | 20U << 8;
	RCC->PLLCFGR = (RCC->PLLCFGR & ~RCC_PLLCFGR_PLLM) | 1U << 4;

	RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLR;  // 00: PLLR = 2, 01: PLLR = 4, 10: PLLR = 6, 11: PLLR = 8	
	RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN; // Enable Main PLL PLLCLK output 

	RCC->CR   |= RCC_CR_PLLON; 
	while((RCC->CR & RCC_CR_PLLRDY) == 0);
	
	// Select PLL selected as system clock
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL; // 00: MSI, 01:HSI, 10: HSE, 11: PLL
	
	// Wait until System Clock has been selected
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
	
	// The maximum frequency of the AHB, the APB1 and the APB2 domains is 80 MHz.
	RCC->CFGR &= ~RCC_CFGR_HPRE;  // AHB prescaler = 1; SYSCLK not divided
	RCC->CFGR &= ~RCC_CFGR_PPRE1; // APB high-speed prescaler (APB1) = 1, HCLK not divided
	RCC->CFGR &= ~RCC_CFGR_PPRE2; // APB high-speed prescaler (APB2) = 1, HCLK not divided
	
	RCC->CR &= ~RCC_CR_PLLSAI1ON;  // SAI1 PLL enable
	while ( (RCC->CR & RCC_CR_PLLSAI1ON) == RCC_CR_PLLSAI1ON );
	
	// Configure and enable PLLSAI1 clock to generate 11.294MHz 
	// 8 MHz * 24 / 17 = 11.294MHz
	RCC->PLLSAI1CFGR &= ~RCC_PLLSAI1CFGR_PLLSAI1N;
	RCC->PLLSAI1CFGR |= 24U<<8;
	
	// SAI1PLL division factor for PLLSAI1CLK
	// 0: PLLSAI1P = 7, 1: PLLSAI1P = 17
	RCC->PLLSAI1CFGR |= RCC_PLLSAI1CFGR_PLLSAI1P;
	RCC->PLLSAI1CFGR |= RCC_PLLSAI1CFGR_PLLSAI1PEN;
	
	RCC->CR |= RCC_CR_PLLSAI1ON;  // SAI1 PLL enable
	while ( (RCC->CR & RCC_CR_PLLSAI1ON) == 0);
	
	RCC->CCIPR &= ~RCC_CCIPR_SAI1SEL;

	RCC->APB2ENR |= RCC_APB2ENR_SAI1EN;

	// Enable the clock to USART 2.
	RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

	// Use the system clock.
	RCC->CCIPR &= ~RCC_CCIPR_USART2SEL;
	RCC->CCIPR |=  RCC_CCIPR_USART2SEL_0;

	// Enable the peripheral clock for the GPIO.
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
	
	GPIOD->MODER   &= ~(0xF << (2*5));
	GPIOD->MODER   |=   0xA << (2*5);      		
	GPIOD->AFR[0]  |=   0x77<< (4*5);       	
	// GPIO Speed: Low speed (00), Medium speed (01), Fast speed (10), High speed (11)
	GPIOD->OSPEEDR |=   0xF<<(2*5); 					 	
	// GPIO Push-Pull: No pull-up, pull-down (00), Pull-up (01), Pull-down (10), Reserved (11)
	GPIOD->PUPDR   &= ~(0xF<<(2*5));
	GPIOD->PUPDR   |=   0x5<<(2*5);    				
	// GPIO Output Type: Output push-pull (0, reset), Output open drain (1) 
	GPIOD->OTYPER  &=  ~(0x3<<5);

	// Disable USART 2.
	USART2->CR1 &= ~USART_CR1_UE;
	
	// Set word length to 8 bits.
	USART2->CR1 &= ~USART_CR1_M;
	
	// Oversample by 16.
	USART2->CR1 &= ~USART_CR1_OVER8;

	// Use one stop bit.
	USART2->CR2 &= ~USART_CR2_STOP;   
        
	// Set baud rate to 9600.
	USART2->BRR  = 0x208D;

	// Enable transmission and reception.
	USART2->CR1  |= (USART_CR1_RE | USART_CR1_TE);

	USART2->ICR |= USART_ICR_TCCF;
	USART1->CR3 |= USART_CR3_DMAT | USART_CR3_DMAR;

	// Enable the USART.	
	USART2->CR1  |= USART_CR1_UE;
	
	// Verify the UART is ready.
	while ( ( USART2->ISR & USART_ISR_TEACK ) == 0 );
	while ( ( USART2->ISR & USART_ISR_REACK ) == 0 );


	// Initialize joystick input.

	// Enable the clock to GPIO Port A.
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

	// Set input mode.
	GPIOA->MODER &= ~GPIO_MODER_MODER0;
}

// Prints the null-terminated ASCII string `s` to a TTY somewhere.
void print( const char * s )
{
	for ( ; *s != '\0'; *s++ )
	{
		while ( !( USART2->ISR & USART_ISR_TXE ) ); // wait until the UART empties
	
		USART2->TDR = *s;
	
		volatile int delay = 2000;
		while ( delay ) delay--;
	
		// Why do we do this?
		while ( !( USART2->ISR & USART_ISR_TC ) );
		USART2->ISR &= ~USART_ISR_TC;
	}
}

// Seeds the PRNG.
void seed()
{
#if 0 // Our STM32L476 doesn't have a hardware random number generator.

	// Disable RNG validation and interrputs.
	RNG->CR = 0;

	// Enable the clock to the RNG.
	RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;

	// Enable the RNG.
	RNG->CR |= RNG_CR_RNGEN;

	// Wait paranoidly for the RNG to settle.
	for ( volatile int i = 0; i < 100; i++ );

	// Assign values.
	while ( ( RNG->SR & RNG_SR_DRDY ) == 0 );
	random_a = RNG->DR;
	while ( ( RNG->SR & RNG_SR_DRDY ) == 0 );
	random_b = RNG->DR;
	while ( ( RNG->SR & RNG_SR_DRDY ) == 0 );
	random_c = RNG->DR;

	// Burn subpar PRNG values.
	for ( volatile int i = 0; i < 10; i++ ) random32();

#elif 1 // Use press-to-start instead.
	uint32_t old = 0;
	print( "Press a button to begin...\n\r" );
	

#endif
}

// Blocks until a button is pressed down somewhere, then returns that movement.
enum move getmove( void )
{
	while ( !( USART2->ISR & USART_ISR_RXNE ) ); // wait until the UART fills
	switch ( USART2->RDR & 0x7F ) // get ASCII keypress
	{
		case 'w': return MOVE_UP;
		case 'a': return MOVE_LEFT;
		case 's': return MOVE_DOWN;
		case 'd': return MOVE_RIGHT;
		case 'q': return MOVE_QUIT;
	}
}


