/*===========================================================================
* ��ַ ��http://www.cdebyte.com/   http://yhmcu.taobao.com/                 *
* ���� ������  ԭ �ں͵��ӹ�����  �� �ڰ��ص��ӿƼ����޹�˾                 * 
* �ʼ� ��yihe_liyong@126.com                                                *
* �绰 ��18615799380                                                        *
============================================================================*/

#include "bsp.h"         

// ��������
#define SEND_MAX        5       // �������ݵ���󳤶�

INT8U   Cnt1ms = 0;             // 1ms����������ÿ1ms��һ               
INT8U   LED_Time = 0;           // ������LED����ʱ��

// ������ر���
INT8U   COM_TxNeed = 0;
INT8U   COM_TimeOut = 0;
INT8U   COM_RxCounter = 0;
INT8U   COM_TxCounter = 0;
INT8U   COM_RxBuffer[65] = {0};
INT8U   COM_TxBuffer[65] = {0};

#define USRAT_SendByte()    USART_SendData8(COM_TxBuffer[COM_TxCounter++])
#define USRAT_RecvByte()    COM_RxBuffer[COM_RxCounter++]=USART_ReceiveData8()

/*===========================================================================
* ���� : DelayMs() => ��ʱ����(ms��)                                        *
* ���� ��x, ��Ҫ��ʱ����(0-255)                                             *
============================================================================*/
void DelayMs(INT8U x)
{
    Cnt1ms = 0;
    while (Cnt1ms <= x);
}

/*===========================================================================
* ���� ��TIM3_1MS_ISR() => ��ʱ��3������, ��ʱʱ���׼Ϊ1ms               *
============================================================================*/
void TIM3_1MS_ISR(void)
{
    Cnt1ms++;

    if (0 != COM_TimeOut)           
    {    
        if (--COM_TimeOut == 0)     
        { 
            if (COM_RxCounter > SEND_MAX)       { COM_RxCounter = 0; }
        }
    }
        
    if (0 != LED_Time)
    {
        if (--LED_Time == 0 )                   { LED_Y_OFF(); }
    }
}

/*=============================================================================
* ���� : USART_Send() => ͨ�����ڷ�������                                     *
* ���� ��buff, ����������     size�� ���ͳ���                                *
=============================================================================*/
void USART_Send(INT8U *buff, INT8U size)
{
    if (size == 0)          { return; }
    
    COM_TxNeed = 0;
    
    while (size --)         { COM_TxBuffer[COM_TxNeed++] = *buff++; }
    
    COM_TxCounter = 0;
    USART_ITConfig(USART_IT_TXE, ENABLE);
}

/*=============================================================================
* ����:  USART_RX_Interrupt() => ���ڽ����ж�                                 *
=============================================================================*/
void USART_RX_Interrupt(void)
{
    USRAT_RecvByte();
    
    if (COM_RxCounter > 30)         { COM_RxCounter = 0; }
        
    COM_TimeOut = 5;
}

/*=============================================================================
* ����:  USART_TX_Interrupt() =>���ڷ����ж�
=============================================================================*/
void USART_TX_Interrupt(void)
{
    if (COM_TxCounter < COM_TxNeed)     { USRAT_SendByte(); }   
    else    
    { 
        USART_ITConfig(USART_IT_TC, ENABLE);
        USART_ITConfig(USART_IT_TXE, DISABLE);

        if (USART_GetFlagStatus(USART_FLAG_TC))      
        {
            USART_ITConfig(USART_IT_TC, DISABLE); 
            COM_TxNeed = 0;
            COM_TxCounter = 0;
        }
    }
}

/*===========================================================================
* ���� ��MCU_Initial() => ��ʼ��CPU����Ӳ��                                 *
* ˵�� ����������Ӳ���ĳ�ʼ���������Ѿ�������C�⣬��bsp.c�ļ�               *
============================================================================*/
void MCU_Initial(void)
{
    SClK_Initial();         // ��ʼ��ϵͳʱ�ӣ�16M     
    GPIO_Initial();         // ��ʼ��GPIO                  
    TIM3_Initial();         // ��ʼ����ʱ��3����׼1ms 
    USART_Initial();        // ��ʼ������
    SPI_Initial();          // ��ʼ��SPI               

    enableInterrupts();     // �����ж�              
}

