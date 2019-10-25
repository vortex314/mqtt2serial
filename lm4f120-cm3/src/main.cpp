<<<<<<< HEAD

#include "proto.h"

//_______________________________________________________________________________________________________________
//

//_______________________________________________________________________________________________________________
//
class LedBlinker : public ProtoThread {
  uint32_t _pin, _delay;

public:
  LedBlinker(uint32_t pin, uint32_t delay) {
    _pin = pin;
    _delay = delay;
  }
  void setup() {
    LOG("LedBlinker started.");
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, 1);
  }
  void loop() {
    PT_BEGIN();
    while (true) {
      timeout(_delay);
      digitalWrite(_pin, 0);
      PT_YIELD_UNTIL(timeout());
      timeout(_delay);
      digitalWrite(_pin, 1);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  }
  void delay(uint32_t d) { _delay = d; }
};
//_______________________________________________________________________________________________________________
//
// LedBlinker ledBlinkerRed(LED_RED_PIN,100);
// LedBlinker ledBlinkerGreen(LED_GREEN_PIN,300);
//_______________________________________________________________________________________________________________
//
class Publisher : public ProtoThread {
  std::string _systemPrefix;
  MqttSerial &_mqtt;
  LedBlinker &_ledBlinker;

public:
  Publisher(MqttSerial &mqtt, LedBlinker &ledBlinker)
      : _mqtt(mqtt), _ledBlinker(ledBlinker){};
  void setup() {
    LOG("Publisher started");
    _systemPrefix = "src/" + Sys::hostname + "/system/";
  }
  void loop() {
    PT_BEGIN();
    while (true) {
      if (_mqtt.isConnected()) {
        _mqtt.publish(_systemPrefix + "upTime", String(millis()));
        _mqtt.publish(_systemPrefix + "build", Sys::build);
        _mqtt.publish(_systemPrefix + "cpu", Sys::cpu);
  //      _mqtt.publish(_systemPrefix + "heap", String(freeMemory()));
        _ledBlinker.delay(1000);
      } else
        _ledBlinker.delay(100);
      timeout(1000);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  }
};
//
//_____________________________________ protothreads running _____
//
#define PIN_LED 2

MqttSerial mqtt(Serial);
LedBlinker ledBlinker(PIN_LED, 100);
Publisher publisher(mqtt, ledBlinker);

void mqttCallback(String topic, String message) {
  Serial.println(" RXD " + topic + "=" + message);
}

void setup() {
  Serial.begin(115200);
  LOG("===== Starting ProtoThreads  build " __DATE__ " " __TIME__);
  Sys::hostname = "esp32";
  Sys::cpu = "esp32";
  mqtt.onMqttPublish(mqttCallback);
  ProtoThread::setupAll();
}

void loop() { ProtoThread::loopAll(); }

void main2(){
    setup();
    while(true) loop();
}

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <stdio.h>
#include <errno.h>

int _write(int file, char *ptr, int len);

static void clock_setup(void)
{
	rcc_clock_setup_in_hse_12mhz_out_72mhz();

	/* Enable GPIOA clock (for LED GPIOs). */
	rcc_periph_clock_enable(RCC_GPIOC);

	/* Enable clocks for GPIO port A (for GPIO_USART2_TX) and USART2. */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_USART2);
}

static void usart_setup(void)
{
	/* Setup GPIO pin GPIO_USART2_RE_TX on GPIO port B for transmit. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART2_TX);

	/* Setup UART parameters. */
	usart_set_baudrate(USART2, 230400);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART2, USART_MODE_TX);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

