#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/uart.h"
#include <math.h>
#include <string.h>

#define CAPTURE_CHANNEL 0
#define STARTING_POINT 19
#define CAPTURE_DEPTH 28
#define CONFIG_SAMPLES 2000
#define DETECT_SAMPLES 50
#define PWM_PIN 16  
#define LED_PIN 25  
#define BUZZER_PIN 12
#define INTERUPT_PIN 17
#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 8
#define UART_RX_PIN 9

uint8_t capture_buf[CAPTURE_DEPTH];
uint dma_chan;
dma_channel_config cfg;

const float reference_sample = 0.2f;
float detect_sample = reference_sample + 0.01f;
float mean = 0.0f;
float distance = 0.0f;
float reference_mean = 255.0f;
float count_threshold = 255.0f;
float detect_means[DETECT_SAMPLES];
float means[CONFIG_SAMPLES];
bool configured = false;
absolute_time_t start_time = 0;
absolute_time_t last_time;
bool plot_histogram_on = false;
float far_detect = 0;
int samples_count = 0;
float detect_mean = 0;
char received_value;
bool flag = false;
int value = 0;

const uint64_t TARGET_LOOP_TIME_US = 5000;

void generate_pwm_signal() {
    gpio_put(PWM_PIN, 0);
    sleep_us(100);
    gpio_put(PWM_PIN, 1);
}

void serial_plot() {
    for (int i = 0; i < CAPTURE_DEPTH; ++i) {
        printf("%-3d ", capture_buf[i]);
    }
    printf("\n");
}

void plot_histogram() {
    printf("%-5.2f ", far_detect);
    printf("%-5.2f ", count_threshold);
    for (int i = 0; i < DETECT_SAMPLES; ++i) {
        printf("%-5.2f ", detect_means[i]);
    }
    printf("\n");
}

void exchange_data() {
    absolute_time_t time = get_absolute_time();
    int64_t elapsed_time = absolute_time_diff_us(last_time, time);
    if (elapsed_time > 20000 || !flag) {
        if(configured){
            uart_putc(UART_ID, (char)('0' + value));
        } else {
            uart_putc(UART_ID, '+');
        }

    flag = true;
    last_time = get_absolute_time();
    }

}

void configure_DMA_ADC() {
    adc_init();
    adc_select_input(CAPTURE_CHANNEL);
    adc_fifo_setup(true, true, 1, false, true);

    adc_set_clkdiv(0);
    dma_chan = dma_claim_unused_channel(true);
    cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);
}

void ADC_reading() {
    adc_fifo_drain();
    dma_channel_configure(dma_chan, &cfg, capture_buf, &adc_hw->fifo, CAPTURE_DEPTH, true);
    adc_run(true);
    dma_channel_wait_for_finish_blocking(dma_chan);
    adc_run(false);
}

void find_mean() {
    float sum = 0.0f;
    for (int i = STARTING_POINT; i < CAPTURE_DEPTH; ++i) {
        sum += (float)capture_buf[i];
    }
    mean = sum / ((float)(CAPTURE_DEPTH - STARTING_POINT));
}

void sort_descending(float arr[], int len) {
    for (int i = 0; i < len - 1; ++i) {
        for (int j = i + 1; j < len; ++j) {
            if (arr[i] < arr[j]) {
                float temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
}

void find_threshold() {
    if(!configured){

        static const int start = 3000;
        static int thres_iter = -1;
        static float sum = 0.0f;

        thres_iter++;
        if (thres_iter > start && thres_iter < start + CONFIG_SAMPLES) {
            sum += mean;
            means[thres_iter - start] = mean;
        }

        if (thres_iter == start + CONFIG_SAMPLES) {
            reference_mean = sum / CONFIG_SAMPLES;
            sort_descending(means, CONFIG_SAMPLES);
            count_threshold = means[(int)(CONFIG_SAMPLES * reference_sample)];
        }

        if (thres_iter == start + CONFIG_SAMPLES + 1) {
            configured = true;
        }
    }
}

void detect() {
    static int detect_iter = 0;
    static float detect_sum = 0.0f;
    
    if(configured){
        detect_means[detect_iter] = mean;
        detect_iter++;

        if (detect_iter == DETECT_SAMPLES) {
            for (int i = 0; i < DETECT_SAMPLES; i++) {
                float prev_mean = detect_means[i];
                detect_sum += prev_mean;
                if (prev_mean > count_threshold) {
                    samples_count++;
                }
            }
            detect_mean = detect_sum / DETECT_SAMPLES;
            float short_detect = samples_count - 0.9f * DETECT_SAMPLES;
            far_detect = samples_count - detect_sample * DETECT_SAMPLES;
            
            if (short_detect > 0) {
                distance = detect_mean / 255.0f;
                gpio_put(INTERUPT_PIN, 0);
            } else if (far_detect > 0) {
                distance = reference_mean*(far_detect/ (0.9f * DETECT_SAMPLES - detect_sample * DETECT_SAMPLES)) / 255.0f;
                gpio_put(INTERUPT_PIN, 0);
            } else {
                distance = 0.0f;
                gpio_put(INTERUPT_PIN, 1);
            }

            if(plot_histogram_on){
                plot_histogram();
            }

            detect_iter = 0;
            samples_count = 0;
            detect_sum = 0.0f;
        }
    }
}

void play_tone() {
    int play_time = 100;
    if (distance > 0.0f) {
        gpio_put(BUZZER_PIN, 1);
        sleep_us((int)(20 + play_time * distance));
        gpio_put(BUZZER_PIN, 0);
    }
    sleep_us((int)(play_time * (1.0f - distance)));
}

void signal_by_led() {
    if (distance > 0.0f) {
        gpio_put(LED_PIN, 1);
    } else {
        gpio_put(LED_PIN, 0);
    }

}
void equal_intervals(){
    absolute_time_t end_time = get_absolute_time();
    int64_t elapsed_time_us = absolute_time_diff_us(start_time, end_time);
    if (elapsed_time_us < TARGET_LOOP_TIME_US) {
        sleep_us(TARGET_LOOP_TIME_US - elapsed_time_us);
    }
    // printf("%lld\n", elapsed_time_us);
}

int main() {
    stdio_init_all();

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    gpio_init(PWM_PIN);
    gpio_set_dir(PWM_PIN, GPIO_OUT);
    gpio_put(PWM_PIN, 1);

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);

    gpio_init(INTERUPT_PIN);
    gpio_set_dir(INTERUPT_PIN, GPIO_OUT);
    gpio_put(INTERUPT_PIN, 0);

    adc_gpio_init(26 + CAPTURE_CHANNEL);
    configure_DMA_ADC();

    sleep_ms(10000);

    last_time = get_absolute_time();

    while (true) {
        start_time = get_absolute_time();

        generate_pwm_signal();
        ADC_reading();
        find_mean();
        find_threshold();
        detect();
        signal_by_led();
        play_tone();
        exchange_data();

        equal_intervals();
    }

    return 0;
}
