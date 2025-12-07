// // main.c -- ESP-IDF (C)
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <time.h>

// #include "driver/adc.h"
// #include "driver/gpio.h"
// #include "driver/i2c.h"
// #include "esp_log.h"
// #include "esp_timer.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/queue.h"
// #include "freertos/semphr.h"
// #include "freertos/task.h"
// #include "i2c-lcd1602.h"
// #include "nvs.h"
// #include "nvs_flash.h"
// #include "smbus.h"

// static const char* TAG = "game4x4";

// /* === Hardware pins (sesuai kode awalmu) === */
// #define LED_A_0 32  // anoda row0
// #define LED_A_1 2   // anoda row1
// #define LED_A_2 4   // anoda row2
// #define LED_A_3 5   // anoda row3
// #define LED_K_0 33  // katoda col0
// #define LED_K_1 25  // katoda col1
// #define LED_K_2 26  // katoda col2
// #define LED_K_3 27  // katoda col3

// #define JOY_X_PIN ADC_CHANNEL_6  // ADC1_CH6 -> GPIO34
// #define JOY_Y_PIN ADC_CHANNEL_7  // ADC1_CH7 -> GPIO35
// #define JOY_SW_PIN 12            // GPIO12 (input)

// #define BUZZER_PIN 14  // GPIO14

// #define I2C_MASTER_SCL_IO 22
// #define I2C_MASTER_SDA_IO 21
// #define I2C_MASTER_NUM I2C_NUM_0
// #define I2C_MASTER_FREQ_HZ 100000
// #define LCD_ADDR 0x27

// /* === Game shared state === */
// static int8_t bird_pos[2] = {0, 0};  // [row, col]
// static int8_t aim_pos[2] = {1, 1};
// static int score = 0;
// static int highscore = 0;
// static int lives = 3;
// static int8_t isHit = 0;

// static const uint8_t row_pins[4] = {LED_A_0, LED_A_1, LED_A_2, LED_A_3};
// static const uint8_t col_pins[4] = {LED_K_0, LED_K_1, LED_K_2, LED_K_3};

// /* Sync primitives */
// static SemaphoreHandle_t state_mutex;
// static QueueHandle_t buzzer_queue;  // send int beep type: 1=hit,2=miss

// /* LCD objects */
// static smbus_info_t* smbus_info = NULL;
// static i2c_lcd1602_info_t* lcd_info = NULL;

// /* === NVS functions === */
// static void init_nvs_storage(void) {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         nvs_flash_erase();
//         nvs_flash_init();
//     }
// }

// static void save_highscore_nvs(int hs) {
//     nvs_handle_t handle;
//     esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
//     if (err != ESP_OK) {
//         ESP_LOGW(TAG, "nvs_open failed");
//         return;
//     }
//     nvs_set_i32(handle, "highscore", hs);
//     nvs_commit(handle);
//     nvs_close(handle);
//     ESP_LOGI(TAG, "highscore saved %d", hs);
// }

// static int load_highscore_nvs(void) {
//     nvs_handle_t handle;
//     int32_t val = 0;
//     esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
//     if (err != ESP_OK) {
//         ESP_LOGI(TAG, "NVS not initialized yet");
//         return 0;
//     }
//     err = nvs_get_i32(handle, "highscore", &val);
//     if (err == ESP_OK) {
//         ESP_LOGI(TAG, "Highscore loaded %d", val);
//     } else {
//         ESP_LOGI(TAG, "No highscore saved");
//         val = 0;
//     }
//     nvs_close(handle);
//     return val;
// }

// /* === GPIO / peripheral init === */
// static void init_gpio_matrix_and_buzzer(void) {
//     // rows and cols as outputs
//     gpio_config_t io_conf = {0};
//     // outputs: row pins + col pins + buzzer
//     uint64_t bitmask = 0;
//     for (int i = 0; i < 4; i++) bitmask |= (1ULL << row_pins[i]);
//     for (int i = 0; i < 4; i++) bitmask |= (1ULL << col_pins[i]);
//     bitmask |= (1ULL << BUZZER_PIN);

//     io_conf.intr_type = GPIO_INTR_DISABLE;
//     io_conf.mode = GPIO_MODE_OUTPUT;
//     io_conf.pin_bit_mask = bitmask;
//     io_conf.pull_down_en = 0;
//     io_conf.pull_up_en = 0;
//     gpio_config(&io_conf);

//     // set default: all rows LOW, all cols HIGH (so no LED lit)
//     for (int i = 0; i < 4; i++) {
//         gpio_set_level(row_pins[i], 0);
//         gpio_set_level(col_pins[i], 1);
//     }
//     gpio_set_level(BUZZER_PIN, 0);

