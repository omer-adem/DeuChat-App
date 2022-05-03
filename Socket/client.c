#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h>                       // HOCAM ODADAN ÇIKTIĞINIZDA ENTER'A BİR TANE DAHA BASIN , Bazı yerlerde 1 saniyelik sleepler var bir de.
#include <pthread.h>
#include <semaphore.h>
#define PORT 3205

char buffer[60] = {0};
char output[100] = {0};
int isInRoom = 0;  // In Room control

void lobby(int sock);
void inroom(int sock);
void privateroom(int sock);
void enter(int sock);

void lobby(int sock)
{
    int valread;
    char command[20] = {0};
    memset(buffer,0,60);
    read(sock,buffer,sizeof(buffer));   // Get the message from server.
    printf("%s",buffer);
   while(1)
   {
       printf("@Lobby- ");
       fgets(command,sizeof(command),stdin);  // Send a command
       int q;
        for(q=0; q<20; q++)
        {
          if(command[q]=='\n')
            command[q]='\0';
        }
       write(sock,command,sizeof(command));  // Send a command
       read(sock,output,sizeof(output));  // Get output
       printf("%s\n",output);
       if(strncmp(command,"-exit",5)==0)
          break;
       else if(strncmp(output,"Creating a Public Room",23)==0)
          inroom(sock);
       else if(strncmp(output,"Joining to Room",16)==0)
          enter(sock);
       else if(strncmp(output,"Creating a Private Room",24)==0)
          privateroom(sock);
       memset(output,0,100);
       memset(command,0,20);
   }
}

void *handlemsg(void* arg)
{
   int sock = (int)arg;
   char msgbox[500] = {0};
   sleep(1);
   while(isInRoom)
   {
       read(sock,msgbox,sizeof(msgbox));
       if(strncmp(msgbox,"Quit Selected",14)==0)  
          break;
       printf("   %s",msgbox);     // at the same time our client should get others messages.
       memset(msgbox,0,500);
   }
   isInRoom=0;
   pthread_exit(NULL);
}

void inroom(int sock)
{
   pthread_t getmsgs;  // at the same time our client should get others messages.
   isInRoom=1;
   char msg[300] = {0};
   char warn[35] = {0};
   read(sock,warn,sizeof(warn)); // Get warning message
   printf("%s",warn);
   pthread_create(&getmsgs,NULL,handlemsg,(void*)sock);  // Creating message handler.
   while(isInRoom)
   {
      if(isInRoom==0)
        break;
      fgets(msg,sizeof(msg),stdin);
      if(isInRoom==0)
        break;
      write(sock,msg,sizeof(msg));   // Chat with others.
      memset(msg,0,300);
   }
}

void privateroom(int sock)
{
    char passwd[10] = {0};
    memset(output,0,100);
    read(sock,output,sizeof(output));
    printf("%s",output);
    fgets(passwd,sizeof(passwd),stdin);   // Password control and go to inroom.
    int j;
    for(j=0;j<10;j++)
     {
        if(passwd[j]=='\n')
          passwd[j]='\0';
     }
     write(sock,passwd,sizeof(passwd));
     inroom(sock);
}

void enter(int sock)
{
   char passwd[10] = {0};
   memset(output,0,100);
   read(sock,output,sizeof(output));
   if(strncmp(output,"Enter Password:",16)==0)
   {
      printf("%s",output);
      fgets(passwd,10,stdin);      // if a room has password , enter it
      int i;
      for(i=0; i<10; i++)
      {
        if(passwd[i]=='\n')
          passwd[i]='\0';
      }
      write(sock,passwd,sizeof(passwd));
      memset(output,0,100);
      read(sock,output,sizeof(output));
      printf("%s\n",output);
      if(strncmp(output,"Correct!",9)==0)
        inroom(sock);
      
      return;
      
   }
   else
   {
     inroom(sock);
   }
   
}
   
int main(int argc, char const *argv[]) 
{ 
    int sock = 0, connection; 
    struct sockaddr_in serv_addr; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
   
    if ((connection = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 

    read(sock,buffer,sizeof(buffer));
    printf("%s",buffer);
    char name[12];     // Enter username.
    fgets(name,sizeof(name),stdin);
    int q;
    for(q=0; q<12; q++)
      {
          if(name[q]=='\n')
            name[q]='\0';
      }
    write(sock,name,sizeof(name));
    lobby(sock);
    
    return 0; 
} 
