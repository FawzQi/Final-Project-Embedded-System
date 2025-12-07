// #include <stdio.h>

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/i2c.h"
// #include "i2c-lcd1602.h"
// #include "smbus.h"

// #define I2C_MASTER_SCL_IO 22
// #define I2C_MASTER_SDA_IO 21
// #define I2C_MASTER_NUM I2C_NUM_0
// #define I2C_MASTER_FREQ_HZ 100000

// #define LCD_ADDR 0x27  // biasanya 0x27 atau 0x3F

// void app_main(void) {
//     // Konfigurasi I2C
//     i2c_config_t conf = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = I2C_MASTER_SDA_IO,
//         .scl_io_num = I2C_MASTER_SCL_IO,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = I2C_MASTER_FREQ_HZ};
//     i2c_param_config(I2C_MASTER_NUM, &conf);
//     i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

//     // Inisialisasi SMBus
//     smbus_info_t* smbus_info = smbus_malloc();
//     smbus_init(smbus_info, I2C_MASTER_NUM, LCD_ADDR);
//     smbus_set_timeout(smbus_info, 1000 / portTICK_PERIOD_MS);

//     // Inisialisasi LCD
//     i2c_lcd1602_info_t* lcd_info = i2c_lcd1602_malloc();
//     i2c_lcd1602_init(lcd_info, smbus_info, true, 2, 16, 0);  // 0 = default 5x8 dots

//     i2c_lcd1602_reset(lcd_info);
//     i2c_lcd1602_set_backlight(lcd_info, true);
//     i2c_lcd1602_clear(lcd_info);
//     i2c_lcd1602_move_cursor(lcd_info, 0, 0);
//     i2c_lcd1602_write_string(lcd_info, "Hello ESP32!");

//     vTaskDelay(pdMS_TO_TICKS(1000));

//     i2c_lcd1602_move_cursor(lcd_info, 0, 1);
//     i2c_lcd1602_write_string(lcd_info, "LCD 16x2 Ready!");
// }
