//
// Created by Calson on 2023/2/4.
//
#include "usart.h"
#include "tim.h"
#include "spi.h"
#include "ads1292.h"
#include "string.h"
#include "stdio.h"

uint8_t Info1[50] = "read default REG: \r\n";
uint8_t Info2[50];
uint8_t ADS1292_REG[12];
uint8_t reg_val[12] = {0x53, 0, 0xe3, 0x10, 0x15, 0x15, 0x2c, 0x0c, 0x08, 0xea, 0x03, 0x0c};

__IO uint8_t ads1292_Cache[9];
__IO uint8_t ads1292_receive_flag = 0;

ADS1292_CONFIG1 	Ads1292_Config1	= {DATA_RATE};											//CONFIG1
ADS1292_CONFIG2 	Ads1292_Config2 = {PDB_LOFF_COMP, PDB_REFBUF, VREF, CLK_EN, INT_TEST};		//CONFIG2
ADS1292_CHSET 		Ads1292_Ch1set = {CHANNEL1_POWER, CHANNEL1_GAIN, CHANNEL1_MUX};				//CH1SET
ADS1292_CHSET 		Ads1292_Ch2set = {CHANNEL2_POWER, CHANNEL2_GAIN, CHANNEL2_MUX};				//CH2SET
ADS1292_RLD_SENS	Ads1292_Rld_Sens = {PDB_RLD, RLD_LOFF_SENSE, RLD2N, RLD2P, RLD1N, RLD1P};	    //RLD_SENS
ADS1292_LOFF_SENS	Ads1292_Loff_Sens = {FLIP2, FLIP1, LOFF2N, LOFF2P, LOFF1N, LOFF1P};				//LOFF_SENS
ADS1292_RESP1		Ads1292_Resp1 = {RESP_DEMOD_EN1, RESP_MOD_EN, RESP_PH, RESP_CTRL};		    //RSP1
ADS1292_RESP2		Ads1292_Resp2 = {CALIB, FREQ, RLDREF_INT};								//RSP2

void delay_us(uint16_t Delay)
{
    __HAL_TIM_SET_COUNTER(&htim17, 0);
    __HAL_TIM_ENABLE(&htim17);
    while (__HAL_TIM_GET_COUNTER(&htim17) < Delay);
    __HAL_TIM_DISABLE(&htim17);
}

int32_t Get_Voltage(uint32_t num)
{
    int32_t temp;
    temp = num;
    temp <<= 8;
    temp >>= 8;
    return temp;
}