//     // joystick button input
//     gpio_config_t in_conf = {0};
//     in_conf.intr_type = GPIO_INTR_DISABLE;
//     in_conf.mode = GPIO_MODE_INPUT;
//     in_conf.pin_bit_mask = (1ULL << JOY_SW_PIN);
//     in_conf.pull_up_en = 0;  // user code earlier set pull-down
//     in_conf.pull_down_en = 1;
//     gpio_config(&in_conf);

//     // ADC init
//     adc1_config_width(ADC_WIDTH_BIT_12);
//     adc1_config_channel_atten(JOY_X_PIN, ADC_ATTEN_DB_11);
//     adc1_config_channel_atten(JOY_Y_PIN, ADC_ATTEN_DB_11);
// }

// /* Helper: light single pixel (row, col) on 4x4 matrix
//    assuming row = anoda, col = katoda:
//    set row HIGH, set col LOW to light
// */
// static void set_pixel(int row, int col) {
//     // disable all rows and set cols to idle
//     for (int r = 0; r < 4; r++) gpio_set_level(row_pins[r], 0);
//     for (int c = 0; c < 4; c++) gpio_set_level(col_pins[c], 1);

//     if (row >= 0 && row < 4 && col >= 0 && col < 4) {
//         gpio_set_level(row_pins[row], 1);
//         gpio_set_level(col_pins[col], 0);
//     }
// }

// /* === Tasks === */

// /* display_task: multiplexing rows for smooth LED.
//    runs at ~200Hz overall (each row ~50Hz). */
// static void display_task(void* arg) {
//     const TickType_t delay_per_row = pdMS_TO_TICKS(3);  // ~3ms per row -> ~83Hz refresh/row
//     while (1) {
//         // if isHit show some effect could be added; here show bird and aim alternately quickly
//         if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
//             int bird_r = bird_pos[0], bird_c = bird_pos[1];
//             int aim_r = aim_pos[0], aim_c = aim_pos[1];
//             xSemaphoreGive(state_mutex);

//             // simple multiplex: for each row, decide what col to light (show bird or aim)
//             for (int r = 0; r < 4; r++) {
//                 // disable all rows first
//                 for (int rr = 0; rr < 4; rr++) gpio_set_level(row_pins[rr], 0);
//                 // set all cols to idle (HIGH)
//                 for (int cc = 0; cc < 4; cc++) gpio_set_level(col_pins[cc], 1);

//                 // if bird on this row -> light its column
//                 if (r == bird_r) {
//                     gpio_set_level(col_pins[bird_c], 0);
//                     gpio_set_level(row_pins[r], 1);
//                 }
//                 // also draw aim (we'll briefly override same row)
//                 if (r == aim_r) {
//                     // if same column, it's same; else we flash aim for short time by toggling
//                     gpio_set_level(col_pins[aim_c], 0);
//                     gpio_set_level(row_pins[r], 1);
//                 }

//                 vTaskDelay(delay_per_row);
//             }
//         } else {
//             vTaskDelay(pdMS_TO_TICKS(10));
//         }
//     }
// }

// /* joystick_task: read adc & button, debounce, update aim_pos or notify button press */
// static void joystick_task(void* arg) {
//     const TickType_t poll_delay = pdMS_TO_TICKS(60);
//     uint32_t last_btn_state = 1;  // pulled down, pressed = 0? In your earlier code you treat gpio_get_level==0 as pressed
//     uint32_t stable_count = 0;

//     while (1) {
//         int raw_x = adc1_get_raw(JOY_X_PIN);  // 0..4095
//         int raw_y = adc1_get_raw(JOY_Y_PIN);

//         // translate raw to -1,0,1 for movements using thresholds
//         int dx = 0, dy = 0;
//         if (raw_x < 1200)
//             dx = -1;
//         else if (raw_x > 3000)
//             dx = 1;
//         if (raw_y < 1200)
//             dy = 1;  // note: y orientation may be inverted
//         else if (raw_y > 3000)
//             dy = -1;

//         // update aim_pos with mutex (bounded 0..3)
//         if ((dx != 0) || (dy != 0)) {
//             if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
//                 int8_t new_r = aim_pos[0] + dy;
//                 int8_t new_c = aim_pos[1] + dx;
//                 if (new_r < 0) new_r = 0;
//                 if (new_r > 3) new_r = 3;
//                 if (new_c < 0) new_c = 0;
//                 if (new_c > 3) new_c = 3;
//                 aim_pos[0] = new_r;
//                 aim_pos[1] = new_c;
//                 xSemaphoreGive(state_mutex);
//             }
//         }

