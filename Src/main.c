#include <stdint.h>
#include <stdio.h>
#include <main.h>

#include "led.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

void idle_task(void);
void task1_handler(void);
void task2_handler(void);
void task3_handler(void);
void task4_handler(void);

void enable_processor_faults(void);

__attribute__((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack);

void init_tasks(void);

void init_systick_timer(uint32_t tick_hz);

__attribute__((naked)) void switch_sp_to_psp(void);

void task_delay(uint32_t tick_count);

void schedule(void);


typedef struct {
	uint32_t psp_value;
	uint32_t block_count;
	uint8_t current_state;
	void (*task_handler)(void);
} TCB_t;

TCB_t user_tasks[MAX_TASKS];

uint8_t current_task = 1;
uint32_t g_tick_count = 0;

int main(void)
{
	printf("Get started\n");
	enable_processor_faults();

	init_scheduler_stack(SCHED_STACK_START);

	init_tasks();

	led_init_all();

	init_systick_timer(TICK_HZ);

	switch_sp_to_psp();

	task1_handler();

    /* Loop forever */
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
		task_delay(1000);
		led_off(LED_GREEN);
		task_delay(1000);

	}
}
void task2_handler(void)
{
	while(1)
	{
		led_on(LED_ORANGE);
		task_delay(500);
		led_off(LED_ORANGE);
		task_delay(500);
	}
}
void task3_handler(void)
{
	while(1)
	{
		led_on(LED_RED);
		task_delay(250);
		led_off(LED_RED);
		task_delay(250);
	}
}
void task4_handler(void)
{
	while(1)
	{
		led_on(LED_BLUE);
		task_delay(125);
		led_off(LED_BLUE);
		task_delay(125);
	}
}

void enable_processor_faults(void)
{
	uint32_t* pSHCSR = (uint32_t*)0xE000ED24; // System Handler Control and State Register
	*pSHCSR |= (1 << 16); // MemManage
	*pSHCSR |= (1 << 17); // BusFault
	*pSHCSR |= (1 << 18); // UsageFault
}

__attribute__((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack)
{
	__asm volatile("MSR MSP,%0": : "r"(sched_top_of_stack) : );
	__asm volatile("BX LR");
}


void init_tasks(void)
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


	volatile uint32_t* pPSP;
	for(uint8_t i = 0; i < MAX_TASKS; i++)
	{
		pPSP = (uint32_t*)user_tasks[i].psp_value;

		pPSP--; // XPSR
		*pPSP = DUMMY_XPSR; // 0x01000000

		pPSP--; // PC
		*pPSP = (uint32_t)user_tasks[i].task_handler;

		pPSP--; // LR
		*pPSP = 0xFFFFFFFD;

		for (uint8_t j = 0; j < 13; j++)
		{
			pPSP--;
			*pPSP = 0x00000000;
		}

		user_tasks[i].psp_value = (uint32_t)pPSP;
	}
}


void save_psp_value(uint32_t stack_addr)
{
	user_tasks[current_task].psp_value = stack_addr;
}

void update_next_task()
{
	int state = TASK_BLOCKED_STATE;

	for (int i = 0; i < (MAX_TASKS); i++)
	{
		current_task++;
		current_task %= MAX_TASKS;
		state = user_tasks[current_task].current_state;
		if ((state == TASK_READY_STATE) && (current_task != 0))
			break;
	}
	if (state != TASK_READY_STATE)
		current_task = 0;
}

uint32_t get_psp_value()
{
	return user_tasks[current_task].psp_value;
}

__attribute__((naked)) void switch_sp_to_psp(void)
{
	__asm volatile("PUSH {LR}"); // Preserve LR to return back to main
	__asm volatile("BL get_psp_value");
	__asm volatile("MSR PSP,R0"); // Initialize PSP
	__asm volatile("POP {LR}");

	__asm volatile("MOV R0, #0x02");
	__asm volatile("MSR CONTROL, R0");
	__asm volatile("BX LR");
}

void init_systick_timer(uint32_t tick_hz)
{
	uint32_t* pSCSR = (uint32_t*)0xE000E010; // SysTick Control Status Register
	uint32_t* pSRVR = (uint32_t*)0xE000E014; // SysTick Reload Value Register

	uint32_t count_value = (SYSTICK_TIM_CLK / tick_hz) - 1;

	*pSRVR &= ~(0x00FFFFFFFF);
	*pSRVR |= count_value;

	*pSCSR |= (1 << 1); // Enable SysTick exception request
	*pSCSR |= (1 << 2); // Enable processor clock source
	*pSCSR |= (1 << 0); // Enable the counter
}

void schedule(void)
{
	uint32_t *pICSR = (uint32_t*)0xE000ED04;
	*pICSR |= (1 << 28);
}

void task_delay(uint32_t tick_count)
{
	disable_irq();
	if (current_task) {
		user_tasks[current_task].block_count = g_tick_count + tick_count;
		user_tasks[current_task].current_state = TASK_BLOCKED_STATE;
		schedule();
	}
	enable_irq();
}

void update_global_tick_count()
{
	g_tick_count++;

}

void unblock_tasks()
{
	for(int i = 1; i < MAX_TASKS; i++)
	{
		if (user_tasks[i].current_state != TASK_READY_STATE)
		{
			if (user_tasks[i].block_count == g_tick_count)
			{
				user_tasks[i].current_state = TASK_READY_STATE;
			}
		}
	}
}

void SysTick_Handler(void)
{
	update_global_tick_count();
	unblock_tasks();
	schedule();
}

__attribute((naked)) void PendSV_Handler(void)
{
	/* Save context of the running task */
	/*
	 * The structure of context
	 * Task3 stack area | Task2 stack area                                                                   | Task1 stack area
	 *                  | Saved and restored manually             | Saved and restored automatically         |
	 *                  | R4 | R5 | R6 | R7 | R8 | R9 | R10 | R11 | R0 | R1 | R2 | R3 | R12 | LR | PC | xPSR |
	 *                  | ^                                                                                  |
	 *                  | SP(PSP)                                                                            |
	 */

	// xPSR, PC, LR, R12,R3, R2, R1, R0 are automatically saved. Save the remaining registers.
	// 1. Get the curent task PSP
	__asm volatile("MRS R0, PSP");
	// 2. Store R4-R11
	__asm volatile("STMDB R0!, {R4-R11}");
	// 3. Save the LR before intermediate operations
	__asm volatile("PUSH {LR}");
	// 3. Save the current task PSP (RSP is already in R0)
	__asm volatile("BL save_psp_value");

	/* Retrieve the context of next task */
	// 1. Decide which task will be started
	__asm volatile("BL update_next_task");

	// 2. Get PSP value
	__asm volatile("BL get_psp_value");

	// 3. Use PSP to retrieve R4-R11
	__asm volatile("LDMIA R0!, {R4-R11}");

	// 4. Update PSP
	__asm volatile("MSR PSP, R0");
	// 5. Restore LR and return
	__asm volatile("POP {LR}");
	__asm volatile("BX LR");
}

void HardFault_Handler(void)
{
	printf("HardFault!\n");
	while(1);
}
void MemManage_Handler(void)
{
	printf("MemManage fault!\n");
	while(1);
}
void BusFault_Handler(void)
{
	printf("BusFault!\n");
	while(1);
}
void UsageFault_Handler(void)
{
	printf("UsageFault!\n");
	while(1);
}


