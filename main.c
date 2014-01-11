# include <unistd.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <string.h>
# include <netinet/in.h>
# include <stdio.h>
# include <stdlib.h>

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

main(){
    int sockfd,retval,n;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    char buf[10000]; 
    int i;

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0){
        perror("sock:");
        exit(1);
    }
    clilen = sizeof(struct sockaddr_in);    
    while(1){
        printf(" before recvfrom\n");   
        n=recvfrom(sockfd,buf,10000,0,(struct sockaddr *)&cliaddr,&clilen);
        printf(" rec'd %d bytes\n",n);

        struct iphdr *ip_hdr = (struct iphdr *)buf;

        printf("IP header is %d bytes.\n", ip_hdr->ihl*4);

        for (i = 0; i < n; i++) {
        printf("%02X%s", (uint8_t)buf[i], (i + 1)%16 ? " " : "\n");
        }
        printf("\n");

        struct icmphdr *icmp_hdr = (struct icmphdr *)((char *)ip_hdr + (4 * ip_hdr->ihl));

        printf("ICMP msgtype=%d, code=%d", icmp_hdr->type, icmp_hdr->code);
    }
}