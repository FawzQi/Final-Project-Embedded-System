// #include "rom/ets_sys.h"
// #include "soc/gpio_struct.h"
// #include "soc/io_mux_reg.h"

// #define LED1_PIN 13
// #define LED2_PIN 15  // kaki katoda LED A
// #define LED3_PIN 14  // kaki anoda LED A
// #define BTN1_PIN 4
// #define BTN2_PIN 5

// void app_main(void) {
//     // Set LED sebagai output
//     GPIO.enable_w1ts = (1 << LED1_PIN) | (1 << LED2_PIN);

//     GPIO.enable_w1tc = (1 << LED3_PIN);
//     GPIO.out_w1tc = (1 << LED3_PIN);

//     // Pilih fungsi pin sebagai GPIO dan aktifkan pull-up
//     PIN_FUNC_SELECT(IO_MUX_GPIO4_REG, PIN_FUNC_GPIO);
//     REG_SET_BIT(IO_MUX_GPIO4_REG, FUN_PU);
//     REG_CLR_BIT(IO_MUX_GPIO4_REG, FUN_PD);

//     PIN_FUNC_SELECT(IO_MUX_GPIO5_REG, PIN_FUNC_GPIO);
//     REG_SET_BIT(IO_MUX_GPIO5_REG, FUN_PU);
//     REG_CLR_BIT(IO_MUX_GPIO5_REG, FUN_PD);

//     ets_printf("Mulai baca tombol via register...\n");

//     while (1) {
//         uint32_t input = GPIO.in;
//         int btn1 = (input >> BTN1_PIN) & 1;
//         int btn2 = (input >> BTN2_PIN) & 1;

//         if (!btn2) {
//             ets_printf("Button B pressed\n");
//             GPIO.out_w1ts = (1 << LED2_PIN);
//             GPIO.out_w1tc = (1 << LED1_PIN) | (1 << LED3_PIN);
//             ets_delay_us(125000);
//             GPIO.out_w1tc = (1 << LED2_PIN);
//             GPIO.out_w1ts = (1 << LED1_PIN) | (1 << LED3_PIN);
//             ets_delay_us(125000);
//             continue;
//         }

//         if (!btn1) {
//             ets_printf("Button A pressed\n");
//             GPIO.out_w1ts = (1 << LED1_PIN) | (1 << LED3_PIN);
//             GPIO.out_w1tc = (1 << LED2_PIN);

//             ets_delay_us(250000);
//             GPIO.out_w1tc = (1 << LED1_PIN) | (1 << LED3_PIN);
//             GPIO.out_w1ts = (1 << LED2_PIN);
//             ets_delay_us(250000);

//             continue;
//         }
//         ets_delay_us(10000);
//     }
// }
