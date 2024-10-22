/*
 * main_user.c
 *
 *  Created on: Aug 8, 2022
 *      Author: cos omak
 */

#include <stdio.h>

//STM32 generated header files
#include "main.h"

//User generated header files
#include "User/main_user.h"
#include "User/util.h"

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h" // added
#include "semphr.h" // added for part 5.2

char main_string[50];
//uint32_t main_counter = 0;

// 5.1 globals
QueueHandle_t xQueue; // Queue handle added for inter-task communication
char receiver_string[50]; // String added to hold the received data value from the queue for printing
int32_t totalViews = 0; // Global counter for total video views (5.1.2)

// 5.2 globals
uint32_t SharedCounter = 0;
static SemaphoreHandle_t mutex;
QueueHandle_t xQueue;
char receiver_string[50];
char main_string[50];
uint32_t main_counter = 0;
SemaphoreHandle_t binarySemaphore; // from bestfriend. for interrupt-triggered task synchronization (like mutex from 5.2)

static void main_task(void *param){ // This is a task handler that prints periodically.	No changes are required. (5.3)


	while(1){
		print_str("Main task loop executing\r\n");
		sprintf(main_string,"Main task iteration: 0x%08lx\r\n",main_counter++);
		print_str(main_string);
		vTaskDelay(1000/portTICK_RATE_MS);
	}
}


static void vSenderTask(void *pvParameters){
	int32_t lValueToSend;
	BaseType_t xStatus;

	lValueToSend = (int32_t) pvParameters; // Get the data to be sent from task parameter

   for(;;) { // for 5.1.2
       // Generate a random number of views (simulate remote servers)
       lValueToSend = (rand() % 10) + 1; // Random value between 1 and 10

       // Send the value to the queue
       xStatus = xQueueSendToBack(xQueue, &lValueToSend, 0);
       if(xStatus != pdPASS) {
           print_str("Could not send to the queue.\r\n");
       }
       vTaskDelay(500 / portTICK_PERIOD_MS); // 1-second delay
   }

}


static void vReceiverTask(void *pvParameters){ // Task responsible for receiving data from the queue and printing it.
	int32_t lReceivedValue;
	BaseType_t xStatus;

	const TickType_t xTicksToWait = pdMS_TO_TICKS(100); // Time to wait for data from queue

   for(;;) { // (5.1.2)
       // Receive data from the queue
       xStatus = xQueueReceive(xQueue, &lReceivedValue, xTicksToWait);
       if(xStatus == pdPASS) {
           // Update total view count
           totalViews += lReceivedValue;

           // Print updated total view count
           sprintf(receiver_string, "Total Views = %ld\r\n", totalViews);
           print_str(receiver_string);
       } else {
           print_str("Could not receive from the queue.\r\n");
       }
   }
}

static void vDirectIncrementTask(void *pvParameters) {
    int32_t lValueToSend;

    for(;;) {
        // Generate a random number of views (simulate remote servers)
        lValueToSend = (rand() % 10) + 1; // Random value between 1 and 10

        // Directly increment the total views without using the queue
        increment(lValueToSend);

        // Print the updated total view count
        sprintf(receiver_string, "Direct Increment - Total Views = %ld\r\n", totalViews);
        print_str(receiver_string);

        // Delay before next update
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}


void increment(int num) { // for 5.1.2
   int buffer = 0;
   buffer = totalViews;    // Read current total views
   buffer += num;          // Increment by num (random views from sender)
   for(int i = 0; i < 100000; i++); // Artificial delay to simulate heavy processing
   totalViews = buffer;     // Update the total views
}

void main_user() {
    util_init(); // Initialize utilities

    // Create a queue capable of holding 5 integer values
    xQueue = xQueueCreate(5, sizeof(int32_t));

    if(xQueue != NULL) {
        // Queue-based sender tasks (remote servers)
        xTaskCreate(vSenderTask, "QueueSender1", 1000, NULL, 1, NULL);
        xTaskCreate(vSenderTask, "QueueSender2", 1000, NULL, 1, NULL);

        // Direct increment sender tasks (to demonstrate race conditions)
        xTaskCreate(vDirectIncrementTask, "DirectIncrementSender1", 1000, NULL, 1, NULL);
        xTaskCreate(vDirectIncrementTask, "DirectIncrementSender2", 1000, NULL, 1, NULL);

        // Create the receiver task (YouTube server) for queue-based increment
        xTaskCreate(vReceiverTask, "Receiver", 1000, NULL, 2, NULL);

        // Start the FreeRTOS scheduler
        vTaskStartScheduler();
    }

    while(1);
}


