/*
 * SWCounter.h
 *
 *  Created on: Jan 29, 2021
 *      Author: August
 */

#ifndef SWCOUNTER_H_
#define SWCOUNTER_H_
typedef enum {CTRL_COUNT,CTRL_WAIT,CTRL_CLEAR} CNTR_CTRL_STATE;

void SWCounterInit(void);
INT32U *SWCountPend(INT16U tout, OS_ERR *os_err);
void SWCounterCntrlSet(INT8U enable, INT8U reset);

#endif /* SWCOUNTER_H_ */
