#include <stdint.h>
#include <stdio.h>
#include <main.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

void task1_handler(void);
void task2_handler(void);
void task3_handler(void);
void task4_handler(void);

void enable_processor_faults(void);
__attribute__((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack);
void init_tasks_stack(void);
void init_systick_timer(uint32_t tick_hz);
__attribute__((naked)) void switch_sp_to_psp(void);


uint32_t psp_of_tasks[MAX_TASKS] = {
		T1_STACK_START,
		T2_STACK_START,
		T3_STACK_START,
		T4_STACK_START
};
uint32_t task_handlers[MAX_TASKS];
uint8_t current_task = 0;

int main(void)
{
	printf("Get started\n");
	enable_processor_faults();

	init_scheduler_stack(SCHED_STACK_START);

	task_handlers[0] = (uint32_t)task1_handler;
	task_handlers[1] = (uint32_t)task2_handler;
	task_handlers[2] = (uint32_t)task3_handler;
	task_handlers[3] = (uint32_t)task4_handler;

	init_tasks_stack();

	init_systick_timer(TICK_HZ);

	switch_sp_to_psp();

	task1_handler();

    /* Loop forever */
	for(;;);
}

void task1_handler(void)
{
	while(1)
	{
		printf("This is task1\n");
	}
}
void task2_handler(void)
{
	while(1)
	{
		printf("This is task2\n");
	}
}
void task3_handler(void)
{
	while(1)
	{
		printf("This is task3\n");
	}
}
void task4_handler(void)
{
	while(1)
	{
		printf("This is task4\n");
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

void init_tasks_stack(void)
{
	volatile uint32_t* pPSP;
	for(uint8_t i = 0; i < MAX_TASKS; i++)
	{
		pPSP = (uint32_t*)psp_of_tasks[i];

		pPSP--; // XPSR
		*pPSP = DUMMY_XPSR; // 0x01000000

		pPSP--; // PC
		*pPSP = task_handlers[i];

		pPSP--; // LR
		*pPSP = 0xFFFFFFFD;

		for (uint8_t j = 0; j < 13; j++)
		{
			pPSP--;
			*pPSP = 0x00000000;
		}

		psp_of_tasks[i] = (uint32_t)pPSP;
	}
}


void save_psp_value(uint32_t stack_addr)
{
	psp_of_tasks[current_task] = stack_addr;
}

void update_next_task()
{
	current_task++;
	current_task %= MAX_TASKS;
}

uint32_t get_psp_value()
{
	return psp_of_tasks[current_task];
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

__attribute((naked)) void SysTick_Handler(void)
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


