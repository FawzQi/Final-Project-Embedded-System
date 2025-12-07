// #include "rom/ets_sys.h"
// #include "soc/gpio_struct.h"
// #include "soc/io_mux_reg.h"

// #define LED1A_PIN 12
// #define LED1K_PIN 13
// #define LED2A_PIN 14
// #define LED2K_PIN 15
// #define BTN1_PIN 2
// #define BTN2A_PIN 4
// #define BTN2K_PIN 5

// void app_main(void) {
//     // Set LED sebagai output
//     GPIO.enable_w1ts = (1 << LED1A_PIN) | (1 << LED2A_PIN) | (1 << LED1K_PIN) | (1 << LED2K_PIN) | (1 << BTN2A_PIN);

//     GPIO.enable_w1tc = (1 << BTN2A_PIN);

//     // Pilih fungsi pin sebagai GPIO dan aktifkan pull-up
//     PIN_FUNC_SELECT(IO_MUX_GPIO2_REG, PIN_FUNC_GPIO);
//     REG_SET_BIT(IO_MUX_GPIO2_REG, FUN_PU);
//     REG_CLR_BIT(IO_MUX_GPIO2_REG, FUN_PD);

//     PIN_FUNC_SELECT(IO_MUX_GPIO5_REG, PIN_FUNC_GPIO);
//     REG_SET_BIT(IO_MUX_GPIO5_REG, FUN_PU);
//     REG_CLR_BIT(IO_MUX_GPIO5_REG, FUN_PD);

//     ets_printf("Mulai baca tombol via register...\n");

//     while (1) {
//         uint32_t input = GPIO.in;
//         int btn1 = (input >> BTN1_PIN) & 1;
//         int btn2 = (input >> BTN2K_PIN) & 1;

//         if (!btn1) {
//             GPIO.out_w1ts = (1 << LED1A_PIN) | (1 << LED2A_PIN);
//             GPIO.out_w1tc = (1 << LED1K_PIN) | (1 << LED2K_PIN);
//             ets_printf("Tombol on ditekan\n");
//         }

//         if (!btn2) {
//             GPIO.out_w1tc = (1 << LED2A_PIN) | (1 << LED2K_PIN);
//             GPIO.out_w1tc = (1 << LED1K_PIN) | (1 << LED1A_PIN);
//             ets_printf("Tombol off ditekan\n");
//         }

//         ets_delay_us(20000);
//     }
// }