static void gpio_setup(void)
{
	gpio_set(GPIOA, GPIO8);

	/* Setup GPIO8 (in GPIO port A) for LED use. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);
}

int _write(int file, char *ptr, int len)
{
	int i;

	if (file == 1) {
		for (i = 0; i < len; i++)
			usart_send_blocking(USART2, ptr[i]);
		return i;
	}

	errno = EIO;
	return -1;
=======
/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2011 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2012-2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \addtogroup Examples
 *
 * Flashes the Red, Green and Blue diodes on the board, in order.
 *
 * RED controlled by PF1
 * Green controlled by PF3
 * Blue controlled by PF2
 */
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/lm4f/systemcontrol.h>
#include <libopencm3/lm4f/rcc.h>
#include <libopencm3/lm4f/gpio.h>
#include <libopencm3/lm4f/nvic.h>

#include <stdbool.h>
#include <stdio.h>
#include <libopencm3/lm4f/uart.h>

static void uart_setup(void)
{
	/* Enable GPIOA in run mode. */
	periph_clock_enable(RCC_GPIOA);
	/* Mux PA0 and PA1 to UART0 (alternate function 1) */
	gpio_set_af(GPIOA, 1, GPIO0 | GPIO1);

	/* Enable the UART clock */
	periph_clock_enable(RCC_UART0);
	/* We need a brief delay before we can access UART config registers */
	__asm__("nop");
	/* Disable the UART while we mess with its setings */
	uart_disable(UART0);
	/* Configure the UART clock source as precision internal oscillator */
	uart_clock_from_piosc(UART0);
	/* Set communication parameters */
	uart_set_baudrate(UART0, 921600);
	uart_set_databits(UART0, 8);
	uart_set_parity(UART0, UART_PARITY_NONE);
	uart_set_stopbits(UART0, 1);
	/* Now that we're done messing with the settings, enable the UART */
	uart_enable(UART0);

}

int mainUart(void)
{
	uint8_t rx;

	gpio_enable_ahb_aperture();
	uart_setup();

	/*
	 * Yes, it's that simple !
	 */
	while(1) {
		rx = uart_recv_blocking(UART0);
		uart_send_blocking(UART0, rx);
	}

	return 0;
}

/* This is how the RGB LED is connected on the stellaris launchpad */
#define RGB_PORT	GPIOF
enum {
	LED_R	= GPIO1,
	LED_G	= GPIO3,
	LED_B	= GPIO2,
};

/* This is how the user switches are connected to GPIOF */
enum {
	USR_SW1	= GPIO4,
	USR_SW2	= GPIO0,
};

/* The divisors we loop through when the user presses SW2 */
enum {
	PLL_DIV_80MHZ 	= 5,
	PLL_DIV_57MHZ 	= 7,
	PLL_DIV_40MHZ 	= 10,
	PLL_DIV_20MHZ 	= 20,
	PLL_DIV_16MHZ 	= 25,
};

static const uint8_t plldiv[] = {
	PLL_DIV_80MHZ,
	PLL_DIV_57MHZ,
	PLL_DIV_40MHZ,
	PLL_DIV_20MHZ,
	PLL_DIV_16MHZ,
	0
};
/* The PLL divisor we are currently on */
static size_t ipll = 0;
/* Are we bypassing the PLL, or not? */
static bool bypass = false;

/*
 * Clock setup:
 * Take the main crystal oscillator at 16MHz, run it through the PLL, and divide
 * the 400MHz PLL clock to get a system clock of 80MHz.
 */
static void clock_setup(void)
{
	rcc_sysclk_config(OSCSRC_MOSC, XTAL_16M, PLL_DIV_80MHZ);
}

/*
 * GPIO setup:
 * Enable the pins driving the RGB LED as outputs.
 */
static void gpio_setup(void)
{
	/*
	 * Configure GPIOF
	 * This port is used to control the RGB LED
	 */
	periph_clock_enable(RCC_GPIOF);
	const uint32_t outpins = (LED_R | LED_G | LED_B);

	gpio_mode_setup(RGB_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, outpins);
	gpio_set_output_config(RGB_PORT, GPIO_OTYPE_PP, GPIO_DRIVE_2MA, outpins);

	/*
	 * Now take care of our buttons
	 */
	const uint32_t btnpins = USR_SW1 | USR_SW2;

	/*
	 * PF0 is a locked by default. We need to unlock it before we can
	 * re-purpose it as a GPIO pin.
	 */
	gpio_unlock_commit(GPIOF, USR_SW2);
	/* Configure pins as inputs, with pull-up. */
	gpio_mode_setup(GPIOF, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, btnpins);
}

/*
 * IRQ setup:
 * Trigger an interrupt whenever a button is depressed.
 */
static void irq_setup(void)
{
	const uint32_t btnpins = USR_SW1 | USR_SW2;
	/* Trigger interrupt on rising-edge (when button is depressed) */
	gpio_configure_trigger(GPIOF, GPIO_TRIG_EDGE_RISE, btnpins);
	/* Finally, Enable interrupt */
	gpio_enable_interrupts(GPIOF, btnpins);
	/* Enable the interrupt in the NVIC as well */
	nvic_enable_irq(NVIC_GPIOF_IRQ);
}

#define FLASH_DELAY 800000
static void delay(void)
{
	int i;
	for (i = 0; i < FLASH_DELAY; i++)       /* Wait a bit. */
		__asm__("nop");
>>>>>>> 00493b578ba1698c24126d5c4d5c6a7409c1dff6
}

int main(void)
{
<<<<<<< HEAD
	int counter = 0;
	float fcounter = 0.0;
	double dcounter = 0.0;

	clock_setup();
	gpio_setup();
	usart_setup();

	/*
	 * Write Hello World, an integer, float and double all over
	 * again while incrementing the numbers.
	 */
	while (1) {
		gpio_toggle(GPIOA, GPIO8);
		printf("Hello World! %i %f %f\r\n", counter, fcounter,
		       dcounter);
		counter++;
		fcounter += 0.01;
		dcounter += 0.01;
	}

	return 0;
=======
	gpio_enable_ahb_aperture();
	clock_setup();
	gpio_setup();
	irq_setup();

	/* Blink each color of the RGB LED in order. */
	while (1) {
		/*
		 * Flash the Red diode
		 */
		gpio_set(RGB_PORT, LED_R);
		delay(); /* Wait a bit. */
		gpio_clear(RGB_PORT, LED_R);
		delay(); /* Wait a bit. */

		/*
		 * Flash the Green diode
		 */
		gpio_set(RGB_PORT, LED_G);
		delay(); /* Wait a bit. */
		gpio_clear(RGB_PORT, LED_G);
		delay(); /* Wait a bit. */

		/*
		 * Flash the Blue diode
		 */
		gpio_set(RGB_PORT, LED_B);
		delay(); /* Wait a bit. */
		gpio_clear(RGB_PORT, LED_B);
		delay(); /* Wait a bit. */
	}

	return 0;
}

void gpiof_isr(void)
{
	if (gpio_is_interrupt_source(GPIOF, USR_SW1)) {
		/* SW1 was just depressed */
		bypass = !bypass;
		if (bypass) {
			rcc_pll_bypass_enable();
			/*
			 * The divisor is still applied to the raw clock.
			 * Disable the divisor, or we'll divide the raw clock.
			 */
			SYSCTL_RCC &= ~SYSCTL_RCC_USESYSDIV;
		}
		else
		{
			rcc_change_pll_divisor(plldiv[ipll]);
		}
		/* Clear interrupt source */
		gpio_clear_interrupt_flag(GPIOF, USR_SW1);
	}

	if (gpio_is_interrupt_source(GPIOF, USR_SW2)) {
		/* SW2 was just depressed */
		if (!bypass) {
			if (plldiv[++ipll] == 0)
				ipll = 0;
			rcc_change_pll_divisor(plldiv[ipll]);
		}
		/* Clear interrupt source */
		gpio_clear_interrupt_flag(GPIOF, USR_SW2);
	}
>>>>>>> 00493b578ba1698c24126d5c4d5c6a7409c1dff6
}