
/*
*********************************************************************************************************
*                                            START
*********************************************************************************************************
*/
#define INIT_GLONALS_DATA

#include "includes.h"
#include "ConfigMode.h"
#include "ConfigFX.h"

#include "HeadFX.h"

/*
*********************************************************************************************************
*                                   LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
// 堆栈
static  OS_STK App_TaskStartStk[APP_TASK_START_STK_SIZE];
static  OS_STK App_TaskDisplayStk[APP_TASK_DISPLAY_STK_SIZE];
static  OS_STK App_TaskDealPICStk[APP_TASK_DEALPIC_STK_SIZE];   
static  OS_STK App_TaskProtocolStk[APP_TASK_PROTOCOL_STK_SIZE];
static  OS_STK App_TaskReplyHostStk[APP_TASK_REPLYHOST_STK_SIZE];

#if (USE_SHELL > 0)
static  OS_STK App_TaskShellStk[APP_TASK_SHELL_STK_SIZE];  
#endif

#if (USE_USB > 0)
static  OS_STK App_TaskDealUSBStk[APP_TASK_DEALUSB_STK_SIZE];
#endif

#if (USE_THN > 0)
static  OS_STK App_TaskDealTHNStk[APP_TASK_DEALTHN_STK_SIZE];
#endif

#if (USE_LOOP > 0)
static  OS_STK App_TaskLoopStk[APP_TASK_LOOP_STK_SIZE];
#endif


// 内存块
INT8U   MemPart1[MEM1_NUM][MEM1_SIZE];
INT8U   MemPart2[MEM2_NUM][MEM2_SIZE];
//INT8U   MemPart3[MEM2_NUM][MEM2_SIZE];
/*
*********************************************************************************************************
*                                      LOCAL TASK PROTOTYPES
*********************************************************************************************************
*/
static  void App_TaskStart(void* p_arg);
/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static  void AppTaskCreate(void);
/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Argument : none.
*
* Return   : none.
*********************************************************************************************************
*/

int main(void)
{
   UINT8 os_err;
   (void)os_err;

   UserTimeDelay(500);          // 上电延时

   BSP_Init();                  // Initialize BSP functions.
   OSInit();                    // Initialize "uC/OS-II, The Real-Time Kernel".
   MemBuf1 = OSMemCreate(MemPart1,MEM1_NUM,MEM1_SIZE,&UselessErr);    // 分配内存块
   MemBuf2 = OSMemCreate(MemPart2,MEM2_NUM,MEM2_SIZE,&UselessErr);    // 分配内存块
   //MemBuf3 = OSMemCreate(MemPart3,MEM3_NUM,MEM3_SIZE,&UselessErr);    // 分配内存块
   
   InitSTM32();                 // STM32外设初始化

   os_err = OSTaskCreateExt((void (*) (void *)) App_TaskStart, 
                            (void *)0, 
                            (OS_STK *)&App_TaskStartStk[APP_TASK_START_STK_SIZE - 1],
                            APP_TASK_START_PRIO,
                            APP_TASK_START_PRIO,
                            (OS_STK *)&App_TaskStartStk[0],
                            APP_TASK_START_STK_SIZE,
                            (void *)0,
                            OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); 
 
#if (OS_TASK_NAME_SIZE >= 11)
   OSTaskNameSet(APP_TASK_START_PRIO, (UINT8 *) "Start Task", &os_err);
#endif

   OSTimeSet(0);               // 时钟滴答 清零

   OSStart();                  // Start multitasking (i.e. give control to uC/OS-II).

   return (0);
}


/*
*********************************************************************************************************
*                                          App_TaskStart()
*
* Description : The startup task.  The uC/OS-II ticker should only be initialize once multitasking starts.
*
* Argument : p_arg       Argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
*
* Return   : none.
*
* Caller   : This is a task.
*
* Note     : none.
*********************************************************************************************************
*/
static  void App_TaskStart(void* p_arg)
{
    (void) p_arg;
    OS_CPU_SysTickInit(); // Initialize the SysTick,interrupt every 1ms.

#if (OS_TASK_STAT_EN > 0)
    OSStatInit();         // 建立任务统计任务  
#endif
    
    AppTaskCreate();      //创建任务不会切换至创建的任务，因为TaskStart是最高优先级任务
    
    USART1Send("__InitOK__\r\n", 0);

    //------------------------------------------------------------------//
    for(;;)
    {
        //OSTaskSuspend(OS_PRIO_SELF);
        OSTaskDel(OS_PRIO_SELF);
    }
}