void ADS1292_Set_RegBuff(void)
{
    /* Register 0 Config */
    ADS1292_REG[ID] = ADS1292_DEVICE;                             //ID只读

    /* Register 1 Config */
    ADS1292_REG[CONFIG1] =	0x00;		                            //0000 0aaa	[7] 0连续转换模式  [6:3] 必须为0
    ADS1292_REG[CONFIG1] |=	Ads1292_Config1.Data_Rate;              //[2:0] aaa 采样率设置采样率

    /* Register 2 Config */
    ADS1292_REG[CONFIG2] =	0x00;		                            //1abc d0e1	[7] 必须为1
                                                                    // [2] 必须为0
                                                                    // [0] 设置测试信号为1HZ、±1mV方波
    ADS1292_REG[CONFIG2] |=	Ads1292_Config2.Pdb_Loff_Comp << 6;	    //[6]a 导联脱落比较器是否掉电
    ADS1292_REG[CONFIG2] |=	Ads1292_Config2.Pdb_Refbuf << 5;		    //[5]b 内部参考缓冲器是否掉电
    ADS1292_REG[CONFIG2] |=	Ads1292_Config2.Vref << 4;				//[4]c 内部参考电压设置，默认2.42V
    ADS1292_REG[CONFIG2] |=	Ads1292_Config2.Clk_EN << 3;				//[3]d CLK引脚输出时钟脉冲？
    ADS1292_REG[CONFIG2] |=	Ads1292_Config2.Int_Test << 1;			//[1]e 是否打开内部测试信号,
    ADS1292_REG[CONFIG2] |=	0x81;                                   //设置默认位

    /* Register 3 Config */
    ADS1292_REG[LOFF] =	0x10;                                       // [7:5]	设置导联脱落比较器阈值
                                                                    // [4]	必须为1
                                                                    // [3:2] 导联脱落电流幅值
                                                                    // [1]	必须为0
                                                                    // [0]	导联脱落检测方式 0 DC 1 AC

    /* Register 4 Config */
    ADS1292_REG[CH1SET] = 0x00;	                                    //abbb cccc
    ADS1292_REG[CH1SET] |= Ads1292_Ch1set.PD<<7;		            //[7]  a 		通道1断电？
    ADS1292_REG[CH1SET] |= Ads1292_Ch1set.GAIN<<4;	                //[6:4]bbb	设置PGA增益
    ADS1292_REG[CH1SET] |= Ads1292_Ch1set.MUX;			            //[3:0]cccc	设置通道1输入方式

    /* Register 5 Config */
    ADS1292_REG[CH2SET] =	0x00;	                                //abbb cccc
    ADS1292_REG[CH2SET] |= Ads1292_Ch2set.PD<<7;		                //[7]  a 		通道2断电？
    ADS1292_REG[CH2SET] |= Ads1292_Ch2set.GAIN<<4;	                //[6:4]bbb	设置PGA增益
    ADS1292_REG[CH2SET] |= Ads1292_Ch2set.MUX;			            //[3:0]cccc	设置通道2输入方式

    /* Register 6 Config */
    ADS1292_REG[RLD_SENS] = 0X00;                                   //11ab cdef
                                                                    //  [7:6] 11 PGA斩波频率	fMOD/4
    ADS1292_REG[RLD_SENS] |= Ads1292_Rld_Sens.Pdb_Rld<<5;			//  [5]a	该位决定RLD缓冲电源状态
    ADS1292_REG[RLD_SENS] |= Ads1292_Rld_Sens.Rld_Loff_Sense<<4;	    //  [4]b	该位使能RLD导联脱落检测功能
    ADS1292_REG[RLD_SENS] |= Ads1292_Rld_Sens.Rld2N<<3;				//[3]c	这个位控制通道2负输入	用于右腿驱动的输出
    ADS1292_REG[RLD_SENS] |= Ads1292_Rld_Sens.Rld2P<<2;				//[2]d	该位控制通道2正输入		用于右腿驱动的输出
    ADS1292_REG[RLD_SENS] |= Ads1292_Rld_Sens.Rld1N<<1;				//[1]e	这个位控制通道1负输入	用于右腿驱动的输出
    ADS1292_REG[RLD_SENS] |= Ads1292_Rld_Sens.Rld1P;					//[0]f	该位控制通道1正输入		用于右腿驱动的输出
    ADS1292_REG[RLD_SENS] |= 0xc0;                               //设置默认位

    /* Register 7 Config */
    ADS1292_REG[LOFF_SENS] = 0X00;                                  //00ab cdef	[7:6] 必须为0
    ADS1292_REG[LOFF_SENS] |=Ads1292_Loff_Sens.Flip2<<5;		    //  [5]a	这个位用于控制导联脱落检测通道2的电流的方向
    ADS1292_REG[LOFF_SENS] |=Ads1292_Loff_Sens.Flip1<<4;		    //[4]b	这个位控制用于导联脱落检测通道1的电流的方向
    ADS1292_REG[LOFF_SENS] |=Ads1292_Loff_Sens.Loff2N<<3;	        //[3]c	该位控制通道2负输入端的导联脱落检测
    ADS1292_REG[LOFF_SENS] |=Ads1292_Loff_Sens.Loff2P<<2;	        //[2]d	该位控制通道2正输入端的导联脱落检测
    ADS1292_REG[LOFF_SENS] |=Ads1292_Loff_Sens.Loff1N<<1;	        //[1]e	该位控制通道1负输入端的导联脱落检测
    ADS1292_REG[LOFF_SENS] |=Ads1292_Loff_Sens.Loff1P;			    //[0]f	该位控制通道1正输入端的导联脱落检测

    /* Register 8 Config */
    ADS1292_REG[LOFF_STAT] =	0x00;		                        //[6]0 设置fCLK和fMOD之间的模分频比 fCLK=fMOD/4
                                                                    // [4:0]只读，导联脱落和电极连接状态

    /* Register 9 Config */
    ADS1292_REG[RESP1] = 0X00;                                      //abcc cc1d
    ADS1292_REG[RESP1] |= Ads1292_Resp1.RESP_DemodEN << 7;             //[7]a		这个位启用和禁用通道1上的解调电路
    ADS1292_REG[RESP1] |= Ads1292_Resp1.RESP_modEN << 6;	            //[6]b		这个位启用和禁用通道1上的调制电路
    ADS1292_REG[RESP1] |= Ads1292_Resp1.RESP_ph << 2;			        //[5:2]c	这些位控制呼吸解调控制信号的相位
    ADS1292_REG[RESP1] |= Ads1292_Resp1.RESP_Ctrl;			        //[0]d		这个位设置呼吸回路的模式
    ADS1292_REG[RESP1] |=	0x02;                                   //设置默认位

    /* Register 10 Config */
    ADS1292_REG[RESP2] = 0x00;                                      //a000 0bc1	[6:3]必须为0
                                                                    // [0]必须为1
    ADS1292_REG[RESP2] |=	Ads1292_Resp2.Calib<<7;				    //[7]a 启动通道偏移校正？
    ADS1292_REG[RESP2] |=	Ads1292_Resp2.freq<<2;				    //[2]b 呼吸频率设置
    ADS1292_REG[RESP2] |=	Ads1292_Resp2.Rldref_Int<<1;	        //[1]c RLDREF信号源外部馈电？
    ADS1292_REG[RESP2] |= 0X01;                                     //设置默认位

    /* Register 11 Config */
    ADS1292_REG[AGPIO] =	0x0C;			                        //GPIO设为输入
                                                                    // [7:4]必须为0
                                                                    // [3:2]11 GPIO为输入
                                                                    // [1:0] 设置输入时，指示引脚电平，设置输出时控制引脚电平
}

