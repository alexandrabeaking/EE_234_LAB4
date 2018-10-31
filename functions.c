#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xil_exception.h"

//Various Registers
#define ICCPMR_BASEADDR 0xF8F00104 // Interrupt Priority Mask Register
#define ICCICR_BASEADDR 0xF8F00100 // CPU Interface Control Register
#define ICDDCR_BASEADDR 0xF8F01000 // Distributor Control Register
#define ICDISER_BASEADDR 0xF8F01100 // Interrupt Set Enable Register
#define ICDICER_BASEADDR 0xF8F01180 // Interrupt Clear/Enable Register
#define ICDIPR_BASEADDR 0xF8F01400 // Interrupt Priority Register Base address
#define ICDIPTR_BASEADDR 0xF8F01800 // Interrupt Processor Targets Register
#define ICDICFR_BASEADDR 0xF8F01C00 // Interrupt Configuration Register
#define ICCIAR_BASEADDR 0xF8F0010C // Interrupt Acknowledge Register
#define ICCEOIR_BASEADDR 0xF8F00110 // End Of Interrupt Register
#define GPIO_MTDATA_OUT_0 0xE000A004 // Maskable data out in bank 0
#define GPIO_INT_DIS_0 0xE000A214 // Interrupt disable bank 0
#define GPIO_INT_EN_1 0xE000A250 // Interrupt enable bank 1
#define GPIO_INT_DIS_1 0xE000A254 // Interrupt disable bank 1
#define GPIO_INT_STAT_1 0xE000A258 // Interrupt status bank 1
#define GPIO_INT_TYPE_1 0xE000A25C // Interrupt type bank 1
#define GPIO_INT_POL_1 0xE000A260 // Interrupt polarity bank 1
#define GPIO_INT_ANY_1 0xE000A264 // Interrupt any edge sensitive bank 1
#define GT_COUNTER0_ADDRESS 0xF8F00200
#define GT_COUNTER1_ADDRESS 0xF8F00204
#define GT_CONTROL_ADDRESS 0xF8F00208
#define GT_INTSTAT_ADDRESS 0xF8F0020C
#define GT_COMP0_ADDRESS 0xF8F00210
#define GT_COMP1_ADDRESS 0xF8F00214
#define GT_AUTOINC_ADDRESS 0xF8F00218
#define SVN_SEG_CTRL 0X4BB03000
#define DIG1_ADDRESS (0X4BB03000 + 4)
#define DIG2_ADDRESS (0X4BB03000 + 8)
#define DIG3_ADDRESS (0X4BB03000 + 12)
#define DIG4_ADDRESS (0X4BB03000 + 16)
#define SVN_SEG_DP (0X4BB03000 + 20)
#define MIO_PIN_16 0xF8000740
#define MIO_PIN_17 0xF8000744
#define MIO_PIN_18 0xF8000748
#define MIO_PIN_50 0xF80007C8
#define MIO_PIN_51 0xF80007CC
#define GPIO_DIRM_0 0xE000A204 // Direction mode bank 0
#define GPIO_OUTE_0 0xE000A208 // Output enable bank 0
#define GPIO_DIRM_1 0xE000A244 // Direction mode bank 1

//Global variables
uint8_t D1 =0;
uint8_t D2 =0;
uint8_t D3 =0;
uint8_t D4 =0;
uint8_t START =0;

//function declarations
void Initialize_SVD();
void Configure_IO();
void Initialize_Global_Timer();
void disable_interrupts();
void configure_GIC_GT();
void configure_GIC_GPIO();
void Initialize_GPIO_Interrupts();
void Initialize_GT_Interrupts();
void enable_interrupts();
void IRQ_Handler(void *data);
void MY_GT_IRQ();
void MY_GPIO_IRQ(uint32_t button_press);
void delay(int i);


//defining functions here
void Initialize_SVD()
{
	*((uint32_t*)SVN_SEG_CTRL) = 0x9;
	*((uint32_t*)DIG1_ADDRESS) = 0x0;
	*((uint32_t*)DIG2_ADDRESS) = 0x0;
	*((uint32_t*)DIG3_ADDRESS) = 0x0;
	*((uint32_t*)DIG4_ADDRESS) = 0x0;
	*((uint32_t*)SVN_SEG_DP) = 0x1;
	*((uint32_t*)D1) =0;
	*((uint32_t*)D2) =0;
	*((uint32_t*)D3) =0;
	*((uint32_t*)D4) =0;
	START=0;
	//I think I need to add some code here that
	return;
}


