/*
 * SWCounter.c
 *  This controls all the counting functionality of the stop watch counter
 *  Created on: Jan 29, 2021
 *  Lasted Edited On: Mar 15 2021
 *      Author: August Byrne
 */
#include "app_cfg.h"
#include "os.h"
#include "MCUType.h"
#include "K65TWR_ClkCfg.h"
#include "K65TWR_GPIO.h"
#include "SWCounter.h"

/*****************************************************************************************
* Variable Defines Here
*****************************************************************************************/
#define ENABLE_ON 1
#define ENABLE_OFF 0
#define RESET_ON 1
#define RESET_OFF 0
#define MAX_COUNT  600000 /* max count of the counter (99:59:99) */
/*****************************************************************************************
* Allocate task control blocks
*****************************************************************************************/
static OS_TCB SWCounterTaskTCB;

/*****************************************************************************************
* Allocate task stack space.
*****************************************************************************************/
static CPU_STK SWCounterTaskStartStk[APP_CFG_TASK3_STK_SIZE];

/*****************************************************************************************
* Task Function Prototypes.
*   - Private if in the same module as startup task. Otherwise public.
*****************************************************************************************/
static void SWCounterTask(void *p_arg);

/*****************************************************************************************
 * Mutex & Semaphores
*****************************************************************************************/
typedef struct {
	INT32U count;
	OS_SEM flag;
} SW_COUNT_BUFFER;
static SW_COUNT_BUFFER SWCountBuffer;

static CNTR_CTRL_STATE SWCntrCntrl;
static OS_MUTEX SWCntrCtrlKey;

/*****************************************************************************************
* SWCounter()
*****************************************************************************************/

//SWCounterInit – Executes all required initialization for the resources in SWCounter.c
void SWCounterInit(void){
	OS_ERR os_err;

	OSMutexCreate(&SWCntrCtrlKey, "Counter Control Key", &os_err);
	OSSemCreate(&(SWCountBuffer.flag),"Count Buffer",0,&os_err);
	OSMutexPend(&SWCntrCtrlKey,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
	SWCntrCntrl = CTRL_CLEAR;
	OSMutexPost(&SWCntrCtrlKey,OS_OPT_POST_NONE,&os_err);

	OSTaskCreate(&SWCounterTaskTCB,                  /* Create Task 1                    */
				"SWCounterTask ",
				SWCounterTask,
				(void *) 0,
				APP_CFG_SWCOUNTER_TASK_PRIO,
				&SWCounterTaskStartStk[0],
				(APP_CFG_TASK3_STK_SIZE / 10u),
				APP_CFG_TASK3_STK_SIZE,
				0,
				0,
				(void *) 0,
				(OS_OPT_TASK_NONE),
				&os_err);
}

//SWCountPend
//This uses a synchronous buffer to signal when the count changes, and
//returns the current count
INT32U *SWCountPend(INT16U tout, OS_ERR *ptr_os_err){
	OSSemPend(&(SWCountBuffer.flag),tout,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,ptr_os_err);
	return (&(SWCountBuffer.count));
}

//SWCounterTask
//This is what controls the active counter. It either counts, waits, or clears
//depending on the state of counterState
static void SWCounterTask(void *p_arg){
	OS_ERR os_err;
	INT8U counterState;
	INT32U counter = 0;
	(void)p_arg;

	while(1){
		DB2_TURN_OFF();                             /* Turn off debug bit while waiting */
		OSTimeDly(10,OS_OPT_TIME_PERIODIC,&os_err);     /* Task period = 10ms   */
		DB2_TURN_ON();                          /* Turn on debug bit while ready/running*/
		OSMutexPend(&SWCntrCtrlKey,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
		counterState = SWCntrCntrl;
		OSMutexPost(&SWCntrCtrlKey,OS_OPT_POST_NONE,&os_err);
		switch (counterState){
			case CTRL_COUNT:
				counter ++;
				if (counter >= MAX_COUNT){
					counter = 0;
				}else{}
				SWCountBuffer.count = counter;
				OSSemPost(&(SWCountBuffer.flag),OS_OPT_POST_NONE,&os_err);
			break;
			case CTRL_WAIT:
				//do nothing to the counter
			break;
			case CTRL_CLEAR:
				if (counter != 0){
					counter = 0;
					SWCountBuffer.count = counter;
					OSSemPost(&(SWCountBuffer.flag),OS_OPT_POST_NONE,&os_err);
				}else{}
			break;
			default:
				//do nothing here as well
			break;
		}
	}
}

//SWCounterSet
//Sets the counter control, and does the following:
//If reset is 1 the counter is reset to zero
//If enable is 1 and reset is 0, the counter counts
//If enable is 0 and reset is 0, the counter is held at the current count
void SWCounterCntrlSet(INT8U enable, INT8U reset){
	OS_ERR os_err;

	if (reset == RESET_OFF){
		if(enable == ENABLE_ON){
			OSMutexPend(&SWCntrCtrlKey,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
			SWCntrCntrl = CTRL_COUNT;
			OSMutexPost(&SWCntrCtrlKey,OS_OPT_POST_NONE,&os_err);
		}else if (enable == ENABLE_OFF){
			OSMutexPend(&SWCntrCtrlKey,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
			SWCntrCntrl = CTRL_WAIT;
			OSMutexPost(&SWCntrCtrlKey,OS_OPT_POST_NONE,&os_err);
		}else{}
	}else if (reset == RESET_ON){
		OSMutexPend(&SWCntrCtrlKey,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
		SWCntrCntrl = CTRL_CLEAR;
		OSMutexPost(&SWCntrCtrlKey,OS_OPT_POST_NONE,&os_err);
	}else{}
}
