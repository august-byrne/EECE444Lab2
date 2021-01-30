/*****************************************************************************************
* EECE 444 Lab 2
* This creates a stop watch timer using a uCOS kernel. Pressing the * symbol on the keypad
* starts, pauses, or clears the timer displayed on the LCD.
*
* Base code was a demo program for uCOS-III based on Todd Morton's Programming.
* It tests multitasking, the timer, and task semaphores.
* 01/29/2021 August Byrne
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
#define CTRL_COUNT 0
#define CTRL_WAIT 1
#define CTRL_CLEAR 2

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
				APP_CFG_TASK1_PRIO,
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
				APP_CFG_TASK2_PRIO,
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

static void AppTimerDisplayTask(void *p_arg){
	OS_ERR os_err;
	INT8U *ptr_disp_time;
	INT8U min;
	INT8U sec;
	INT8U milisec;

	(void)*p_arg;
	while(1){
		ptr_disp_time = (INT8U *) SWCountPend(0,&os_err);
		min = ((int)ptr_disp_time/1000)/60;
		sec = ((int)ptr_disp_time/1000)%60;
		milisec = (int)((INT8U) ptr_disp_time);
		LcdDispByte(LCD_ROW_1,LCD_COL_1,LCD_LAYER_TIMER,min);
		LcdDispString(LCD_ROW_1,LCD_COL_2,LCD_LAYER_TIMER,":");
		LcdDispByte(LCD_ROW_1,LCD_COL_3,LCD_LAYER_TIMER,sec);
		LcdDispString(LCD_ROW_1,LCD_COL_4,LCD_LAYER_TIMER,":");
		LcdDispByte(LCD_ROW_1,LCD_COL_5,LCD_LAYER_TIMER,milisec);
	}

}

static void AppTimerControlTask(void *p_arg){
	OS_ERR os_err;
	INT8U kchar;
	CNTR_CTRL_STATE current_state;
	(void)p_arg;

	while(1){
	DB3_TURN_OFF();
	kchar = KeyPend(0, &os_err);
	if (kchar == '*'){
		current_state = SWCounterGet();
		if (current_state == CTRL_CLEAR){
			SWCounterCntrlSet(1,0);
		}else if (current_state == CTRL_WAIT){
			SWCounterCntrlSet(0,1);
		}else if (current_state == CTRL_COUNT){
			SWCounterCntrlSet(0,0);
		}
	}
	DB3_TURN_ON();

	}

}

