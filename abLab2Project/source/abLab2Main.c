/*****************************************************************************************
* EECE 444 Lab 2
* This creates a stop watch timer using a uCOS kernel. Pressing the * symbol on the keypad
* starts, pauses, or clears the timer displayed on the LCD. Pressing the # symbol sets the
* lap function the record the current time on the second row of the LCD.
*
* Base code was a demo program for uCOS-III based on Todd Morton's Programming.
* It tests multitasking, the timer, and task semaphores.
* Created On: 01/29/2021
* Last Edited On: 03/16/2021
*       Author: August Byrne
*****************************************************************************************/
#include "app_cfg.h"
#include "os.h"
#include "MCUType.h"
#include "K65TWR_ClkCfg.h"
#include "K65TWR_GPIO.h"
#include "MemTest.h"
#include "LcdLayered.h"
#include "uCOSKey.h"
#include "SWCounter.h"

/*****************************************************************************************
* Variable Defines Here
*****************************************************************************************/
#define LOWADDR (INT32U) 0x00000000		//low memory address
#define HIGHADRR (INT32U) 0x001FFFFF		//high memory address

/*****************************************************************************************
* Allocate task control blocks
*****************************************************************************************/
static OS_TCB AppTaskStartTCB;
static OS_TCB AppTimerDisplayTaskTCB;
static OS_TCB AppTimerControlTaskTCB;

/*****************************************************************************************
* Allocate task stack space.
*****************************************************************************************/
static CPU_STK AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static CPU_STK AppTimerDisplayTaskStk[APP_CFG_TASK1_STK_SIZE];
static CPU_STK AppTimerControlTaskStk[APP_CFG_TASK2_STK_SIZE];

/*****************************************************************************************
* Task Function Prototypes. 
*   - Private if in the same module as startup task. Otherwise public.
*****************************************************************************************/
static void AppStartTask(void *p_arg);
static void AppTimerDisplayTask(void *p_arg);
static void AppTimerControlTask(void *p_arg);

/*****************************************************************************************
 * Mutex & Semaphores
*****************************************************************************************/
static OS_MUTEX appTimerCntrKey;
static INT8U appTimerCount[3];  /* accessed through the mutex. contains min, sec, & centisec */

/*****************************************************************************************
* main()
*****************************************************************************************/
void main(void) {
	OS_ERR  os_err;

	K65TWR_BootClock();
	CPU_IntDis();               /* Disable all interrupts, OS will enable them  */

	OSInit(&os_err);                    /* Initialize uC/OS-III                         */

	OSTaskCreate(&AppTaskStartTCB,                  /* Address of TCB assigned to task */
				 "Start Task",                      /* Name you want to give the task */
				 AppStartTask,                      /* Address of the task itself */
				 (void *) 0,                        /* p_arg is not used so null ptr */
				 APP_CFG_TASK_START_PRIO,           /* Priority you assign to the task */
				 &AppTaskStartStk[0],               /* Base address of task�s stack */
				 (APP_CFG_TASK_START_STK_SIZE/10u), /* Watermark limit for stack growth */
				 APP_CFG_TASK_START_STK_SIZE,       /* Stack size */
				 0,                                 /* Size of task message queue */
				 0,                                 /* Time quanta for round robin */
				 (void *) 0,                        /* Extension pointer is not used */
				 (OS_OPT_TASK_NONE), /* Options */
				 &os_err);                          /* Ptr to error code destination */

	OSStart(&os_err);               /*Start multitasking(i.e. give control to uC/OS)    */
}

