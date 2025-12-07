#include <stdint.h>
#include <stdio.h>

#include "driver/adc.h"
#include "driver/i2c.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c-lcd1602.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "smbus.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"

#define LED_A_0 32  // sisi anoda LED
#define LED_A_1 2   // sisi anoda LED
#define LED_A_2 4   // sisi anoda LED
#define LED_A_3 5   // sisi anoda LED
#define LED_K_0 33  // sisi katoda LED
#define LED_K_1 25  // sisi katoda LED
#define LED_K_2 26  // sisi katoda LED
#define LED_K_3 27  // sisi katoda LED

#define JOY_X_PIN ADC_CHANNEL_6  // GPIO34
#define JOY_Y_PIN ADC_CHANNEL_7  // GPIO35
#define JOY_SW_PIN 12            // GPIO12

#define BUZZER_PIN 14  // GPIO14

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

#define BIRD_GAME 0
#define SNAKE_GAME 1

#define LCD_ADDR 0x27  // biasanya 0x27 atau 0x3F

int mode = BIRD_GAME;
int bird_highscore = 0;
int button_counter = 0;

int body_snake[8][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
int length_snake = 0;
int direction_snake = 0;  // 0=up,1=down,2=left,3=right
int food_pos[2];
int food_eaten = 0;
int snake_highscore = 0;
int snake_dead = 0;

void init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

void save_highscore(int score, int game_mode) {
    nvs_handle_t handle;
    esp_err_t err;

    // buka NVS namespace 'storage'
    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        printf("Error membuka NVS\n");
        return;
    }

    if (game_mode == BIRD_GAME) {  // tulis value
        err = nvs_set_i32(handle, "highscore", score);
        if (err != ESP_OK) {
            printf("Error menulis highscore\n");
            nvs_close(handle);
            return;
        }
    } else if (game_mode == SNAKE_GAME) {
        err = nvs_set_i32(handle, "highscore_snake", score);
        if (err != ESP_OK) {
            printf("Error menulis highscore\n");
            nvs_close(handle);
            return;
        }
    }

    // commit wajib!
    nvs_commit(handle);

    nvs_close(handle);
    printf("High score %d berhasil disimpan!\n", score);
}

int load_highscore(int game_mode) {
    nvs_handle_t handle;
    esp_err_t err;
    int32_t score = 0;

    // buka NVS
    err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        printf("NVS belum diinisialisasi\n");
        return 0;
    }

    // baca nilai (default = 0)
    if (game_mode == BIRD_GAME) {
        err = nvs_get_i32(handle, "highscore", &score);
        switch (err) {
            case ESP_OK:
                printf("High score tersimpan");
                break;
            case ESP_ERR_NOT_FOUND:
                printf("Belum ada high score\n");
                score = 0;
                break;
            default:
                printf("Error membaca highscore\n");
        }
    } else if (game_mode == SNAKE_GAME) {
        err = nvs_get_i32(handle, "highscore_snake", &score);
        switch (err) {
            case ESP_OK:
                printf("High score tersimpan");
                break;
            case ESP_ERR_NOT_FOUND:
                printf("Belum ada high score\n");
                score = 0;
                break;
            default:
                printf("Error membaca highscore\n");
        }
    }

    nvs_close(handle);
    return score;
}

int8_t bird_pos[2] = {0, 0};
int8_t aim_pos[2] = {1, 1};
uint8_t row_leds[4] = {LED_A_0, LED_A_1, LED_A_2, LED_A_3};
uint8_t col_leds[4] = {LED_K_0, LED_K_1, LED_K_2, LED_K_3};
int x_val = 0;
int y_val = 0;
int8_t isHit = 0;

int nyawa = 3;
int bird_score = 0;
int bird_speed = 1000;  // ms

void setup_joystick() {
    // Konfigurasi ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(JOY_X_PIN, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(JOY_Y_PIN, ADC_ATTEN_DB_11);

    // Konfigurasi switch
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << JOY_SW_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_config_t boot_conf = {
        .pin_bit_mask = 1ULL << 0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&boot_conf);
}

long millis() {
    return esp_timer_get_time() / 1000;
}

void setup_led_matrix() {
    GPIO.enable_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3) |
                       (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
    GPIO.enable1_w1ts.val = (1 << (LED_A_0 - 32)) | (1 << (LED_K_0 - 32));

    GPIO.out_w1ts = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
    GPIO.out1_w1ts.val = (1 << (LED_K_0 - 32));

    GPIO.out_w1tc = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
    GPIO.out1_w1tc.val = (1 << (LED_A_0 - 32));
}

