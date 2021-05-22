#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdbool.h>
#define PORT    8002
#define BUF_SIZE (512)		//Max buffer size of the data in a frame

/*A packet with unique id, length and data*/
struct pkt {
	long int ID;
	long int length;
	char data[BUF_SIZE];
    long int ack;
  
};


int main(){

	struct sockaddr_in sv_addr, cl_addr;
	struct stat st;
	struct pkt pkt_test; //test pkt
	struct timeval t_out = {0, 0};
    struct pkt pkta[11];  //array of pkets to be received
	char   msg_recv[BUF_SIZE]; //buffer to received file name     
	ssize_t numRead; //variable to store bytes of file name
	ssize_t length;//length of cl_addr
	off_t f_size; 	//size of file to be sent
	int    ack_num = 1;  //Recieve file size and name packet acknowledgement
    int    total_pkts = 0;//total packtes in file
	int    sockfd; // sockt descriptor
    int    pktsnd=0;//sent pkets
    bool   check[11]={false}; //array for correct arrangement of pkts
    int    flag=1; // 10 pkts recveived flag
    int    drop_flag=0;
    int    resnd=0;
    int    rcv_num=0;//variable to store received acks
    FILE    *fptr;
/*****************************************************************************************/

	/*Clear the server structure - 'sv_addr' and populate it with port and IP address*/
	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(PORT);
	sv_addr.sin_addr.s_addr = INADDR_ANY;

            /*************************************************/


	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		perror("Server socket creation failed\n");
    else{
        printf("Socket created:\n");
    }


             /*************************************************/
     //BINDING

	if (bind(sockfd, (struct sockaddr *) &sv_addr, sizeof(sv_addr)) == -1)
		perror("Server bind failed:\n");
    else{
        printf("Binding done:\n");
    }
             /*************************************************/

    printf("Server: Waiting for client to connect\n");
	memset(msg_recv, 0, sizeof(msg_recv));
	
             /*************************************************/
    length = sizeof(cl_addr);
	if((numRead= recvfrom(sockfd, msg_recv, BUF_SIZE, 0, (struct sockaddr *) &cl_addr, (socklen_t *) &length)) == -1)
			perror("File name failed to receive:\n");
    else{
        printf("File name received:\n");
    }

             /*************************************************/


	//printf("Server: Recieved file name of %ld  from %s\n", numRead, cl_addr.sin_addr.s_addr);
	printf("Server: The recieved message ---> %s\n", msg_recv);
    printf("Server: Get called with file name --> %s\n", msg_recv);

    if (access(msg_recv, F_OK) == 0) {			//Check if file exist
				
	    int resend_frame = 0, drop_frame = 0, t_out_flag = 0;				
		stat(msg_recv, &st);
		f_size = st.st_size;//Size of the file
		t_out.tv_sec = 2;			
		t_out.tv_usec = 0;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval));   //Set timeout option for recvfrom

             /*************************************************/

		fptr = fopen(msg_recv, "rb");       //open the file to be sent			
		if ((f_size % BUF_SIZE) != 0)
		   total_pkts = (f_size / BUF_SIZE) + 1;//Total number of packets to be sent
		else
			total_pkts = (f_size / BUF_SIZE);

             /*************************************************/

		printf("Total number of packets ---> %d\n", total_pkts);
		length = sizeof(cl_addr);
		sendto(sockfd, &(total_pkts), sizeof(total_pkts), 0, (struct sockaddr *) &cl_addr, sizeof(cl_addr));	//Send number of packets (to be transmitted) to reciever
		recvfrom(sockfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *) &cl_addr, (socklen_t *) &length); //ack of sent number of pkts
       
		while (ack_num != total_pkts)	{
					/*keep Retrying until the ack matches*/
					sendto(sockfd, &(total_pkts), sizeof(total_pkts), 0, (struct sockaddr *) &cl_addr, sizeof(cl_addr)); 
					recvfrom(sockfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *) &cl_addr, (socklen_t *) &length);
					resend_frame++;

					/*Enable timeout flag even if it fails after 20 tries*/
					if (resend_frame == 20) {
						t_out_flag = 1;
						break;
					}
		 }
  
              
        /*when while loop starts
        /pktrecv is zero,total Packets are number of packets to  be sent
         */
        /*transmit data packets sequentially followed by an acknowledgement matching*/
		while(total_pkts!=pktsnd)
		{
            rcv_num=0;
            ack_num=0;
            //if 10 acks has come then resend =0
            if(resnd==0){
                /*************************************************/

            //Read 10 pkets from file and store them in pkt array
            printf("READING FILE:\n");
            for(int k=1;k<=10;k++){
				if((pkta[k].length = fread((pkta[k].data), 1, BUF_SIZE, fptr))!=0){
                     printf("file bytes read --- %d\n",k);
                     pkta[k].ID=k;//set ID of pkts
                     pktsnd+=1;//total pkts sent yet
                }
                //for last pkts
                if(total_pkts==pktsnd){ 
                      strncpy(pkta[k+1].data,"EOF",500);//copy end of file in last buffer
                      printf("EOF copied\n");
                      break;}
                if(pkta[k].length==0 && total_pkts!=pktsnd){
                      printf("error reading file\n");
                      exit(1);}                    
                 }
                /*************************************************/


             //now send 10 pkts 
             printf("SENDING TO CLIENT:\n");
             for (int k=1;k<=10;k++){
			    if((sendto(sockfd, &(pkta[k]), sizeof(pkta[k]), 0, (struct sockaddr *) &cl_addr, sizeof(cl_addr)))>0){
                     printf("Packets sent ----> %d  Packet ID%ld\n",k,pkta[k].ID);
                     check[k]=false;//if pkt sent then tick its corrosponding entry in check array
                 }		//send the Packet       
                if((strncmp(pkta[k+1].data, "EOF",500)==0)){
                     break;}
                  
			   }}
                /*************************************************/
             //now receive 10 pkts acks
              printf("RECEIVINF ACKS:\n");
              for (int k=1;k<=10;k++){                 
                   if((recvfrom(sockfd, &(pkta[k].ack), sizeof(pkta[k].ack), 0, (struct sockaddr *) &cl_addr, (socklen_t *) &length))>0){  	//Recieve the acknowledgement
                        
                      printf("Packet ----> %ld	Ack ----> %ld  Packet length ----> %ld \n",pkta[k].ack,pkta[k].ack, pkta[k].length);
                     if( check[pkta[k].ack]!=true){   //if ticked entery is detected 
                      check[pkta[k].ack]=true;
                      ack_num+=1;//sent pkt has acknowlged
                      }      
                   }
                     if((strncmp(pkta[k+1].data, "EOF",500)==0)){//if next buffer is end of file
                        break;}
                     if(pkta[k].length!=512){
                        break;}
               }

                   
                 //check the array and see for pkts which are not acknowlged yet
                   for (int i=1;i<11;i++){
                       if(check[i]==false){
                        printf("flase flag\n");
                        drop_flag=1;
                        ack_num=resnd;
                        resnd=i;
                        }
                    }

                   if(resnd==0){
                      printf("No drop occured\n");   
                   }
                   drop_flag=0;
                   if(resnd!=0){
                      printf("PKT LOSS%d\n",resnd);
                      exit(1);
                   }
                   drop_flag=0;
                   for (int i=1;i<11;i++){
                          check[i]==false;
                   }

                   
                   sendto(sockfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *) &cl_addr, sizeof(cl_addr));	//Send number of packets (to be transmitted) to reciever
				   recvfrom(sockfd, &(rcv_num), sizeof(rcv_num), 0, (struct sockaddr *) &cl_addr, (socklen_t *) &length);
                   printf("ack number---> %d   recv number%d\n",ack_num,rcv_num);
                    
                    //selective repeat
                    while (ack_num != rcv_num)  //Check for ack
					{
						//keep retrying until the ack matches
						sendto(sockfd, &(pkta[rcv_num].data), sizeof(pkta[rcv_num].data), 0, (struct sockaddr *) &cl_addr, sizeof(cl_addr));
						recvfrom(sockfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *) &cl_addr, (socklen_t *) &length);
						printf("ack ---> %ld	dropped, %ld times\n");
						
						resend_frame++;

						printf("frame ---> %ld	dropped, %ld times\n");

						//Enable the timeout flag even if it fails after 200 tries
						if (resend_frame == 200) {
							t_out_flag = 1;
							break;
						}
					}

                    if(rcv_num==ack_num){
                        ack_num=0;
                        printf("sliding window\n");
                     }
					resend_frame = 0;
					drop_frame = 0;
					/*File transfer fails if timeout occurs*/
					if (t_out_flag == 1) {
						printf("File not sent\n");
						break;
					}

				}
                printf("Total Bytes sent ---> %d\n",total_pkts);
                printf("sent file size %ld\n",f_size);
                
				fclose(fptr);
				t_out.tv_sec = 0;
				t_out.tv_usec = 0;
				setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval)); //Disable the timeout option
			}
			else {	
				printf("Invalid Filename\n");
			}		
            close(sockfd);
            exit(EXIT_SUCCESS);
            return 0;
		}

