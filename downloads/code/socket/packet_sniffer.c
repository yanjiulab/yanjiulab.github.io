#include<netinet/in.h>
#include<errno.h>
#include<netdb.h>
#include<stdio.h>	//For standard things
#include<stdlib.h>	//malloc
#include<string.h>	//strlen

#include<netinet/ip_icmp.h>	//Provides declarations for icmp header
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/tcp.h>	//Provides declarations for tcp header
#include<netinet/ip.h>	//Provides declarations for ip header
#include<netinet/if_ether.h>	//For ETH_P_ALL
#include<net/ethernet.h>	//For ether_header
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<sys/time.h>
#include<sys/types.h>
#include<unistd.h>

void ProcessPacket(unsigned char* , int);
void print_ip_header(unsigned char* , int);
void print_tcp_packet(unsigned char * , int );
void print_udp_packet(unsigned char * , int );
void print_icmp_packet(unsigned char* , int );
void PrintData (unsigned char* , int);

FILE *logfile;
struct sockaddr_in source,dest;
int tcp=0,udp=0,icmp=0,others=0,igmp=0,total=0,i,j;	

int main()
{
	int saddr_size , data_size;
	struct sockaddr saddr;
		
	unsigned char *buffer = (unsigned char *) malloc(65536); //Its Big!
	
	logfile=fopen("log.txt","w");
	if(logfile==NULL) 
	{
		printf("Unable to create log.txt file.");
	}
	printf("Starting...\n");
	
	int sock_raw = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_ALL)) ;
	//setsockopt(sock_raw , SOL_SOCKET , SO_BINDTODEVICE , "eth0" , strlen("eth0")+ 1 );
	
	if(sock_raw < 0)
	{
		//Print the error with proper message
		perror("Socket Error");
		return 1;
	}
	while(1)
	{
		saddr_size = sizeof saddr;
		//Receive a packet
		data_size = recvfrom(sock_raw , buffer , 65536 , 0 , &saddr , (socklen_t*)&saddr_size);
		if(data_size <0 )
		{
			printf("Recvfrom error , failed to get packets\n");
			return 1;
		}
		//Now process the packet
		ProcessPacket(buffer , data_size);
	}
	close(sock_raw);
	printf("Finished");
	return 0;
}

void ProcessPacket(unsigned char* buffer, int size)
{
	//Get the IP Header part of this packet , excluding the ethernet header
	struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
	++total;
	switch (iph->protocol) //Check the Protocol and do accordingly...
	{
		case 1:  //ICMP Protocol
			++icmp;
			print_icmp_packet( buffer , size);
			break;
		
		case 2:  //IGMP Protocol
			++igmp;
			break;
		
		case 6:  //TCP Protocol
			++tcp;
			print_tcp_packet(buffer , size);
			break;
		
		case 17: //UDP Protocol
			++udp;
			print_udp_packet(buffer , size);
			break;
		
		default: //Some Other Protocol like ARP etc.
			++others;
			break;
	}
	printf("TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\r", tcp , udp , icmp , igmp , others , total);
}