/*
*********************************************************************************************************
*                                         AppTaskCreate()
*********************************************************************************************************
*/
static  void AppTaskCreate(void)
{
    UINT8 os_err;
    (void)os_err;

    // 建立处理图片任务
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskDealPIC,            // 任务函数
        (void *) 0,                                     // 传递参数
        (OS_STK *) &App_TaskDealPICStk[APP_TASK_DEALPIC_STK_SIZE - 1], // 堆栈顶指针
        (INT8U) APP_TASK_DEALPIC_PRIO,                  // 任务优先级
        (INT8U) APP_TASK_DEALPIC_PRIO,                  // 任务ID，暂时无用
        (OS_STK *)&App_TaskDealPICStk[0],               // 堆栈低指针，用于测量任务堆栈大小
        (INT32U)APP_TASK_DEALPIC_STK_SIZE,              // 堆栈尺寸大小，按照位宽32计算
        (void *)0,                                      // 扩展OS_TCB，添加用户自己信息
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // 允许检验堆栈，且把堆栈清为0


    // 建立协议处理任务
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskDisplay,            // 任务函数                        
        (void *) 0,                                     // 传递参数  
        (OS_STK *) &App_TaskDisplayStk[APP_TASK_DISPLAY_STK_SIZE - 1], // 堆栈顶指针
        (INT8U) APP_TASK_DISPLAY_PRIO,                  // 任务优先级
        (INT8U) APP_TASK_DISPLAY_PRIO,                  // 任务ID，暂时无用
        (OS_STK *)&App_TaskDisplayStk[0],               // 堆栈低指针，用于测量任务堆栈大小
        (INT32U)APP_TASK_DISPLAY_STK_SIZE,              // 堆栈尺寸大小，按照位宽32计算
        (void *)0,                                      // 扩展OS_TCB，添加用户自己信息
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // 允许检验堆栈，且把堆栈清为0
    

    // 建立串口协议任务
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskProtocol,            // 任务函数                        
        (void *) 0,                                      // 传递参数  
        (OS_STK *) &App_TaskProtocolStk[APP_TASK_PROTOCOL_STK_SIZE - 1], // 堆栈顶指针
        (INT8U) APP_TASK_PROTOCOL_PRIO,                  // 任务优先级
        (INT8U) APP_TASK_PROTOCOL_PRIO,                  // 任务ID，暂时无用
        (OS_STK *)&App_TaskProtocolStk[0],               // 堆栈低指针，用于测量任务堆栈大小
        (INT32U)APP_TASK_PROTOCOL_STK_SIZE,              // 堆栈尺寸大小，按照位宽32计算
        (void *)0,                                       // 扩展OS_TCB，添加用户自己信息
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // 允许检验堆栈，且把堆栈清为0

    
    // 建立回复任务
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskReplyHost,           // 任务函数                        
        (void *) 0,                                      // 传递参数  
        (OS_STK *) &App_TaskReplyHostStk[APP_TASK_REPLYHOST_STK_SIZE - 1], // 堆栈顶指针
        (INT8U) APP_TASK_REPLYHOST_PRIO,                 // 任务优先级
        (INT8U) APP_TASK_REPLYHOST_PRIO,                 // 任务ID，暂时无用
        (OS_STK *)&App_TaskReplyHostStk[0],              // 堆栈低指针，用于测量任务堆栈大小
        (INT32U)APP_TASK_REPLYHOST_STK_SIZE,             // 堆栈尺寸大小，按照位宽32计算
        (void *)0,                                       // 扩展OS_TCB，添加用户自己信息
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // 允许检验堆栈，且把堆栈清为0
        
#if (USE_SHELL > 0)
    OSTaskCreateExt(
        (void (*) (void *)) App_TaskShell,               // 任务函数
        (void *) 0,                                      // 传递参数
        (OS_STK *) &App_TaskShellStk[APP_TASK_SHELL_STK_SIZE - 1], // 堆栈顶指针
        (INT8U) APP_TASK_SHELL_PRIO,                     // 任务优先级
        (INT8U) APP_TASK_SHELL_PRIO,                     // 任务ID，暂时无用
        (OS_STK *)&App_TaskShellStk[0],                  // 堆栈低指针，用于测量任务堆栈大小
        (INT32U)APP_TASK_SHELL_STK_SIZE,                 // 堆栈尺寸大小，按照位宽32计算
        (void *)0,                                       // 扩展OS_TCB，添加用户自己信息
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // 允许检验堆栈，且把堆栈清为0

        // 建立Shell任务邮箱
        MqeueShell = OSQCreate(QeueShell, 20);
#endif

#if (USE_USB > 0)
    // 建立THN任务
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskDealUSB,             // 任务函数                        
        (void *) 0,                                      // 传递参数  
        (OS_STK *) &App_TaskDealUSBStk[APP_TASK_DEALUSB_STK_SIZE - 1], // 堆栈顶指针
        (INT8U) APP_TASK_DEALUSB_PRIO,                   // 任务优先级
        (INT8U) APP_TASK_DEALUSB_PRIO,                   // 任务ID，暂时无用
        (OS_STK *)&App_TaskDealUSBStk[0],                // 堆栈低指针，用于测量任务堆栈大小
        (INT32U)APP_TASK_DEALUSB_STK_SIZE,               // 堆栈尺寸大小，按照位宽32计算
        (void *)0,                                       // 扩展OS_TCB，添加用户自己信息
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // 允许检验堆栈，且把堆栈清为0
#endif

#if (USE_THN > 0)
    // 建立THN任务
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskDealTHN,             // 任务函数                        
        (void *) 0,                                      // 传递参数  
        (OS_STK *) &App_TaskDealTHNStk[APP_TASK_DEALTHN_STK_SIZE - 1], // 堆栈顶指针
        (INT8U) APP_TASK_DEALTHN_PRIO,                   // 任务优先级
        (INT8U) APP_TASK_DEALTHN_PRIO,                   // 任务ID，暂时无用
        (OS_STK *)&App_TaskDealTHNStk[0],                // 堆栈低指针，用于测量任务堆栈大小
        (INT32U)APP_TASK_DEALTHN_STK_SIZE,               // 堆栈尺寸大小，按照位宽32计算
        (void *)0,                                       // 扩展OS_TCB，添加用户自己信息
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // 允许检验堆栈，且把堆栈清为0
#endif

#if (USE_LOOP > 0)
    // 建立循环任务(ADC)
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskLoop,                // 任务函数                        
        (void *) 0,                                      // 传递参数  
        (OS_STK *) &App_TaskLoopStk[APP_TASK_LOOP_STK_SIZE - 1], // 堆栈顶指针
        (INT8U) APP_TASK_LOOP_PRIO,                      // 任务优先级
        (INT8U) APP_TASK_LOOP_PRIO,                      // 任务ID，暂时无用
        (OS_STK *)&App_TaskLoopStk[0],                   // 堆栈低指针，用于测量任务堆栈大小
        (INT32U)APP_TASK_LOOP_STK_SIZE,                  // 堆栈尺寸大小，按照位宽32计算
        (void *)0,                                       // 扩展OS_TCB，添加用户自己信息
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // 允许检验堆栈，且把堆栈清为0
#endif

    // 建立显示信号量
    SemDisplay = OSSemCreate(0);
    // 建立使用串口信号量
    MboxDealProtocol = OSMboxCreate(NULL);
    // 建立回复信息邮箱
    MboxReplyHost = OSMboxCreate(NULL);
    // 建立THN任务邮箱
#if (USE_THN > 0)
    MboxDealTHN = OSMboxCreate(NULL);
#endif
}

/*
*********************************************************************************************************
*                                          中断函数
*
* Description : 系统时钟中断.
*
* Argument : none.
*
* Return   : none.
*
* Caller   : none.
*
* Note     : none.
*********************************************************************************************************
*/
void SysTickHandler(void)
{
    OS_CPU_SR  cpu_sr;

    OS_ENTER_CRITICAL();  //保存全局中断标志,关总中断/* Tell uC/OS-II that we are starting an ISR*/
    OSIntNesting++;
    OS_EXIT_CRITICAL();   //恢复全局中断标志

    OSTimeTick();         /* Call uC/OS-II's OSTimeTick(),在os_core.c文件里定义,主要判断延时的任务是否计时到*/

    OSIntExit();          //在os_core.c文件里定义,如果有更高优先级的任务就绪了,则执行一次任务切换            
}


/*
*********************************************************************************************************
*                                  串口1读中断     USART1_IRQHandler()
*********************************************************************************************************
*/
void USART1_IRQHandler(void)
{
    OS_CPU_SR  cpu_sr;

    OSIntEnter();

    if(USART_GetITStatus(USART1, USART_IT_ORE) != RESET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_ORE);
        USART_ReceiveData(USART1);
    }
    
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) 
    {
        USART_ClearITPendingBit(USART1,USART_IT_RXNE);
        gpFUAData->RcvData = USART_ReceiveData(USART1);

#if (USE_SHELL > 0)
        OSQPost(MqeueShell, (void*)gpFUAData->RcvData);
#endif 
			
        if(gpFUAData->State == FUAPROTOCOL_WAIT)
        {
            if(gpFUAData->RcvData == PFUAHEAD)
            {
                gpGlobalData->ProtocolType = PFUA;
            }
            else if(gpFUAData->RcvData == PUSERHEAD)
            {
                gpGlobalData->ProtocolType = PUSER;
            }
            else
            {
                gpGlobalData->ProtocolType = PNONE;
            }
        }

        if(gpGlobalData->ProtocolType == PFUA)
        {
            if(DealFUA_ReciveProtocol() == COMPLETE)
            {
                gpFUAData->State = FUAPROTOCOL_WAIT;
                OS_ENTER_CRITICAL();
                MboxDealProtocol->OSEventCnt = 0;
                OS_EXIT_CRITICAL();
                OSMboxPost(MboxDealProtocol, (void*)PFUA);
            }
        }
        else if(gpGlobalData->ProtocolType == PUSER)
        {
            if(DealUser_ReciveProtocol() == COMPLETE)
            {
                gpFUAData->State = FUAPROTOCOL_WAIT;
                OS_ENTER_CRITICAL();
                MboxDealProtocol->OSEventCnt = 0;
                OS_EXIT_CRITICAL();
                OSMboxPost(MboxDealProtocol, (void*)PUSER);
            }
        }
    }

    OSIntExit();
}

