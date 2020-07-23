#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include<stdbool.h>
#include <arpa/inet.h> 
#include <time.h>
#include <errno.h>  
#include <sys/types.h>  
#include <math.h>
#include <signal.h> 
#define SIGINT 2//ctrl+c
float ber=0;
pid_t pid=-1;
char key[]={'1','0','0','0','0','0','1','1','1','\0'};
int keysize;
int socketarray[1000],socketvar;
struct sockaddr_in sev_address,cli_address;
bool iscorrupt(char* str);
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

int getseqnum(char* str)
{
    if(str[0]=='0')
        return 0;
    else
    {
        return 1;
    }
    
}

bool iscorrupt(char* str)
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
        {
          //  printf("got error\n");
            return true;
        }
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



bool createAckNak(char *serverMsg,char *buffer)
{
    if(iscorrupt(buffer))
    {
        serverMsg[1]='0';///nak
        return 0;
    }
    else
    {
       serverMsg[1]='1';//ack
       return 1;
    }
    
}

void printwelcome(struct sockaddr_in cli_address)
{
    printf("New connection  ip is : %s , port : %d  \n" ,  inet_ntoa(cli_address.sin_addr) , ntohs(cli_address.sin_port)); 

}

int binaryToDecimal(int n) 
{ 
    int num = n; 
    int dec_value = 0; 
  
    // Initializing base value to 1, i.e 2^0 
    int base = 1; 
  
    int temp = num; 
    while (temp) { 
        int last_digit = temp % 10; 
        temp = temp / 10; 
  
        dec_value += last_digit * base; 
  
        base = base * 2; 
    } 
  
    return dec_value; 
}

void generateoriginalMsg(char buffer[],char *original)
{

    int i,j,k,x;
    keysize=strlen(key);
    x=strlen(buffer)-(keysize-1);
    int fun;
    char temp[]="";
    int decimal[125];
    char decimal_c[125];
    int count=-1;
    for(i=1; i<x; i++)
    {  
        if((i)%7==0&&i!=0)
        {
            strncat(temp, &buffer[i], 1); 
            //printf(" else : a ");
            //printf("temp : %s",temp);
            fun=atoi(temp);
           
            decimal[++count]=binaryToDecimal(fun);
            temp[0]='\0';
        }
        else
        {
            strncat(temp, &buffer[i], 1); 
            //printf(" else : a ");

        }
        
    }

    printf("\n");
    for(i=0;i<=count;++i)
    {
        decimal_c[i]=decimal[i];
    }
    for(i=0;i<=count;++i)
    {
        original[i]=decimal_c[i];
    }
   
}

void manuel_handling(int sig)
{
    int var;
    int varsize=sizeof(var);
       // signal(SIGINT, manuel_handling);
       // printf("Gracefully releasing all sockets connected to clients \n");
    if(pid!=0)
       { //printf("Gracefully releasing all sockets connected to clients \n");
       if(socketvar>=0)
        printf("\n Gracefully releasing server socket \n");
        if(getsockopt(socketarray[0], SOL_SOCKET, SO_ERROR, &var, &varsize)==0)
            {   
                close(socketarray[0]);
            }
            exit(0);
       }
       else
       {

            for(int i=1;i<=socketvar;i++)
            {
                if(getsockopt(socketarray[i], SOL_SOCKET, SO_ERROR, &var, &varsize)==0)
                {   
                    close(socketarray[i]);
                }

            }
            if(socketvar>0)
         printf("Socket closed for Client ip : %s port : %d \n",inet_ntoa(cli_address.sin_addr) , ntohs(cli_address.sin_port));
           exit(0);
       }
       
}

