#include "stm32f1xx_hal.h"
#include "OLED_Font.h"

/* 引脚定义 - 使用HAL GPIO库 */
#define OLED_W_SCL(x)   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (GPIO_PinState)(x))
#define OLED_W_SDA(x)   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, (GPIO_PinState)(x))

/* 毫秒级延时 */
static void OLED_Delay(uint32_t ms)
{
    HAL_Delay(ms);
}

/* 微秒级延时，用于I2C时序 */
static void OLED_Delay_us(uint32_t us)
{
    uint32_t i;
    for (i = 0; i < us * 8; i++)
    {
        __NOP();
    }
}

/* GPIO初始化，开漏输出（外部须接上拉电阻） */
void OLED_I2C_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 配置SCL - PB8 */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* 配置SDA - PB9 */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* 总线空闲状态 */
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/* I2C起始信号 */
void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_Delay_us(5);
    OLED_W_SDA(0);
    OLED_Delay_us(5);
    OLED_W_SCL(0);
}

/* I2C停止信号 */
void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_Delay_us(5);
    OLED_W_SDA(1);
    OLED_Delay_us(5);
}

/* I2C发送一个字节 */
void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        OLED_W_SDA((Byte & (0x80 >> i)) ? 1 : 0);
        OLED_Delay_us(1);
        OLED_W_SCL(1);
        OLED_Delay_us(2);
        OLED_W_SCL(0);
        OLED_Delay_us(1);
    }
    /* 第9个时钟（供从机应答，此处未检测ACK位） */
    OLED_W_SCL(1);
    OLED_Delay_us(2);
    OLED_W_SCL(0);
    OLED_Delay_us(1);
}

/* 写命令 */
void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);        // 从机地址/写模式
    OLED_I2C_SendByte(0x00);        // 命令控制位
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

/* 写数据 */
void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);        // 从机地址/写模式
    OLED_I2C_SendByte(0x40);        // 数据控制位
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

/* 设置光标位置 */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);                    // 设置页地址
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));    // 设置列起始高4位
    OLED_WriteCommand(0x00 | (X & 0x0F));           // 设置列起始低4位
}

/* 清屏 */
void OLED_Clear(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for(i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

/* 显示单个字符 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);
    }
}

/* 显示字符串 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/* 次方计算（内部使用） */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/* 显示无符号数字（十进制） */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/* 显示有符号数字（十进制） */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/* 显示十六进制数字 */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10)
        {
            OLED_ShowChar(Line, Column + i, SingleNumber + '0');
        }
        else
        {
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
        }
    }
}

/* 显示二进制数字 */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

/* OLED初始化 */
void OLED_Init(void)
{
    OLED_Delay(100);       // 上电延时

    OLED_I2C_Init();

    OLED_WriteCommand(0xAE);    // 关闭显示

    OLED_WriteCommand(0xD5);    // 设置显示时钟分频比/振荡器频率
    OLED_WriteCommand(0x80);

    OLED_WriteCommand(0xA8);    // 设置多路复用比
    OLED_WriteCommand(0x3F);

    OLED_WriteCommand(0xD3);    // 设置显示偏移
    OLED_WriteCommand(0x00);

    OLED_WriteCommand(0x40);    // 设置显示起始行

    OLED_WriteCommand(0xA1);    // 设置左右方向，0xA1正常

    OLED_WriteCommand(0xC8);    // 设置上下方向，0xC8正常

    OLED_WriteCommand(0xDA);    // 设置COM引脚硬件配置
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81);    // 设置对比度
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);    // 设置预充电周期
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);    // 设置VCOMH取消选择级别
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);    // 恢复正常显示（显示GDDRAM内容，区别于全屏点亮）

    OLED_WriteCommand(0xA6);    // 设置正常/反转显示

    OLED_WriteCommand(0x8D);    // 设置电荷泵
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAF);    // 开启显示

    OLED_Clear();
}

/**
  * @brief  OLED显示浮点数（带符号，可指定小数位数）
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Number 要显示的浮点数
  * @param  intLen 整数部分显示位数
  * @param  fracLen 小数部分显示位数
  * @retval 无
  */
void OLED_ShowFloat(uint8_t Line, uint8_t Column, double Number, uint8_t intLen, uint8_t fracLen)
{
    uint8_t i;
    uint32_t intPart;
    uint32_t fracPart;

    // 处理符号
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        intPart = (uint32_t)Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        intPart = (uint32_t)(-Number);
    }

    // 显示整数部分，右对齐，高位补空格
    for (i = 0; i < intLen; i++)
    {
        uint8_t digit = intPart / OLED_Pow(10, intLen - i - 1) % 10;
        OLED_ShowChar(Line, Column + i + 1, digit + '0');
    }

    // 显示小数点
    OLED_ShowChar(Line, Column + intLen + 1, '.');

    // 计算并显示小数部分
    double temp = (Number >= 0 ? Number : -Number);
    temp = temp - (int32_t)temp;  // 取小数部分
    fracPart = (uint32_t)(temp * OLED_Pow(10, fracLen) + 0.5);  // 四舍五入

    // 小数部分固定位数补零
    for (i = 0; i < fracLen; i++)
    {
        uint8_t digit = fracPart / OLED_Pow(10, fracLen - i - 1) % 10;
        OLED_ShowChar(Line, Column + intLen + 2 + i, digit + '0');
    }
}