int buzzer_state = 0;

void buzzer_task(void* pv) {
    // setup buzzer
    gpio_config_t buzzer_conf = {
        .pin_bit_mask = 1ULL << BUZZER_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&buzzer_conf);

    GPIO.out_w1tc = 1 << BUZZER_PIN;

    while (1) {
        if (mode == BIRD_GAME || mode == SNAKE_GAME) {
            if (buzzer_state == 0) {
                GPIO.out_w1tc = 1 << BUZZER_PIN;
            } else if (buzzer_state == 1) {
                GPIO.out_w1ts = 1 << BUZZER_PIN;
                vTaskDelay(pdMS_TO_TICKS(150));
                buzzer_state = 0;
            } else if (buzzer_state == 2) {
                for (int i = 0; i < 6; i++) {
                    GPIO.out_w1ts = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(25));
                    GPIO.out_w1tc = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(75));
                }
                for (int i = 0; i < 6; i++) {
                    GPIO.out_w1ts = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(50));
                    GPIO.out_w1tc = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                for (int i = 0; i < 6; i++) {
                    GPIO.out_w1ts = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(75));
                    GPIO.out_w1tc = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(25));
                }
                for (int i = 0; i < 6; i++) {
                    GPIO.out_w1ts = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                buzzer_state = 0;
            } else if (buzzer_state == 3) {
                for (int i = 0; i < 6; i++) {
                    GPIO.out_w1ts = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                for (int i = 0; i < 6; i++) {
                    GPIO.out_w1ts = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(75));
                    GPIO.out_w1tc = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(25));
                }
                for (int i = 0; i < 6; i++) {
                    GPIO.out_w1ts = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(50));
                    GPIO.out_w1tc = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                for (int i = 0; i < 6; i++) {
                    GPIO.out_w1ts = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(25));
                    GPIO.out_w1tc = 1 << BUZZER_PIN;
                    vTaskDelay(pdMS_TO_TICKS(75));
                }
                buzzer_state = 0;
            } else if (buzzer_state == 4) {
                for (int j = 0; j < 2; j++) {
                    for (int i = 0; i < 8; i++) {
                        GPIO.out_w1ts = 1 << BUZZER_PIN;
                        vTaskDelay(pdMS_TO_TICKS(25));
                        GPIO.out_w1tc = 1 << BUZZER_PIN;
                        vTaskDelay(pdMS_TO_TICKS(25));
                    }
                    for (int i = 0; i < 8; i++) {
                        GPIO.out_w1ts = 1 << BUZZER_PIN;
                        vTaskDelay(pdMS_TO_TICKS(50));
                        GPIO.out_w1tc = 1 << BUZZER_PIN;
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    for (int i = 0; i < 8; i++) {
                        GPIO.out_w1ts = 1 << BUZZER_PIN;
                        vTaskDelay(pdMS_TO_TICKS(20));
                        GPIO.out_w1tc = 1 << BUZZER_PIN;
                        vTaskDelay(pdMS_TO_TICKS(20));
                    }
                    for (int i = 0; i < 8; i++) {
                        GPIO.out_w1ts = 1 << BUZZER_PIN;
                        vTaskDelay(pdMS_TO_TICKS(50));
                        GPIO.out_w1tc = 1 << BUZZER_PIN;
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                }
                buzzer_state = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void enable_led(int row, int col) {
    if (row < 0 || row > 3 || col < 0 || col > 3) return;

    // aktifkan row
    if (row_leds[row] < 32) {
        GPIO.out_w1ts = (1 << row_leds[row]);
    } else {
        GPIO.out1_w1ts.val = (1 << (row_leds[row] - 32));
    }

    // aktifkan col
    if (col_leds[col] < 32) {
        GPIO.out_w1tc = (1 << col_leds[col]);
    } else {
        GPIO.out1_w1tc.val = (1 << (col_leds[col] - 32));
    }

    // matikan row/col lain
    for (uint8_t i = 0; i < 4; i++) {
        if (i != row) {
            if (row_leds[i] < 32) {
                GPIO.out_w1tc = (1 << row_leds[i]);
            } else {
                GPIO.out1_w1tc.val = (1 << (row_leds[i] - 32));
            }
        }
        if (i != col) {
            if (col_leds[i] < 32) {
                GPIO.out_w1ts = (1 << col_leds[i]);
            } else {
                GPIO.out1_w1ts.val = (1 << (col_leds[i] - 32));
            }
        }
    }
}
void enable_row(int row) {
    if (row < 0 || row > 3) return;

    if (row == 0) {
        GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));
        GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
        GPIO.out_w1tc = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
        GPIO.out_w1tc = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
    } else if (row == 1) {
        GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));
        GPIO.out_w1tc = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
        GPIO.out1_w1tc.val = (1 << (LED_A_0 - 32));
        GPIO.out_w1ts = (1 << LED_A_1);
        GPIO.out_w1tc = (1 << LED_A_2) | (1 << LED_A_3);
    } else if (row == 2) {
        GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));
        GPIO.out_w1tc = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
        GPIO.out1_w1tc.val = (1 << (LED_A_0 - 32));
        GPIO.out_w1ts = (1 << LED_A_2);
        GPIO.out_w1tc = (1 << LED_A_1) | (1 << LED_A_3);
    } else if (row == 3) {
        GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));
        GPIO.out_w1tc = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
        GPIO.out1_w1tc.val = (1 << (LED_A_0 - 32));
        GPIO.out_w1ts = (1 << LED_A_3);
        GPIO.out_w1tc = (1 << LED_A_2) | (1 << LED_A_1);
    }
}

