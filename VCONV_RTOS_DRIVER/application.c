#include "driver.h"
#include <stdio.h>
#include <stdbool.h>


// Buffer to hold the input image from user
uint32_t input_image_buffer[IMAGE_WIDTH][IMAGE_HEIGHT];

// Command buffer (7 commands, 4 bytes each)
uint32_t command_buffer[7] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

// Output buffer to hold the output 
uint32_t output_buffer[OUTPUT_WIDTH][OUTPUT_HEIGHT];

bool is_interrupt_triggered = false;

// Function to simulate taking input from the user
int load_user_input(uint32_t input_image_buffer[IMAGE_WIDTH][IMAGE_HEIGHT]) {
    for (int i = 0; i < IMAGE_WIDTH; i++) {
        for (int j = 0; j < IMAGE_HEIGHT; j++) {
            input_image_buffer[i][j] = 0x5;  
        }
    }
    if (input_image_buffer == NULL) 
    { 
        printf("Error: Input image buffer is null.\n");
        return FAIL;
    }
    return SUCCESS;
}

int main(void) {
    // Load user input (image) into the input_image_buffer
    if (load_user_input(input_image_buffer) != SUCCESS)
    {
        printf("User input not loaded into input_image_buffer.\n");   
    	return -1;
    }
    printf("User input loaded into input_image_buffer.\n");

    // Initialize convolution with input image and command buffers
    if (convolution_init((uint32_t *)input_image_buffer, command_buffer) != SUCCESS)
    {
        printf("Convolution initialization failed.\n"); 
    	return -1;
    }
         printf("Convolution initialized.\n");    

    // Start the convolution process
    if (convolution_start() != SUCCESS)
    {
        printf("Convolution start failed.\n");
    	return -1;
    }
        printf("Convolution process started.\n");


    // Check if the interrupt has been triggered
    if (wait_for_interrupt_and_clear() != SUCCESS) {
        printf("Interrupt handling failed.\n");
        return -1;  // Failure: Interrupt not triggered or not cleared
    }
    printf("Interrupt triggered and cleared.\n");

    //Stop the convolution process
    if (convolution_stop() !=SUCCESS)
    {
        printf("Convolution stop failed.\n");
    	return -1;
    }
        printf("Convolution process stopped.\n");

    // Read the output buffer from DDR
    if (convolution_read() !=SUCCESS)
    {
       printf("Convolution read failed.\n");
       return -1;
    }
      printf("Convolution output read successfully.\n");
        
    return 0;
}