/*
*********************************************************************************************************
*                                  定时器2中断     TIM2_IRQHandler()
*********************************************************************************************************
*/
void TIM5_IRQHandler(void)
{
    //除了时钟滴答，此中断是最高优先级，不会有数据被重入
    OSIntEnter();

    if(TIM_GetITStatus(TIM5, TIM_IT_Update) == SET)//检查指定的TIM中断发生与否
    {
        TIM_ClearITPendingBit(TIM5, TIM_IT_Update); //清中断标志 

        { // 延时计数++
            UINT16 i;
            // 图片内容等待时间计数
            for(i=0; i<gpConfigInfo->RegionTotal; i++)
            {
                gpRegionData[i].DelayCount++;
                gpGlobalData->TimeCount[i]++;
            }
            // FUA协议超时计数
            if((++gpFUAData->OverTime) > FUA_WAITSEC)  // 4秒后协议状态变为等待
            {
                gpFUAData->OverTime = 0;
                gpFUAData->State = FUAPROTOCOL_WAIT;
            }
        }
    }

    OSIntExit();
}
/*
*********************************************************************************************************
*                                  定时器3中断     TIM3_IRQHandler()
*********************************************************************************************************
*/
void TIM3_IRQHandler(void)
{
    OS_CPU_SR  cpu_sr;

    OSIntEnter();

    if(TIM_GetITStatus(TIM3, TIM_IT_CC1) == SET)//检查指定的TIM中断发生与否
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);//清中断标志 
        TIM3Peripheral(DISABLE);

        OS_ENTER_CRITICAL();
        SemDisplay->OSEventCnt = 0;
        OS_EXIT_CRITICAL();
        OSSemPost(SemDisplay);
    }

    OSIntExit();
}


