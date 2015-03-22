
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
// ��ջ
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


// �ڴ��
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

   UserTimeDelay(500);          // �ϵ���ʱ

   BSP_Init();                  // Initialize BSP functions.
   OSInit();                    // Initialize "uC/OS-II, The Real-Time Kernel".
   MemBuf1 = OSMemCreate(MemPart1,MEM1_NUM,MEM1_SIZE,&UselessErr);    // �����ڴ��
   MemBuf2 = OSMemCreate(MemPart2,MEM2_NUM,MEM2_SIZE,&UselessErr);    // �����ڴ��
   //MemBuf3 = OSMemCreate(MemPart3,MEM3_NUM,MEM3_SIZE,&UselessErr);    // �����ڴ��
   
   InitSTM32();                 // STM32�����ʼ��

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

   OSTimeSet(0);               // ʱ�ӵδ� ����

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
    OSStatInit();         // ��������ͳ������  
#endif
    
    AppTaskCreate();      //�������񲻻��л���������������ΪTaskStart��������ȼ�����
    
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

    // ��������ͼƬ����
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskDealPIC,            // ������
        (void *) 0,                                     // ���ݲ���
        (OS_STK *) &App_TaskDealPICStk[APP_TASK_DEALPIC_STK_SIZE - 1], // ��ջ��ָ��
        (INT8U) APP_TASK_DEALPIC_PRIO,                  // �������ȼ�
        (INT8U) APP_TASK_DEALPIC_PRIO,                  // ����ID����ʱ����
        (OS_STK *)&App_TaskDealPICStk[0],               // ��ջ��ָ�룬���ڲ��������ջ��С
        (INT32U)APP_TASK_DEALPIC_STK_SIZE,              // ��ջ�ߴ��С������λ��32����
        (void *)0,                                      // ��չOS_TCB������û��Լ���Ϣ
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // ��������ջ���ҰѶ�ջ��Ϊ0


    // ����Э�鴦������
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskDisplay,            // ������                        
        (void *) 0,                                     // ���ݲ���  
        (OS_STK *) &App_TaskDisplayStk[APP_TASK_DISPLAY_STK_SIZE - 1], // ��ջ��ָ��
        (INT8U) APP_TASK_DISPLAY_PRIO,                  // �������ȼ�
        (INT8U) APP_TASK_DISPLAY_PRIO,                  // ����ID����ʱ����
        (OS_STK *)&App_TaskDisplayStk[0],               // ��ջ��ָ�룬���ڲ��������ջ��С
        (INT32U)APP_TASK_DISPLAY_STK_SIZE,              // ��ջ�ߴ��С������λ��32����
        (void *)0,                                      // ��չOS_TCB������û��Լ���Ϣ
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // ��������ջ���ҰѶ�ջ��Ϊ0
    

    // ��������Э������
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskProtocol,            // ������                        
        (void *) 0,                                      // ���ݲ���  
        (OS_STK *) &App_TaskProtocolStk[APP_TASK_PROTOCOL_STK_SIZE - 1], // ��ջ��ָ��
        (INT8U) APP_TASK_PROTOCOL_PRIO,                  // �������ȼ�
        (INT8U) APP_TASK_PROTOCOL_PRIO,                  // ����ID����ʱ����
        (OS_STK *)&App_TaskProtocolStk[0],               // ��ջ��ָ�룬���ڲ��������ջ��С
        (INT32U)APP_TASK_PROTOCOL_STK_SIZE,              // ��ջ�ߴ��С������λ��32����
        (void *)0,                                       // ��չOS_TCB������û��Լ���Ϣ
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // ��������ջ���ҰѶ�ջ��Ϊ0

    
    // �����ظ�����
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskReplyHost,           // ������                        
        (void *) 0,                                      // ���ݲ���  
        (OS_STK *) &App_TaskReplyHostStk[APP_TASK_REPLYHOST_STK_SIZE - 1], // ��ջ��ָ��
        (INT8U) APP_TASK_REPLYHOST_PRIO,                 // �������ȼ�
        (INT8U) APP_TASK_REPLYHOST_PRIO,                 // ����ID����ʱ����
        (OS_STK *)&App_TaskReplyHostStk[0],              // ��ջ��ָ�룬���ڲ��������ջ��С
        (INT32U)APP_TASK_REPLYHOST_STK_SIZE,             // ��ջ�ߴ��С������λ��32����
        (void *)0,                                       // ��չOS_TCB������û��Լ���Ϣ
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // ��������ջ���ҰѶ�ջ��Ϊ0
        
