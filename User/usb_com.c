
#if 1

#include "lwrb.h"
#include "usbd_cdc_if.h"
static lwrb_t rb;

// 将 USB 导出到 stdio
#define USB_BIND_STDIO

void usb_com_init(void)
{
    static uint8_t rx_buffer[512];
    lwrb_init(&rb, rx_buffer, sizeof(rx_buffer));
}

/**
 * \brief 处理 USB 数据
 *        当 USB 接收到数据时, 调用该函数将数据缓存到环形缓冲区
 *        需要将该函数放到 usb 接收回调
 * \param dat 处理的数据
 * \param len 数据的长度
 */
void usb_process_data(uint8_t *dat, size_t len)
{
    lwrb_write(&rb, dat, len);
}

/**
 * @brief 直接向 USB 写入数据, 不要在中断中调用
 * 
 * @param dat 数据内容
 * @param len 长度
 * @param timeout_ms 超时
 * @return 0 成功, 其他: 失败
 */
int usb_write(void *dat, size_t len, int timeout_ms)
{
    int time_cout = 0;
    while (time_cout < timeout_ms && CDC_Transmit_FS(dat, len) != 0)
    {
        time_cout++;
        HAL_Delay(1);
    }

    if (timeout_ms != 0 && time_cout > timeout_ms)
        return -1;

    return 0;
}

/**
 * @brief 从 USB 接收缓冲区读数据
 * 
 * @param dat 接收的数据
 * @param len 期望数据接收长度
 * 
 * @return 实际接收的数据量
 */
int usb_read_from_rb(uint8_t *dat, size_t len)
{
    return lwrb_read(&rb, dat, len);
}

/* -------------------- 将串口绑定到标准输入输出 -------------------------- */

#ifdef USB_BIND_STDIO

#include <stdio.h>

int fputc(int ch, FILE *fd)
{
   uint8_t dat = ch;
   if (fd == stdout)
       usb_write(&dat, 1, 10);

   return ch;
}

int fgetc(FILE *fd)
{
   uint8_t ch;
   while (fd == stdin && usb_read_from_rb(&ch, 1) <= 0)
       ;

   return ch;
}

#endif

#endif  // ENABLE Module