void enable_col(int col) {
    if (col < 0 || col > 3) return;

    if (col == 0) {
        GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
        GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));
        GPIO.out_w1ts = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
        GPIO.out_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
    } else if (col == 1) {
        GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
        GPIO.out1_w1ts.val = (1 << (LED_K_0 - 32));
        GPIO.out_w1tc = (1 << LED_K_1);
        GPIO.out_w1ts = (1 << LED_K_2) | (1 << LED_K_3);
        GPIO.out_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
    } else if (col == 2) {
        GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
        GPIO.out1_w1ts.val = (1 << (LED_K_0 - 32));
        GPIO.out_w1tc = (1 << LED_K_2);
        GPIO.out_w1ts = (1 << LED_K_1) | (1 << LED_K_3);
        GPIO.out_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
    } else if (col == 3) {
        GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
        GPIO.out1_w1ts.val = (1 << (LED_K_0 - 32));
        GPIO.out_w1tc = (1 << LED_K_3);
        GPIO.out_w1ts = (1 << LED_K_2) | (1 << LED_K_1);
        GPIO.out_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
    }
}
// Task untuk tombol
void button_task(void* pv) {
    setup_joystick();
    while (1) {
        int sw_val = gpio_get_level(JOY_SW_PIN) == 0 ? 1 : 0;
        int x_val = adc1_get_raw(JOY_X_PIN);
        int y_val = adc1_get_raw(JOY_Y_PIN);
        int boot_val = gpio_get_level(0);
        static long last_polling_time = 0;
        printf("X: %d, Y: %d, SW: %d, BOOT: %d\n", x_val, y_val, sw_val, boot_val);
        if (boot_val == 0) {
            mode = (mode == BIRD_GAME) ? SNAKE_GAME : BIRD_GAME;
            printf("Mode diubah ke: %s\n", (mode == BIRD_GAME) ? "BIRD_GAME" : "SNAKE_GAME");
            if (mode == BIRD_GAME) {
                nyawa = 3;
                bird_score = 0;
                bird_pos[0] = rand() % 4;
                bird_pos[1] = rand() % 4;
                aim_pos[0] = rand() % 4;
                aim_pos[1] = rand() % 4;
            } else if (mode == SNAKE_GAME) {
                length_snake = 1;
                body_snake[0][0] = 1;
                body_snake[0][1] = 1;
                direction_snake = 3;  // right
                food_pos[0] = rand() % 4;
                food_pos[1] = rand() % 4;
            }
            vTaskDelay(pdMS_TO_TICKS(500));  // debounce 500ms
        }

        if (millis() - last_polling_time > 175 && isHit == 0 && nyawa > 0 && sw_val != 0 && mode == BIRD_GAME) {
            if (x_val < 1300) {
                aim_pos[1] -= 1;
                if (aim_pos[1] < 0) aim_pos[1] = 0;
            } else if (x_val > 2500) {
                aim_pos[1] += 1;
                if (aim_pos[1] > 3) aim_pos[1] = 3;
            }
            if (y_val > 2500) {
                aim_pos[0] -= 1;
                if (aim_pos[0] < 0) aim_pos[0] = 0;
            } else if (y_val < 1300) {
                aim_pos[0] += 1;
                if (aim_pos[0] > 3) aim_pos[0] = 3;
            }
            last_polling_time = millis();
        }

        if (millis() - last_polling_time > 175 && mode == SNAKE_GAME && !snake_dead) {
            if (x_val < 300) {
                direction_snake = 2;  // left
            } else if (x_val > 3700) {
                direction_snake = 3;  // right
            }
            if (y_val > 3000) {
                direction_snake = 0;  // up
            } else if (y_val < 300) {
                direction_snake = 1;  // down
            }
            last_polling_time = millis();
        }

        if (sw_val == 0) {
            if (mode == BIRD_GAME) {
                button_counter++;
                if ((bird_pos[0] == aim_pos[0]) && (bird_pos[1] == aim_pos[1])) {
                    isHit = 1;
                    bird_score++;
                    buzzer_state = 2;
                    if (bird_score > bird_highscore) {
                        bird_highscore = bird_score;
                        save_highscore(bird_highscore, BIRD_GAME);
                    }
                    for (int i = 0; i < 5; i++) {
                        enable_row(0);
                        esp_rom_delay_us(100000);
                        // GPIO.out_w1tc = 1 << BUZZER_PIN;

                        enable_row(1);
                        esp_rom_delay_us(100000);

                        enable_row(2);
                        esp_rom_delay_us(100000);

                        // GPIO.out_w1ts = 1 << BUZZER_PIN;
                        enable_row(3);
                        esp_rom_delay_us(100000);
                    }

                    bird_pos[0] = rand() % 4;
                    bird_pos[1] = rand() % 4;
                    isHit = 0;
                    enable_led(bird_pos[0], bird_pos[1]);
                    printf("Tombol ditekan\n");
                    vTaskDelay(pdMS_TO_TICKS(200));  // debounce 200ms

                } else {
                    // if (sw_val == 0 && isHit == 0) {
                    buzzer_state = 1;
                    isHit = 0;
                    nyawa--;
                    if (nyawa <= 0) {
                        printf("Game Over! Score: %d\n", bird_score);
                        buzzer_state = 3;
                        //
                        for (int i = 0; i < 5; i++) {
                            enable_row(0);
                            esp_rom_delay_us(100000);
                            // GPIO.out_w1tc = 1 << BUZZER_PIN;

                            enable_row(1);
                            esp_rom_delay_us(100000);

                            enable_row(2);
                            esp_rom_delay_us(100000);

                            // GPIO.out_w1ts = 1 << BUZZER_PIN;
                            enable_row(3);
                            esp_rom_delay_us(100000);
                        }

                        nyawa = 3;
                        bird_score = 0;
                    }
                    vTaskDelay(pdMS_TO_TICKS(200));  // debounce 200ms

                    // }
                }
                if (button_counter % 10 == 0 && button_counter != 0) {
                    button_counter = 0;
                    buzzer_state = 4;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));  // polling tiap 20ms
    }
}

// Task untuk LED random
void game_logic_task(void* pv) {
    while (1) {
        if (mode == BIRD_GAME) {
            if (isHit || nyawa <= 0) {
                vTaskDelay(pdMS_TO_TICKS(20));
                continue;
            }
            if (bird_score < 2)
                bird_speed = 1000;
            else if (bird_score < 4)
                bird_speed = 800;
            else if (bird_score < 8)
                bird_speed = 600;
            else if (bird_score < 12)
                bird_speed = 400;
            else
                bird_speed = 250;

        bird_move:
            int dir = rand() % 4;
            switch (dir) {
                case 0:
                    if (bird_pos[0] + 1 <= 3)
                        bird_pos[0]++;
                    else
                        // bird_pos[0] = 0;
                        goto bird_move;
                    break;
                case 1:
                    if (bird_pos[0] - 1 >= 0)
                        bird_pos[0]--;
                    else
                        // bird_pos[0] = 3;
                        goto bird_move;
                    break;
                case 2:
                    if (bird_pos[1] + 1 <= 3)
                        bird_pos[1]++;
                    else
                        // bird_pos[1] = 0;
                        goto bird_move;
                    break;
                case 3:
                    if (bird_pos[1] - 1 >= 0)
                        bird_pos[1]--;
                    else
                        // bird_pos[1] = 3;
                        goto bird_move;
                    break;
            }
            vTaskDelay(pdMS_TO_TICKS(bird_speed));  // pindah tiap 1ms
        } else if (mode == SNAKE_GAME) {
            if (snake_dead) {
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
            // Gerakkan snake
            for (int i = length_snake - 1; i > 0; i--) {
                body_snake[i][0] = body_snake[i - 1][0];
                body_snake[i][1] = body_snake[i - 1][1];
            }
            // Update kepala snake
            if (direction_snake == 0) {
                body_snake[0][0]--;
            } else if (direction_snake == 1) {
                body_snake[0][0]++;
            } else if (direction_snake == 2) {
                body_snake[0][1]--;
            } else if (direction_snake == 3) {
                body_snake[0][1]++;
            }

            // Cek tabrakan dengan dinding
            if (body_snake[0][0] < 0 || body_snake[0][0] > 3 || body_snake[0][1] < 0 || body_snake[0][1] > 3) {
                // Game over
                snake_dead = 1;
                buzzer_state = 3;
                length_snake = 1;
                body_snake[0][0] = 1;
                body_snake[0][1] = 1;
                food_eaten = 0;
                for (int i = 0; i < 5; i++) {
                    enable_row(0);
                    esp_rom_delay_us(100000);
                    // GPIO.out_w1tc = 1 << BUZZER_PIN;

                    enable_row(1);
                    esp_rom_delay_us(100000);

                    enable_row(2);
                    esp_rom_delay_us(100000);

                    // GPIO.out_w1ts = 1 << BUZZER_PIN;
                    enable_row(3);
                    esp_rom_delay_us(100000);
                }
                snake_dead = 0;
            }

            // cek tabrakan dengan diri sendiri
            for (int i = 1; i < length_snake; i++) {
                if (body_snake[0][0] == body_snake[i][0] && body_snake[0][1] == body_snake[i][1]) {
                    // Game over
                    snake_dead = 1;
                    buzzer_state = 3;
                    length_snake = 1;
                    body_snake[0][0] = 1;
                    body_snake[0][1] = 1;
                    food_eaten = 0;
                    for (int i = 0; i < 5; i++) {
                        enable_row(0);
                        esp_rom_delay_us(100000);
                        // GPIO.out_w1tc = 1 << BUZZER_PIN;

                        enable_row(1);
                        esp_rom_delay_us(100000);

                        enable_row(2);
                        esp_rom_delay_us(100000);

                        // GPIO.out_w1ts = 1 << BUZZER_PIN;
                        enable_row(3);
                        esp_rom_delay_us(100000);
                    }
                    snake_dead = 0;
                }
            }

            // Cek makan makanan
            if (body_snake[0][0] == food_pos[0] && body_snake[0][1] == food_pos[1]) {
                food_eaten++;
                if (food_eaten > snake_highscore) {
                    snake_highscore = food_eaten;
                    save_highscore(snake_highscore, SNAKE_GAME);
                }
                length_snake++;
                if (length_snake > 5) length_snake = 5;
                buzzer_state = 1;
                for (int i = length_snake - 1; i > 0; i--) {
                    body_snake[i][0] = body_snake[i - 1][0];
                    body_snake[i][1] = body_snake[i - 1][1];
                }
                // Tempatkan makanan baru dan pastikan tidak di tubuh snake
                int valid_food = 0;
                while (!valid_food) {
                    food_pos[0] = rand() % 4;
                    food_pos[1] = rand() % 4;
                    valid_food = 1;
                    for (int i = 0; i < length_snake; i++) {
                        if (food_pos[0] == body_snake[i][0] && food_pos[1] == body_snake[i][1]) {
                            valid_food = 0;
                            break;
                        }
                    }
                }
            }

            vTaskDelay(pdMS_TO_TICKS(500));  // gerak tiap 500ms
        }
    }
}

void led_task(void* pv) {
    setup_led_matrix();
    GPIO.out_w1ts = (1 << LED_A_1) | (1 << LED_A_2) | (1 << LED_A_3);
    GPIO.out1_w1ts.val = (1 << (LED_A_0 - 32));
    GPIO.out_w1tc = (1 << LED_K_1) | (1 << LED_K_2) | (1 << LED_K_3);
    GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));

    GPIO.out_w1ts = (1 << LED_A_2);
    GPIO.out1_w1tc.val = (1 << (LED_K_0 - 32));

    while (1) {
        if (mode == BIRD_GAME) {
            if (isHit || nyawa <= 0) {
                vTaskDelay(pdMS_TO_TICKS(20));
                continue;
            }

            enable_led(bird_pos[0], bird_pos[1]);
            vTaskDelay(pdMS_TO_TICKS(100));
            enable_led(aim_pos[0], aim_pos[1]);
            vTaskDelay(pdMS_TO_TICKS(10));
        } else if (mode == SNAKE_GAME) {
            if (snake_dead) {
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
            // Tampilkan snake
            for (int i = 0; i < length_snake; i++) {
                if (i >= 8) break;
                enable_led(body_snake[i][0], body_snake[i][1]);
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            enable_led(food_pos[0], food_pos[1]);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void lcd_task(void* pv) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ};
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    // Inisialisasi SMBus
    smbus_info_t* smbus_info = smbus_malloc();
    smbus_init(smbus_info, I2C_MASTER_NUM, LCD_ADDR);
    smbus_set_timeout(smbus_info, 1000 / portTICK_PERIOD_MS);

    // Inisialisasi LCD
    i2c_lcd1602_info_t* lcd_info = i2c_lcd1602_malloc();
    i2c_lcd1602_init(lcd_info, smbus_info, true, 2, 16, 0);  // 0 = default 5x8 dots

    i2c_lcd1602_reset(lcd_info);
    i2c_lcd1602_set_backlight(lcd_info, true);
    i2c_lcd1602_clear(lcd_info);
    i2c_lcd1602_move_cursor(lcd_info, 0, 0);
    i2c_lcd1602_write_string(lcd_info, "Hello ESP32!");

    i2c_lcd1602_clear(lcd_info);
    while (1) {
        if (mode == BIRD_GAME) {
            vTaskDelay(pdMS_TO_TICKS(10));
            if (isHit) {
                i2c_lcd1602_move_cursor(lcd_info, 0, 0);
                i2c_lcd1602_write_string(lcd_info, "      HIT!      ");
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                i2c_lcd1602_write_string(lcd_info, "                ");

            } else if (nyawa <= 0) {
                i2c_lcd1602_move_cursor(lcd_info, 0, 0);
                i2c_lcd1602_write_string(lcd_info, "    GAME OVER   ");
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                i2c_lcd1602_write_string(lcd_info, "                ");
            } else {
                int sw_val = gpio_get_level(JOY_SW_PIN) == 0 ? 1 : 0;
                printf("X: %4d | Y: %4d | SW: %s\n",
                       x_val, y_val, (sw_val == 0) ? "Pressed" : "Released");
                i2c_lcd1602_move_cursor(lcd_info, 0, 0);
                char line1[17];
                sprintf(line1, "Score:%d Lives:%d ", bird_score, nyawa);
                i2c_lcd1602_write_string(lcd_info, line1);
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char line2[17];
                sprintf(line2, "Highscore:%d     ", bird_highscore);
                i2c_lcd1602_write_string(lcd_info, line2);
            }
        } else if (mode == SNAKE_GAME) {
            if (snake_dead) {
                i2c_lcd1602_move_cursor(lcd_info, 0, 0);
                i2c_lcd1602_write_string(lcd_info, "    GAME OVER   ");
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                i2c_lcd1602_write_string(lcd_info, "                ");
            } else {
                i2c_lcd1602_move_cursor(lcd_info, 0, 0);
                char line1[17];
                sprintf(line1, "Food Eaten:%d   ", food_eaten);
                i2c_lcd1602_write_string(lcd_info, line1);
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char line2[17];
                sprintf(line2, "Highscore:%d     ", snake_highscore);
                i2c_lcd1602_write_string(lcd_info, line2);
            }
        }
    }
}

void app_main(void) {
    init_nvs();
    bird_highscore = load_highscore(BIRD_GAME);
    snake_highscore = load_highscore(SNAKE_GAME);

    // Buat task
    xTaskCreatePinnedToCore(button_task, "button_task", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(game_logic_task, "game_logic_task", 2048, NULL, 0, NULL, 1);
    xTaskCreatePinnedToCore(led_task, "led_task", 2048, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(buzzer_task, "buzzer_task", 2048, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(lcd_task, "lcd_task", 4096, NULL, 3, NULL, 0);
}