//         // button reading & debounce
//         uint32_t cur = gpio_get_level(JOY_SW_PIN);
//         if (cur == last_btn_state) {
//             stable_count++;
//         } else {
//             stable_count = 0;
//             last_btn_state = cur;
//         }
//         if (stable_count > 2) {  // stable for ~3 polls
//             // button pressed if level == 0 (your original code used 0 as pressed)
//             if (cur == 0) {
//                 // notify game: we simply set a flag under mutex
//                 if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
//                     // handle press only once: use isHit? We'll create local flag in state: use score update in game_task
//                     // push an event by setting a sentinel variable, but better use queue. We'll use direct boolean variable:
//                     // To avoid complexity, push an integer to a freeRTOS queue signalling "button pressed"
//                     // Create queue in app_main: buzzer_queue used for buzzer, so create game_event queue? Instead reuse buzzer_queue for beep only.
//                     // We'll create a separate queue for button events via RTOS queue pointer passed via arg (not used here).
//                     xSemaphoreGive(state_mutex);
//                     // For simplicity, create an OS queue for button events globally (below). We'll send 1 for pressed.
//                     // But as we didn't declare that queue globally earlier, let's create one named button_queue globally.
//                 }
//             }
//         }

//         vTaskDelay(poll_delay);
//     }
// }

// /* To avoid complexity above, implement button event detection with falling-edge and a queue.
//    We'll implement a proper joystick_task + button_queue in final code below.
// */

// /* game_task: waits on button_queue; checks aim vs bird and updates score/lives; sends buzzer events */
// static QueueHandle_t button_queue;

// static void game_task(void* arg) {
//     int btn_event;
//     while (1) {
//         if (xQueueReceive(button_queue, &btn_event, pdMS_TO_TICKS(1000))) {
//             // button pressed
//             if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
//                 // check hit
//                 if (aim_pos[0] == bird_pos[0] && aim_pos[1] == bird_pos[1]) {
//                     score++;
//                     if (score > highscore) {
//                         highscore = score;
//                         save_highscore_nvs(highscore);
//                     }
//                     isHit = 1;
//                     // send buzzer hit
//                     int beep = 1;
//                     xQueueSend(buzzer_queue, &beep, 0);
//                     // respawn bird
//                     bird_pos[0] = rand() % 4;
//                     bird_pos[1] = rand() % 4;
//                     isHit = 0;
//                 } else {
//                     lives--;
//                     int beep = 2;
//                     xQueueSend(buzzer_queue, &beep, 0);
//                     if (lives <= 0) {
//                         ESP_LOGI(TAG, "Game Over! Score:%d", score);
//                         score = 0;
//                         lives = 3;
//                     }
//                 }
//                 xSemaphoreGive(state_mutex);
//             }
//         }
//         // small delay to yield CPU
//         vTaskDelay(pdMS_TO_TICKS(20));
//     }
// }

// /* buzzer_task: play simple tones; beep_type: 1=hit (short melody), 2=miss (single short beep) */
// static void buzzer_task(void* arg) {
//     int beep_type;
//     while (1) {
//         if (xQueueReceive(buzzer_queue, &beep_type, portMAX_DELAY)) {
//             if (beep_type == 1) {
//                 // short melody: 3 beeps
//                 for (int i = 0; i < 3; i++) {
//                     gpio_set_level(BUZZER_PIN, 1);
//                     vTaskDelay(pdMS_TO_TICKS(60));
//                     gpio_set_level(BUZZER_PIN, 0);
//                     vTaskDelay(pdMS_TO_TICKS(40));
//                 }
//             } else if (beep_type == 2) {
//                 gpio_set_level(BUZZER_PIN, 1);
//                 vTaskDelay(pdMS_TO_TICKS(150));
//                 gpio_set_level(BUZZER_PIN, 0);
//             }
//         }
//         vTaskDelay(pdMS_TO_TICKS(20));  // yield
//     }
// }

// /* lcd_task: periodically update lcd with score/lives/highscore */
// static void lcd_task(void* arg) {
//     if (!lcd_info) vTaskDelete(NULL);
//     char line1[17], line2[17];
//     while (1) {
//         if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
//             snprintf(line1, sizeof(line1), "Score:%d Lives:%d ", score, lives);
//             snprintf(line2, sizeof(line2), "High:%d          ", highscore);
//             xSemaphoreGive(state_mutex);

