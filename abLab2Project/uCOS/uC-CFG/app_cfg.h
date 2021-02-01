/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                              (c) Copyright 2013; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      APPLICATION CONFIGURATION
*
*                                        Freescale Kinetis K60
*                                               on the
*
*                                        Freescale TWR-K60N512
*                                          Evaluation Board
*
* Filename      : app_cfg.h
* Version       : V1.00
* Programmer(s) : DC
*********************************************************************************************************
*/

#ifndef  APP_CFG_MODULE_PRESENT
#define  APP_CFG_MODULE_PRESENT


/*
*********************************************************************************************************
*                                       ADDITIONAL uC/MODULE ENABLES
*********************************************************************************************************
*/

#define  APP_CFG_SERIAL_EN                          DEF_DISABLED //Change to disabled. TDM


/*
*********************************************************************************************************
*                                            TASK PRIORITIES
*********************************************************************************************************
*/

#define APP_CFG_TASK_START_PRIO               2u        //used by me
#define APP_CFG_KEY_TASK_PRIO                 6u        //used by uCOSKey.c
#define APP_CFG_TIMER_DISPLAY_TASK_PRIO       8u       //used to display timer
#define APP_CFG_LCD_TASK_PRIO                 10u       //used by LcdLayered.c
#define APP_CFG_SWCOUNTER_TASK_PRIO           12u        //used by the counter
#define APP_CFG_TIMER_CONTROL_TASK_PRIO       14u       //used by the timer control task



/*
*********************************************************************************************************
*                                            TASK STACK SIZES
*********************************************************************************************************
*/

#define APP_CFG_TASK_START_STK_SIZE 128u
#define APP_CFG_TASK1_STK_SIZE      128u
#define APP_CFG_TASK2_STK_SIZE      128u
#define APP_CFG_TASK3_STK_SIZE      128u
#define APP_CFG_LCD_TASK_STK_SIZE   128u
#define APP_CFG_KEY_TASK_STK_SIZE   128u

#endif
