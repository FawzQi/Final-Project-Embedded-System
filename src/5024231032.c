// #include <stdio.h>

// #include "driver/adc.h"
// #include "driver/i2c.h"
// #include "esp_timer.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "i2c-lcd1602.h"
// #include "smbus.h"
// #include "soc/gpio_reg.h"
// #include "soc/gpio_struct.h"

// #define LED_A_0 32  // sisi anoda LED
// #define LED_A_1 2   // sisi anoda LED
// #define LED_A_2 4   // sisi anoda LED
// #define LED_A_3 5   // sisi anoda LED
// #define LED_K_0 33  // sisi katoda LED
// #define LED_K_1 25  // sisi katoda LED
// #define LED_K_2 26  // sisi katoda LED
// #define LED_K_3 27  // sisi katoda LED
// // #define BUTTON_PIN 15

// #define JOY_X_PIN ADC_CHANNEL_6  // GPIO34
// #define JOY_Y_PIN ADC_CHANNEL_7  // GPIO35
// #define JOY_SW_PIN 15            // GPIO14

// #define I2C_MASTER_SCL_IO 22
// #define I2C_MASTER_SDA_IO 21
// #define I2C_MASTER_NUM I2C_NUM_0
// #define I2C_MASTER_FREQ_HZ 100000

// #define LCD_ADDR 0x27  // biasanya 0x27 atau 0x3F

// void setup_lcd1602() {
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
// }

// void setup_joystick() {
//     // Konfigurasi ADC
//     adc1_config_width(ADC_WIDTH_BIT_12);
//     adc1_config_channel_atten(JOY_X_PIN, ADC_ATTEN_DB_11);
//     adc1_config_channel_atten(JOY_Y_PIN, ADC_ATTEN_DB_11);

//     // Konfigurasi switch
//     gpio_config_t io_conf = {
//         .pin_bit_mask = 1ULL << JOY_SW_PIN,
//         .mode = GPIO_MODE_INPUT,
//         .pull_up_en = GPIO_PULLUP_ENABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE,
//     };
//     gpio_config(&io_conf);
// }

// int8_t pos[2] = {0, 0};
// uint8_t row_leds[4] = {LED_A_0, LED_A_1, LED_A_2, LED_A_3};
// uint8_t col_leds[4] = {LED_K_0, LED_K_1, LED_K_2, LED_K_3};

// void setup_led_matrix() {
//     GPIO.enable_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3) |
//                        (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
//     GPIO.enable1_w1ts.val = (1 << (LED_A_0 - 32)) | (1 << (LED_K_0 - 32));

//     GPIO.out_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3) |
//                     (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
//     GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32)) | (1 << (LED_K_0 - 32));
// }

// void enable_led(int row, int col) {
//     if (row < 0 || row > 3 || col < 0 || col > 3) return;

//     // aktifkan row
//     if (row_leds[row] < 32) {
//         GPIO.out_w1ts = (1 << row_leds[row]);
//     } else {
//         GPIO.out1_w1ts.val = (1 << (row_leds[row] - 32));
//     }

//     // aktifkan col
//     if (col_leds[col] < 32) {
//         GPIO.out_w1tc = (1 << col_leds[col]);
//     } else {
//         GPIO.out1_w1tc.val = (1 << (col_leds[col] - 32));
//     }

//     // matikan row/col lain
//     for (uint8_t i = 0; i < 4; i++) {
//         if (i != row) {
//             if (row_leds[i] < 32) {
//                 GPIO.out_w1tc = (1 << row_leds[i]);
//             } else {
//                 GPIO.out1_w1tc.val = (1 << (row_leds[i] - 32));
//             }
//         }
//         if (i != col) {
//             if (col_leds[i] < 32) {
//                 GPIO.out_w1ts = (1 << col_leds[i]);
//             } else {
//                 GPIO.out1_w1ts.val = (1 << (col_leds[i] - 32));
//             }
//         }
//     }
// }
// void enable_row(int row) {
//     if (row < 0 || row > 3) return;

//     if (row == 0) {
//         GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));
//         GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
//         GPIO.out_w1tc = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
//         GPIO.out_w1tc = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
//     } else if (row == 1) {
//         GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));
//         GPIO.out_w1tc = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
//         GPIO.out1_w1tc.val = (1 << (LED_A_0 - 32));
//         GPIO.out_w1ts = (1 << LED_A_1);
//         GPIO.out_w1tc = (1 << LED_A_2) | (1 << LED_A_3);
//     } else if (row == 2) {
//         GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));
//         GPIO.out_w1tc = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
//         GPIO.out1_w1tc.val = (1 << (LED_A_0 - 32));
//         GPIO.out_w1ts = (1 << LED_A_2);
//         GPIO.out_w1tc = (1 << LED_A_1) | (1 << LED_A_3);
//     } else if (row == 3) {
//         GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));
//         GPIO.out_w1tc = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
//         GPIO.out1_w1tc.val = (1 << (LED_A_0 - 32));
//         GPIO.out_w1ts = (1 << LED_A_3);
//         GPIO.out_w1tc = (1 << LED_A_2) | (1 << LED_A_1);
//     }
// }

