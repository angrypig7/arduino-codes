/*===========================================================================
* ��ַ ��http://www.cdebyte.com/   http://yhmcu.taobao.com/                 *
* ���� ������  ԭ �ں͵��ӹ�����  �� �ڰ��ص��ӿƼ����޹�˾                 * 
* �ʼ� ��yihe_liyong@126.com                                                *
* �绰 ��18615799380                                                        *
============================================================================*/

#include "bsp.h"

/*===========================================================================
* ���� ��SClK_Initial() => ��ʼ��ϵͳʱ�ӣ�ϵͳʱ�� = 16MHZ                 *
============================================================================*/
void SClK_Initial(void)
{
	CLK_MasterPrescalerConfig(CLK_MasterPrescaler_HSIDiv1); // 16M
}

/*===========================================================================
* ���� ��GPIO_Initial() => ��ʼ��ͨ��IO�˿�
============================================================================*/
void GPIO_Initial(void)
{
    // ����LED���ź�KEY���� KEY(PB1), LED(PB0)
    GPIO_Init(PORT_KEY, PIN_KEY, GPIO_Mode_In_PU_No_IT);
    
    GPIO_Init(PORT_LED, PIN_LED, GPIO_Mode_Out_PP_High_Slow);
    GPIO_SetBits(PORT_LED, PIN_LED);

    // ����nRF24L01P��ؿ������� IRQ(PA3), CE(PA2), CSN(PB4)
    GPIO_Init(PORT_L01_IRQ, PIN_L01_IRQ, GPIO_Mode_In_PU_No_IT);
    
    GPIO_Init(PORT_L01_CE, PIN_L01_CE, GPIO_Mode_Out_PP_High_Slow);
    GPIO_SetBits(PORT_L01_CE, PIN_L01_CE);
    
    GPIO_Init(PORT_L01_CSN, PIN_L01_CSN, GPIO_Mode_Out_PP_High_Slow);
    GPIO_SetBits(PORT_L01_CSN, PIN_L01_CSN);

    // ����OLED��ؿ������� CMD(PD0), CSN(PB3), RST(PB2)
    GPIO_Init(PORT_OLED_CMD, PIN_OLED_CMD, GPIO_Mode_Out_PP_High_Slow);
    GPIO_SetBits(PORT_OLED_CMD, PIN_OLED_CMD);
    
    GPIO_Init(PORT_OLED_CSN, PIN_OLED_CSN, GPIO_Mode_Out_PP_High_Slow);
    GPIO_SetBits(PORT_OLED_CSN, PIN_OLED_CSN);
    
    GPIO_Init(PORT_OLED_RST, PIN_OLED_RST, GPIO_Mode_Out_PP_High_Slow);
    GPIO_SetBits(PORT_OLED_RST, PIN_OLED_RST);
}

/*===========================================================================
* ���� ��SPI_Initial() => ��ʼ��SPI                                         *
============================================================================*/
void SPI_Initial(void)
{
	CLK_PeripheralClockConfig(CLK_Peripheral_SPI, ENABLE);
	
	SPI_DeInit();
	
	// ����SPI��ز���,2��Ƶ��8MHZ��
	SPI_Init(SPI_FirstBit_MSB, SPI_BaudRatePrescaler_2,
             SPI_Mode_Master, SPI_CPOL_Low, SPI_CPHA_1Edge,
             SPI_Direction_2Lines_FullDuplex, SPI_NSS_Soft);

	SPI_Cmd(ENABLE);
	
	// SPI���IO������
	GPIO_Init(PORT_SPI, PIN_MISO, GPIO_Mode_In_PU_No_IT);       // MISO (PB7)
	GPIO_Init(PORT_SPI, PIN_SCLK, GPIO_Mode_Out_PP_High_Slow);  // SCLK (PB5)
	GPIO_Init(PORT_SPI, PIN_MOSI, GPIO_Mode_Out_PP_High_Slow);  // MOSI (PB6)
}

/*===========================================================================
* ���� ��TIM3_Initial() => ��ʼ����ʱ��3����ʱʱ��Ϊ1ms                     *
============================================================================*/
void TIM3_Initial(void)
{
    TIM3_DeInit();

    CLK_PeripheralClockConfig(CLK_Peripheral_TIM3, ENABLE);

    // ����Timer3��ز�����ʱ��Ϊ16/16 = 1MHZ����ʱʱ�� = 1000/1000000 = 1ms
    TIM3_TimeBaseInit(TIM3_Prescaler_16, TIM3_CounterMode_Up, 1000);
    TIM3_ITConfig(TIM3_IT_Update, ENABLE);

    TIM3_Cmd(ENABLE);
}

/*===========================================================================
* ���� ��SPI_ExchangeByte() => ͨ��SPI�������ݽ���                          * 
* ���� ����Ҫд��SPI��ֵ                                                    * 
* ��� ��ͨ��SPI������ֵ                                                    * 
============================================================================*/
INT8U SPI_ExchangeByte(INT8U input)
{
	while (RESET == SPI_GetFlagStatus(SPI_FLAG_TXE));   // �ȴ����ݴ������	
	SPI_SendData(input);
	while (RESET == SPI_GetFlagStatus(SPI_FLAG_RXNE));  // �ȴ����ݽ������
	return (SPI_ReceiveData());
}

/*===========================================================================
-----------------------------------�ļ�����----------------------------------
===========================================================================*/
