#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "uart_com.h"
#include "usart.h"
#include "i2c.h"
#include "test_command.h"
#include "usb_com.h"

void uart_send_string(const char *str)
{
    HAL_UART_Transmit(&huart4, (uint8_t*)str, strlen(str), 1000);
}

void test_command_handler(void)
{
    static char input_string[1024];
    static int ofs;
    uint8_t ch;

    /* 若有数据则一次性处理完 */
    while (usb_read_from_rb(&ch, 1) > 0)
    {
        if(ch != '\n')
        {
            input_string[ofs++] = ch;
            if(ofs >= sizeof(input_string) - 1)
                ofs = 0;
            continue;
        }

        input_string[ofs] = '\0';
        test_command(input_string);
        ofs = 0;
        printf("\n#sh ");
    }
}

int app_main(void)
{
    uart_com_init();
		usb_com_init();

    printf("\n#sh ");

    while (1)
    {
        test_command_handler();

        HAL_Delay(10);
    }
}

#define MY_I2C_HANDLE &hi2c4

static int i2cget(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <device_address> <register_address>\n", argv[0]);
        return -1;
    }

    // 提取参数
    uint8_t device_address = (uint8_t)strtol(argv[1], NULL, 0);   // 设备地址
    uint8_t register_address = (uint8_t)strtol(argv[2], NULL, 0); // 寄存器地址

    // 发送寄存器地址
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(MY_I2C_HANDLE, device_address << 1, &register_address, 1, 10);
    if (ret != HAL_OK)
    {
        printf("Error accessing device 0x%02X at register 0x%02X\n", device_address, register_address);
        return -1;
    }

    // 读取寄存器内容
    uint8_t data;
    ret = HAL_I2C_Master_Receive(MY_I2C_HANDLE, device_address << 1, &data, 1, 10);
    if (ret != HAL_OK)
    {
        printf("Error reading from device 0x%02X at register 0x%02X\n", device_address, register_address);
        return -1;
    }

    printf("Read 0x%02X from register 0x%02X of device 0x%02X\n", data, register_address, device_address);
    return 0;
}
EXPORT_TEST_COMMAND(i2cget, "i2cget", "i2cget");

static int i2cset(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Usage: %s <device_address> <register_address> <data>\n", argv[0]);
        return -1;
    }

    // 提取参数
    uint8_t device_address = (uint8_t)strtol(argv[1], NULL, 0);   // 设备地址
    uint8_t register_address = (uint8_t)strtol(argv[2], NULL, 0); // 寄存器地址
    uint8_t data = (uint8_t)strtol(argv[3], NULL, 0);             // 要写入的数据

    // 准备数据缓冲区
    uint8_t buffer[2] = {register_address, data};

    // 进行 I2C 写入操作
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(MY_I2C_HANDLE, device_address << 1, buffer, sizeof(buffer), 10);
    if (ret != HAL_OK)
    {
        printf("Error writing to device 0x%02X at register 0x%02X\n", device_address, register_address);
        return -1;
    }

    printf("Successfully wrote 0x%02X to register 0x%02X of device 0x%02X\n", data, register_address, device_address);
    return 0;
}
EXPORT_TEST_COMMAND(i2cset, "i2cset", "i2cset");

static int i2cdetect(int argc, char *argv[])
{
    uint8_t devices[128] = {0};

    // 扫描 I2C 地址
    for (int i = 1; i < 128; i++)
    {
        uint8_t reg_addr = 0;
        HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(MY_I2C_HANDLE, (i << 1) + 1, &reg_addr, 1, 100);
        if (ret == HAL_OK) 
            devices[i] = i;
    }

    // 打印设备地址矩阵
    printf("    ");
    for (int col = 0; col < 16; col++)
    {
        printf("%2x ", col);
    }
    printf("\n");

    // 打印行标号和设备地址
    for (int row = 0; row < 8; row++)
    {
        printf("%02x: ", row * 16);
        for (int col = 0; col < 16; col++)
        {
            int index = row * 16 + col;
            if (devices[index] != 0)
                printf("%02x ", devices[index]);
            else
                printf("-- ");
        }
        printf("\n");
    }

    return 0;
}
EXPORT_TEST_COMMAND(i2cdetect, "i2cdetect", "i2cdetect");

static int i2cdump(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <device_address>\n", argv[0]);
        return -1;
    }

    // 提取参数
    uint8_t device_address = (uint8_t)strtol(argv[1], NULL, 0); // 设备地址

    // 打印表头
    printf("    ");
    for (int i = 0; i < 16; i++)
        printf("%2x ", i);
    printf("\n");

    // 扫描寄存器并打印内容
    // 行
    for (int i = 0; i < 16; i++)
    {
        printf("%02x: ", 16 * i);
        uint8_t line_data[16];

        // 列
        for (int j = 0; j < 16; j++)
        {
            uint8_t reg_addr = i * 16 + j;
            HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(MY_I2C_HANDLE, device_address << 1, &reg_addr, 1, 10);
            if (ret != HAL_OK)
            {
                line_data[j] = 0;
                printf("xx ");
                continue;
            }

            uint8_t data;
            ret = HAL_I2C_Master_Receive(MY_I2C_HANDLE, (device_address << 1) + 1, &data, 1, 10);
            if (ret != HAL_OK)
            {
                printf("xx ");
                line_data[j] = 0;
            }

            else
            {
                printf("%02x ", data);
                line_data[j] = data;
            }
        }

        // 打印 ASCII 字符
        printf("   ");
        for (int j = 0; j < 16; j++)
        {
            char data = (char)line_data[j];
            printf("%c", data >= 32 && data <= 126 ? data : '.');
        }
        printf("\n"); // 换行
    }

    return 0;
}
EXPORT_TEST_COMMAND(i2cdump, "i2cdump", "i2cdump");
