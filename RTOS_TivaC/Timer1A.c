// Timer1A.c
// Runs on Tiva-C

// Adapted from SysTick.c from the book:
/* "Embedded Systems: Introduction to MSP432 Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015
   Volume 1, Program 4.7
*/

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "Timer1A.h"
#include "SSI2.h"

uint32_t sysClkFreq1 = 80000000; // Assume 80 MHz clock by default

// Set clock freq. so Timer1A_Wait10ms delays for exactly 10 ms if clock is not 80 MHz
void Timer1A_Init( uint32_t clkFreq ){
  sysClkFreq1 = clkFreq / 200;

  // SYSCTL_RCGCTIMER_R |= 0x00000002;  // 0) Activate Timer1
  // TIMER1_CTL_R &= ~0x00000001;       // 1) Disable Timer1A during setup
  // TIMER1_CFG_R = 0;                  // 2) Configure for 32-bit timer mode
  // TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;                 // 3) Configure for one-shot mode
  // TIMER1_TAILR_R = sysClkFreq1;      // 4) Specify reload value
  // TIMER1_TAPR_R = 0;                 // 5) No prescale
  // TIMER1_IMR_R = 1;                  // 6-9) No interrupts
  // TIMER1_CTL_R |= 0x00000001;        // 10) Enable Timer1A

  // // ISR for Timer1A
  //  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|0x00006000; // priority 3
  //  NVIC_EN0_R = 0x00800000;

  SYSCTL_RCGCTIMER_R |= 0x02;   // 1) activate TIMER1
  TIMER1_CTL_R = 0x00;    // 2) disable TIMER1A during setup
  TIMER1_CFG_R = 0x00;    // 3) configure for 32-bit mode
  TIMER1_TAMR_R = 0x02;   // 4) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R = sysClkFreq1;    // 5) reload value
  TIMER1_TAPR_R = 0;      // 6) bus clock resolution
  TIMER1_ICR_R = 0x1;     // 7) clear TIMER1A timeout flag
  TIMER1_IMR_R |= 0x01;   // 8) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|0x00008000; // 9) priority 4
  NVIC_EN0_R = 1<<21;     // 10) enable IRQ 21 in NVIC
  TIMER1_CTL_R = 0x01;    // 11) enable TIMER1A

  return;
}

volatile uint8_t currentDigit = 0;

void Timer1A_Handler(void)
{
  // // Clear the interrupt flag
  // TIMER1_ICR_R = TIMER_ICR_TATOCINT;

  // // Write the digit pattern to the seven-segment display
  // const static unsigned char digitPattern[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
  // SSI2_write(digitPattern[currentDigit], 0x80);

  // // Select the next digit
  // currentDigit = (currentDigit + 1) % 10;

  // // Reload Timer1A to generate the next interrupt
  // TIMER1_TAILR_R = sysClkFreq1;

  // Clear the interrupt flag
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;

  // Write the digit pattern to the seven-segment display
  const static unsigned char digitPattern[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
  SSI2_write(digitPattern[currentDigit], 0x80);
  SSI2_write(digitPattern[currentDigit], 0x80);

  // Select the next digit
  currentDigit = (currentDigit + 1) % 10;

  // Reload Timer1A to generate the next interrupt
  TIMER1_TAILR_R = sysClkFreq1;

}

// Time delay using busy wait
// The delay parameter is in units of the core clock (units of 12.5 nsec for 80 MHz clock)
//   Adapted from Program 9.8 from the book:
/*   "Embedded Systems: Introduction to ARM Cortex-M Microcontrollers",
     ISBN: 978-1477508992, Jonathan Valvano, copyright (c) 2013
     Volume 1, Program 9.8
*/
void Timer1A_Wait( uint32_t delay ){

  if(delay <= 1){ return; } // Immediately return if requested delay less than one clock

  SYSCTL_RCGCTIMER_R |= 0x00000002;  // 0) Activate Timer1
  TIMER1_CTL_R &= ~0x00000001;       // 1) Disable Timer1A during setup
  TIMER1_CFG_R = 0;                  // 2) Configure for 32-bit timer mode
  TIMER1_TAMR_R = 1;                 // 3) Configure for one-shot mode
  TIMER1_TAILR_R = delay - 1;        // 4) Specify reload value
  TIMER1_TAPR_R = 0;                 // 5) No prescale
  TIMER1_IMR_R = 0;                  // 6-9) No interrupts
  TIMER1_CTL_R |= 0x00000001;        // 10) Enable Timer1A

  //while( TIMER1_TAR_R ){} // Doesn't work; Wait until timer expires (value equals 0)
  // Or, clear interrupt and wait for raw interrupt flag to be set
  TIMER1_ICR_R = 1;
  while( !(TIMER1_RIS_R & 0x1) ){}
  return;
}

// Time delay using busy wait
// This assumes 80 MHz system clock
void Timer1A_Wait1ms( uint32_t delay ){
  uint32_t i;
  for( i = 0; i < delay; i++ ){
    Timer1A_Wait(sysClkFreq1/1000);  // wait 1ms
  }
  return;
}