/*****************************************************************************************
* STARTUP TASK
* This should run once and be suspended. Could restart everything by resuming.
* (Resuming not tested)
* Todd Morton, 01/06/2016
*****************************************************************************************/
static void AppStartTask(void *p_arg) {
	INT16U math_val = 0;
	OS_ERR os_err;

	(void)p_arg;                        /* Avoid compiler warning for unused variable   */

	OS_CPU_SysTickInitFreq(SYSTEM_CLOCK);
	/* Initialize StatTask. This must be called when there is only one task running.
	 * Therefore, any function call that creates a new task must come after this line.
	 * Or, alternatively, you can comment out this line, or remove it. If you do, you
	 * will not have accurate CPU load information                                       */
	//    OSStatTaskCPUUsageInit(&os_err);
	GpioDBugBitsInit();
	LcdInit();
	KeyInit();
	SWCounterInit();

	//Initial program checksum, which is displayed on the second row of the LCD
	math_val = CalcChkSum((INT8U *)LOWADDR,(INT8U *)HIGHADRR);
	LcdDispString(LCD_ROW_2,LCD_COL_1,LCD_LAYER_CHKSM,"CS: ");
	LcdDispByte(LCD_ROW_2,LCD_COL_4,LCD_LAYER_CHKSM,(INT8U)math_val);
	LcdDispByte(LCD_ROW_2,LCD_COL_6,LCD_LAYER_CHKSM,(INT8U)(math_val << 8));	//display first byte then <<8 and display next byte

	OSTaskCreate(&AppTimerDisplayTaskTCB,                  /* Create Task 1                    */
				"AppTimerDisplayTask ",
				AppTimerDisplayTask,
				(void *) 0,
				APP_CFG_TIMER_DISPLAY_TASK_PRIO,
				&AppTimerDisplayTaskStk[0],
				(APP_CFG_TASK1_STK_SIZE / 10u),
				APP_CFG_TASK1_STK_SIZE,
				0,
				0,
				(void *) 0,
				(OS_OPT_TASK_NONE),
				&os_err);

	OSTaskCreate(&AppTimerControlTaskTCB,    /* Create Task 2                    */
				"AppTimerControlTask ",
				AppTimerControlTask,
				(void *) 0,
				APP_CFG_TIMER_CONTROL_TASK_PRIO,
				&AppTimerControlTaskStk[0],
				(APP_CFG_TASK2_STK_SIZE / 10u),
				APP_CFG_TASK2_STK_SIZE,
				0,
				0,
				(void *) 0,
				(OS_OPT_TASK_NONE),
				&os_err);
	OSTaskDel((OS_TCB *)0, &os_err);
}

//AppTimerDisplayTask
//This controls how the timer is displayed on the LCD
static void AppTimerDisplayTask(void *p_arg){
	OS_ERR os_err;
	INT32U *ptr_disp_time;
	INT8U min;
	INT8U sec;
	INT8U centisec;
	(void)p_arg;

	while(1){
		DB1_TURN_OFF();
		ptr_disp_time = SWCountPend(0,&os_err);
		DB1_TURN_ON();
		centisec = ((*ptr_disp_time)%100);
		sec = ((*ptr_disp_time-centisec)/100)%60;
		min = ((*ptr_disp_time-centisec)/100-sec)/60;
		LcdDispTime(LCD_ROW_1,LCD_COL_1,LCD_LAYER_TIMER,min,sec,centisec);

		OSMutexPend(&appTimerCntrKey, 0, OS_OPT_PEND_BLOCKING, (CPU_TS *)0, &os_err);
		appTimerCount[0] = centisec;
		appTimerCount[1] = sec;
		appTimerCount[2] = min;
		OSMutexPost(&appTimerCntrKey, OS_OPT_POST_NONE, &os_err);
	}
}

//AppTimerControlTask
//This controls the actions that happen when you press a button
//on the keypad
static void AppTimerControlTask(void *p_arg){
	OS_ERR os_err;
	INT8U kchar = 0;
	CNTR_CTRL_STATE current_state = CTRL_WAIT;
	(void)p_arg;

	while(1){
		DB0_TURN_OFF();
		kchar = KeyPend(0, &os_err);
		DB0_TURN_ON();
		if (kchar == '*'){
			if (current_state == CTRL_CLEAR){
				current_state = CTRL_COUNT;
				SWCounterCntrlSet(1,0);
			}else if (current_state == CTRL_WAIT){
				current_state = CTRL_CLEAR;
				SWCounterCntrlSet(0,1);
			}else if (current_state == CTRL_COUNT){
				current_state = CTRL_WAIT;
				SWCounterCntrlSet(0,0);
			}else{}
		}else if(kchar == '#'){
			OSMutexPend(&appTimerCntrKey, 0, OS_OPT_PEND_BLOCKING, (CPU_TS *)0, &os_err);
			LcdDispTime(LCD_ROW_2, LCD_COL_1, LCD_LAYER_LAP,appTimerCount[2],appTimerCount[1],appTimerCount[0]);
			OSMutexPost(&appTimerCntrKey, OS_OPT_POST_NONE, &os_err);
		}else{}
	}
}

