/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 平衡车 - 主程序 (精简版)
  *
  * 架构说明:
  *   - 初始化: HAL -> 时钟 -> 外设 -> OLED/MPU6050 -> Balance
  *   - 主循环: 读 MPU6050 -> 同步到 Balance -> OLED 显示 -> 遥控待扩展
  *   - 实时控制: TIM1 1ms 中断 -> Balance_TimerCallback() -> PID + 电机
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
#include "MPU6050.h"
#include "Encoder.h"
#include "Store.h"
#include "PID.h"
#include "Motor.h"
#include "Serial.h"
#include "BlueSerial.h"
#include "balance.h"
#include "DebugMode.h"
#include "Key.h"
#include "stdio.h"
#include <string.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
	MPU6050_Init(&hi2c2);
	Key_Init();
	Balance_Init();
	BlueSerial_Init();
	
  OLED_Clear();
  OLED_ShowString(1, 1, "Balance Car");
  OLED_ShowString(2, 1, "Init MPU6050...");
	HAL_Delay(500);
	
	OLED_Clear();
  OLED_ShowString(1, 1, "Balance Ready");
  Serial_Printf("System Ready\r\n");
	HAL_Delay(500);
	OLED_Clear();
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		
		{
        static uint32_t last_bt = 0;
        uint32_t now = HAL_GetTick();
        if (now - last_bt >= 100) {
            last_bt = now;
            BlueSerial_Printf("[D,%.1f,%d,%d,%d]\r\n",
                Angle,                // 当前倾角
                PWML, PWMR,                  // 左右轮PWM
                RunFlag);                    // 运行状态
        }
    }

		
			if (Key_Check(KEY_1, KEY_SINGLE))		//K1按下，启动/停止
			{
				if (RunFlag == 0)
				{
					PID_Init(&AnglePID);
					PID_Init(&SpeedPID);
					PID_Init(&TurnPID);
					Angle = AngleAcc_Filter;
					RunFlag = 1;
				}
				else
				{
					RunFlag = 0;
				}
				if (RunFlagUpdate)
				{
					RunFlagUpdate = 0;
				}
			}
		if (Key_Check(KEY_4, KEY_SINGLE))		//K4按下，进入调试模式
		{
			DebugFlag = 1;
			RunFlag = 0;
			Motor_SetPWM(1, 0);
			Motor_SetPWM(2, 0);
			Key_Clear();
			OLED_Clear ();
			DebugMode();	//执行调试模式函数
		}			
    /* USER CODE BEGIN 3 */
    float pitch =Angle;

    OLED_ShowString(1, 1, "pitch:");
    OLED_ShowFloat(1, 7, pitch, 3, 2);
    OLED_ShowString(3, 1, "Run:");
    OLED_ShowString(3, 6, RunFlag ? "ON " : "OFF");
    OLED_ShowString(4, 1, "PWM:");
    OLED_ShowFloat(4, 6, (float)PWML, 1, 0);
    OLED_ShowString(4, 11, "/");
    OLED_ShowFloat(4, 12, (float)PWMR, 1, 0);
		
		/*蓝牙串口*/
		if (BlueSerial_ReceiveFlag())	//判断是否收到蓝牙数据包
		{
			BlueSerial_Receive();		//收到数据包，接收并解析数据包
			
			/*摇杆数据包，格式为：[joystick,LH,LV,RH,RV]*/
			if (strcmp(BlueSerial_StringArray[0], "joystick") == 0)
			{
				/*得到摇杆数据，转换为数值*/
				int8_t LH = atoi(BlueSerial_StringArray[1]);
				int8_t LV = atoi(BlueSerial_StringArray[2]);
				int8_t RH = atoi(BlueSerial_StringArray[3]);
				int8_t RV = atoi(BlueSerial_StringArray[4]);
				
				/*设定目标速度和目标转向幅度*/
				SpeedPID.Target = LV / 100.0 * SpeedLevel;
				TurnPID.Target = RH / 100.0 * SpeedLevel;
			}
			/*按键数据包，格式为：[key,按键名称,down/up]*/
			else if (strcmp(BlueSerial_StringArray[0], "key") == 0)
			{
				if (strcmp(BlueSerial_StringArray[1], "1") == 0 
				 && strcmp(BlueSerial_StringArray[2], "up") == 0)		//收到数据包[key,1,up]
				{
					Key_Flag[KEY_1] |= KEY_SINGLE;		//触发平衡车K1键按下
				}
				else if (strcmp(BlueSerial_StringArray[1], "2") == 0 
					  && strcmp(BlueSerial_StringArray[2], "up") == 0)	//收到数据包[key,2,up]
				{
					decelerate();
				}
				else if (strcmp(BlueSerial_StringArray[1], "3") == 0 
					  && strcmp(BlueSerial_StringArray[2], "up") == 0)	//收到数据包[key,3,up]
				{
					accelerate();
				}
			}
		}		
  /* USER CODE END 3 */
	}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* 注: HAL_TIM_PeriodElapsedCallback 定义在 balance.c 中,
       不要在此处重复定义 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
