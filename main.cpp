#include "pico/stdlib.h"
#include "SEGGER_RTT.h"
#include "etl/vector.h"

int main() {
  // Initialize SEGGER RTT
  SEGGER_RTT_Init();
  SEGGER_RTT_WriteString(0, "System Initializing over RTT...\r\n");

  // LD2 (Green LED) is tied directly to GPIO25 on the W6300-EVB-Pico2
  const uint LED_PIN = 25;

  // Initialize the chosen GPIO pin
  gpio_init(LED_PIN);

  // Set the pin direction to output
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // Define an ETL vector of integers with a capacity of 5
  etl::vector<int, 5> numbers;
  numbers.push_back(10);
  numbers.push_back(20);
  numbers.push_back(30);

  SEGGER_RTT_printf(0, "ETL Vector initialized. size: %d, capacity: %d\r\n", 
                    static_cast<int>(numbers.size()), static_cast<int>(numbers.capacity()));

  for (size_t i = 0; i < numbers.size(); ++i) {
    SEGGER_RTT_printf(0, "  numbers[%d] = %d\r\n", static_cast<int>(i), numbers[i]);
  }

  uint32_t loop_count = 0;

  // Infinite loop to toggle the LED status
  while (true) {
    gpio_put(LED_PIN, 1); // Turn the LED on
    sleep_ms(500);        // Delay for 500 milliseconds
    gpio_put(LED_PIN, 0); // Turn the LED off
    sleep_ms(500);        // Delay for 500 milliseconds

    loop_count++;
    SEGGER_RTT_printf(0, "Blinky Loop Count: %u\r\n", loop_count);
  }
}
