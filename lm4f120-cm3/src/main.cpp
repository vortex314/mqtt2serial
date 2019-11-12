
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/lm4f/systemcontrol.h>
#include <libopencm3/lm4f/rcc.h>
#include <libopencm3/lm4f/gpio.h>
#include <libopencm3/lm4f/nvic.h>
#include <libopencm3/lm4f/uart.h>

#include <stdbool.h>
#include <stdio.h>

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
}

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
	uart_set_baudrate(UART0, 115200);
	uart_set_databits(UART0, 8);
	uart_set_parity(UART0, UART_PARITY_NONE);
	uart_set_stopbits(UART0, 1);
	/* Now that we're done messing with the settings, enable the UART */
	uart_enable(UART0);
}

static void uart_irq_setup(void)
{
	/* Gimme and RX interrupt */
	uart_enable_rx_interrupt(UART0);
	/* Make sure the interrupt is routed through the NVIC */
	nvic_enable_irq(NVIC_UART0_IRQ);
}

/*
 * uart0_isr is declared as a weak function. When we override it here, the
 * libopencm3 build system takes care that it becomes our UART0 ISR.
 */
void uart0_isr(void)
{
	uint8_t rx;
	uint32_t irq_clear = 0;

	if (uart_is_interrupt_source(UART0, UART_INT_RX)) {
		rx = uart_recv(UART0);
		uart_send(UART0, rx);
		irq_clear |= UART_INT_RX;
	}

	uart_clear_interrupt_flag(UART0, (uart_interrupt_flag)irq_clear);
}

extern void app_main();

int main(void)
{
	gpio_enable_ahb_aperture();   
	clock_setup();
	gpio_setup();
	irq_setup();
	uart_setup();
	uart_irq_setup();

	for(int i=0;i<100;i++) uart_send_blocking(UART0,'H');
	app_main();

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
}