#if (USE_SHELL > 0)
    OSTaskCreateExt(
        (void (*) (void *)) App_TaskShell,               // ������
        (void *) 0,                                      // ���ݲ���
        (OS_STK *) &App_TaskShellStk[APP_TASK_SHELL_STK_SIZE - 1], // ��ջ��ָ��
        (INT8U) APP_TASK_SHELL_PRIO,                     // �������ȼ�
        (INT8U) APP_TASK_SHELL_PRIO,                     // ����ID����ʱ����
        (OS_STK *)&App_TaskShellStk[0],                  // ��ջ��ָ�룬���ڲ��������ջ��С
        (INT32U)APP_TASK_SHELL_STK_SIZE,                 // ��ջ�ߴ��С������λ��32����
        (void *)0,                                       // ��չOS_TCB������û��Լ���Ϣ
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // ��������ջ���ҰѶ�ջ��Ϊ0

        // ����Shell��������
        MqeueShell = OSQCreate(QeueShell, 20);
#endif

#if (USE_USB > 0)
    // ����THN����
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskDealUSB,             // ������                        
        (void *) 0,                                      // ���ݲ���  
        (OS_STK *) &App_TaskDealUSBStk[APP_TASK_DEALUSB_STK_SIZE - 1], // ��ջ��ָ��
        (INT8U) APP_TASK_DEALUSB_PRIO,                   // �������ȼ�
        (INT8U) APP_TASK_DEALUSB_PRIO,                   // ����ID����ʱ����
        (OS_STK *)&App_TaskDealUSBStk[0],                // ��ջ��ָ�룬���ڲ��������ջ��С
        (INT32U)APP_TASK_DEALUSB_STK_SIZE,               // ��ջ�ߴ��С������λ��32����
        (void *)0,                                       // ��չOS_TCB������û��Լ���Ϣ
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // ��������ջ���ҰѶ�ջ��Ϊ0
#endif

#if (USE_THN > 0)
    // ����THN����
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskDealTHN,             // ������                        
        (void *) 0,                                      // ���ݲ���  
        (OS_STK *) &App_TaskDealTHNStk[APP_TASK_DEALTHN_STK_SIZE - 1], // ��ջ��ָ��
        (INT8U) APP_TASK_DEALTHN_PRIO,                   // �������ȼ�
        (INT8U) APP_TASK_DEALTHN_PRIO,                   // ����ID����ʱ����
        (OS_STK *)&App_TaskDealTHNStk[0],                // ��ջ��ָ�룬���ڲ��������ջ��С
        (INT32U)APP_TASK_DEALTHN_STK_SIZE,               // ��ջ�ߴ��С������λ��32����
        (void *)0,                                       // ��չOS_TCB������û��Լ���Ϣ
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // ��������ջ���ҰѶ�ջ��Ϊ0
#endif

#if (USE_LOOP > 0)
    // ����ѭ������(ADC)
    os_err = OSTaskCreateExt(
        (void (*) (void *)) App_TaskLoop,                // ������                        
        (void *) 0,                                      // ���ݲ���  
        (OS_STK *) &App_TaskLoopStk[APP_TASK_LOOP_STK_SIZE - 1], // ��ջ��ָ��
        (INT8U) APP_TASK_LOOP_PRIO,                      // �������ȼ�
        (INT8U) APP_TASK_LOOP_PRIO,                      // ����ID����ʱ����
        (OS_STK *)&App_TaskLoopStk[0],                   // ��ջ��ָ�룬���ڲ��������ջ��С
        (INT32U)APP_TASK_LOOP_STK_SIZE,                  // ��ջ�ߴ��С������λ��32����
        (void *)0,                                       // ��չOS_TCB������û��Լ���Ϣ
        (INT16U)OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR); // ��������ջ���ҰѶ�ջ��Ϊ0
#endif

    // ������ʾ�ź���
    SemDisplay = OSSemCreate(0);
    // ����ʹ�ô����ź���
    MboxDealProtocol = OSMboxCreate(NULL);
    // �����ظ���Ϣ����
    MboxReplyHost = OSMboxCreate(NULL);
    // ����THN��������
#if (USE_THN > 0)
    MboxDealTHN = OSMboxCreate(NULL);
#endif
}