void Configure_IO()
{
	*((uint32_t *) 0xF8000000+0x8/4) = 0x0000DF0D; //Write unlock code to enable writing into system level
		//Control unlock Register
		*((uint32_t*) MIO_PIN_50) = 0x00000600; // BTN4 6th group of four is: 0110
		*((uint32_t*) MIO_PIN_51) = 0x00000600; //BTN5 6th group of four is: 0110
		*((uint32_t*) GPIO_DIRM_0) = 0x00070000; //direction mode for bank 0
		*((uint32_t*) GPIO_OUTE_0) = 0x00070000; //output enable for bank 0
		*((uint32_t*) GPIO_DIRM_1) = 0x00000000; //direction mode for bank 1
		return;

}


void Initialize_Global_Timer()
{
	*((uint32_t*) GT_CONTROL_ADDRESS) = 0x0000; // Turn off the timer and disable the interrupt.
	*((uint32_t*) GT_COUNTER0_ADDRESS) = 0x00000000; // Set the Counter to zero
	*((uint32_t*) GT_COUNTER1_ADDRESS) = 0x00000000;
	*((uint32_t*) GT_INTSTAT_ADDRESS) = 0x00000001; // set the interrupt to be cleared.
	*((uint32_t*) GT_COMP0_ADDRESS) = 3333333; // set the comparator to 1 second under prescale of 1.
	*((uint32_t*) GT_COMP1_ADDRESS) = 0x00000000;
	*((uint32_t*) GT_CONTROL_ADDRESS) = 0x0100; // the prescale of the global timer is 1.
	return;
}


void disable_interrupts()
{

	uint32_t mode = 0xDF; // System mode [4:0] and IRQ disabled [7], D == 1101, F == 1111
	//what does this mean???
	uint32_t read_cpsr=0; // used to read previous CPSR value, read status register values
	uint32_t bit_mask = 0xFF; // used to clear bottom 8 bits
	__asm__ __volatile__("mrs %0, cpsr\n" : "=r" (read_cpsr) ); // execute the assembly instruction MSR
	__asm__ __volatile__("msr cpsr,%0\n" : : "r" ((read_cpsr & (~bit_mask))| mode)); // only change the
	//lower 8 bits
	return;
}


void configure_GIC_GT(){
// GT ID# 27
*((uint32_t*) ICDIPTR_BASEADDR+0x7) = 0x00000000; // remove processors
*((uint32_t*) ICDICER_BASEADDR) = 0x08000000; // clear interrupt ID 27
*((uint32_t*) ICDDCR_BASEADDR) = 0x0; //
*((uint32_t*) ICDIPR_BASEADDR+0x7) = 0x90000000; // Set priority to 9 for ID 27
*((uint32_t*) ICDIPTR_BASEADDR+0x7) = 0x01000000; // Enable interrupt #27 for CPU0
*((uint32_t*) ICDICFR_BASEADDR+0x1) = 0x7DC00000; // ID 27 = edge sensitive only
*((uint32_t*) ICDISER_BASEADDR) = 0x08000000; // Enable ID 27
*((uint32_t*) ICCPMR_BASEADDR) = 0xFF; //
*((uint32_t*) ICCICR_BASEADDR) = 0x3; //
*((uint32_t*) ICDDCR_BASEADDR) = 0x1; //
return;
}


void configure_GIC_GPIO()
{
	*((uint32_t*) ICDIPTR_BASEADDR+13) = 0x00000000;
	*((uint32_t*) ICDICER_BASEADDR+1) = 0x00000000;
	*((uint32_t*) ICDDCR_BASEADDR) = 0x0;
	*((uint32_t*) ICDIPR_BASEADDR+13) = 0x000000A0;
	*((uint32_t*) ICDIPTR_BASEADDR+13) = 0x00000001;
	*((uint32_t*) ICDICFR_BASEADDR+3) = 0x55555555;
	*((uint32_t *) ICDISER_BASEADDR+1) = 0x00100000;
	*((uint32_t*) ICCPMR_BASEADDR) = 0xFF;
	*((uint32_t*) ICCICR_BASEADDR) = 0x3;
	*((uint32_t*) ICDDCR_BASEADDR) = 0x1;
	return;
//add your own code here
	return;
}


void Initialize_GPIO_Interrupts(){
*((uint32_t*) GPIO_INT_DIS_1) = 0xFFFFFFFF;
*((uint32_t*) GPIO_INT_DIS_0) = 0xFFFFFFFF;
*((uint32_t*) GPIO_INT_STAT_1) = 0xFFFFFFFF; // Clear Status register
*((uint32_t*) GPIO_INT_TYPE_1) = 0x0C0000; // Type of interrupt rising edge
*((uint32_t*) GPIO_INT_POL_1) = 0x0C0000; // Polarity of interrupt
*((uint32_t*) GPIO_INT_ANY_1) = 0x000000; // Interrupt any edge sensitivity
*((uint32_t*) GPIO_INT_EN_1) = 0x0C0000; // Enable interrupts in bank 0
return;
}


