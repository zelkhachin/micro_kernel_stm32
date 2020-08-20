/**
  ******************************************************************************
  * @file    main.c
  * @author  zelkhachin
  ******************************************************************************
*/

#include<stdio.h>
#include<stdint.h>
#include"main.h"
#include "led.h"

/* task handler function prototypes */
void task1_handler(void);
void task2_handler(void); 
void task3_handler(void); 
void task4_handler(void); 

void init_systick_timer(uint32_t tick_hz);
__attribute__((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack);
void init_tasks_stack(void);
void enable_processor_faults(void);
__attribute__((naked)) void switch_sp_to_psp(void);
uint32_t get_psp_value(void);

void task_delay(uint32_t tick_count);

/* This variable tracks the current_task being executed on the CPU */
uint8_t current_task = 1; //task1 is running

/* This variable gets updated from systick handler for every systick interrupt */
uint32_t g_tick_count = 0;

/* Each task has its own TCB */
TCB_t user_tasks[MAX_TASKS];

int main(void)
{

	enable_processor_faults();

	init_scheduler_stack(SCHED_STACK_START); 
	
	init_tasks_stack(); 

	led_init_all();

	init_systick_timer(TICK_HZ);

	switch_sp_to_psp();

	task1_handler();

	for(;;);
}

void idle_task(void) 
{
	while(1);
}


void task1_handler(void)
{
	while(1)
	{
		led_on(LED_GREEN);
		task_delay(1000); //Block current task until 1s has elapsed
		led_off(LED_GREEN);
		task_delay(1000);
	}

}

void task2_handler(void)
{
	while(1)
	{
		led_on(LED_ORANGE);
		task_delay(500); //Block current task until 500ms has elapsed
		led_off(LED_ORANGE);
		task_delay(500);
	}	

}

void task3_handler(void)
{
	while(1)
	{
		led_on(LED_BLUE);
		task_delay(250);//Block current task until 250ms has elapsed
		led_off(LED_BLUE);
		task_delay(250);
	}

}

void task4_handler(void)

{
	while(1)
	{
		led_on(LED_RED);
		task_delay(125);//Block current task until 125ms has elapsed
		led_off(LED_RED);
		task_delay(125);
	}


}


void init_systick_timer(uint32_t tick_hz)
{
	uint32_t *pSRVR = (uint32_t*)0xE000E014;
	uint32_t *pSCSR = (uint32_t*)0xE000E010;

    /* calculation of reload value */
    uint32_t count_value = (SYSTICK_TIM_CLK/tick_hz)-1;

    //Clear the value of SVR
    *pSRVR &= ~(0x00FFFFFFFF);

    //load the value in to SVR
    *pSRVR |= count_value;

    //do some settings
    *pSCSR |= ( 1 << 1);  //Enables SysTick exception requestin the NVIC
    *pSCSR |= ( 1 << 2);  //Indicates the clock source, processor clock source

    //enable the systick
    *pSCSR |= ( 1 << 0); //enables the counter

}

/*Initialize Scheduler stack (MSP) to SCHED_STACK_STAR */
__attribute__((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack)
{
     __asm volatile("MSR MSP,%0": :  "r" (sched_top_of_stack)  :   );
     __asm volatile("BX LR");

}



/* this function stores dummy stack contents for each task */

void init_tasks_stack(void)
{

	user_tasks[0].current_state = TASK_READY_STATE;
	user_tasks[1].current_state = TASK_READY_STATE;
	user_tasks[2].current_state = TASK_READY_STATE;
	user_tasks[3].current_state = TASK_READY_STATE;
	user_tasks[4].current_state = TASK_READY_STATE;

	user_tasks[0].psp_value = IDLE_STACK_START;
	user_tasks[1].psp_value = T1_STACK_START;
	user_tasks[2].psp_value = T2_STACK_START;
	user_tasks[3].psp_value = T3_STACK_START;
	user_tasks[4].psp_value = T4_STACK_START;

	user_tasks[0].task_handler = idle_task;
	user_tasks[1].task_handler = task1_handler;
	user_tasks[2].task_handler = task2_handler;
	user_tasks[3].task_handler = task3_handler;
	user_tasks[4].task_handler = task4_handler;


	uint32_t *pPSP;

	for(int i = 0 ; i < MAX_TASKS ;i++)
	{
		pPSP = (uint32_t*) user_tasks[i].psp_value;

		pPSP--;
		*pPSP = DUMMY_XPSR;//0x01000000

		pPSP--; //PC
		*pPSP = (uint32_t) user_tasks[i].task_handler;

		pPSP--; //LR
		*pPSP = 0xFFFFFFFD;

		for(int j = 0 ; j < 13 ; j++)
		{
			pPSP--;
		    *pPSP = 0;

		}

		user_tasks[i].psp_value = (uint32_t)pPSP;


	}

}

void enable_processor_faults(void)
{
	uint32_t *pSHCSR = (uint32_t*)0xE000ED24;

	*pSHCSR |= ( 1 << 16); //mem manage
	*pSHCSR |= ( 1 << 17); //bus fault
	*pSHCSR |= ( 1 << 18); //usage fault
}


uint32_t get_psp_value(void)
{

	return user_tasks[current_task].psp_value;
}


void save_psp_value(uint32_t current_psp_value)
{
	user_tasks[current_task].psp_value = current_psp_value;
}


void update_next_task(void)
{
	int state = TASK_BLOCKED_STATE;

	for(int i= 0 ; i < (MAX_TASKS) ; i++)
	{
		current_task++;
	    current_task %= MAX_TASKS;
		state = user_tasks[current_task].current_state;
		if( (state == TASK_READY_STATE) && (current_task != 0) )
			break;
	}

	if(state != TASK_READY_STATE)
		current_task = 0;
}




__attribute__((naked)) void switch_sp_to_psp(void)
{
    /* initialize the PSP with TASK1 stack start address*/ 

	//get the value of psp of current_task
	__asm volatile ("PUSH {LR}"); //preserve LR which connects back to main()
	__asm volatile ("BL get_psp_value");
	__asm volatile ("MSR PSP,R0"); //initialize psp
	__asm volatile ("POP {LR}");  //pops back LR value

	//change SP to PSP using CONTROL register
	__asm volatile ("MOV R0,#0X02");
	__asm volatile ("MSR CONTROL,R0");
	__asm volatile ("BX LR");
}


void schedule(void)
{
	
	uint32_t *pICSR = (uint32_t*)0xE000ED04;//pend the pendsv exception
	*pICSR |= ( 1 << 28);

}




void task_delay(uint32_t tick_count)
{
	INTERRUPT_DISABLE();// disabling interrupt to prevent shared data problems on the user_task variable


	if(current_task)
	{
	   user_tasks[current_task].block_count = g_tick_count + tick_count;
	   user_tasks[current_task].current_state = TASK_BLOCKED_STATE;
	   schedule();
	}

	INTERRUPT_ENABLE();	//enabling interrupt

}


__attribute__((naked)) void PendSV_Handler(void)
{

	/*Save the context of current task */

	__asm volatile("MRS R0,PSP");			//Get current running task's PSP value
	
	__asm volatile("STMDB R0!,{R4-R11}");	//Using that PSP value store SF2( R4 to R11)

	__asm volatile("PUSH {LR}");


    __asm volatile("BL save_psp_value");	//Save the current value of PSP



	/*Retrieve the context of next task */
    __asm volatile("BL update_next_task");	//Decide next task to run

	__asm volatile ("BL save_psp_value");	//get its past PSP value

	__asm volatile ("LDMIA R0!,{R4-R11}");  //Using that PSP value retrieve SF2(R4 to R11)
	
	__asm volatile("MSR PSP,R0");			//update PSP and exit

	__asm volatile("POP {LR}");

	__asm volatile("BX LR");



}


void update_global_tick_count(void)
{
	g_tick_count++;
}

void unblock_tasks(void)
{
	for(int i = 1 ; i < MAX_TASKS ; i++)
	{
		if(user_tasks[i].current_state != TASK_READY_STATE)
		{
			if(user_tasks[i].block_count == g_tick_count)
			{
				user_tasks[i].current_state = TASK_READY_STATE;
			}
		}

	}

}
 

void  SysTick_Handler(void)
{
    update_global_tick_count();

    unblock_tasks();
	
	
	uint32_t *pICSR = (uint32_t*)0xE000ED04;

	 
    *pICSR |= ( 1 << 28);//pend the pendsv exception
}

void HardFault_Handler(void)
{
	printf("Exception : Hardfault\n");
	while(1);
}


void MemManage_Handler(void)
{
	printf("Exception : MemManage\n");
	while(1);
}

void BusFault_Handler(void)
{
	printf("Exception : BusFault\n");
	while(1);
}
