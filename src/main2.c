// #include <stdint.h>
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

// #define JOY_X_PIN ADC_CHANNEL_6  // GPIO34
// #define JOY_Y_PIN ADC_CHANNEL_7  // GPIO35
// #define JOY_SW_PIN 15            // GPIO14

// #define I2C_MASTER_SCL_IO 22
// #define I2C_MASTER_SDA_IO 21
// #define I2C_MASTER_NUM I2C_NUM_0
// #define I2C_MASTER_FREQ_HZ 100000

// #define LCD_ADDR 0x27  // biasanya 0x27 atau 0x3F

// // void setup_lcd1602() {
// //     // Konfigurasi I2C
// //     i2c_config_t conf = {
// //         .mode = I2C_MODE_MASTER,
// //         .sda_io_num = I2C_MASTER_SDA_IO,
// //         .scl_io_num = I2C_MASTER_SCL_IO,
// //         .sda_pullup_en = GPIO_PULLUP_ENABLE,
// //         .scl_pullup_en = GPIO_PULLUP_ENABLE,
// //         .master.clk_speed = I2C_MASTER_FREQ_HZ};
// //     i2c_param_config(I2C_MASTER_NUM, &conf);
// //     i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

// //     // Inisialisasi SMBus
// //     smbus_info_t* smbus_info = smbus_malloc();
// //     smbus_init(smbus_info, I2C_MASTER_NUM, LCD_ADDR);
// //     smbus_set_timeout(smbus_info, 1000 / portTICK_PERIOD_MS);

// //     // Inisialisasi LCD
// //     i2c_lcd1602_info_t* lcd_info = i2c_lcd1602_malloc();
// //     i2c_lcd1602_init(lcd_info, smbus_info, true, 2, 16, 0);  // 0 = default 5x8 dots

// //     i2c_lcd1602_reset(lcd_info);
// //     i2c_lcd1602_set_backlight(lcd_info, true);
// //     i2c_lcd1602_clear(lcd_info);
// //     i2c_lcd1602_move_cursor(lcd_info, 0, 0);
// //     i2c_lcd1602_write_string(lcd_info, "Hello ESP32!");
// // }

// int8_t bird_pos[2] = {0, 0};
// int8_t aim_pos[2] = {1, 1};
// uint8_t row_leds[4] = {LED_A_0, LED_A_1, LED_A_2, LED_A_3};
// uint8_t col_leds[4] = {LED_K_0, LED_K_1, LED_K_2, LED_K_3};
// int x_val = 0;
// int y_val = 0;
// int8_t isHit = 0;

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

// long millis() {
//     return esp_timer_get_time() / 1000;
// }

// void setup_led_matrix() {
//     // Gabungkan semua pin LED
//     uint64_t mask = 0;
//     uint8_t all_leds[8] = {LED_A_0, LED_A_1, LED_A_2, LED_A_3,
//                            LED_K_0, LED_K_1, LED_K_2, LED_K_3};

//     for (int i = 0; i < 8; i++) {
//         mask |= (1ULL << all_leds[i]);
//     }

//     gpio_config_t io_conf = {
//         .pin_bit_mask = mask,
//         .mode = GPIO_MODE_OUTPUT,
//         .pull_up_en = GPIO_PULLUP_DISABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE,
//     };

//     gpio_config(&io_conf);

//     // Default semua OFF
//     for (int i = 0; i < 4; i++) {
//         gpio_set_level(row_leds[i], 0);  // anoda 0
//         gpio_set_level(col_leds[i], 1);  // katoda 1
//     }
// }

// void enable_led(int row, int col) {
//     if (row < 0 || row > 3 || col < 0 || col > 3) return;

//     // Matikan semua row
//     for (int i = 0; i < 4; i++) {
//         gpio_set_level(row_leds[i], 0);
//     }

//     // Matikan semua column
//     for (int i = 0; i < 4; i++) {
//         gpio_set_level(col_leds[i], 1);
//     }

//     // Aktifkan row (anoda HIGH)
//     gpio_set_level(row_leds[row], 1);

//     // Aktifkan col (katoda LOW)
//     gpio_set_level(col_leds[col], 0);
// }

// void enable_row(int row) {
//     if (row < 0 || row > 3) return;

//     // Matikan semua row & col
//     for (int i = 0; i < 4; i++) {
//         gpio_set_level(row_leds[i], 0);
//         gpio_set_level(col_leds[i], 1);
//     }

//     // Aktifkan row saja
//     gpio_set_level(row_leds[row], 1);
// }

// void enable_col(int col) {
//     if (col < 0 || col > 3) return;

//     // Matikan semua row & col
//     for (int i = 0; i < 4; i++) {
//         gpio_set_level(row_leds[i], 0);
//         gpio_set_level(col_leds[i], 1);
//     }

