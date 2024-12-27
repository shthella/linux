#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define IMAGE_PATH  "input.bin"
#define CMD_DATA  "cmd.bin"
#define OUTPUT_FILE "output.bin"

#define DEVICE_NAME "/dev/conv_ip"
#define INPUT_SIZE 224*224*4
#define OUTPUT_SIZE 112*112*4
#define CMD_SIZE 28

#define IOCTL_INPUT_IMAGE       _IOW('C', 1, unsigned long)
#define IOCTL_CMD               _IOW('C',2,unsigned long)
#define IOCTL_START_CONV        _IO('C', 3)
#define IOCTL_STOP_CONV         _IO('C', 4)



int main(){
    int fd;
    FILE *infile, *outfile, *cmdfile; //files to store input,commands and output    
    char input_data[INPUT_SIZE];
    char output_data[OUTPUT_SIZE];
    char cmd_data[CMD_SIZE];

   // char *input = argv[1];

    // Open the device file
    fd = open(DEVICE_NAME, O_RDWR); //fd has unique value for this open call store in fd table in (fops) to open in kernel. 
    if (fd < 0){
        printf("Failed to open device");
        return -1;
    }
    printf("Device opened successfully\n");
    
/*-------FILE open and read image to buffer and read cmd.tx and read to buffer--------------*/
 
    // open image from input file
    infile = fopen(IMAGE_PATH, "rb");
    if (!infile){
        printf("Failed to open image.bin");
        close(fd);
        return -1;
    }
    
    //read command data to cmdfile
    cmdfile = fopen(CMD_DATA,"rb");
    if(!cmdfile){
    	printf("error for open cmd.bin");
    	close(fd);
    	return -1;
    	}
    printf("the bin files are opened\n");
    //read the data to input data buffer from the infile file  and store in input_data
    if(fread(input_data,INPUT_SIZE,1, infile)<0){
    	perror("Failed to write to inputfile");
        close(fd);
        return -1;
    } 
       
    //read the dat from cmd file to cmd_data
    if(fread(cmd_data,CMD_SIZE,1,cmdfile) < 0){
    	perror("Failed to write to cmdfile");
        close(fd);
        return -1;
    } 
    	
    
    fclose(cmdfile);
    fclose(infile);
    printf("image.bin and cmd.bin is loaded siuccessfully to files\n");
/*--------------------end-------------------*/


/*----after reading to buffer then write to kernel using write system interface-------------*/

    //Ioctl image 
    
    if(ioctl(fd,IOCTL_INPUT_IMAGE,NULL)){
        printf("failed to get image from user");
        close(fd);
        return -1;
    }

    // Write input data to the driver
    if(write(fd, input_data, INPUT_SIZE) < 0){ //from input_data to fd , fd links the kernel (to,from and size )
       perror("Failed to input to device");
        close(fd);
        return -1;
    } 
    	
    
    //ioctl cmd
    if(ioctl(fd,IOCTL_CMD,NULL)){
        printf("failed to get image from user");
        close(fd);
        return -1;
    }
    
    if(write(fd,cmd_data,CMD_SIZE) < 0){
    	perror("Failed to cmd to device");
        close(fd);
        return -1;
    } 
    	
    	
    printf("Input data sent to driver\n");
/*--------------------end-------------------*/

//then start the convolution using ioctl     
    
    // Start convolution
    if (ioctl(fd, IOCTL_START_CONV, NULL) < 0){
        printf("Failed to start convolution");
        close(fd);
        return -1;
    }
    printf("Convolution started\n");
/*--------------------end-------------------*/
  
    // Read output data from the driver
    if(read(fd, output_data, OUTPUT_SIZE) < 0){
    	perror("Failed to read to user");
        close(fd);
        return -1;
    } 
    printf("data is %x\n" ,output_data);
       
    printf("Output data received from driver\n");
/*--------------------end-------------------*/

/*-------after read data from keernel store it in buffer-----------*/

 // Save output data to file
    outfile = fopen(OUTPUT_FILE, "wb");
    if (!outfile){
        printf("Failed to open output file");
        close(fd);
        return -1;
    }

/*-----------the write to file from buffer--------------*/

     //write the output data into a output file  
    if(fwrite(output_data, 1, OUTPUT_SIZE, outfile) < 0){
    	perror("Failed to write to outputfile");
        close(fd);
        return -1;
    } 
       
    fclose(outfile);
    printf("Output data saved to file\n");
/*--------------------end-------------------*/

    // Stop convolution 
    if (ioctl(fd, IOCTL_STOP_CONV, NULL) < 0){
        printf("Failed to stop convolution");
        close(fd);
        return -1;
    }
    printf("Convolution stopped\n");

    // Close the device
    close(fd);
    printf("Device closed\n");
    return 0;
}