// void enable_col(int col) {
//     if (col < 0 || col > 3) return;

//     if (col == 0) {
//         GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
//         GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));
//         GPIO.out_w1ts = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
//         GPIO.out_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
//     } else if (col == 1) {
//         GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
//         GPIO.out1_w1ts.val = (1 << (LED_K_0 - 32));
//         GPIO.out_w1tc = (1 << LED_K_1);
//         GPIO.out_w1ts = (1 << LED_K_2) | (1 << LED_K_3);
//         GPIO.out_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
//     } else if (col == 2) {
//         GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
//         GPIO.out1_w1ts.val = (1 << (LED_K_0 - 32));
//         GPIO.out_w1tc = (1 << LED_K_2);
//         GPIO.out_w1ts = (1 << LED_K_1) | (1 << LED_K_3);
//         GPIO.out_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
//     } else if (col == 3) {
//         GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
//         GPIO.out1_w1ts.val = (1 << (LED_K_0 - 32));
//         GPIO.out_w1tc = (1 << LED_K_3);
//         GPIO.out_w1ts = (1 << LED_K_2) | (1 << LED_K_1);
//         GPIO.out_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
//     }
// }
// // Task untuk tombol
// void button_task(void* pv) {
//     while (1) {
//         int sw_val = gpio_get_level(JOY_SW_PIN);
//         // (((GPIO.in >> BUTTON_PIN) & 0x1) == 0)
//         if (sw_val == 0 && ((pos[0] == 1 && pos[1] == 1))) {
//             for (int i = 0; i < 5; i++) {
//                 enable_row(0);
//                 esp_rom_delay_us(100000);
//                 enable_row(1);
//                 esp_rom_delay_us(100000);
//                 enable_row(2);
//                 esp_rom_delay_us(100000);
//                 enable_row(3);
//                 esp_rom_delay_us(100000);
//             }

//             pos[0] = 3;
//             pos[1] = 3;
//             enable_led(pos[0], pos[1]);
//             printf("Tombol ditekan\n");
//             vTaskDelay(pdMS_TO_TICKS(200));  // debounce 200ms
//         }
//         vTaskDelay(pdMS_TO_TICKS(20));  // polling tiap 20ms
//     }
// }

// // Task untuk LED random
// void led_task(void* pv) {
//     while (1) {
//         int dir = rand() % 4;
//         switch (dir) {
//             case 0:
//                 if (pos[0] + 1 <= 3)
//                     pos[0]++;
//                 else
//                     pos[0] = 0;
//                 break;
//             case 1:
//                 if (pos[0] - 1 >= 0)
//                     pos[0]--;
//                 else
//                     pos[0] = 3;
//                 break;
//             case 2:
//                 if (pos[1] + 1 <= 3)
//                     pos[1]++;
//                 else
//                     pos[1] = 0;
//                 break;
//             case 3:
//                 if (pos[1] - 1 >= 0)
//                     pos[1]--;
//                 else
//                     pos[1] = 3;
//                 break;
//         }

//         enable_led(pos[0], pos[1]);
//         vTaskDelay(pdMS_TO_TICKS(400));  // pindah tiap 1ms
//     }
// }

// void app_main(void) {
//     setup_led_matrix();
//     setup_joystick();
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

//     // Set button sebagai input
//     // GPIO.enable_w1tc = (1 << BUTTON_PIN);

//     // Buat task
//     xTaskCreate(button_task, "button_task", 2048, NULL, 1, NULL);
//     xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);
//     while (1) {
//         i2c_lcd1602_clear(lcd_info);
//         int x_val = adc1_get_raw(JOY_X_PIN);
//         int y_val = adc1_get_raw(JOY_Y_PIN);
//         int sw_val = gpio_get_level(JOY_SW_PIN);
//         printf("X: %4d | Y: %4d | SW: %s\n",
//                x_val, y_val, (sw_val == 0) ? "Pressed" : "Released");

//         char buf[7];
//         static char bufx[7] = "";
//         static char bufy[7] = "";
//         sprintf(buf, "%4d", x_val);
//         if (strcmp(bufx, buf) != 0) {
//             strcpy(bufx, buf);
//             i2c_lcd1602_move_cursor(lcd_info, 0, 0);
//             i2c_lcd1602_write_string(lcd_info, "X:");
//             i2c_lcd1602_write_string(lcd_info, buf);
//         }
//         sprintf(buf, "%4d", y_val);
//         if (strcmp(bufy, buf) != 0) {
//             strcpy(bufy, buf);
//             i2c_lcd1602_move_cursor(lcd_info, 0, 1);
//             i2c_lcd1602_write_string(lcd_info, "Y:");
//             i2c_lcd1602_write_string(lcd_info, buf);
//         }
//         vTaskDelay(pdMS_TO_TICKS(10));
//     }
// }