//             i2c_lcd1602_move_cursor(lcd_info, 0, 0);
//             i2c_lcd1602_write_string(lcd_info, line1);
//             i2c_lcd1602_move_cursor(lcd_info, 0, 1);
//             i2c_lcd1602_write_string(lcd_info, line2);
//         }
//         vTaskDelay(pdMS_TO_TICKS(300));
//     }
// }

// /* Proper joystick and button implementation (replaced earlier stub) */
// static void joystick_button_task(void* arg) {
//     const TickType_t poll_delay = pdMS_TO_TICKS(80);
//     int prev_btn = 1;
//     int stable = 0;

//     while (1) {
//         int raw_x = adc1_get_raw(JOY_X_PIN);  // 0..4095
//         int raw_y = adc1_get_raw(JOY_Y_PIN);
//         int btn = gpio_get_level(JOY_SW_PIN);  // 0 pressed (as original)

//         int dx = 0, dy = 0;
//         if (raw_x < 1200)
//             dx = -1;
//         else if (raw_x > 3000)
//             dx = 1;
//         if (raw_y < 1200)
//             dy = 1;
//         else if (raw_y > 3000)
//             dy = -1;

//         // move aim with rate limiting: update only when change occurs
//         static int last_dx = 0, last_dy = 0;
//         if (dx != last_dx || dy != last_dy) {
//             if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
//                 int new_r = aim_pos[0] + dy;
//                 int new_c = aim_pos[1] + dx;
//                 if (new_r < 0) new_r = 0;
//                 if (new_r > 3) new_r = 3;
//                 if (new_c < 0) new_c = 0;
//                 if (new_c > 3) new_c = 3;
//                 aim_pos[0] = new_r;
//                 aim_pos[1] = new_c;
//                 xSemaphoreGive(state_mutex);
//             }
//             last_dx = dx;
//             last_dy = dy;
//         }

//         // button debounce and falling edge detect
//         if (btn == prev_btn) {
//             stable++;
//         } else {
//             stable = 0;
//             prev_btn = btn;
//         }
//         if (stable > 1 && btn == 0) {  // stable pressed
//             int evt = 1;
//             xQueueSend(button_queue, &evt, 0);
//             // wait until release to avoid multiple events
//             while (gpio_get_level(JOY_SW_PIN) == 0) vTaskDelay(pdMS_TO_TICKS(20));
//             stable = 0;
//             prev_btn = 1;
//         }

//         vTaskDelay(poll_delay);
//     }
// }

// /* === app_main === */
// void app_main(void) {
//     ESP_LOGI(TAG, "Starting game (ESP-IDF)");

//     init_nvs_storage();
//     highscore = load_highscore_nvs();

//     // create mutex and queues
//     state_mutex = xSemaphoreCreateMutex();
//     buzzer_queue = xQueueCreate(8, sizeof(int));
//     button_queue = xQueueCreate(8, sizeof(int));

//     // init hardware
//     init_gpio_matrix_and_buzzer();

//     // I2C + LCD init (like original)
//     i2c_config_t conf = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = I2C_MASTER_SDA_IO,
//         .scl_io_num = I2C_MASTER_SCL_IO,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = I2C_MASTER_FREQ_HZ};
//     i2c_param_config(I2C_MASTER_NUM, &conf);
//     i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

//     smbus_info = smbus_malloc();
//     smbus_init(smbus_info, I2C_MASTER_NUM, LCD_ADDR);
//     smbus_set_timeout(smbus_info, 1000 / portTICK_PERIOD_MS);

//     lcd_info = i2c_lcd1602_malloc();
//     i2c_lcd1602_init(lcd_info, smbus_info, true, 2, 16, 0);
//     i2c_lcd1602_reset(lcd_info);
//     i2c_lcd1602_set_backlight(lcd_info, true);
//     i2c_lcd1602_clear(lcd_info);

//     // spawn initial bird pos
//     bird_pos[0] = rand() % 4;
//     bird_pos[1] = rand() % 4;

//     // create tasks with priorities (as previously suggested)
//     xTaskCreate(display_task, "display_task", 2048, NULL, 7, NULL);
//     xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 6, NULL);
//     xTaskCreate(game_task, "game_task", 4096, NULL, 5, NULL);
//     xTaskCreate(joystick_button_task, "joy_task", 3072, NULL, 4, NULL);
//     xTaskCreate(lcd_task, "lcd_task", 3072, NULL, 2, NULL);

//     ESP_LOGI(TAG, "Tasks created. Game running.");
// }
