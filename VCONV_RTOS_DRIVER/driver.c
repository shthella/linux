#include "driver.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

// Function to load the input buffer into DDR
int set_input_buffer(uint32_t start_address, uint32_t* input_image_buffer, uint32_t size) {
   uint32_t *ddr_address = (uint32_t *)start_address;
   if (input_image_buffer == NULL || ddr_address == NULL) {
        printf("NULL Error\n");
        return NULL_ADDRESS_ERROR;
   }
  
   if ((ddr_address + size) > DDR_END_ADDRESS )
   {
     printf("Invalid address\n");
     return INVALID_DDR_ADDRESS;
   }
   
   memcpy(ddr_address, input_image_buffer, size);
   if (memcmp(ddr_address, input_image_buffer, size) != 0 )
   {
     printf("Error: Input buffer copy failed.\n");
     return COPY_FAIL;
   } 
   else
   {
      printf("Input buffer loaded into DDR at address 0x%X with size %d bytes.\n", start_address, size);
   }
   return SUCCESS;
 }

// Function to load the command buffer into DDR
int set_command_buffer(uint32_t start_address, uint32_t* command_buffer, size_t size) {
   uint32_t *ddr_address = (uint32_t *)start_address;
   if (command_buffer == NULL || ddr_address == NULL) {
        printf("NULL Error\n");
        return NULL_ADDRESS_ERROR;
    }
   if ((ddr_address + size) > DDR_END_ADDRESS )
   {
     printf("Invalid address\n");
     return INVALID_DDR_ADDRESS;
   }
   
   memcpy(ddr_address, command_buffer, size);
   if (memcmp(ddr_address, command_buffer, size) != 0 )
   {
      printf("Error: command buffer copy failed.\n");
      return COPY_FAIL;
   }
   else 
   {
      printf("Command buffer loaded into DDR at address 0x%X with size %d bytes.\n", start_address, size);
      
   }
   return SUCCESS;
}

// Function to read the output from DDR
int set_output_buffer(uint32_t start_address,uint32_t *output_buffer, uint32_t size) {
    uint32_t *ddr_address = (uint32_t *)start_address;
    if (output_buffer == NULL || ddr_address == NULL) {
        printf("NULL Error.\n");
        return NULL_ADDRESS_ERROR;
    }
    if ((ddr_address + size) > DDR_END_ADDRESS )
    {
      printf("Invalid address\n");
      return INVALID_DDR_ADDRESS;
    }
    memcpy(output_buffer, ddr_address, size);
    if (memcmp(output_buffer, ddr_address, size) != 0 )
    {
	printf("Error: output copy failed\r\n");
	return COPY_FAIL;
    }
    else 
    {
	printf("Output loaded into DDR at address 0x%X with size %d bytes.\n", start_address, size);
    }
    return SUCCESS;
}

// Function to wait for interrupt and clear it if the operation is complete
int wait_for_interrupt_and_clear(void) {
    uint32_t *interrupt_status_reg = (uint32_t *)INTERRUPT_STATUS_REGISTER;
    uint32_t *interrupt_clear_reg = (uint32_t *)INTERRUPT_CLEAR_REGISTER;
    
 // Wait until the interrupt is triggered
    while (!is_interrupt_triggered) {
        // Check the hardware status register
        if (*interrupt_status_reg & 0x01) {
            printf("Hardware interrupt triggered.\n");
            is_interrupt_triggered = true;
        }
        else
        {
          printf("Hardware interrupt not triggered.\n");
        }
    }

    // Attempt to clear the interrupt
    *interrupt_clear_reg = 0x01;

    // Verify if the interrupt was cleared successfully
    if ((*interrupt_status_reg & 0x01) == 0) {
        is_interrupt_triggered = false;
        return 0;  // Success case
    } else {
        printf("Interrupt clearing failed.\n");
        return -1;  // Failure case: Interrupt not cleared
    }
    return SUCCESS;
}

// Function to start the convolution process
int convolution_start(void) {
    uint32_t *start_reg = (uint32_t *)START_REGISTER;
    *start_reg = 0x01;  // Start convolution
    if ((*start_reg & 0x01) == 0){
    	printf("Convolution start Failed.\n");
    	return FAIL;
    }
    printf("Convolution started.\n");
    return SUCCESS;
}

// Function to stop the convolution process
int convolution_stop(void) {
    uint32_t *stop_reg = (uint32_t *)STOP_REGISTER;
    *stop_reg = 0x01;  // Stop convolution
    if ((*stop_reg & 0x01) == 0){
    printf("Convolution stop Failed.\n");
    return FAIL;
    }
    printf("Convolution stopped.\n");
    return SUCCESS;
}

// Initialize convolution with input image and command buffers
int convolution_init(uint32_t *input_image_buffer, uint32_t *command_buffer) {
    if (input_image_buffer == NULL || command_buffer == NULL) {
        printf("Error: Buffers cannot be NULL during initialization.\n");
        return NULL_BUFFER;
    }
	
    if (set_input_buffer(INPUT_START_ADDRESS, input_image_buffer, IMAGE_SIZE) != SUCCESS)
    {
    	return FAIL;
    }
    if (set_command_buffer(DDR_BASE_ADDRESS,command_buffer,CMD_SIZE) != SUCCESS)
    {
    	return FAIL;
    }
    return SUCCESS;
}

// Function to read the output buffer from DDR
int convolution_read(void) {
    if (set_output_buffer(OUTPUT_START_ADDRESS, output_buffer,OUTPUT_SIZE) != SUCCESS)
    {
    	return FAIL;
    }
    return SUCCESS;
}

