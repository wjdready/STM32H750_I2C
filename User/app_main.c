#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "uart_com.h"
#include "usart.h"
#include "i2c.h"
#include "test_command.h"

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
    while (uart_read_from_rb(&huart4, &ch, 1) > 0)
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
    printf("\n#sh ");


    while (1)
    {
        test_command_handler();

        HAL_Delay(10);
    }
}


static int write_i2c(int argc, char *argv[])
{

    if (argc > 2)
    {
        char *endptr1, *endptr2;
        uint8_t reg_addr = strtol(argv[1], &endptr1, 16);
        uint8_t data = strtol(argv[2], &endptr2, 16);

        if (*endptr1 != '\0' || *endptr2 != '\0')
        {
            printf("输入错误: 参数不是十六进制\n");
            return -1;
        }

        uint8_t data_buf[] = {reg_addr, data};
        int ret1 = HAL_I2C_Master_Transmit(&hi2c3, 0x22, data_buf, 2, 100);

        printf("write reg_addr: %02X value: %02X ret1 %d\n", reg_addr, data, ret1);
    }

    else
    {
        printf("输入错误: example: %s 0x00 0xf1\n", argv[0]);
    }

    return 0;
}
EXPORT_TEST_COMMAND(write_i2c, "wr", "写入 i2c");

static int read_all(int argc, char *argv[])
{
    for (int i = 0; i < 8; i ++)
    {
        uint8_t reg_addr = i;
        uint8_t data;

        int ret1 = HAL_I2C_Master_Transmit(&hi2c3, 0x22, &reg_addr, 1, 100);
        int ret2 = HAL_I2C_Master_Receive(&hi2c3, 0x22 + 1, &data, 1, 100);

        printf("read reg_addr: %02X value: %02X ret1 %d ret2 %d\n", reg_addr, data, ret1, ret2);
    }

    return 0;
}
EXPORT_TEST_COMMAND(read_all, "rd", "读取所有寄存器");

