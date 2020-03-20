/* 
Minjun Wu, 2020.02.07
check machine IP address

ref: 
https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/
https://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine

final ref: 
https://www.includehelp.com/c-programs/get-ip-address-in-linux.aspx

 */

/*C program to get IP Address of Linux Computer System.*/
 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
 
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
 
int main()
{
    unsigned char ip_address[15];
    int fd;
    struct ifreq ifr;
     
    /*AF_INET - to define network interface IPv4*/
    /*Creating soket for it.*/
    fd = socket(AF_INET, SOCK_DGRAM, 0);
     
    /*AF_INET - to define IPv4 Address type.*/
    ifr.ifr_addr.sa_family = AF_INET;
     
    /*eth0 - define the ifr_name - port name
    where network attached.*/
    memcpy(ifr.ifr_name, "eno1", IFNAMSIZ-1);
     
    /*Accessing network interface information by
    passing address using ioctl.*/
    ioctl(fd, SIOCGIFADDR, &ifr);
    /*closing fd*/
    close(fd);
     
    /*Extract IP Address*/
    strcpy(ip_address,inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
     
    printf("System IP Address is: %s\n",ip_address);
     
    return 0;
}