void ADS1292_Send_CMD(uint8_t data)
{
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

uint8_t ADS1292_ReadData(uint8_t * data)
{
    uint8_t read_data = 0x00;
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, &read_data, data, 9, 500);
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
    return 0;
}

void ADS1292_WriteRead_REGS(uint8_t reg, uint8_t len, uint8_t * data)
{
    uint8_t i;
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &reg, 1, 10);
    HAL_SPI_Transmit(&hspi1, &len, 1, 10);
    if (reg & 0x40) {
        for(i = 0;i < len; i++)
        {
            delay_us(100);
            HAL_SPI_Transmit(&hspi1, data, 1, 10);
            data++;
        }
    }
    else {
        HAL_SPI_Receive(&hspi1, data, len, 10);
    }
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);

}

uint8_t ADS1292_Write_RegBuff(void)
{
    uint8_t res = 0;
    uint8_t REG_Cache[12];
    ADS1292_Set_RegBuff();
    ADS1292_WriteRead_REGS(WREG | CONFIG1, 11, ADS1292_REG+1);
    HAL_Delay(10);
    ADS1292_WriteRead_REGS(RREG | ID, 12, REG_Cache);
    HAL_Delay(10);

#ifdef DEBUG_ADS1292
    sprintf((char *)Info2, "WRITE REG:\r\n");
    HAL_UART_Transmit(&huart1, Info2, strlen((char *) Info2), 10);

    for (int i = 0; i < 12; i++) {
        sprintf((char *)Info2, "%d %x\r\n", i, ADS1292_REG[i]);
        HAL_UART_Transmit(&huart1, Info2, strlen((char *) Info2), 10);
    }

    sprintf((char *) Info2, "READ REG: \r\n");
    HAL_UART_Transmit(&huart1, Info2, strlen((char *) Info2), 10);
#endif

    for (int i = 0; i < 12; i++) {
        if (ADS1292_REG[i] != REG_Cache[i]) {
            if (i != 0 && i != 8 && i != 11)
                res = 1;
        }

#ifdef DEBUG_ADS1292
        sprintf((char*)Info2, "%d %x\r\n", i, REG_Cache[i]);
        HAL_UART_Transmit(&huart1, Info2, strlen((char *) Info2), 10);
#endif

    }

#ifdef DEBUG_ADS1292
    if (res == 0) {
        sprintf((char *)Info2, "REG write success\r\n");
        HAL_UART_Transmit(&huart1, Info2, strlen((char *) Info2), 100);
    }
    else {
        sprintf((char *)Info2, "REG write failed\r\n");
        HAL_UART_Transmit(&huart1, Info2, strlen((char *) Info2), 100);
    }
#endif

    return res;
}