/*===========================================================================
* ���� ��RF_Initial() => ��ʼ��RFоƬ                                       *
* ˵�� ��L01+�Ĳ������Ѿ�������C�⣬��nRF24L01.c�ļ��� �ṩSPI��CSN������	*
         ���ɵ������ڲ����к����û������ٹ���L01+�ļĴ����������⡣			*
============================================================================*/
void RF_Initial(void)
{
	L01_Init();             // ��ʼ��L01�Ĵ���
	
	L01_SetTRMode(RX_MODE); // ����ģʽ      
	L01_FlushRX();          // ��λ����FIFOָ��    
    L01_FlushTX();          // ��λ����FIFOָ��
    L01_ClearIRQ(IRQ_ALL);  // ��������ж�
    L01_CE_HIGH();          // CE = 1, ��������          
}

/*===========================================================================
* ����: System_Initial() => ��ʼ��ϵͳ��������                              *
============================================================================*/
void System_Initial(void)
{
    MCU_Initial();      // ��ʼ��CPU����Ӳ��   
    RF_Initial();       // ��ʼ������оƬ,����ģʽ
    
    LED_Y_ON();
    LED_Time = 250;
}

/*===========================================================================
* ���� ��RF_RecvHandler() => �������ݽ��մ���                               * 
============================================================================*/
void RF_RecvHandler(void)
{
    INT8U length=0, recv_buffer[64]={0};
           
    if (0 == L01_IRQ_READ())                    // �������ģ���Ƿ���������ж� 
    {
        if (L01_ReadIRQSource() & (1<<RX_DR))   // �������ģ���Ƿ���յ�����
        {
            // ��ȡ���յ������ݳ��Ⱥ���������
            length = L01_ReadRXPayload(recv_buffer);
            
            // �жϽ��������Ƿ���ȷ
            if (length <= SEND_MAX)
            {
                LED_Y_ON();                      // ��ɫLED��˸������ָʾ�յ�����
                LED_Time = 200;                
                USART_Send((INT8U*)recv_buffer, length);
            }                
        }    
        
        L01_FlushRX();                          // ��λ����FIFOָ��    
        L01_ClearIRQ(IRQ_ALL);                  // ����ж�            
    }
}

/*===========================================================================
* ���� : BSP_RF_SendPacket() => ���߷������ݺ���                            *
============================================================================*/
void RF_SendPacket(void)
{
    INT8U i=0, length=0, buffer[65]={0};
    
    if ((0==COM_TimeOut) && (COM_RxCounter>0))
    {
        LED_R_ON();
        
        length = COM_RxCounter;
        COM_RxCounter = 0;
        
        for (i=0; i<length; i++)   { buffer[i] = COM_RxBuffer[i]; }
        
        L01_CE_LOW();               // CE = 0, �رշ���    
    
        L01_SetTRMode(TX_MODE);     // ����Ϊ����ģʽ      	
        L01_WriteTXPayload_NoAck(buffer, length);  
            
        L01_CE_HIGH();              // CE = 1, �������� 

        DelayMs(250);
        
        // �ȴ������жϲ���
        while (0 != L01_IRQ_READ());
        while (0 == L01_ReadIRQSource());
        
        L01_CE_LOW();               // CE = 0, �رշ���

        L01_FlushRX();              // ��λ����FIFOָ��
        L01_FlushTX();              // ��λ����FIFOָ��    
    	L01_ClearIRQ(IRQ_ALL);      // ����ж� 
        L01_SetTRMode(RX_MODE);     // ����ģʽ    
        L01_CE_HIGH();              // ��������

        LED_R_OFF(); 
    }   
}

/*===========================================================================
* ���� : main() => ���������������                                         *
============================================================================*/
void main(void)
{
	System_Initial();       // ��ʼ��ϵͳ��������               

	while (1)
	{
	    RF_SendPacket();
	    RF_RecvHandler();
	}
}
