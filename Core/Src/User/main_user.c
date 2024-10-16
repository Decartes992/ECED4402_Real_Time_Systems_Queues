/*
 * main_user.c
 *
 *  Created on: Aug 8, 2022
 *      Author: Andre Hendricks
 * 
 * Description: This program simulates a YouTube server keeping track of the number of views
 * a video receives. Sender tasks simulate different users accessing the video, while the
 * receiver task (server) keeps track of total views by receiving data from the queue.
 */

#include <stdio.h>

// STM32 generated header files
#include "main.h"

// User generated header files
#include "User/main_user.h"
#include "User/util.h"

// Required FreeRTOS header files
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Global queue handle and string buffers
QueueHandle_t xQueue;
char receiver_string[50];  // Buffer for receiver messages
char main_string[50];      // Buffer for main task messages
uint32_t main_counter = 0; // Counter for main task
int global_var = 0;        // Global variable to store total views

/*
 * increment function
 *
 * This function simulates a delay while incrementing the global view count.
 * It adds the passed number to the global variable that tracks total views.
 */
void increment(int num) {
    int buffer = 0;
    buffer = global_var;
    buffer += num;

    // Artificial delay to simulate network latency or processing delay
    for (int i = 0; i < 100000; i++);

    global_var = buffer;
}

// Thread index for the sender task
int i_thread = 0;

/****************************************************
 * vSenderTask_Viewers
 * 
 * This task simulates viewers (users) accessing the video. It sends random view counts
 * to the queue. Each value represents the number of users accessing the video at a given time.
 ****************************************************/
static void vSenderTask_Viewers(void) {
    // Predefined array of random view counts
    int32_t randoms[] = {23, 5, 7, 2, 5, 8, 9, 4, 2, 4, 6, 4, 3, 2, 3, 6, 3, 2, 6, 7, 8, 6}; // 22 elements
    int32_t lValueToSend;  // Value to send to the queue
    BaseType_t xStatus;     // Status of the queue operation

    // Loop through each element in the randoms array
    for (i_thread = 0; i_thread < 23; i_thread++) {
        lValueToSend = (uint32_t) randoms[i_thread];  // Get the current view count

        // Send the view count to the back of the queue
        xStatus = xQueueSendToBack(xQueue, &lValueToSend, 0);
        if (xStatus != pdPASS) {
            print_str("Could not send to the queue.\r\n");
        }

        // Delay to simulate viewers accessing the video at intervals
        vTaskDelay(100);
    }

    // No more viewers to send after all values in the array are used
    if (i_thread >= 23) print_str("No More Viewers.\r\n");

    // Delay before stopping the task
    vTaskDelay(10000);
}

/****************************************************
 * vReceiverTask_YoutubeServer
 * 
 * This task simulates the YouTube server receiving data from the queue.
 * It retrieves view counts, updates the total, and prints the current view count.
 ****************************************************/
static void vReceiverTask_YoutubeServer(void *pvParameters) {
    int32_t lReceivedValue;    // Value received from the queue
    BaseType_t xStatus;        // Status of the queue operation
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100);  // Maximum wait time for queue

    for (;;) {
        // Check if the queue is not empty
        if (uxQueueMessagesWaiting(xQueue) != 0) {
            print_str("Queue is not empty!\r\n");
        }

        // Try to receive data from the queue
        xStatus = xQueueReceive(xQueue, &lReceivedValue, xTicksToWait);

        // If data was successfully received
        if (xStatus == pdPASS) {
            // Increment the total view count using the received value
            increment(lReceivedValue);

            // Print the number of views just added
            sprintf(receiver_string, "Viewed just now = %ld\r\n", lReceivedValue);
            print_str(receiver_string);

            // Delay between receiving new data
            vTaskDelay(100);
        } else {
            // Queue is empty or data not received, print the current total view count
            print_str("Could not receive from the queue.\r\n");
            sprintf(receiver_string, "Total Viewers = %ld\r\n", global_var);
            print_str(receiver_string);

            // Delay before checking the queue again
            vTaskDelay(100);
        }
    }
}

/****************************************************
 * main_task
 * 
 * A simple task that runs periodically and prints a message to the console.
 * It increments and prints a counter to indicate the task is running.
 ****************************************************/
static void main_task(void *param) {
    while (1) {
        print_str("Main task loop executing\r\n");

        // Print the current iteration of the main task
        sprintf(main_string, "Main task iteration: 0x%08lx\r\n", main_counter++);
        print_str(main_string);

        // Delay before the next iteration
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

/****************************************************
 * main_user
 * 
 * Initializes the system, creates the tasks, and starts the FreeRTOS scheduler.
 * It creates a sender task to simulate viewers and a receiver task to act as the YouTube server.
 ****************************************************/
void main_user() {
    // Initialize utilities
    util_init();

    // Create the queue with space for 5 int32_t values
    xQueue = xQueueCreate(5, sizeof(int32_t));

    // If the queue was created successfully
    if (xQueue != NULL) {
        // Create the sender task to simulate viewers
        xTaskCreate(vSenderTask_Viewers, "Sender1", 1000, NULL, 1, NULL);

        // Create the receiver task to simulate the YouTube server
        xTaskCreate(vReceiverTask_YoutubeServer, "Receiver", 1000, NULL, 1, NULL);

        // Start the scheduler to begin task execution
        vTaskStartScheduler();
    }

    // Infinite loop to prevent program from exiting
    while (1);
}
