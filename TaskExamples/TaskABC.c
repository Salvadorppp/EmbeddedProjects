#include <driver/ledc.h>
#include <esp_event.h>
#include <soc/ledc_reg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <time.h>

#define ESP_INTR_FLAG_DEFAULT 0
#define Switch1 GPIO_NUM_22
#define Switch2 GPIO_NUM_23
#define LED_DUTY (1024)
#define LED_FADE_TIME (2000)

static const uint8_t msg_queue_len = 10;
static QueueHandle_t msg_queue;
static SemaphoreHandle_t mutex;
int Pausa;
int cola;
//--------------TAREA A---------------------------//
static void Tarea_A(void *parameter){
	while(1){
			if(xSemaphoreTake(mutex,25)==pdTRUE){
				int state1 = gpio_get_level(Switch1);

				if(state1 == 1){
					cola = 0;
					xQueueSend(msg_queue, &cola, 10);
				}
				xSemaphoreGive(mutex);
				Pausa = 200;
				vTaskDelay(Pausa / portTICK_PERIOD_MS);
			}
		  }
}


//--------------TAREA B----------------------------//
static void Tarea_B(void *parameter){
	while(1){
			if(xSemaphoreTake(mutex,25)==pdTRUE){
				int state2 = gpio_get_level(Switch2);

				if(state2 == 1){
					cola = 1;
					xQueueSend(msg_queue, &cola, 10);
				}
				xSemaphoreGive(mutex);
				Pausa = 200;
				vTaskDelay(Pausa / portTICK_PERIOD_MS);
			}
		  }
	}


//---------------TAREA C----------------------------//
static void Tarea_C(void *parameter){
	int item;
	//Configuracion del Timer
	ledc_timer_config_t LED_Timer = {
			.duty_resolution = LEDC_TIMER_10_BIT,
			.freq_hz = 1000,
			.speed_mode = LEDC_HIGH_SPEED_MODE,
			.timer_num = LEDC_TIMER_0,
			.clk_cfg = LEDC_AUTO_CLK,
	};
   	//Configuracion del canal
	ledc_channel_config_t LED_Channel = {
			.gpio_num = 2,
			.speed_mode = LEDC_HIGH_SPEED_MODE,
			.channel = LEDC_CHANNEL_0,
			.intr_type = LEDC_INTR_DISABLE,
			.timer_sel = LEDC_TIMER_0,
			.duty = 0,
			.hpoint = 0
   	};
	ledc_timer_config(&LED_Timer);
	ledc_channel_config(&LED_Channel);
	ledc_fade_func_install(ESP_INTR_FLAG_IRAM|ESP_INTR_FLAG_SHARED);
	while(true){
		if(xQueueReceive(msg_queue, (void *)&item, 0)==pdTRUE){
			printf("%d \n",item);
			switch (item){
				case 0:
					ledc_set_fade_with_time(LED_Channel.speed_mode, LED_Channel.channel, LED_DUTY, LED_FADE_TIME);
					ledc_fade_start(LED_Channel.speed_mode, LED_Channel.channel, LEDC_FADE_WAIT_DONE);
					break;
				case 1:
					ledc_set_fade_with_time(LED_Channel.speed_mode, LED_Channel.channel, 0, LED_FADE_TIME);
					ledc_fade_start(LED_Channel.speed_mode, LED_Channel.channel, LEDC_FADE_WAIT_DONE);
					break;
				default:
					gpio_set_level(GPIO_NUM_2, 0);
					break;
			}
		}
		vTaskDelay(1000/portTICK_PERIOD_MS);
	}
}

void app_main(void){
	gpio_config_t io_conf;
	io_conf.pin_bit_mask = 1ULL<<GPIO_NUM_22 | 1ULL<<GPIO_NUM_23;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    gpio_config(&io_conf);


    TaskHandle_t TaskHandle_m = NULL;
    msg_queue = xQueueCreate(msg_queue_len, sizeof(uint32_t));
    mutex = xSemaphoreCreateMutex();
    xTaskCreate(Tarea_C, "Tarea_C", 2048, NULL, 2, &TaskHandle_m);
    xTaskCreate(Tarea_A, "Tarea_A", 2048, NULL, 1, NULL);
    xTaskCreate(Tarea_B, "Tarea_C", 2048, NULL, 1, NULL);

	vTaskDelete(NULL);
    while(1){
    }
}