/*
*********************************************************************************************************
*                                  定时器4中断     TIM4_IRQHandler()
*********************************************************************************************************
*/
void TIM4_IRQHandler(void)
{
    OSIntEnter();

    if(TIM_GetITStatus(TIM4, TIM_IT_CC1) == SET)    //检查指定的TIM中断发生与否
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);     //清中断标志 
        TIM4ITConfig(DISABLE);
        if(gpConfigInfo->BrightLevel < (BRIGHTLEVEL_NUM-1))   //小于最大亮度关屏
        {
            DriveLED_CtrlOE(OFF);
        }
    }

    OSIntExit();
}


/*
*********************************************************************************************************
*                                  RTC中断     RTC_IRQHandler()
*********************************************************************************************************
*/
void RTC_IRQHandler(void)
{
    UINT32 TimeCount;

    OSIntEnter();

    if (RTC_GetITStatus(RTC_IT_SEC) != RESET)
    {
        RTC_ClearITPendingBit(RTC_IT_SEC);
        RTC_WaitForLastTask();

        TimeCount = RTC_GetCounter();
        RTC_WaitForLastTask();
        gpTimeData->SaveFlag = YES;
        DealRTC_CalculateTime(TimeCount,gpTimeData);
    }
    
    OSIntExit();
}
/*
*********************************************************************************************************
*********************************************************************************************************
*                                          uC/OS-II APP HOOKS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      TASK TIMETICK HOOK (APPLICATION)
*
* Description : 
*
* Argument : 
*
* Note     : 
*********************************************************************************************************
*/
#if (OS_APP_HOOKS_EN > 0)
void App_TimeTickHook(void)
{
    IWDG_ReloadCounter();
}