void Initialize_GT_Interrupts(){
*((uint32_t*) GT_INTSTAT_ADDRESS) = 0x1;
return;
}


void enable_interrupts(){
uint32_t read_cpsr=0; // used to read previous CPSR value
uint32_t mode = 0x5F; // System mode [4:0] and IRQ enabled [7]
uint32_t bit_mask = 0xFF; // used to clear bottom 8 bits
__asm__ __volatile__("mrs %0, cpsr\n" : "=r" (read_cpsr) );
__asm__ __volatile__("msr cpsr,%0\n" : : "r" ((read_cpsr & (~bit_mask))| mode));
return;
}


void IRQ_Handler(void *data)
{
uint32_t interrupt_ID = *((uint32_t*)ICCIAR_BASEADDR);
	if (interrupt_ID == 52) //checking if the interrupt is from the GPIO
	{
	uint32_t GPIO_INT = *((uint32_t*)GPIO_INT_STAT_1);
	uint32_t button_press = 0xC0000 & GPIO_INT;
	MY_GPIO_IRQ( button_press); // Deals with BTNs
	}
	else if (interrupt_ID == 27)
	{
	MY_GT_IRQ(); // updates the seven segment display.
	}
*((uint32_t*)ICCEOIR_BASEADDR) = interrupt_ID; // Clears the GIC flag bit.
}


void MY_GT_IRQ(){
 D4=D4+1;
 if (D4<10)
	 *((uint32_t*) DIG4_ADDRESS) = D4;
 else{
	 D4=0;
	 *((uint32_t*) DIG4_ADDRESS) = D4;
	 D3=D3+1;
	 if (D3<10)
		 *((uint32_t*) DIG3_ADDRESS) = D3;
	 else{
		 D3=0;
		 *((uint32_t*) DIG3_ADDRESS) = D3;
		 D2=D2+1;
		 if (D2<10)
			 *((uint32_t*) DIG2_ADDRESS) = D2;
		 else{
			 D2=0;
			 *((uint32_t*) DIG2_ADDRESS) = D2;
			 D1= D1+1;
			 if (D1<10)
				 *((uint32_t*) DIG1_ADDRESS) = D1;
			 else {
				 D1=0;
				 *((uint32_t*) DIG1_ADDRESS) = D1;
			 }
		 }
	 }
 }
*((uint32_t*) GT_COUNTER0_ADDRESS) = 0x00000000; // reset Counter
*((uint32_t*) GT_COUNTER1_ADDRESS) = 0x00000000;
*((uint32_t*) GT_CONTROL_ADDRESS) = 0x010F; // Start Timer
*((uint32_t*) GT_INTSTAT_ADDRESS) = 0x1; // clear Global Timer Interrupt Flag bit.
}


void MY_GPIO_IRQ(uint32_t button_press){
uint32_t BTN5=0x80000;
uint32_t BTN4=0x40000;
delay(100000); // button rebounce
	if (button_press == BTN5)
	{
		*((uint32_t*) GT_COUNTER0_ADDRESS) = 0x00000000; // Set the Counter to zero
		*((uint32_t*) GT_COUNTER1_ADDRESS) = 0x00000000;
		*((uint32_t*) GT_CONTROL_ADDRESS) = 0x0;
		*((uint32_t*) GT_INTSTAT_ADDRESS) = 0x1;
		//*((uint32_t*) GT_INTSTAT_ADDRESS) = 0x00000001; // set the interrupt to be cleared.
		//*((uint32_t*) GPIO_INT_STAT_1) = 0xFFFFFFFF; // Clear Status register
		//*((uint32_t*) ICDICER_BASEADDR) = 0x08000000; // clear interrupt ID 27
		Initialize_SVD();
	// Enter your code to reset SVD and Timer
	}
	else if (button_press == BTN4)
	{
			if (START == 0){
					START =1;
					*((uint32_t*) GT_CONTROL_ADDRESS) = 0x010F; // Start Timer
					*((uint32_t*) GT_INTSTAT_ADDRESS) = 0x1; // clear Global Timer Interrupt Flag bit.
					// Enter your code to start the timer
				}
			else {
					START=0;
					*((uint32_t*) GT_CONTROL_ADDRESS) = 0x0;
					*((uint32_t*) GT_INTSTAT_ADDRESS) = 0x0;
				}

	}

*((uint32_t*)GPIO_INT_STAT_1) = 0xFFFFFF;

}



void delay(int i)
{
int k = 0;
while (k<i)
k++;
}

