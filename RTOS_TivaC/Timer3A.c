// Timer2A.c
// Runs on Tiva-C

// Adapted from Program 8.2 from the book:
/* "Embedded Systems: Introduction to ARM Cortex-M Microcontrollers",
   ISBN: 978-1477508992, Jonathan Valvano, copyright (c) 2013
   Volume 1, Program 9.8
*/

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "Timer3A.h"

// Using PB0 for input capture (T2CCP0)
void Timer3A_Init()
{
  SYSCTL_RCGCTIMER_R |= 0x00000004;                                  // Activate Timer2
  SYSCTL_RCGCGPIO_R |= 0x00000002;                                   // Activate Port B
  GPIO_PORTB_DEN_R |= 0x01;                                          // Enable digital I/O on PB0
  GPIO_PORTB_AFSEL_R |= 0x01;                                        // Enable alternate function on PB0
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xFFFFFFF0) | 0x00000007; // Enable T2CCP0
  TIMER3_CTL_R &= ~TIMER_CTL_TAEN;                                   // Disable Timer2A during setup
  TIMER3_CFG_R = TIMER_CFG_16_BIT;                                   // Configure for 16-bit timer mode
  TIMER3_TAMR_R = TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP;            // Configure for capture mode
  TIMER3_CTL_R &= ~(TIMER_CTL_TAEVENT_POS | 0xC);                    // Configure for rising-edge event
  TIMER3_TAILR_R = 0x0000FFFF;                                       // Start value
  TIMER3_IMR_R |= TIMER_IMR_CAEIM;                                   // Enable capture match interrupt
  TIMER3_ICR_R = TIMER_ICR_CAECINT;                                  // Clear Timer2A capture match flag
  TIMER3_CTL_R |= TIMER_CTL_TAEN;                                    // Enable Timer2A
  NVIC_PRI5_R = (NVIC_PRI5_R & 0x00FFFFFF) | 0x40000000;             // Timer2A = Priority 2
  NVIC_EN0_R = 0x00800000;                                           // Enable interrupt 23 in NVIC
  EnableInterrupts();
  return;
}