/*
*********************************************************************************************************
*                                      TASK CREATION HOOK (APPLICATION)
*
* Description : This function is called when a task is created.
*
* Argument : ptcb   is a pointer to the task control block of the task being created.
*
* Note     : (1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void App_TaskCreateHook(OS_TCB* ptcb)
{
}

/*
*********************************************************************************************************
*                                    TASK DELETION HOOK (APPLICATION)
*
* Description : This function is called when a task is deleted.
*
* Argument : ptcb   is a pointer to the task control block of the task being deleted.
*
* Note     : (1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void App_TaskDelHook(OS_TCB* ptcb)
{
   (void) ptcb;
}

/*
*********************************************************************************************************
*                                      IDLE TASK HOOK (APPLICATION)
*
* Description : This function is called by OSTaskIdleHook(), which is called by the idle task.  This hook
*               has been added to allow you to do such things as STOP the CPU to conserve power.
*
* Argument : none.
*
* Note     : (1) Interrupts are enabled during this call.
*********************************************************************************************************
*/
//空闲任务
#if OS_VERSION >= 251
void App_TaskIdleHook(void)
{
//  USART1SEND("IDLE_Task\r\n", 0);
}
#endif

/*
*********************************************************************************************************
*                                        STATISTIC TASK HOOK (APPLICATION)
*
* Description : This function is called by OSTaskStatHook(), which is called every second by uC/OS-II's
*               statistics task.  This allows your application to add functionality to the statistics task.
*
* Argument : none.
*********************************************************************************************************
*/
//统计任务
void App_TaskStatHook(void)
{  
/*
    static UINT16 i=0;
    if(++i == 100)
    {
      i = 0;
      USART1SEND("STAT_Task\r\n", 0);
      DebugStkChk(APP_TASK_START_PRIO);
      DebugStkChk(APP_TASK_DISPLAY_PRIO);
      DebugStkChk(APP_TASK_DEALPIC_PRIO);
      DebugStkChk(APP_TASK_PROTOCOL_PRIO);
      DebugStkChk(APP_TASK_REPLYHOST_PRIO);
      DebugStkChk(APP_TASK_DEALTHN_PRIO);
      DebugCpuUsage();
    }
*/
}

/*
*********************************************************************************************************
*                                        TASK SWITCH HOOK (APPLICATION)
*
* Description : This function is called when a task switch is performed.  This allows you to perform other
*               operations during a context switch.
*
* Argument : none.
*
* Note     : 1 Interrupts are disabled during this call.
*
*            2 It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
*                   will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
*                  task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

#if OS_TASK_SW_HOOK_EN > 0
void App_TaskSwHook(void)
{
}
#endif

/*
*********************************************************************************************************
*                                     OS_TCBInit() HOOK (APPLICATION)
*
* Description : This function is called by OSTCBInitHook(), which is called by OS_TCBInit() after setting
*               up most of the TCB.
*
* Argument : ptcb    is a pointer to the TCB of the task being created.
*
* Note     : (1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/

#if OS_VERSION >= 204
void App_TCBInitHook(OS_TCB* ptcb)
{
   (void) ptcb;
}
#endif//OS_VERSION >= 204


/*
*********************************************************************************************************
*                                     OS_TCBInit() HOOK (APPLICATION)
*
* Description : 
*
* Argument : 
*
* Note     : 
*********************************************************************************************************
*/
#if OS_APP_HOOKS_EN > 0u
void App_TaskReturnHook (OS_TCB *ptcb) 
{

}
#endif//OS_APP_HOOKS_EN >= 0

#endif//(OS_APP_HOOKS_EN > 0)
