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
* Allocate task control blocks
*****************************************************************************************/
//static OS_TCB AppTaskStartTCB;
//static OS_TCB AppTask1TCB;
//static OS_TCB AppTask2TCB;

/*****************************************************************************************
* Allocate task stack space.
*****************************************************************************************/
//static CPU_STK AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];
//static CPU_STK AppTask1Stk[APP_CFG_TASK1_STK_SIZE];
//static CPU_STK AppTask2Stk[APP_CFG_TASK2_STK_SIZE];

/*****************************************************************************************
* Task Function Prototypes.
*   - Private if in the same module as startup task. Otherwise public.
*****************************************************************************************/
//static void  AppStartTask(void *p_arg);
void  SWCounterInit(void *p_arg);
INT32U SWCountPend(INT16U tout, OS_ERR *os_err);
void SWCounterCntrlSet(INT8U enable, INT8U reset);
void SWCounterSet(CNTR_CTRL_STATE CntrCtrlState);
CNTR_CTRL_STATE SWCounterGet(void);

typedef struct {
	INT8U count[8];
	OS_SEM flag;
} SW_COUNT_BUFFER;
static SW_COUNT_BUFFER SWCountBuffer;

static CNTR_CTRL_STATE SWCntrCtrl;
typedef enum {CTRL_COUNT,CTRL_WAIT,CTRL_CLEAR} CNTR_CTRL_STATE;
OS_MUTEX SWCntrCtrlKey;
/*****************************************************************************************
* SWCounter()
*****************************************************************************************/


//SWCounterInit – Executes all required initialization for the resources in SWCounter.c
void SWCounterInit(void){




	SWCounterSet(CTRL_WAIT);
	OSMutexCreate(&SWCntrCtrlKey, "Counter Control Key", &os_err);
	OSSemCreate(&(SWCountBuffer.flag),"Count Buffer",0,&os_err);
	//SWCountBuffer.count = 0;


}

//SWCountPend – Uses a simple synchronous buffer to signal when the count changes and
//returns the current count.
INT32U SWCountPend(INT16U tout, OS_ERR *ptr_os_err){
	OSSemPend(&(SWCountBuffer.flag),tout,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,ptr_os_err);

	//return THING
}

//SWCounterSet - Sets the counter control. Shall have the following function:
//If reset is 1 the counter shall be reset to zero
//If enable is 1 and reset is 0, the counter shall count
//If enable is 0 and reset is 0, the counter shall be held at the current count
void SWCounterCntrlSet(INT8U enable, INT8U reset){
	if (reset == RESET_OFF){
		if(enable == ENABLE_ON){
			SWCounterSet(CTRL_COUNT);
		}else{
			SWCounterSet(CTRL_WAIT);
		}
	}else{
		SWCounterSet(CTRL_CLEAR);
	}
}

void SWCounterSet(CNTR_CTRL_STATE CntrCtrlState){
	OSMutexPend(&SWCntrCtrlKey,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
	SWCntrCntrl = CntrCtrlState;
	OSMutexPost(&SWCntrCtrlKey,0,OS_OPT_POST_NONE,&os_err);
}

CNTR_CTRL_STATE SWCounterGet(void){
	CNTR_CTRL_STATE lstate;
	OSMutexPend(&SWCntrCtrlKey,0,OS_OPT_PEND_BLOCKING,(CPU_TS *)0,&os_err);
	lstate = SWCntrCntrl;
	OSMutexPost(&SWCntrCtrlKey,0,OS_OPT_POST_NONE,&os_err);
	return lstate;
}