int main(int argc, char const *argv[])
{   socketvar=-1;
    signal(SIGINT, manuel_handling);
    char* ip="127.0.0.1";
    bool isACK;
    keysize=strlen(key);
    char original[125];
     if(argc<2)//arg[1]=server ip....arg[2]=server port
    {
        printf("port not given \n");
        return 0;
    }
    printf("Please input Bit Error Rate between 0 and 1 \n");
    scanf("%f",&ber);
    while(ber<0 || ber>1)
    {
        printf("Please input a value between 0 and 1 \n");
        //fflush( stdout );
        scanf("%f",&ber);
    }
    int sock,sevPort,finalsize,cliSock,seq_number,cliRead;
    bool rcv_successful,isSockOpen;
    char serverMsg[100];
    //pid_t pid;
    seq_number=0;
    char buffer[1024];
    int var;
    var=1;///..............................................
     
	socklen_t addrlen = sizeof(sev_address); 
	socklen_t  cliaddrlen=sizeof(cli_address);
    sevPort=atoi(argv[1]);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation failed \n"); 
        return 0; 
    } 
    socketarray[++socketvar]=sock;
if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &var, sizeof(var))) 
	{   printf("setsocket fail");
		
		exit(1); 
	} 
    sev_address.sin_family = AF_INET; 
	sev_address.sin_addr.s_addr = inet_addr(ip); 
	sev_address.sin_port = htons( sevPort ); 

	if (bind(sock, (struct sockaddr *)&sev_address, 
								sizeof(sev_address))<0) 
	{  printf(" server socket bind fail");
		
		exit(1); 
	} 
    if (listen(sock, 3) < 0) 
	{ 
         printf("server socket listen fail\n");
		
		exit(1); 
	} 

    while (1)
	{
      
		if ((cliSock = accept(sock, (struct sockaddr *)&cli_address, &cliaddrlen))<0) 
		{ 
		//printf("Accepting new client connection fail\n");
		exit(EXIT_FAILURE); 
		}
        else
        {
             printwelcome(cli_address);
        }
        
           socketarray[++socketvar]=cliSock;

        
    
        isSockOpen=1; 
        serverMsg[0]='0';
        serverMsg[1]='0';
		 if((pid=fork())==0)
		{	close(sock);
          
			while (isSockOpen)
			{
              buffer[0]='\0';
               
               bzero(buffer,1024);
               //printf("buffer size %d xxx",strlen(buffer));
                cliRead=read( cliSock , buffer, 1024);
                serverMsg[0]='\0';
                 bzero(serverMsg,100);
                if(cliRead==0)
                {
                   printf("\nClient ip : %s port : %d disconnected\n",inet_ntoa(cli_address.sin_addr) , ntohs(cli_address.sin_port));
                    isSockOpen=0;
                }
                else
                {
                   if(strlen(buffer)>0)
                   {    seq_number=getseqnum(buffer);
                      //  printf("buffer size %d xxx",strlen(buffer));
                       
                       printf("From ip is : %s , port : %d message:: %s\n",inet_ntoa(cli_address.sin_addr) , ntohs(cli_address.sin_port),buffer ); 
                       if(seq_number==1)
                       {
                           serverMsg[0]='1';
                       }
                       else
                       {
                           serverMsg[0]='0';
                       }
                       
                       isACK=createAckNak(serverMsg,buffer);
                       for(int i=0;i<keysize-1;i++)
                       strcat(serverMsg,"0");
                        crc(serverMsg);
                        finalsize=strlen(serverMsg);
                        corruptmsg(serverMsg,finalsize);
                        
                        send(cliSock,serverMsg,finalsize,0); 
                        if(isACK)
                       {    //original msg
                            original[0]='\0';
                            bzero(original,125);
                            generateoriginalMsg(buffer,original);
                            printf("Message received with no crc error from %s  %d \n original message :: %s\n",inet_ntoa(cli_address.sin_addr) , ntohs(cli_address.sin_port),original);
                            printf("Sending ACK to  %s  %d message:: %s\n",inet_ntoa(cli_address.sin_addr) , ntohs(cli_address.sin_port),serverMsg);

                       }
                        else
                        {
                            printf("Sending NAK to  %s  %d message:: %s\n",inet_ntoa(cli_address.sin_addr) , ntohs(cli_address.sin_port),serverMsg);

                        }
                        
                       
                       
                   }
                }
                
                
			}
			
			
		}
		close(cliSock);
	
	
	}

}