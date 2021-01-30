/*
 * SWCounter.c
 *
 *  Created on: Jan 29, 2021
 *      Author: August Byrne
 */
#include "app_cfg.h"
#include "os.h"
#include "MCUType.h"
#include "K65TWR_ClkCfg.h"
#include "K65TWR_GPIO.h"

/*****************************************************************************************
* Variable Defines Here
*****************************************************************************************/
#define ENABLE_ON 1
#define ENABLE_OFF 0
#define RESET_ON 1
#define RESET_OFF 0
typedef enum {CTRL_COUNT,CTRL_WAIT,CTRL_CLEAR} CNTR_CTRL_STATE;
/*****************************************************************************************
* Allocate task control blocks
*****************************************************************************************/
static OS_TCB SWCounterTaskTCB;
//static OS_TCB AppTask1TCB;
//static OS_TCB AppTask2TCB;

/*****************************************************************************************
* Allocate task stack space.
*****************************************************************************************/
static CPU_STK SWCounterTaskStartStk[APP_CFG_TASK3_STK_SIZE];
//static CPU_STK AppTask1Stk[APP_CFG_TASK1_STK_SIZE];
//static CPU_STK AppTask2Stk[APP_CFG_TASK2_STK_SIZE];

/*****************************************************************************************
* Task Function Prototypes.
*   - Private if in the same module as startup task. Otherwise public.
*****************************************************************************************/
//static void  AppStartTask(void *p_arg);
void  SWCounterInit(void);
INT32U SWCountPend(INT16U tout, OS_ERR *os_err);
void SWCounterCntrlSet(INT8U enable, INT8U reset);
void SWCounterSet(CNTR_CTRL_STATE CntrCtrlState);
CNTR_CTRL_STATE SWCounterGet(void);
void SWCounterTask(void *p_arg);

typedef struct {
	INT8U count[8];
	OS_SEM flag;
} SW_COUNT_BUFFER;
static SW_COUNT_BUFFER SWCountBuffer;



static CNTR_CTRL_STATE SWCntrCntrl;
OS_MUTEX SWCntrCtrlKey;
/*****************************************************************************************
* SWCounter()
*****************************************************************************************/


//SWCounterInit – Executes all required initialization for the resources in SWCounter.c
void SWCounterInit(void){
	OS_ERR os_err;

	SWCounterSet(CTRL_WAIT);
	OSMutexCreate(&SWCntrCtrlKey, "Counter Control Key", &os_err);
	OSSemCreate(&(SWCountBuffer.flag),"Count Buffer",0,&os_err);
	//SWCountBuffer.count = 0;
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

//SWCountPend – Uses a simple synchronous buffer to signal when the count changes and
//returns the current count.
INT32U SWCountPend(INT16U tout, OS_ERR *ptr_os_err){
	OSSemPend(&(SWCountBuffer.flag),tout,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,ptr_os_err);
	return *(&(SWCountBuffer.count[0]));
}



//SWCounterTask
void SWCounterTask(void *p_arg){
	OS_ERR os_err;
	INT8U counterState;
	INT32U counter = 0;
	(void)p_arg;
	while(1){
		DB1_TURN_OFF();                             /* Turn off debug bit while waiting */
		OSTimeDly(10,OS_OPT_TIME_PERIODIC,&os_err);     /* Task period = 10ms   */
		DB1_TURN_ON();                          /* Turn on debug bit while ready/running*/
		counterState = SWCounterGet();
		switch (counterState){
			case CTRL_COUNT:
				counter ++;
				if (counter >= 3599994){
					counter = 0;
				}
				OSSemPend(&(SWCountBuffer.flag),0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
				SWCountBuffer.count[0] = counter;
				OSSemPost(&(SWCountBuffer.flag),OS_OPT_POST_NONE,&os_err);
				//OSMutexPost(&(SWCountBuffer).flag,OS_OPT_POST_NONE,&os_err);
			break;
			case CTRL_WAIT:
				//do nothing to the counter
			break;
			case CTRL_CLEAR:
				counter = 0;
				OSSemPend(&(SWCountBuffer.flag),0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
				SWCountBuffer.count[0] = counter;
				OSSemPost(&(SWCountBuffer.flag),OS_OPT_POST_NONE,&os_err);
			break;
			default:
				//do nothing here as well
			break;
		}
	}
}

//SWCounterSet - Sets the counter control. Shall have the following function:
//If reset is 1 the counter shall be reset to zero
//If enable is 1 and reset is 0, the counter shall count
//If enable is 0 and reset is 0, the counter shall be held at the current count
void SWCounterCntrlSet(INT8U enable, INT8U reset){
	if (reset == RESET_OFF){
		if(enable == ENABLE_ON){
			SWCounterSet(CTRL_COUNT);
		}else if (enable == ENABLE_OFF){
			SWCounterSet(CTRL_WAIT);
		}else{}
	}else if (reset == RESET_ON){
		SWCounterSet(CTRL_CLEAR);
	}else{}
}

void SWCounterSet(CNTR_CTRL_STATE CntrCtrlState){
	OS_ERR os_err;
	OSMutexPend(&SWCntrCtrlKey,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
	SWCntrCntrl = CntrCtrlState;
	OSMutexPost(&SWCntrCtrlKey,OS_OPT_POST_NONE,&os_err);
}

CNTR_CTRL_STATE SWCounterGet(void){
	OS_ERR os_err;
	CNTR_CTRL_STATE lstate;
	OSMutexPend(&SWCntrCtrlKey,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
	lstate = SWCntrCntrl;
	OSMutexPost(&SWCntrCtrlKey,OS_OPT_POST_NONE,&os_err);
	return lstate;
}

