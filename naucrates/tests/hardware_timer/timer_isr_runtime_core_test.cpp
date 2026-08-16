#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

#define INTERVAL_US 1000 // 100 ms

uint32_t s_core0_alarm_index = 99;
uint32_t s_core1_alarm_index = 99;

// --- Core 0 ISR & Setup ---
void core0_isr() {
	timer_hw_t*    timer = TIMER_INSTANCE(get_core_num()); // timer0 on core 0
	const uint32_t alarm = s_core0_alarm_index;

	// Clear the interrupt flag
	timer->intr = 1u << alarm;

	// Re-arm alarm for next tick
	timer->alarm[alarm] = timer->timerawl + INTERVAL_US;

	// Perform Core 0 task work here...
	gpio_xor_mask(1u << 0); // core 0 isr toggle GPIO pin 0
}

void setup_core0_timer() {
	// Must run on Core 0
	timer_hw_t* timer  = TIMER_INSTANCE(get_core_num());
	s_core0_alarm_index = timer_hardware_alarm_claim_unused(timer, true);

	const uint irq_num = timer_hardware_alarm_get_irq_num(timer, s_core0_alarm_index);
	irq_set_exclusive_handler(irq_num, core0_isr);
	irq_set_enabled(irq_num, true);

	// Unmask Alarm interrupt
	timer->inte |= (1u << s_core0_alarm_index);

	// Arm initial alarm
	timer->alarm[s_core0_alarm_index] = timer->timerawl + INTERVAL_US;
}

// --- Core 1 ISR & Setup ---
void core1_isr() {
	timer_hw_t*    timer = TIMER_INSTANCE(get_core_num()); // timer1 on core 1
	const uint32_t alarm = s_core1_alarm_index;

	// Clear the interrupt flag
	timer->intr = 1u << alarm;

	// Re-arm alarm for next tick
	timer->alarm[alarm] = timer->timerawl + INTERVAL_US / 10;

	// Perform Core 1 task work here...
	gpio_xor_mask(1u << 1); // core 1 isr toggle GPIO pin 1
}

void setup_core1_timer() {
	// Must run on Core 1
	timer_hw_t* timer  = TIMER_INSTANCE(get_core_num());
	s_core1_alarm_index = timer_hardware_alarm_claim_unused(timer, true);

	const uint irq_num = timer_hardware_alarm_get_irq_num(timer, s_core1_alarm_index);
	irq_set_exclusive_handler(irq_num, core1_isr);
	irq_set_enabled(irq_num, true);

	// Unmask Alarm interrupt
	timer->inte |= (1u << s_core1_alarm_index);

	// Arm initial alarm
	timer->alarm[s_core1_alarm_index] = timer->timerawl + INTERVAL_US;
}

void core1_entry() {
	// Must run ON Core 1 so the NVIC registers the interrupt locally
	setup_core1_timer();

	while (true) {
		tight_loop_contents();
	}
}

int main() {
	stdio_init_all();

	gpio_init(0);
	gpio_set_dir(0, GPIO_OUT);
	gpio_init(1);
	gpio_set_dir(1, GPIO_OUT);

	// Launch Core 1 execution
	multicore_launch_core1(core1_entry);

	// Setup timer locally on Core 0
	setup_core0_timer();

	while (true) {
		tight_loop_contents();
	}
}
