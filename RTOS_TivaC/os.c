// os.c
// Runs on LM4F120/TM4C123
// A very simple real time operating system with minimal features.
// Daniel Valvano
// January 29, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

   Programs 4.4 through 4.12, section 4.2

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#include <stdint.h>
#include "os.h"
#include "PLL.h"
#include "SSI2.h"
#include "LCD.h"
#include "Timer0A.h"
#include "Timer1A.h"
#include "Timer2A.h"
#include "Timer3A.h"
#include "tm4c123gh6pm.h"

uint32_t Slicecount = 0;

uint32_t TIN;

// function definitions in OSasm.s
void OS_DisableInterrupts(void); // Disable interrupts
void OS_EnableInterrupts(void);  // Enable interrupts
int32_t StartCritical(void);
void EndCritical(int32_t primask);
void StartOS(void);

#define NUMTHREADS  3        // maximum number of threads
#define STACKSIZE   100      // number of 32-bit words in stack
struct tcb{
  int32_t *sp;       // pointer to stack (valid for threads not running
  struct tcb *next;  // linked-list pointer
  uint32_t TIN;  // thread identification number
};
typedef struct tcb tcbType;
tcbType tcbs[NUMTHREADS];
tcbType *RunPt;
int32_t Stacks[NUMTHREADS][STACKSIZE];

// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: systick, 8 MHz PLL
// Inputs: none
// Outputs: none
void OS_Init(void){
  OS_DisableInterrupts();
  PLL_Init(Bus8MHz);         // set processor clock to 8 MHz
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|0xE0000000; // priority 7

  // initialize Timer0A
  Timer0A_Init(8000000); // 10 ms
  
  // initialize Timer1A 
  Timer1A_Init(8000000); // 10 ms

  // initialize Timer2A
  Timer2A_Init(8000000); // 10 ms

  // initialize SSI2
  SSI2_init();

  // initialize 7SDisplay
  sevenseg_init();

  //const static unsigned char digitPattern[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
  // uint32_t i;


  // // test loop for 7-segment display
  // while(1) {
  //   SSI2_write(digitPattern[i], 0x80);
  //   Timer1A_Wait(80000); // 1 sec
  //   i = (i + 1) % 10;
  // }


}

void SetInitialStack(int i){
  tcbs[i].sp = &Stacks[i][STACKSIZE-16]; // thread stack pointer
  Stacks[i][STACKSIZE-1] = 0x01000000;   // thumb bit
  Stacks[i][STACKSIZE-3] = 0x14141414;   // R14
  Stacks[i][STACKSIZE-4] = 0x12121212;   // R12
  Stacks[i][STACKSIZE-5] = 0x03030303;   // R3
  Stacks[i][STACKSIZE-6] = 0x02020202;   // R2
  Stacks[i][STACKSIZE-7] = 0x01010101;   // R1
  Stacks[i][STACKSIZE-8] = 0x00000000;   // R0
  Stacks[i][STACKSIZE-9] = 0x11111111;   // R11
  Stacks[i][STACKSIZE-10] = 0x10101010;  // R10
  Stacks[i][STACKSIZE-11] = 0x09090909;  // R9
  Stacks[i][STACKSIZE-12] = 0x08080808;  // R8
  Stacks[i][STACKSIZE-13] = 0x07070707;  // R7
  Stacks[i][STACKSIZE-14] = 0x06060606;  // R6
  Stacks[i][STACKSIZE-15] = 0x05050505;  // R5
  Stacks[i][STACKSIZE-16] = 0x04040404;  // R4
}


// ******** OS_AddThread ***************
// add three foreground threads to the scheduler
// Inputs: three pointers to a void/void foreground tasks
// Outputs: 1 if successful, 0 if this thread can not be added
int OS_AddThreads(void(*task0)(void),
                 void(*task1)(void),
                 void(*task2)(void)){ int32_t status;
  status = StartCritical();
  tcbs[0].next = &tcbs[1]; // 0 points to 1
  tcbs[1].next = &tcbs[2]; // 1 points to 2
  tcbs[2].next = &tcbs[0]; // 2 points to 0
  SetInitialStack(0); Stacks[0][STACKSIZE-2] = (int32_t)(task0); // PC
  SetInitialStack(1); Stacks[1][STACKSIZE-2] = (int32_t)(task1); // PC
  SetInitialStack(2); Stacks[2][STACKSIZE-2] = (int32_t)(task2); // PC
  RunPt = &tcbs[0];       // thread 0 will run first
  EndCritical(status);
  return 1;               // successful
}

/// ******** OS_Launch ***************
// start the scheduler, enable interrupts
// Inputs: number of bus clock cycles for each time slice
//         (maximum of 24 bits)
// Outputs: none (does not return)
void OS_Launch(uint32_t theTimeSlice){
  NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
  NVIC_ST_CTRL_R = 0x00000007; // enable, core clock and interrupt arm
  StartOS();                   // start on the first task
}

// enable SSI2 and associated GPIO pins
void sevenseg_init(void)
{
  SYSCTL_RCGCGPIO_R |= 0x02; // enable clock to GPIOB
  SYSCTL_RCGCGPIO_R |= 0x04; // enable clock to GPIOC
  SYSCTL_RCGCSSI_R |= 0x04;  // enable clock to SSI2

  // PORTB 7, 4 for SSI2 TX and SCLK
  GPIO_PORTB_AMSEL_R &= ~0x90;      // turn off analog of PORTB 7, 4
  GPIO_PORTB_AFSEL_R |= 0x90;       // PORTB 7, 4 for alternate function
  GPIO_PORTB_PCTL_R &= ~0xF00F0000; // clear functions for PORTB 7, 4
  GPIO_PORTB_PCTL_R |= 0x20020000;  // PORTB 7, 4 for SSI2 function
  GPIO_PORTB_DEN_R |= 0x90;         // PORTB 7, 4 as digital pins

  // PORTC 7 for SSI2 slave select
  GPIO_PORTC_AMSEL_R &= ~0x80; // disable analog of PORTC 7
  GPIO_PORTC_DATA_R |= 0x80;   // set PORTC 7 idle high
  GPIO_PORTC_DIR_R |= 0x80;    // set PORTC 7 as output for SS
  GPIO_PORTC_DEN_R |= 0x80;    // set PORTC 7 as digital pin

  SSI2_CR1_R = 0;      // turn off SSI2 during configuration
  SSI2_CC_R = 0;       // use system clock
  SSI2_CPSR_R = 16;    // clock prescaler divide by 16 gets 1 MHz clock
  SSI2_CR0_R = 0x0007; // clock rate div by 1, phase/polarity 0 0, mode freescale, data size 8
  SSI2_CR1_R = 2;      // enable SSI2 as master
}
