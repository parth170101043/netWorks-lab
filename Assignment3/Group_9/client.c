#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include<stdbool.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#define SndMsgSize 1001//1 bit for sequence number...1000 bit for message
float ber=-1;
char key[]={'1','0','0','0','0','0','1','1','1','\0'};
int keysize;
bool iscorrupt(char str[]);

char inputArray[SndMsgSize-1];
int decToBinary(int n) 
{ 
    // Size of an integer is assumed to be 32 bits 
    for (int i = 6; i >= 0; i--) { 
        int k = n >> i; 
        if (k & 1) 
            strcat(inputArray,"1");
        else
            strcat(inputArray,"0"); 
    } 
} 

bool check_bit(char* temp)
{keysize=strlen(key);
   // return temp[0]=='0'?0:1;
   if(temp[0]=='0')
   {
       //printf("shifted \n");
       for(int i=1;i<keysize;i++)
       {temp[i-1]=temp[i];}
       temp[keysize-1]='0';
       return 0;
   }
   return 1;
}

void XOR(char* temp)
{    
    keysize=strlen(key);
    char temp1[keysize];
    int i;
    for(i=0;i<keysize;++i)
    {
        temp1[i]='0';
    }

    for(i=1;i<keysize;++i)
    {   
        if((temp[i]=='0' && key[i]=='0')||( temp[i]=='1' && key[i]=='1'))
            temp1[i-1]='0';
        else
            temp1[i-1]='1';
    }
    
    for(i=0;i<keysize;i++)
    temp[i]=temp1[i];
}

void corruptmsg(char* str,int size)
{   double a=ber*size;
    int errors=a;
    int x;
    for(int i=0;i<errors;i++)
    {
        x=rand()%size-1;
        if(str[x]=='0')
        { 
            str[x]='1';
        }
        else
        {
            str[x]='0';
        }
        
    }
}

bool iscorrupt(char str[])
{
    char temp[keysize];
   

    int i,j;

    int n=strlen(str);
     n=n-keysize+1;

   for(i=0;i<keysize;++i)
   {
       temp[i]=str[i];
   }

   for(i=0;i<n;++i)
   {   
        if(check_bit(temp))
           {
               XOR(temp);
           }
        
           
        temp[keysize-1]=str[keysize+i];
    }

    for(i=0;i<strlen(temp);i++)
    {
        if(temp[i]!='0')
        return true;
    }

    return false;
}



void crc(char* clientMsg)
{
     keysize=strlen(key);
    char temp[keysize];
   

    int i,j;

    int n=strlen(clientMsg);
     n=n-keysize+1;

   for(i=0;i<keysize;++i)
   {
       temp[i]=clientMsg[i];
   }

   for(i=0;i<n;++i)
   {   
        if(check_bit(temp))
           {
               XOR(temp);
           }
        
           
        temp[keysize-1]=clientMsg[keysize+i];
    }
 
    for(i=0;i<keysize-1;++i)
    {
        clientMsg[n+i]=temp[i];
    }
    
}


bool isACK(char str[],bool seq_num)
{
    //this will return true only if ack is received for correct sequence num...else return false
    //str first bit=seq_num,2nd bit=ack/nak..
    if((str[0]=='0' && seq_num==0)||(str[0]=='1' && seq_num==1))
    {
        if(str[1]=='1')
        return 1;
    }
    return 0;
}


int main(int argc, char const *argv[])
{   char us[90];
    if(argc<3)//arg[1]=server ip....arg[2]=server port
    {
        printf("port or server_ip missing \n");
        return 0;
    }
    char clientMsg[SndMsgSize];
    char str[125];
    int i,j;

    int sock,sevPort,finalsize,sevRead;
    bool seq_number;
    bool rcv_successful;
    char buffer[1024];
    seq_number=0;//initial swquence number 0
    keysize=strlen(key);
    struct sockaddr_in sevAddr;
    printf("Please input Bit Error Rate between 0 and 1 \n");
    scanf("%f ",&ber);
    while(ber<0 || ber>1)
    {
      
        scanf(" \n %f ",&ber);
    }
    sevPort=atoi(argv[2]);
    //printf("port %d",sevPort);///
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        
        printf("\n Socket creation failed \n"); 
        return 0; 
    } 
    
    sevAddr.sin_family=AF_INET;
    sevAddr.sin_port=htons(sevPort);
    if(inet_pton(AF_INET, argv[1], &sevAddr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return 0; 
    } 
    if (connect(sock, (struct sockaddr *)&sevAddr, sizeof(sevAddr)) < 0) 
    { printf("\n");
        printf("\nConnection Failed \n"); 
        return 0; 
    } 
    //after connection established successfully...start sending message to server..
   // printf("Starting conversation .....\n");
  
    while (1)
    {  
   
         rcv_successful=0;
    
     bzero(clientMsg,SndMsgSize);
     bzero(inputArray,SndMsgSize-1);
     if(seq_number==0)
     {
         clientMsg[0]='0';
     }
     else
     {
        clientMsg[0]='1';
     }
     
    gets(str);
       // gets(inputArray);
       for(i=0; str[i]!='\0'; ++i)
    {
        j=str[i];
        decToBinary(j);
    }
  //  printf("%s",str);
    strcat(clientMsg,inputArray);

   
    for(int i=0;i<keysize-1;i++)
    strcat(clientMsg,"0");
    crc(clientMsg);
    printf("CRC %s\n",clientMsg);
    finalsize=strlen(clientMsg);
    corruptmsg(clientMsg,finalsize);
    send(sock,clientMsg,finalsize,0); 
    printf("Message sent:: %s\n",clientMsg);
    clock_t strt_Timer = clock();
    double time_Taken;
    while (!rcv_successful)
    {   buffer[0]='\0';
        bzero(buffer,1024);
        if((sevRead=read( sock , buffer, 1024))==0)
        {
           printf("Disconnected from server");
           exit(1);
        }
         
        time_Taken=(double)(clock()-strt_Timer)/CLOCKS_PER_SEC;
        
        if(sevRead<0)///assumed timeout=1second
        { //  printf("\n error here");
            corruptmsg(clientMsg,finalsize);
            send(sock,clientMsg,finalsize,0); 
            strt_Timer = clock();
            printf("Message sent:: %s\n",clientMsg);

            continue;
        }
        //////////////no need
        if(time_Taken>=3)///assumed timeout=1second
        {   //printf("\n error here111");
            printf("Timeout\nretransmitting\n");
            corruptmsg(clientMsg,finalsize);
            send(sock,clientMsg,finalsize,0); 
             printf("Message sent:: %s\n",clientMsg);
            strt_Timer = clock();
            continue;
        }
        if(isACK(buffer,seq_number)&&!iscorrupt(buffer))
        {
            printf("Ack received successfull \n");
            rcv_successful=1;
        }
        else
        { if(!isACK(buffer,seq_number))
            {
                printf("NAK received from server\n retransmitting...\n");
            }
            if(iscorrupt(buffer))
            {
                printf("Corrupted Message from server\nretransmitting....\n");
            }
           corruptmsg(clientMsg,finalsize);
            send(sock,clientMsg,finalsize,0); 
             printf("Message sent:: %s\n",clientMsg);
             
            strt_Timer = clock();
            continue;
        }
    }
    
    
    if(seq_number==0)
    {
        seq_number=1;
    }
    else
    {
        seq_number=0;
    }
    

    }
  
    
}