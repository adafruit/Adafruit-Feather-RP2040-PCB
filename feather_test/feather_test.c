#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"


bool test_pins(uint pina, uint pinb);
#define UART_ID uart0

uint8_t allpins[] = {2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 18, 19, 20, 24, 25, 29};
#define NUM_PINS sizeof(allpins)
#define PIN_NEOPIX 16
#define BRIGHTNESS 0.2


static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r * BRIGHTNESS) << 8) |
            ((uint32_t) (g * BRIGHTNESS) << 16) |
            (uint32_t) (b * BRIGHTNESS);
}


uint32_t Wheel(uint8_t WheelPos) {
  if (WheelPos < 85) {
    return urgb_u32(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    return urgb_u32(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return urgb_u32(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

int main()
{
  stdio_init_all();
  
  puts("Feather RP2040 demo & tester");

  // todo get free sm
  PIO pio = pio0;
  int sm = 0;
  uint offset = pio_add_program(pio, &ws2812_program);
  
  ws2812_program_init(pio, sm, offset, PIN_NEOPIX, 800000, true);
  
  // LED output
  gpio_init(13);
  gpio_set_dir(13, GPIO_OUT);

  uint8_t i=0;
  bool LED = false;

  while (1) {
    put_pixel(Wheel(i++));
    sleep_ms(10);
    if (i % 10 == 0) {
      gpio_put(13, LED);
      LED = !LED;
    }

    // check if we have a test command waiting
    if (uart_is_readable(UART_ID)) {
      uint8_t ch = uart_getc(UART_ID);
      if (ch == 0xAF) { // oh so ya wanna do a test?
        break;
      }
    }
  }


  for (int i=0; i<NUM_PINS; i++)  {
    printf("GPIO init %d\n", allpins[i]);
    gpio_init(allpins[i]);
  }
  adc_init();
  adc_gpio_init(26);
  adc_gpio_init(27);
  adc_gpio_init(28);


  printf("**Test GPIO!\n");
  
  while (1) {
    sleep_ms(100);
    if (! test_pins(13, 11)) continue; // D13 and D11
    if (! test_pins(12, 10)) continue; // D12 and D10
    if (! test_pins(9, 7)) continue;   // D9 and D5
    if (! test_pins(8, 3)) continue;  // D6 and SCL
    if (! test_pins(6, 2)) continue;  // D4 and SDA
    if (! test_pins(20, 24)) continue;  // MO and 24
    if (! test_pins(19, 25)) continue;  // MI and 25
    if (! test_pins(18, 29)) continue;  // SCK and A3

    // read 1/2 of VBAT, should be about 2.1V
    adc_select_input(0);
    float vbat = adc_read();
    vbat *= 3.3;
    vbat *= 2.0;
    vbat /= 4096.0;
    printf("VBat ADC = %0.1f V\n", vbat);
    if (fabs(vbat - 4.2) > 0.2) {
      printf("VBat should be 4.2V but isn't\n");
      continue;
    }

    // read 1/2 of VUSB, should be 2.5 V
    adc_select_input(2);
    float vusb = adc_read();
    vusb *= 3.3;
    vusb *= 2.0;
    vusb /= 4096.0;
    printf("VUSB ADC = %0.1f V\n", vusb);
    if ((vusb < 4.5) || (vusb > 5.5)) {
      printf("VUSB should be 5V but isn't\n");
      continue;
    }

    // read A1 should be ground
    adc_select_input(1);
    float vgnd = adc_read();
    vgnd *= 3.3;
    vgnd /= 4096.0;

    printf("ADC1 = %0.1f V\n", vgnd);
    if (vgnd > 0.2) {
      printf("ADC1 should be 0V but isn't\n");
      continue;
    }

    break;
  }

  printf("**GPIO+ADC OK!\n");

  return 0;
}


bool test_pins(uint a, uint b) {
  printf("testing GPIOpin %d and %d\n", a, b);

  // set both to inputs
  gpio_set_dir(a, GPIO_IN);
  gpio_set_dir(b, GPIO_IN);

  // turn on 'a' pullup
  gpio_pull_up(a);
  gpio_disable_pulls(b);

  sleep_ms(3);

  // verify neither are grounded
  if (! gpio_get(a) || ! gpio_get(b)) {
    printf("Ground test 1 fail\n");
    return false;
  }
  
 
  // make a an output, b with a pullup
  gpio_set_dir(a, GPIO_OUT);
  gpio_set_dir(b, GPIO_IN);

  gpio_disable_pulls(a);
  gpio_pull_up(b);

  gpio_put(a, false);

  sleep_ms(3);

  // make sure b is low
  if (gpio_get(b)) {
    printf("Low test fail on %d\n", b);
    return false;
  }

  // make b an output, a with a pullup
  gpio_set_dir(a, GPIO_IN);
  gpio_set_dir(b, GPIO_OUT);

  gpio_pull_up(a);
  gpio_disable_pulls(b);

  gpio_put(b, false);

  sleep_ms(3);

  // make sure a is low
  if (gpio_get(a)) {
    printf("Low test fail on %d\n", a);
    return false;
  }
 
  // a is an input, b is an output
  gpio_set_dir(a, GPIO_IN);
  gpio_set_dir(b, GPIO_OUT);

  gpio_pull_down(a);

  gpio_put(b, true);

  sleep_ms(3);

  // verify a is not grounded
  if (! gpio_get(a)) {
    printf("Ground2 test fail\n");
    return false;
  }
  
  // make sure no pins are shorted to pin a or b
  for (uint8_t i=0; i<NUM_PINS; i++)  {
    gpio_set_dir(allpins[i], GPIO_IN);
    gpio_pull_up(allpins[i]);
  }

  gpio_set_dir(a, GPIO_OUT);
  gpio_set_dir(b, GPIO_OUT);
  gpio_put(a, false);
  gpio_put(b, false);

  sleep_ms(3);

  for (uint8_t i=0; i<NUM_PINS; i++) {
    if ((allpins[i] == a) || (allpins[i] == b)) {
      continue;
    }

    //printf("\tPin #%d -> %d\n", allpins[i], gpio_get(allpins[i]));

    if (! gpio_get(allpins[i])) {
      printf("%d is shorted?\n", allpins[i]);
      return false;
    }
  }

  gpio_set_dir(a, GPIO_IN);
  gpio_disable_pulls(a);
  gpio_set_dir(b, GPIO_IN);
  gpio_disable_pulls(b);

  sleep_ms(3);

  return true;
}
