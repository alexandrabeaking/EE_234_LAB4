#include "lab5.h"


int main(void)
{
	init_platform();
	Initialize_SVD();
	Configure_IO();
	Initialize_Global_Timer();
	disable_interrupts();
	configure_GIC_GT();
	//configure_GIC_GPIO(); //this one doesn't have any code
	Initialize_GPIO_Interrupts();
	Initialize_GT_Interrupts();
	 enable_interrupts();
	 Xil_ExceptionRegisterHandler(5, IRQ_Handler, NULL);
	 while(1){
	  ;
	  }
	 cleanup_platform();
	return 0;
}