void ADS1292_PowerOnInit(void)
{
    uint8_t REG_Cache[12];

    ADS1292_Send_CMD(SDATAC);
    HAL_Delay(100);
    ADS1292_Send_CMD(ARESET);
    HAL_Delay(1000);
    ADS1292_Send_CMD(SDATAC);
    HAL_Delay(100);

#ifdef DEBUG_ADS1292
    ADS1292_WriteRead_REGS(RREG | ID, 12, REG_Cache);
    HAL_UART_Transmit(&huart1, Info1, strlen((char *)Info1), 100);
    for (uint8_t i = 0; i < 12; i++) {
        sprintf((char *)Info2, "%d %x\r\n", i, REG_Cache[i]);
        HAL_UART_Transmit(&huart1, Info2, strlen((char *) Info2), 100);
    }

#endif

}

uint8_t ADS1292_Single_Test(void)
{
    uint8_t res = 0;
    Ads1292_Config2.Int_Test = INT_TEST_ON;
    Ads1292_Ch1set.MUX = MUX_Test_signal;
    Ads1292_Ch2set.MUX = MUX_Test_signal;
    Ads1292_Ch2set.GAIN = GAIN_1;
    if (ADS1292_Write_RegBuff())
        res = 1;
    HAL_Delay(10);
    return res;
}

uint8_t ADS1292_Noise_Test(void)
{
    uint8_t res=0;
    Ads1292_Config2.Int_Test = INT_TEST_OFF;
    Ads1292_Ch1set.MUX = MUX_input_shorted;
    Ads1292_Ch2set.MUX = MUX_input_shorted;

    if(ADS1292_Write_RegBuff())
        res=1;
    HAL_Delay(10);
    return res;
}

uint8_t ADS1292_Single_Read(void)
{
    uint8_t res = 0;
    Ads1292_Config2.Int_Test = INT_TEST_OFF;
    Ads1292_Ch1set.MUX = MUX_Normal_input;
    Ads1292_Ch2set.MUX = MUX_Normal_input;

    if(ADS1292_Write_RegBuff())
        res=1;
    HAL_Delay(10);
    return res;
}

uint8_t Set_ADS1292_Collect(uint8_t mode)
{
    uint8_t res;

    HAL_Delay(10);
    switch (mode) {
        case 0:
            res = ADS1292_Single_Read();
            break;
        case 1:
            res = ADS1292_Single_Test();
            break;
        case 2:
            res = ADS1292_Noise_Test();
            break;
    }
    if(res)
        return 1;
    HAL_GPIO_WritePin(START_GPIO_Port, START_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    ADS1292_Send_CMD(RDATAC);
    HAL_Delay(10);
//    ADS1292_Send_CMD(ASTART);
    HAL_GPIO_WritePin(START_GPIO_Port, START_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
    return 0;
}