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

#define SEND_GAP        1000    // ÿ���1s����һ������        
#define SEND_LENGTH     5       // ��������ÿ���ĳ���

INT8U   Cnt1ms = 0;             // 1ms����������ÿ1ms��һ 
INT8U   SendFlag = 0;           // =1�������������ݣ�=0������
INT16U  SendTime = 1;           // �������ݷ��ͼ��ʱ��                
INT16U  SendCnt = 0;            // �������͵����ݰ���                

// ��Ҫ���͵�����  
INT8U   SendBuffer[SEND_LENGTH] = { "123\r\n" }; 

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
    
    if (0 != SendTime)      // 1msʱ�䵽����λSendFlag��־����������ѯ��������      
    { 
        if (--SendTime == 0)    { SendTime = SEND_GAP; SendFlag = 1; }
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
    RF_Initial(TX);     // ��ʼ������оƬ,����ģʽ           
}

/*===========================================================================
* ���� : BSP_RF_SendPacket() => ���߷������ݺ���                            *
* ˵�� ��Sendbufferָ������͵����ݣ�length�������ݳ���                     *
============================================================================*/
INT8U RF_SendPacket(INT8U *Sendbuffer, INT8U length)
{
    INT8U tmp = 0;
    
    L01_CE_LOW();               // CE = 0, �رշ���    
    
    L01_SetTRMode(TX_MODE);     // ����Ϊ����ģʽ      	
    L01_WriteTXPayload_NoAck(SendBuffer, length);  
        
    L01_CE_HIGH();              // CE = 1, �������� 
    
    LED0_TOG();    
    USART_Send("Transmit ok\r\n", 13);
    
    DelayMs(250);
    
    // �ȴ������жϲ���
    while (0 != L01_IRQ_READ());
    while (0 == (tmp=L01_ReadIRQSource()));
    
    L01_FlushTX();              // ��λ����FIFOָ��    
	L01_ClearIRQ(IRQ_ALL);      // ����ж� 
	             
    L01_CE_LOW();               // CE = 0, �رշ���    
    
    return (tmp & (1<<TX_DS));  // ���ط����Ƿ�ɹ�   
}

/*===========================================================================
* ���� : main() => ���������������                                         *
* ˵�� ��û1s����һ�����ݣ�ÿ�����ݳ���Ϊ5���ֽڣ���123/r/n��                 *
============================================================================*/
void main(void)
{
	System_Initial();                       // ��ʼ��ϵͳ��������               

	while (1)
	{
	    if (0 != SendFlag)                  // 1s������������
        {
            // ���ݷ��ͳɹ�(�յ����շ���Ӧ��)
            if (RF_SendPacket(SendBuffer, SEND_LENGTH))
            {
                LED1_ON();                  // LED����������ָʾӦ��ɹ�
                USART_Send("Ack ok\r\n", 8);
            }
            else
            {
                LED1_OFF();
                USART_Send("Ack error\r\n", 11);
            }
            
            SendFlag = 0;                
        }
	}
}