/*
*********************************************************************************************************
*                                          �жϺ���
*
* Description : ϵͳʱ���ж�.
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

    OS_ENTER_CRITICAL();  //����ȫ���жϱ�־,�����ж�/* Tell uC/OS-II that we are starting an ISR*/
    OSIntNesting++;
    OS_EXIT_CRITICAL();   //�ָ�ȫ���жϱ�־

    OSTimeTick();         /* Call uC/OS-II's OSTimeTick(),��os_core.c�ļ��ﶨ��,��Ҫ�ж���ʱ�������Ƿ��ʱ��*/

    OSIntExit();          //��os_core.c�ļ��ﶨ��,����и������ȼ������������,��ִ��һ�������л�            
}


/*
*********************************************************************************************************
*                                  ����1���ж�     USART1_IRQHandler()
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
*                                  ��ʱ��2�ж�     TIM2_IRQHandler()
*********************************************************************************************************
*/
void TIM5_IRQHandler(void)
{
    //����ʱ�ӵδ𣬴��ж���������ȼ������������ݱ�����
    OSIntEnter();

    if(TIM_GetITStatus(TIM5, TIM_IT_Update) == SET)//���ָ����TIM�жϷ������
    {
        TIM_ClearITPendingBit(TIM5, TIM_IT_Update); //���жϱ�־ 

        { // ��ʱ����++
            UINT16 i;
            // ͼƬ���ݵȴ�ʱ�����
            for(i=0; i<gpConfigInfo->RegionTotal; i++)
            {
                gpRegionData[i].DelayCount++;
                gpGlobalData->TimeCount[i]++;
            }
            // FUAЭ�鳬ʱ����
            if((++gpFUAData->OverTime) > FUA_WAITSEC)  // 4���Э��״̬��Ϊ�ȴ�
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
*                                  ��ʱ��3�ж�     TIM3_IRQHandler()
*********************************************************************************************************
*/
void TIM3_IRQHandler(void)
{
    OS_CPU_SR  cpu_sr;

    OSIntEnter();

    if(TIM_GetITStatus(TIM3, TIM_IT_CC1) == SET)//���ָ����TIM�жϷ������
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);//���жϱ�־ 
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
*                                  ��ʱ��4�ж�     TIM4_IRQHandler()
*********************************************************************************************************
*/
void TIM4_IRQHandler(void)
{
    OSIntEnter();

    if(TIM_GetITStatus(TIM4, TIM_IT_CC1) == SET)    //���ָ����TIM�жϷ������
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);     //���жϱ�־ 
        TIM4ITConfig(DISABLE);
        if(gpConfigInfo->BrightLevel < (BRIGHTLEVEL_NUM-1))   //С��������ȹ���
        {
            DriveLED_CtrlOE(OFF);
        }
    }

    OSIntExit();
}


/*
*********************************************************************************************************
*                                  RTC�ж�     RTC_IRQHandler()
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
//��������
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
//ͳ������
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
