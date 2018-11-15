/*===========================================================================
* ��ַ ��http://www.cdebyte.com/   http://yhmcu.taobao.com/                 *
* ���� ������  ԭ �ں͵��ӹ�����  �� �ڰ��ص��ӿƼ����޹�˾                 * 
* �ʼ� ��yihe_liyong@126.com                                                *
* �绰 ��18615799380                                                        *
============================================================================*/

#include "bsp.h"         

// ��������
#define TX              1       // ����ģʽ
#define RX              0       // ����ģʽ
       
#define SEND_LENGTH     5       // ��������ÿ���ĳ���

INT8U   Cnt1ms = 0;             // 1ms����������ÿ1ms��һ                
INT16U  RecvCnt = 0;            // �������յ����ݰ���                

// ������ر���
INT8U   COM_TxNeed = 0;
INT8U   COM_RxCounter = 0;
INT8U   COM_TxCounter = 0;
INT8U   COM_RxBuffer[65] = { 0 };
INT8U   COM_TxBuffer[65] = { 0 };

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
* ���� ��mode, =0,����ģʽ�� else,����ģʽ                                  *
* ˵�� ��L01+�Ĳ������Ѿ�������C�⣬��nRF24L01.c�ļ��� �ṩSPI��CSN������	*
         ���ɵ������ڲ����к����û������ٹ���L01+�ļĴ����������⡣			*
============================================================================*/
void RF_Initial(INT8U mode)
{
	L01_Init();                                     // ��ʼ��L01�Ĵ���
	if (RX == mode)     { L01_SetTRMode(RX_MODE); } // ����ģʽ      
	L01_WriteHoppingPoint(60);                      // ����Ƶ��            
	L01_SetSpeed(SPD_1M);                           // ���ÿ���Ϊ1M        
	
	L01_FlushRX();                                  // ��λ����FIFOָ��    
    L01_FlushTX();                                  // ��λ����FIFOָ��
    L01_ClearIRQ(IRQ_ALL);                          // ��������ж�
    if (RX == mode)     { L01_CE_HIGH(); }          // CE = 1, ��������          
}

/*===========================================================================
* ����: System_Initial() => ��ʼ��ϵͳ��������                              *
============================================================================*/
void System_Initial(void)
{
    MCU_Initial();      // ��ʼ��CPU����Ӳ��
    RF_Initial(RX);     // ��ʼ������оƬ,����ģʽ           
}

/*===========================================================================
* ���� ��RF_RecvHandler() => �������ݽ��մ���                               * 
============================================================================*/
void RF_RecvHandler(void)
{
    INT8U i=0, length=0, recv_buffer[10]={ 0 };
           
    if (0 == L01_IRQ_READ())                    // �������ģ���Ƿ���������ж� 
    {
        if (L01_ReadIRQSource() & (1<<RX_DR))   // �������ģ���Ƿ���յ�����
        {
            // �������㣬��ֹ���ж�
            for (i=0; i<SEND_LENGTH; i++)   { recv_buffer[0] = 0; } 
                
            // ��ȡ���յ������ݳ��Ⱥ���������
            length = L01_ReadRXPayload(recv_buffer);
            
            // �жϽ��������Ƿ���ȷ
            if ((SEND_LENGTH==length) && (recv_buffer[0]=='1')\
            && (recv_buffer[1]=='2') && (recv_buffer[2]=='3'))
            {
                LED0_TOG();                     // LED��˸������ָʾ�յ���ȷ����
                RecvCnt++;
                USART_Send("Receive ok\r\n", 12);
            }                
        }    
        
        L01_FlushRX();                          // ��λ����FIFOָ��    
        L01_ClearIRQ(IRQ_ALL);                  // ����ж�            
    }
}

/*===========================================================================
* ���� : main() => ���������������                                         *
* ˵�� ��������յ����ݳ���Ϊ5���ֽڣ���ǰ�����ֽ�Ϊ��123�������ݽ�����ȷ     *
============================================================================*/
void main(void)
{
	System_Initial();       // ��ʼ��ϵͳ��������               

	while (1)
	{
	    RF_RecvHandler();   // �������ݽ��մ���               
	}
}

