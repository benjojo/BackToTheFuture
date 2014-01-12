# include <unistd.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <string.h>
# include <netinet/in.h>
# include <stdio.h>
# include <stdlib.h>

# include <netinet/ip.h>
# include <netinet/ip_icmp.h>

main(){
    int sockfd,retval,n;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    char buf[10000]; // Set the buffer size to 10k
    int i;

    // Make the raw socket, but only make it look for ICMP packets.
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); 
    if (sockfd < 0){
        perror("sock:");
        exit(1);
    }
    clilen = sizeof(struct sockaddr_in);    
    while(1){
        printf(" before recvfrom\n");   
        n = recvfrom(sockfd,buf,10000,0,(struct sockaddr *)&cliaddr,&clilen);
        printf(" rec'd %d bytes\n",n);

        struct iphdr *ip_hdr = (struct iphdr *)buf;

        printf("IP header is %d bytes.\n", ip_hdr->ihl*4);
        // Print the packet in hex
        for (i = ip_hdr->ihl*4; i < n; i++) {
            printf("%02X%s", (uint8_t)buf[i], (i + 1)%16 ? " " : "\n");
        }
        printf("\n");
        // notes: The IPv4 host sender is +12 bytes from the beggining of the IP packet.
        struct icmphdr *icmp_hdr = (struct icmphdr *)((char *)ip_hdr + (4 * ip_hdr->ihl));

        printf("ICMP msgtype=%d, code=%d\n", icmp_hdr->type, icmp_hdr->code);
        if(icmp_hdr->type == 8 && icmp_hdr->code == 0) {
            // We need to start the prediction round thing here.
            unsigned char IPSrc[4];
            for(i = 12; i < 12 + 4; i++) {
                IPSrc[i-12] = buf[i];
            }
            for (i = 0; i < 4; i++) {
                printf("%02X%s", (uint8_t)IPSrc[i], (i + 1)%16 ? " " : "\n");
            }

            /*
                Okay so, the idea goes is that you have a stack (that has a ~400 entry limit)
                with the source IP and the time.

                When a ping arrives it will scan thought the stack and check if it has 2 
                entries of that source IP, if it does it will calcualte the avg diff between requests
                and will schedule a reply -5ms before it expects the next one to arrive.
            */
        }
    }
}