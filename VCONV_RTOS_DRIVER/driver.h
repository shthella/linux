#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


#define IMAGE_WIDTH       224
#define IMAGE_HEIGHT      224
#define PIXEL_SIZE        4  
#define IMAGE_SIZE        (IMAGE_WIDTH * IMAGE_HEIGHT * PIXEL_SIZE)

#define OUTPUT_WIDTH      112
#define OUTPUT_HEIGHT     112
#define OUTPUT_SIZE       (OUTPUT_WIDTH * OUTPUT_HEIGHT * PIXEL_SIZE)
#define CMD_SIZE           28

#define INPUT_START_ADDRESS  0x1FFFFFFF
#define DDR_BASE_ADDRESS      0x00000000
#define OUTPUT_START_ADDRESS  0x3FFFFFFF
#define DDR_END_ADDRESS       0x7FFFFFFF

// Define register addresses
#define INPUT_REGISTER            0x10000000
#define COMMAND_REGISTER          0x10000004
#define OUTPUT_REGISTER           0x10000008
#define START_REGISTER            0x1000000C
#define STOP_REGISTER             0x10000010
#define INTERRUPT_STATUS_REGISTER 0x10000014
#define INTERRUPT_CLEAR_REGISTER  0x10000018


#define SUCCESS 0
#define FAIL 1
#define NULL_ADDRESS_ERROR 2
#define INVALID_DDR_ADDRESS 3
#define COPY_FAIL 4
#define NULL_BUFFER 5

extern bool is_interrupt_triggered;
extern uint32_t output_buffer[OUTPUT_WIDTH][OUTPUT_HEIGHT];

// Function declarations
int set_input_buffer(uint32_t start_address, uint32_t* input_image_buffer, uint32_t size);
int set_command_buffer(uint32_t start_address, uint32_t* cmd_buffer, size_t size);
int set_output_buffer(uint32_t start_address,uint32_t* output_buffer, uint32_t size);
int wait_for_interrupt_and_clear(void);

int convolution_init(uint32_t *input_image_buffer, uint32_t *command_buffer);
int convolution_start(void);
int convolution_stop(void);
int convolution_read(void);

#endif

