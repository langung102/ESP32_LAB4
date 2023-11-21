#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define QUEUE_SIZE 5

#define TOTAL_TASK  3

enum TASK_ID {GET_TICK, GET_NUM_TASK_IN_QUEUE, GET_GPIO4_STATE};

struct request_t {
    int taskID;
    char* data;
};

typedef struct request_t Request;

QueueHandle_t xQueue;
bool isRead[TOTAL_TASK];

int genTaskID() {
    return rand() % 5;
}

void clearFlag() {
    for (int i = 0; i < TOTAL_TASK; ++i) {
        isRead[i] = false;
    }
}

bool isAllRead() {
    bool result = true;
    for (int i = 0; i < TOTAL_TASK; ++i) {
        result = result && isRead[i];
    }
    return result;
}

void printQueueInfo(QueueHandle_t xQueue) {
    printf("-----------------\n");
    printf("QUEUE INFORMATION\n");
    printf("Number of messages in queue   : %d\n", uxQueueMessagesWaiting(xQueue));
    printf("Number of free spaces in queue: %d\n", uxQueueSpacesAvailable(xQueue));
    printf("-----------------\n");
}

void ReceptionTask(void *pvParameter)
{
    Request xRequest;
    while(1)
	{
        xRequest.taskID = genTaskID();
        switch (xRequest.taskID)
        {
        case GET_TICK:
            xRequest.data = (char*)malloc(10*sizeof(char));
            sprintf(xRequest.data, "%lu", xTaskGetTickCount());
            break;
        case GET_NUM_TASK_IN_QUEUE:
            xRequest.data = (char*)malloc(10*sizeof(char));
            sprintf(xRequest.data, "%d", uxQueueMessagesWaiting(xQueue));
            break;
        case GET_GPIO4_STATE:
            xRequest.data = (char*)malloc(10*sizeof(char));
            sprintf(xRequest.data, "%d", gpio_get_level(GPIO_NUM_4));
            break;
        default:
            xRequest.data = (char*)malloc(30*sizeof(char));
            sprintf(xRequest.data, "%s", "from the other tasks\n");
            break;
        }
        if(xQueueSendToBack(xQueue, &xRequest, (TickType_t)10) == pdPASS) {
            printf("ReceptionTask sent ID: %d\n",xRequest.taskID);
        } else {
            printf("ReceptionTask sent ID failed!\n");
        }
        // printQueueInfo(xQueue);
        vTaskDelay(pdMS_TO_TICKS(rand()%(2000-200 + 1) + 200));
	}
}   

void FunctionalTaskA(void *pvParameter)
{
    Request xRequest;
    while(1)
	{
        if(xQueuePeek(xQueue, &xRequest, (TickType_t)10) == pdPASS) {
            if (xRequest.taskID == GET_TICK) {
                xQueueReceive(xQueue, &xRequest, (TickType_t)10);
                clearFlag();
                // HANDLE
                printf("Hello from Task A with tick count is: %s\n", xRequest.data);
                free(xRequest.data);
                vTaskDelay(pdMS_TO_TICKS(2000));
            } else {
                isRead[GET_TICK] = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
	}
}   

void FunctionalTaskB(void *pvParameter)
{
    Request xRequest;
    while(1)
	{
        if(xQueuePeek(xQueue, &xRequest, (TickType_t)10) == pdPASS) {
            if (xRequest.taskID == GET_NUM_TASK_IN_QUEUE) {
                xQueueReceive(xQueue, &xRequest, (TickType_t)10);
                clearFlag();
                // HANDLE
                printf("Hello from Task B with num task in queue is %s\n", xRequest.data);
                free(xRequest.data);
                vTaskDelay(pdMS_TO_TICKS(2000));
            } else {
                isRead[GET_NUM_TASK_IN_QUEUE] = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
	}
}  

void FunctionalTaskC(void *pvParameter)
{
    Request xRequest;
    while(1)
	{
        if(xQueuePeek(xQueue, &xRequest, (TickType_t)10) == pdPASS) {
            if (xRequest.taskID == GET_GPIO4_STATE) {
                xQueueReceive(xQueue, &xRequest, (TickType_t)10);
                clearFlag();
                // HANDLE
                printf("Hello from task C with GPIO4 level is %s\n", xRequest.data);
                free(xRequest.data);
                vTaskDelay(pdMS_TO_TICKS(2000));
            } else {
                isRead[GET_GPIO4_STATE] = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
	}
}  

void ErrorHandlerTask(void *pvParameter)
{
    Request xRequest;
    while(1)
	{
        if(xQueuePeek(xQueue, &xRequest, (TickType_t)10) == pdPASS) {
            if (isAllRead()) {
                xQueueReceive(xQueue, &xRequest, (TickType_t)10);
                printf("\n  Ignoring request with task ID: %d\n\n", xRequest.taskID);
                free(xRequest.data);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
	}
}   


void app_main(void)
{
    xQueue = xQueueCreate(QUEUE_SIZE, sizeof(Request));
    clearFlag();

    xTaskCreate(&ReceptionTask, "ReceptionTask", 2048, NULL, 1, NULL);
    xTaskCreate(&FunctionalTaskA, "FunctionalTaskA", 2048, NULL, 1, NULL);
    xTaskCreate(&FunctionalTaskB, "FunctionalTaskB", 2048, NULL, 1, NULL);
    xTaskCreate(&FunctionalTaskC, "FunctionalTaskC", 2048, NULL, 1, NULL);
    xTaskCreate(&ErrorHandlerTask, "ErrorHandlerTask", 2048, NULL, 1, NULL);
}