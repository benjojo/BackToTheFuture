# include <unistd.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <sys/queue.h>
# include <string.h>
# include <netinet/in.h>
# include <stdio.h>
# include <stdlib.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <stdbool.h>
# include <sys/queue.h>


# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>

static int in_cksum(unsigned short *buf, int sz)
{
    int nleft = sz;
    int sum = 0;
    unsigned short *w = buf;
    unsigned short ans = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(unsigned char *) (&ans) = *(unsigned char *) w;
        sum += ans;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    ans = ~sum;
    return (ans);
}

static char *hostname = NULL;

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
                // Copy the IP address from the IP header on the one that was sent, into this buffer
                IPSrc[i-12] = buf[i];
            }
            for (i = 0; i < 4; i++) {
                // for debugging reasons. Print the IP out.
                printf("%02X%s", (uint8_t)IPSrc[i], (i + 1)%16 ? " " : "\n");
            }
            int pingsock, c;
            if ((pingsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
                perror("ping: creating a raw socket to reply with! :(");
            } else {
                struct sockaddr_in pingaddr;
                struct icmp *pkt;
                struct hostent *h;
                char packet[n - 20];
                // Going to read out the old IPv4 packet and change the reply code and then
                // blank out the checksum.
                for(i = ip_hdr->ihl*4; i < n; i++) {
                    packet[i-ip_hdr->ihl*4] = buf[i];
                }
                packet[0] = 0;
                packet[2] = 0;
                packet[3] = 0;

                pingaddr.sin_family = AF_INET;
                char full[50];
                sprintf(full, "%d.%d.%d.%d", (int)IPSrc[0], (int)IPSrc[1], (int)IPSrc[2], (int)IPSrc[3]);
                fprintf(stderr, "ping: Got a ping from %s\n", full);
                if (!(h = gethostbyname(full))) {
                    fprintf(stderr, "ping: unknown host %s\n", full);
                    exit(1);
                }
                memcpy(&pingaddr.sin_addr, h->h_addr, sizeof(pingaddr.sin_addr));
                hostname = h->h_name;

                pkt = (struct icmp *) packet;
                pkt->icmp_cksum = in_cksum((unsigned short *) pkt, sizeof(packet));

                printf("wat %d",sizeof(packet));
                // pkt->icmp_sequence = 
                c = sendto(pingsock, packet, sizeof(packet), 0,
                    (struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in));
                if (c < 0 || c != sizeof(packet)) {
                    if (c < 0)
                        perror("ping: sendto");
                    fprintf(stderr, "ping: write incomplete\n");
                    // exit(1);
                }

                close(pingsock);
            }

            /*
                Okay so here is my new idea, You have a array of structs of src IP's and time stamps.
                you also then have a 2nd var to point who is next to die. Each ping scan though it, calc
                the avg time between and sleep that time -10ms from that time, then send a ping back with
                the seq 1 up
            */
        }
    }
}