void print_ethernet_header(unsigned char* Buffer, int Size)
{
	struct ethhdr *eth = (struct ethhdr *)Buffer;
	
	fprintf(logfile , "\n");
	fprintf(logfile , "Ethernet Header\n");
	fprintf(logfile , "   |-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_dest[0] , eth->h_dest[1] , eth->h_dest[2] , eth->h_dest[3] , eth->h_dest[4] , eth->h_dest[5] );
	fprintf(logfile , "   |-Source Address      : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_source[0] , eth->h_source[1] , eth->h_source[2] , eth->h_source[3] , eth->h_source[4] , eth->h_source[5] );
	fprintf(logfile , "   |-Protocol            : %u \n",(unsigned short)eth->h_proto);
}

void print_ip_header(unsigned char* Buffer, int Size)
{
	print_ethernet_header(Buffer , Size);
  
	unsigned short iphdrlen;
		
	struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr) );
	iphdrlen =iph->ihl*4;
	
	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = iph->saddr;
	
	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = iph->daddr;
	
	fprintf(logfile , "\n");
	fprintf(logfile , "IP Header\n");
	fprintf(logfile , "   |-IP Version        : %d\n",(unsigned int)iph->version);
	fprintf(logfile , "   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
	fprintf(logfile , "   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
	fprintf(logfile , "   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
	fprintf(logfile , "   |-Identification    : %d\n",ntohs(iph->id));
	//fprintf(logfile , "   |-Reserved ZERO Field   : %d\n",(unsigned int)iphdr->ip_reserved_zero);
	//fprintf(logfile , "   |-Dont Fragment Field   : %d\n",(unsigned int)iphdr->ip_dont_fragment);
	//fprintf(logfile , "   |-More Fragment Field   : %d\n",(unsigned int)iphdr->ip_more_fragment);
	fprintf(logfile , "   |-TTL      : %d\n",(unsigned int)iph->ttl);
	fprintf(logfile , "   |-Protocol : %d\n",(unsigned int)iph->protocol);
	fprintf(logfile , "   |-Checksum : %d\n",ntohs(iph->check));
	fprintf(logfile , "   |-Source IP        : %s\n",inet_ntoa(source.sin_addr));
	fprintf(logfile , "   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
}

void print_tcp_packet(unsigned char* Buffer, int Size)
{
	unsigned short iphdrlen;
	
	struct iphdr *iph = (struct iphdr *)( Buffer  + sizeof(struct ethhdr) );
	iphdrlen = iph->ihl*4;
	
	struct tcphdr *tcph = (struct tcphdr*)(Buffer + iphdrlen + sizeof(struct ethhdr));
			
	int header_size =  sizeof(struct ethhdr) + iphdrlen + tcph->doff*4;
	
	fprintf(logfile , "\n\n***********************TCP Packet*************************\n");	
		
	print_ip_header(Buffer,Size);
		
	fprintf(logfile , "\n");
	fprintf(logfile , "TCP Header\n");
	fprintf(logfile , "   |-Source Port      : %u\n",ntohs(tcph->source));
	fprintf(logfile , "   |-Destination Port : %u\n",ntohs(tcph->dest));
	fprintf(logfile , "   |-Sequence Number    : %u\n",ntohl(tcph->seq));
	fprintf(logfile , "   |-Acknowledge Number : %u\n",ntohl(tcph->ack_seq));
	fprintf(logfile , "   |-Header Length      : %d DWORDS or %d BYTES\n" ,(unsigned int)tcph->doff,(unsigned int)tcph->doff*4);
	//fprintf(logfile , "   |-CWR Flag : %d\n",(unsigned int)tcph->cwr);
	//fprintf(logfile , "   |-ECN Flag : %d\n",(unsigned int)tcph->ece);
	fprintf(logfile , "   |-Urgent Flag          : %d\n",(unsigned int)tcph->urg);
	fprintf(logfile , "   |-Acknowledgement Flag : %d\n",(unsigned int)tcph->ack);
	fprintf(logfile , "   |-Push Flag            : %d\n",(unsigned int)tcph->psh);
	fprintf(logfile , "   |-Reset Flag           : %d\n",(unsigned int)tcph->rst);
	fprintf(logfile , "   |-Synchronise Flag     : %d\n",(unsigned int)tcph->syn);
	fprintf(logfile , "   |-Finish Flag          : %d\n",(unsigned int)tcph->fin);
	fprintf(logfile , "   |-Window         : %d\n",ntohs(tcph->window));
	fprintf(logfile , "   |-Checksum       : %d\n",ntohs(tcph->check));
	fprintf(logfile , "   |-Urgent Pointer : %d\n",tcph->urg_ptr);
	fprintf(logfile , "\n");
	fprintf(logfile , "                        DATA Dump                         ");
	fprintf(logfile , "\n");
		
	fprintf(logfile , "IP Header\n");
	PrintData(Buffer,iphdrlen);
		
	fprintf(logfile , "TCP Header\n");
	PrintData(Buffer+iphdrlen,tcph->doff*4);
		
	fprintf(logfile , "Data Payload\n");	
	PrintData(Buffer + header_size , Size - header_size );
						
	fprintf(logfile , "\n###########################################################");
}

void print_udp_packet(unsigned char *Buffer , int Size)
{
	
	unsigned short iphdrlen;
	
	struct iphdr *iph = (struct iphdr *)(Buffer +  sizeof(struct ethhdr));
	iphdrlen = iph->ihl*4;
	
	struct udphdr *udph = (struct udphdr*)(Buffer + iphdrlen  + sizeof(struct ethhdr));
	
	int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof(udph);
	
	fprintf(logfile , "\n\n***********************UDP Packet*************************\n");
	
	print_ip_header(Buffer,Size);			
	
	fprintf(logfile , "\nUDP Header\n");
	fprintf(logfile , "   |-Source Port      : %d\n" , ntohs(udph->source));
	fprintf(logfile , "   |-Destination Port : %d\n" , ntohs(udph->dest));
	fprintf(logfile , "   |-UDP Length       : %d\n" , ntohs(udph->len));
	fprintf(logfile , "   |-UDP Checksum     : %d\n" , ntohs(udph->check));
	
	fprintf(logfile , "\n");
	fprintf(logfile , "IP Header\n");
	PrintData(Buffer , iphdrlen);
		
	fprintf(logfile , "UDP Header\n");
	PrintData(Buffer+iphdrlen , sizeof udph);
		
	fprintf(logfile , "Data Payload\n");	
	
	//Move the pointer ahead and reduce the size of string
	PrintData(Buffer + header_size , Size - header_size);
	
	fprintf(logfile , "\n###########################################################");
}

void print_icmp_packet(unsigned char* Buffer , int Size)
{
	unsigned short iphdrlen;
	
	struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr));
	iphdrlen = iph->ihl * 4;
	
	struct icmphdr *icmph = (struct icmphdr *)(Buffer + iphdrlen  + sizeof(struct ethhdr));
	
	int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof icmph;
	
	fprintf(logfile , "\n\n***********************ICMP Packet*************************\n");	
	
	print_ip_header(Buffer , Size);
			
	fprintf(logfile , "\n");
		
	fprintf(logfile , "ICMP Header\n");
	fprintf(logfile , "   |-Type : %d",(unsigned int)(icmph->type));
			
	if((unsigned int)(icmph->type) == 11)
	{
		fprintf(logfile , "  (TTL Expired)\n");
	}
	else if((unsigned int)(icmph->type) == ICMP_ECHOREPLY)
	{
		fprintf(logfile , "  (ICMP Echo Reply)\n");
	}
	
	fprintf(logfile , "   |-Code : %d\n",(unsigned int)(icmph->code));
	fprintf(logfile , "   |-Checksum : %d\n",ntohs(icmph->checksum));
	//fprintf(logfile , "   |-ID       : %d\n",ntohs(icmph->id));
	//fprintf(logfile , "   |-Sequence : %d\n",ntohs(icmph->sequence));
	fprintf(logfile , "\n");

	fprintf(logfile , "IP Header\n");
	PrintData(Buffer,iphdrlen);
		
	fprintf(logfile , "UDP Header\n");
	PrintData(Buffer + iphdrlen , sizeof icmph);
		
	fprintf(logfile , "Data Payload\n");	
	
	//Move the pointer ahead and reduce the size of string
	PrintData(Buffer + header_size , (Size - header_size) );
	
	fprintf(logfile , "\n###########################################################");
}

void PrintData (unsigned char* data , int Size)
{
	int i , j;
	for(i=0 ; i < Size ; i++)
	{
		if( i!=0 && i%16==0)   //if one line of hex printing is complete...
		{
			fprintf(logfile , "         ");
			for(j=i-16 ; j<i ; j++)
			{
				if(data[j]>=32 && data[j]<=128)
					fprintf(logfile , "%c",(unsigned char)data[j]); //if its a number or alphabet
				
				else fprintf(logfile , "."); //otherwise print a dot
			}
			fprintf(logfile , "\n");
		} 
		
		if(i%16==0) fprintf(logfile , "   ");
			fprintf(logfile , " %02X",(unsigned int)data[i]);
				
		if( i==Size-1)  //print the last spaces
		{
			for(j=0;j<15-i%16;j++) 
			{
			  fprintf(logfile , "   "); //extra spaces
			}
			
			fprintf(logfile , "         ");
			
			for(j=i-i%16 ; j<=i ; j++)
			{
				if(data[j]>=32 && data[j]<=128) 
				{
				  fprintf(logfile , "%c",(unsigned char)data[j]);
				}
				else 
				{
				  fprintf(logfile , ".");
				}
			}
			
			fprintf(logfile ,  "\n" );
		}
	}
}

// Note :

// 1. If you want to sniff only IP and ARP packets for example then you can try this :
// sock_raw = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_IP|ETH_P_ARP)) ;

// The complete list of protocols is found in /usr/include/linux/if_ether.h
// /*
//  *	These are the defined Ethernet Protocol ID's.
//  */

// #define ETH_P_LOOP	0x0060		/* Ethernet Loopback packet	*/
// #define ETH_P_PUP	0x0200		/* Xerox PUP packet		*/
// #define ETH_P_PUPAT	0x0201		/* Xerox PUP Addr Trans packet	*/
// #define ETH_P_IP	0x0800		/* Internet Protocol packet	*/
// #define ETH_P_X25	0x0805		/* CCITT X.25			*/
// #define ETH_P_ARP	0x0806		/* Address Resolution packet	*/
// #define	ETH_P_BPQ	0x08FF		/* G8BPQ AX.25 Ethernet Packet	[ NOT AN OFFICIALLY REGISTERED ID ] */
// #define ETH_P_IEEEPUP	0x0a00		/* Xerox IEEE802.3 PUP packet */
// #define ETH_P_IEEEPUPAT	0x0a01		/* Xerox IEEE802.3 PUP Addr Trans packet */
// #define ETH_P_DEC       0x6000          /* DEC Assigned proto           */
// #define ETH_P_DNA_DL    0x6001          /* DEC DNA Dump/Load            */
// #define ETH_P_DNA_RC    0x6002          /* DEC DNA Remote Console       */
// #define ETH_P_DNA_RT    0x6003          /* DEC DNA Routing              */
// #define ETH_P_LAT       0x6004          /* DEC LAT                      */
// #define ETH_P_DIAG      0x6005          /* DEC Diagnostics              */
// #define ETH_P_CUST      0x6006          /* DEC Customer use             */
// #define ETH_P_SCA       0x6007          /* DEC Systems Comms Arch       */
// #define ETH_P_TEB	0x6558		/* Trans Ether Bridging		*/
// #define ETH_P_RARP      0x8035		/* Reverse Addr Res packet	*/
// #define ETH_P_ATALK	0x809B		/* Appletalk DDP		*/
// #define ETH_P_AARP	0x80F3		/* Appletalk AARP		*/
// #define ETH_P_8021Q	0x8100          /* 802.1Q VLAN Extended Header  */
// #define ETH_P_IPX	0x8137		/* IPX over DIX			*/
// #define ETH_P_IPV6	0x86DD		/* IPv6 over bluebook		*/
// #define ETH_P_PAUSE	0x8808		/* IEEE Pause frames. See 802.3 31B */
// #define ETH_P_SLOW	0x8809		/* Slow Protocol. See 802.3ad 43B */
// #define ETH_P_WCCP	0x883E		/* Web-cache coordination protocol
// 					 * defined in draft-wilson-wrec-wccp-v2-00.txt */
// #define ETH_P_PPP_DISC	0x8863		/* PPPoE discovery messages     */
// #define ETH_P_PPP_SES	0x8864		/* PPPoE session messages	*/
// #define ETH_P_MPLS_UC	0x8847		/* MPLS Unicast traffic		*/
// #define ETH_P_MPLS_MC	0x8848		/* MPLS Multicast traffic	*/
// #define ETH_P_ATMMPOA	0x884c		/* MultiProtocol Over ATM	*/
// #define ETH_P_LINK_CTL	0x886c		/* HPNA, wlan link local tunnel */
// #define ETH_P_ATMFATE	0x8884		/* Frame-based ATM Transport
// 					 * over Ethernet
// 					 */
// #define ETH_P_PAE	0x888E		/* Port Access Entity (IEEE 802.1X) */
// #define ETH_P_AOE	0x88A2		/* ATA over Ethernet		*/
// #define ETH_P_TIPC	0x88CA		/* TIPC 			*/
// #define ETH_P_1588	0x88F7		/* IEEE 1588 Timesync */
// #define ETH_P_FCOE	0x8906		/* Fibre Channel over Ethernet  */
// #define ETH_P_FIP	0x8914		/* FCoE Initialization Protocol */
// #define ETH_P_EDSA	0xDADA		/* Ethertype DSA [ NOT AN OFFICIALLY REGISTERED ID ] */

// /*
//  *	Non DIX types. Won't clash for 1500 types.
//  */

// #define ETH_P_802_3	0x0001		/* Dummy type for 802.3 frames  */
// #define ETH_P_AX25	0x0002		/* Dummy protocol id for AX.25  */
// #define ETH_P_ALL	0x0003		/* Every packet (be careful!!!) */
// #define ETH_P_802_2	0x0004		/* 802.2 frames 		*/
// #define ETH_P_SNAP	0x0005		/* Internal only		*/
// #define ETH_P_DDCMP     0x0006          /* DEC DDCMP: Internal only     */
// #define ETH_P_WAN_PPP   0x0007          /* Dummy type for WAN PPP frames*/
// #define ETH_P_PPP_MP    0x0008          /* Dummy type for PPP MP frames */
// #define ETH_P_LOCALTALK 0x0009		/* Localtalk pseudo type 	*/
// #define ETH_P_CAN	0x000C		/* Controller Area Network      */
// #define ETH_P_PPPTALK	0x0010		/* Dummy type for Atalk over PPP*/
// #define ETH_P_TR_802_2	0x0011		/* 802.2 frames 		*/
// #define ETH_P_MOBITEX	0x0015		/* Mobitex (kaz@cafe.net)	*/
// #define ETH_P_CONTROL	0x0016		/* Card specific control frames */
// #define ETH_P_IRDA	0x0017		/* Linux-IrDA			*/
// #define ETH_P_ECONET	0x0018		/* Acorn Econet			*/
// #define ETH_P_HDLC	0x0019		/* HDLC frames			*/
// #define ETH_P_ARCNET	0x001A		/* 1A for ArcNet :-)            */
// #define ETH_P_DSA	0x001B		/* Distributed Switch Arch.	*/
// #define ETH_P_TRAILER	0x001C		/* Trailer switch tagging	*/
// #define ETH_P_PHONET	0x00F5		/* Nokia Phonet frames          */
// #define ETH_P_IEEE802154 0x00F6		/* IEEE802.15.4 frame		*/
// #define ETH_P_CAIF	0x00F7		/* ST-Ericsson CAIF protocol	*/