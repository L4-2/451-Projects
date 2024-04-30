// Timer2A.h
// Runs on Tiva-C

// Adapted from Program 8.2 from the book:
/* "Embedded Systems: Introduction to ARM Cortex-M Microcontrollers",
   ISBN: 978-1477508992, Jonathan Valvano, copyright (c) 2013
   Volume 1, Program 9.8
*/

#ifndef __TIMER2A_H__
#define __TIMER2A_H__

void EnableInterrupts();
void Timer2A_Init(); // Using PB0 for input capture (T2CCP0)
uint32_t Timer2A_getInterval(void);

#endif
