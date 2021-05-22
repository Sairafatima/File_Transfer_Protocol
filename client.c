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
#define BUF_SIZE 512	//Max buffer size of the data in a frame

/*A frame packet with unique id, length and data and acks*/
struct pkt {
	long int ID;
	long int length;
	char data[BUF_SIZE];
    long int ack;
};


int main(){
	struct sockaddr_in servddr, cliaddr;      //server
	struct stat st;                       
	struct pkt pkt_test; //pkt for testing
	struct timeval t_out = {0, 0};
    struct pkt pkta[11];  //array of 10 pkts of type "pkt"
	char   cmd_send[50]; // buffer to send file name    
    ssize_t length;//length of cl_addr
    long int total_Packets = 0; //variable to store number of packets to be sent
    long int bytes_rec = 0; //variable to store total Bytes received when file is received completely
	int    sockfd = 0; //server descriptors
    int    pktrecv=0; //variable to store number of paktets received from server
	FILE   *fptr; //file pointer
    int    count=1;//testing variable
    bool   check[11]={false};
    int    drop_flag=0;
  int rcv_num=1;
  int ack_num=1;
    int    resnd=0;
  int resend_frame=0;
int 	t_out_flag = 0;
/*****************************************************************************************/

	/*Clear all the data buffer and structure*/
	memset(&servddr, 0, sizeof(servddr));
	memset(& cliaddr, 0, sizeof( cliaddr));

            /*************************************************/
	/*Populate servddr structure with IP address and Port*/
	servddr.sin_family = AF_INET;
	servddr.sin_port =htons(PORT);
	servddr.sin_addr.s_addr = INADDR_ANY;
            
             /*************************************************/

   
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		perror("Socket creation failed");
    else{
        printf("Socket Created:\n");
        }

/***************************************************************************************/
        memset(cmd_send, 0, sizeof(cmd_send)); //cmd_send is buffer used to send file name
	
		printf("Enter your file name:\n");
		scanf(" %[^\n]%*c", cmd_send);//taking file name as input

		
        //sending file name
	    if (sendto(sockfd, cmd_send, sizeof(cmd_send), 0, (struct sockaddr *) &servddr, sizeof(servddr)) == -1) {
			perror("File name sending failed");
        }
               /*************************************************/

			
	    t_out.tv_sec = 2;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval)); 	//Enable the timeout option if server does not respond
                /*************************************************/
		 
	    //Receive number of packets to be sent
        recvfrom(sockfd, &(total_Packets), sizeof(total_Packets), 0, (struct sockaddr *) & cliaddr, (socklen_t *) &length); //Get the total number of frame to recieve
	    t_out.tv_sec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval)); 	//Disable the timeout option
			
	     if (total_Packets > 0) {
            //ack of paktes number which are received earlier
		    sendto(sockfd, &(total_Packets), sizeof(total_Packets), 0, (struct sockaddr *) &servddr, sizeof(servddr));
		    printf("Total Packtets to be sent----> %ld\n", total_Packets);
				
			fptr = fopen("result.wav", "wb");	//open the file in write mode

			/*Recieve all the frames with window size 10 and send the acknowledgement according to pkt ID*/
     
             pkt_test.length=512;
             /*when while loop starts
             /pktrecv is zero,total Packets are number of packets to  be sent
              */
           
			 while(total_Packets!=pktrecv)
	    	 {
                    ack_num=0;
					memset(&pkt_test, 0, sizeof(pkt_test));
                    //window size is 10 so we will runt this loop ,reveive 10 packets and then wait
                    printf("RECEIVING FROM SERVER:\n");
                    for(int k=1;k<=10;k++)
                    {   
                      
					    if((recvfrom(sockfd, &(pkta[k]), sizeof(pkta[k]), 0, (struct sockaddr *) & cliaddr, (socklen_t *) &length))>0);  //Recieve the frame
                        {
                          printf("Packet received ---> %ld	Packet.length ---> %ld\n", pkta[k].ID,pkta[k].length);
                          pktrecv+=1;  //increment it with every pkt received
                          if( check[pkta[k].ID]==true){
                          check[pkta[k].ID]=false;}
                        }
                        
                  
                        if( pkta[k].length!=512){ //if last packets is received befor k=10
                          break;
                         }
                        }
                     
                     
                     
                    
                    


                     //10 packets are received
              /*************************************************/
                     //now send ID of received pkts as ack
                     printf("SENDING ACKS:\n");
                     for(int k=1;k<=10;k++){

					    if((sendto(sockfd, &(pkta[k].ID), sizeof(pkta[k].ID), 0, (struct sockaddr *) &servddr, sizeof(servddr)))>0){	//Send the ack
                             printf("ack sent--->%ld\n", pkta[k].ID);
                              if(check[pkta[k].ID]!=true){
                              check[pkta[k].ID]=true;}
                             ack_num+=1;
                         }
                         //if file is completely received before k=10 we have to break the loop
                        if( pkta[k].length!=512){ //if last packets is received befor k=10
                          break;
                        }
                  
                      }
                                         for (int i=1;i<11;i++){
                   
                       if(check[i]==false){
                        printf("flase flag\n");
                      
                        resnd=i;
                          ack_num=resnd;
                        }
                      }



                      if(resnd==0){
                      printf("No drop occured%ld\n",drop_flag);
                       
                       }
                      if(resnd!=0){
                      printf("PKT LOSS%ld\n",resnd);
                       exit(1);
                      }
     
                      for (int i=1;i<11;i++){
                          check[i]==false;}
                      //acks are sent accordinf to pkt ID
               /*************************************************/


                    recvfrom(sockfd, &( rcv_num), sizeof(rcv_num), 0, (struct sockaddr *) & cliaddr, (socklen_t *) &length);
                    sendto(sockfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *) &servddr, sizeof(servddr));
                    
                   while (ack_num != rcv_num)  //Check for ack
					{
						//keep retrying until the ack matches
						
						recvfrom(sockfd, &(pkta[ack_num].data), sizeof(pkta[ack_num].data), 0, (struct sockaddr *) &cliaddr, (socklen_t *) &length);
                        sendto(sockfd, &(rcv_num), sizeof(rcv_num), 0, (struct sockaddr *) &servddr, sizeof(servddr));
						printf("ack ---> %ld	dropped, %d times\n");
						
						resend_frame++;

						printf("frame ---> %ld	dropped, %d times\n");

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
                      printf("WRITTING TO FILE:\n");
					  for(int k=1; k<=10;k++){
						fwrite(pkta[k].data, 1, pkta[k].length, fptr);   /*Write the recieved data to the file*/
						printf("Packet.ID ---> %ld	Packet length written to file---> %ld\n", pkta[k].ID,pkta[k].length);
                        bytes_rec += pkta[k].length;
                        pkt_test.length=pkta[k].length;
                        //if file received completely for k<10 we have to break the loop(when last packtes are received)
                       if( pkta[k].length!=512){ //if last packets is received befor k=10
                          break;
                        } }
				   	  
                      
                      
                       //10 packets are written in file
                  /*************************************************/

                       //if all packets received
					   if (pktrecv == total_Packets) {
					      printf("File recieved\n");
					   }
				  } //end of while loop 


			printf("Total Packets recieved ---> %ld\n",pktrecv );
            printf("Total Bytes recieved ---> %ld\n",  bytes_rec);
			fclose(fptr);
			}
			else { //if server has sent number of packets to be received=0
				printf("File is empty\n");
			}
            close(sockfd);
            exit(EXIT_SUCCESS);
return 0;
}
