#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static void gpio_init(void);
static void cycle_rg(void *arg);
static double moving_average(double new_speed);
static void calculate_speed(void *arg);

// Configuring GPIO pins
#define RED_LED GPIO_NUM_32
#define GREEN_LED GPIO_NUM_33
#define TRIGGER GPIO_NUM_25
#define ECHO GPIO_NUM_26

// Initialize traffic light states
uint8_t red_SW = 1;
uint8_t green_SW = 0;

// For calculating moving average
#define WINDOW_SIZE 30
double speed_buffer[WINDOW_SIZE] = {0};
int buffer_index = 0;
int buffer_count = 0;
double sum = 0;

void app_main(void)
{
    // Initialize configured peripherals
    gpio_init();

    xTaskCreatePinnedToCore(
        calculate_speed,
        "Calculate_Speed",
        4096,
        NULL,
        10,
        NULL,
        1
    );
    
    xTaskCreatePinnedToCore(
        cycle_rg,
        "Cycle_Lights",
        2048,
        NULL,
        5,
        NULL,
        0
    );
}

static void gpio_init(void)
{
    // Configure GPIOs for traffic light LEDs
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);

    // Configure GPIOs for HCSR04
    gpio_set_direction(TRIGGER, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO, GPIO_MODE_INPUT);
}

// Cycle traffic lights
static void cycle_rg(void *arg)
{
    while (1)
    {
        gpio_set_level(RED_LED, red_SW);
        gpio_set_level(GREEN_LED, green_SW);
        vTaskDelay(pdMS_TO_TICKS(5000));
        red_SW = ~red_SW & 0x01;
        green_SW = ~green_SW & 0x01;
    }

}

// Calculate moving average of speed

static double moving_average(double new_speed)
{
    // Subtract the oldest value from the sum if the buffer is full
    if (buffer_count == WINDOW_SIZE)
        sum -= speed_buffer[buffer_index];
    else
        buffer_count++;

    // Add the new value
    speed_buffer[buffer_index] = new_speed;
    sum += new_speed;
    
    buffer_index = (buffer_index + 1) % WINDOW_SIZE;

    return sum / buffer_count;
}

// Calculate speed using HCSR04
static void calculate_speed(void *arg)
{

    printf("Running calculate_speed()..\n");

    const int PULSE_DELAY_ms = 10; // 50 milliseconds = 50000 microseconds
    int timeout = 0;
    uint64_t pulse_start = 0,
            pulse_end = 0,
            pulse_duration = 0,
            prev_pulse_duration = 0;
    double time_interval = 0;

    const double SPEED_OF_SOUND = 0.0343; // cm per us
    double speed = 0,
            d1 = 0,
            d2 = 0;

    while(1)
    {
        // Send the pulse to the TRIGGER pin
        gpio_set_level(TRIGGER, 0);
        vTaskDelay(PULSE_DELAY_ms / portTICK_PERIOD_MS); // Delay 50 milliseconds
        gpio_set_level(TRIGGER, 1);
        esp_rom_delay_us(10); // Delay 10 microseconds
        gpio_set_level(TRIGGER, 0);

        // Wait for ECHO signal to go HIGH with a timeout
        timeout = 3000;
        while (gpio_get_level(ECHO) == 0 && timeout-- > 0)
        {
            esp_rom_delay_us(10);
        }
        if (timeout <= 0) continue;
        pulse_start = esp_timer_get_time();

        // Wait for ECHO signal to go LOW with a timeout
        timeout = 10000;
        while (gpio_get_level(ECHO) == 1 && timeout-- > 0)
        {
            esp_rom_delay_us(10);
        }
        if (timeout <= 0) continue;
        pulse_end = esp_timer_get_time();

        pulse_duration = pulse_end - pulse_start;
        d1 = (pulse_duration * SPEED_OF_SOUND) / 2.0;

        // Calculate the speed
        if (prev_pulse_duration != 0)
        {
            time_interval = (PULSE_DELAY_ms * 1000.0 + (pulse_duration + prev_pulse_duration) / 2.0) / 1000000.0; // seconds
            speed = (d2 - d1) / time_interval; // cm/s
            printf(
                "%d %.2f %.2f %.2f\n",
                red_SW,
                esp_timer_get_time() / 1000000.0,
                moving_average(speed),
                d1
            );
        }

        d2 = d1;
        prev_pulse_duration = pulse_duration;
    }

}