//     // Aktifkan kolom
//     gpio_set_level(col_leds[col], 0);
// }

// // Task untuk tombol
// void button_task(void* pv) {
//     while (1) {
//         int sw_val = gpio_get_level(JOY_SW_PIN);
//         int x_val = adc1_get_raw(JOY_X_PIN);
//         int y_val = adc1_get_raw(JOY_Y_PIN);
//         static long last_press_time;
//         if (millis() - last_press_time > 300) {
//             if (x_val < 1300) {
//                 aim_pos[1] -= 1;
//                 if (aim_pos[1] < 0) aim_pos[1] = 0;
//             } else if (x_val > 2500) {
//                 aim_pos[1] += 1;
//                 if (aim_pos[1] > 3) aim_pos[1] = 3;
//             }
//             if (y_val > 2500) {
//                 aim_pos[0] -= 1;
//                 if (aim_pos[0] < 0) aim_pos[0] = 0;
//             } else if (y_val < 1300) {
//                 aim_pos[0] += 1;
//                 if (aim_pos[0] > 3) aim_pos[0] = 3;
//             }
//             last_press_time = millis();
//         }

//         // (((GPIO.in >> BUTTON_PIN) & 0x1) == 0)
//         if (sw_val == 0 && ((bird_pos[0] == aim_pos[0] && bird_pos[1] == aim_pos[1]))) {
//             isHit = 1;
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

//             bird_pos[0] = rand() % 4;
//             bird_pos[1] = rand() % 4;
//             isHit = 0;
//             enable_led(bird_pos[0], bird_pos[1]);
//             printf("Tombol ditekan\n");
//             vTaskDelay(pdMS_TO_TICKS(200));  // debounce 200ms
//         }
//         vTaskDelay(pdMS_TO_TICKS(20));  // polling tiap 20ms
//     }
// }

// // Task untuk LED random
// void bird_task(void* pv) {
//     while (1) {
//         if (isHit) continue;
//         int dir = rand() % 4;
//         switch (dir) {
//             case 0:
//                 if (bird_pos[0] + 1 <= 3)
//                     bird_pos[0]++;
//                 else
//                     bird_pos[0] = 0;
//                 break;
//             case 1:
//                 if (bird_pos[0] - 1 >= 0)
//                     bird_pos[0]--;
//                 else
//                     bird_pos[0] = 3;
//                 break;
//             case 2:
//                 if (bird_pos[1] + 1 <= 3)
//                     bird_pos[1]++;
//                 else
//                     bird_pos[1] = 0;
//                 break;
//             case 3:
//                 if (bird_pos[1] - 1 >= 0)
//                     bird_pos[1]--;
//                 else
//                     bird_pos[1] = 3;
//                 break;
//         }
//         vTaskDelay(pdMS_TO_TICKS(400));  // pindah tiap 1ms
//     }
// }

// void led_task(void* pv) {
//     while (1) {
//         if (isHit) {
//             vTaskDelay(pdMS_TO_TICKS(20));
//             continue;
//         }

//         enable_led(bird_pos[0], bird_pos[1]);
//         vTaskDelay(pdMS_TO_TICKS(20));
//         enable_led(aim_pos[0], aim_pos[1]);
//         vTaskDelay(pdMS_TO_TICKS(10));
//     }
// }

// void app_main(void) {
//     setup_led_matrix();
//     setup_joystick();

//     // I2C LCD
//     i2c_config_t conf = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = I2C_MASTER_SDA_IO,
//         .scl_io_num = I2C_MASTER_SCL_IO,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = I2C_MASTER_FREQ_HZ};
//     i2c_param_config(I2C_MASTER_NUM, &conf);
//     i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

//     smbus_info_t* smbus_info = smbus_malloc();
//     smbus_init(smbus_info, I2C_MASTER_NUM, LCD_ADDR);

//     i2c_lcd1602_info_t* lcd_info = i2c_lcd1602_malloc();
//     i2c_lcd1602_init(lcd_info, smbus_info, true, 2, 16, 0);

//     i2c_lcd1602_clear(lcd_info);
//     i2c_lcd1602_write_string(lcd_info, "Hello ESP32!");

//     // Task
//     xTaskCreate(button_task, "button_task", 2048, NULL, 1, NULL);
//     xTaskCreate(bird_task, "bird_task", 2048, NULL, 5, NULL);
//     xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);

//     while (1) {
//         int sw_val = gpio_get_level(JOY_SW_PIN);

//         char buf[16];
//         sprintf(buf, "X:%d Y:%d", aim_pos[0], aim_pos[1]);

//         i2c_lcd1602_clear(lcd_info);
//         i2c_lcd1602_write_string(lcd_info, buf);

//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }
