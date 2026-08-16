#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

#define INTERVAL_US 100 // 100 us

// --- Core 0 ISR & Setup (TIMER0) ---
void __not_in_flash_func(timer0_isr)() {
	gpio_xor_mask(1u << 1); // probe: isr entry (a/b parity with timerisrstatic)
	
	// Clear the interrupt flag for TIMER0 Alarm 0
	timer0_hw->intr = 1u << 0;

	// Re-arm alarm for next tick
	timer0_hw->alarm[0] = timer0_hw->timerawl + INTERVAL_US;

	// Perform Core 0 task work here...
	gpio_xor_mask(1u << 0); // core 0 isr toggle GPIO pin 0
}

void setup_core0_timer() {
	// Must run on Core 0
	irq_set_exclusive_handler(TIMER0_IRQ_0, timer0_isr);
	irq_set_enabled(TIMER0_IRQ_0, true);

	// Unmask Alarm 0 interrupt on TIMER0
	timer0_hw->inte |= (1u << 0);

	// Arm initial alarm
	timer0_hw->alarm[0] = timer0_hw->timerawl + INTERVAL_US;
}

// --- Core 1 ISR & Setup (TIMER1) ---
void timer1_isr() {
	// Clear the interrupt flag for TIMER1 Alarm 0
	timer1_hw->intr = 1u << 0;

	// Re-arm alarm for next tick
	timer1_hw->alarm[0] = timer1_hw->timerawl + INTERVAL_US/10;

	// Perform Core 1 task work here...
	//gpio_xor_mask(1u << 1); // core 1 isr toggle GPIO pin 1
}

void core1_entry() {
	// Must run ON Core 1 so the NVIC registers the interrupt locally
	irq_set_exclusive_handler(TIMER1_IRQ_0, timer1_isr);
	irq_set_enabled(TIMER1_IRQ_0, true);

	// Unmask Alarm 0 interrupt on TIMER1
	timer1_hw->inte |= (1u << 0);

	// Arm initial alarm
	timer1_hw->alarm[0] = timer1_hw->timerawl + INTERVAL_US;

	while (true) {
		tight_loop_contents();
	}
}

int main() {
	//stdio_init_all();
    
	gpio_init(0);
	gpio_set_dir(0, GPIO_OUT);
	gpio_put(0, false);
	gpio_init(1);
	gpio_set_dir(1, GPIO_OUT);
	gpio_put(1, false);


	// Launch Core 1 execution
	multicore_launch_core1(core1_entry);

	// Setup Timer 0 locally on Core 0
	setup_core0_timer();

	while (true) {
		tight_loop_contents();
	}
}
