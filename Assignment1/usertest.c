#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<linux/fcntl.h>
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>      //open
#include<sys/stat.h>    //close
#include<sys/ioctl.h>  //ioctl

char buf[32];
int32_t channel_num;
char alignment;
#define MAGIC_NUM 100
#define channel_no _IOW(MAGIC_NUM,0,int32_t*)
#define align _IOW(MAGIC_NUM,1,char*)

int main()
{

	printf("Starting Device...!\n");
/*---------------------------OPEN-------------------------------*/

    int fd;
    char options;
    fd = open("/dev/adc_char_dev",O_RDWR);
    if(fd<0) 
	{ 
		printf("Error in Opening\n"); 
	}
		return 0;

   		
	while(1);
	{
	printf("Give the options\n");
	printf("1.Select the ADC channel\n");
   	printf("2.Select the alignment\n");
   	printf("3.Read ADC value from the channel which is selected\n");
	printf("4.exit\n");

scanf("  %c",&options);
		printf("Selection is %c\n\n",options);
	
		switch(options)
		{
		
		case '1':
			printf("Enter the channel no. from 0 to 7\n\n");
			scanf("%d",&channel_num);
			if(channel_num <= 7 && channel_num >=0)
			{
			printf("Writing the value to the driver\n\n");
			printf("Selected channel number is %d\n\n",channel_num);
			ioctl(fd,channel_no,(int32_t*)&channel_num);
			}
			else
			{
			printf("Please select a channel from 0 to 7 range\n\n");
			}
			break; 	

		case '2':
			printf("Select the alignment for the reading (L/R\n\n");
			scanf("   %c",&alignment);
			if(alignment == 'L' | alignment == 'R')
			{
			printf("Writ the value to the driver\n\n");
			printf("Your selected alignment is %c\n\n",alignment);
			ioctl(fd,align,(char*)&alignment);
			}
			else
			{	
			printf("Enter a valid alignment\n\n");
			}
			break;	
		
		case '3':
			printf("Data is reading...");
			read(fd,buf,10);
			printf("Done\n");
			printf("ADC Channel Reading = %s\n\n",buf); 
			break;			


		case '4': 
			close(fd);
			exit(1);
			break;		

		default:
			printf("Please enter a valid option = %c\n",options);
			break;
		}
	}
	close(fd);

}
			












