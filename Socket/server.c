#include<stdio.h>
#include<string.h>    // for strlen
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h> // for inet_addr
#include<unistd.h>    // for write
#include<pthread.h>   // for threading, link with lpthread
#include<semaphore.h>

#define ROOMCOUNT 6
#define CLIENTCOUNT 5

sem_t getin[ROOMCOUNT];
sem_t getout[ROOMCOUNT];   // For get in to room or out.
sem_t mutex;  
char buffer[100] = {0};  // buffer for general purpose
int cur_room = 4;

typedef struct client{
   int socket_id;
   char username[12];   // Client Struct
}Client;

typedef struct room{
   char name[12];
   struct client clients[CLIENTCOUNT];  // Room Struct
   int quota;
   char type[3];   // PR or PU (Private-Public)
   char passwd[10];
}Room;

struct room rooms[ROOMCOUNT];

void inroom(Room oda,Client user,int slctd,int cltindx);  //While in a room
void list();
void create(char* name,Client user);
void pcreate(char* name,Client user);
void enter(char* name,Client user);


void *lobby(void *socket_desc)
{  
    int sock_id = *((int*)socket_desc);   
 
    char command[20] = {0};  // Get commands with this
    char nick[12] = {0};  // username
    char msg[60]= "Enter your nickname! : ";

    Client user;   
    user.socket_id = sock_id;
    write(sock_id,msg,sizeof(msg));  //Send warn msg
    read(sock_id,nick,sizeof(nick)); //Get username
    strcpy(user.username,nick);

    strcpy(msg,"\nWelcome to DEU CHAT Lobby! Enter a command pls!\n\n");
    write(sock_id,msg,sizeof(msg));


    read(sock_id,command,sizeof(command)); //Wait for a command

    while(1)
    {

      if(strncmp(command,"-list",5)== 0)
        {
           sem_wait(&mutex);
           memset(buffer,0,100);  // List rooms.
           list();
           write(sock_id,buffer,sizeof(buffer));
           sem_post(&mutex);
        }
      else if(strncmp(command,"-pcreate",8)==0)
       {pcreate(command,user);}
      else if(strncmp(command,"-enter",6)==0)
       {enter(command,user);}
      else if(strncmp(command,"-create",7)==0)
       {create(command,user);}
      else if(strncmp(command,"-exit",5)==0)
       {write(sock_id,"You choose to exit",19); break;}
      else if(strncmp(command,"-whoami",7)==0)
       {write(sock_id,user.username,sizeof(user.username));}
      else
      { write(sock_id,"wrong command",14);}
      
      memset(command,0,20);

      read(sock_id,command,sizeof(command));  // Read a command again.
    }
    close(sock_id);
    free(socket_desc); //Free the socket pointer
    pthread_exit(NULL);     
}

void enter(char* name,Client user)
{
    char passwd[10]={0};
    int i;
    for(i=0;i<7;i++)
      name++;
    int cont; int slcted;  //control and selected index.
    cont = 0;
    for(i=0;i<ROOMCOUNT;i++){
      if(strcmp(rooms[i].name,"") !=0 && strcmp(rooms[i].name,name)==0)
       { cont=1; slcted = i;}  // if there is that room make cont true(1)
    }
    if(cont==0)
      {write(user.socket_id,"Wrong Room Name!",17);return;} // Room not found.

    sem_wait(&getin[slcted]);
    int index = rooms[slcted].quota;
    if(index==CLIENTCOUNT)
      {write(user.socket_id,"Room is Full",13);return;}  // return if full
    write(user.socket_id,"Joining to Room",16);
    if(strcmp(rooms[slcted].type,"PR")==0)
      {
        sleep(1);
        write(user.socket_id,"Enter Password:",16);   // For Private Room
        read(user.socket_id,passwd,sizeof(passwd));
        if(strcmp(rooms[slcted].passwd,passwd)!=0)
          {write(user.socket_id,"Wrong Pass",11); sem_post(&getin[slcted]);return; }
        else
        {
          write(user.socket_id,"Correct!",9);
        }
      }
    else
    {
      sleep(1);
      write(user.socket_id,"NoPass",7);  // Continue normally if no Pass.
    }
    int q;
    for(q=0;q<CLIENTCOUNT;q++)
    {
      if(rooms[slcted].clients[q].socket_id==-1)
        {
           rooms[slcted].clients[q].socket_id = user.socket_id;
           strcpy(rooms[slcted].clients[q].username,user.username);
           index = q;
           break;
        }
    }
    rooms[slcted].quota++;
    sem_post(&getin[slcted]);
    sleep(1);
    inroom(rooms[slcted],user,slcted,q);
}

void pcreate(char* name,Client user)
{
    char passwd[10];
    if(cur_room>5)
      {write(user.socket_id,"Max Capacity!",14);return;}
    int i;
    for(i=0;i<9;i++)
      name++;
    int cont;            // Enter , create , pcreate are similar.
    cont = 1;
    for(i=0;i<ROOMCOUNT;i++){
      if(strcmp(rooms[i].name,"") !=0 && strcmp(rooms[i].name,name)==0)
       { cont=0;  }
    }
    if(cont==0)
      {write(user.socket_id,"Room already exists!",21);return;}

    sem_wait(&getin[cur_room]);
    write(user.socket_id,"Creating a Private Room",24);
    sleep(1);
    write(user.socket_id,"Enter a password:",18);
    read(user.socket_id,passwd,sizeof(passwd));
    int index = rooms[cur_room].quota;
    rooms[cur_room].clients[index].socket_id = user.socket_id;
    strcpy(rooms[cur_room].clients[index].username,user.username);
    strcpy(rooms[cur_room].name,name);
    rooms[cur_room].quota = index+1;
    strcpy(rooms[cur_room].type,"PR");
    strcpy(rooms[cur_room].passwd,passwd);
    cur_room++;
    sem_post(&getin[cur_room-1]);
    sleep(1);
    inroom(rooms[cur_room-1],user,cur_room-1,index);
}

void create(char* name,Client user)
{
    if(cur_room>5)
      {write(user.socket_id,"Max Capacity!",14);return;}
    int i;
    for(i=0;i<8;i++)
      name++;
    int cont;
    cont = 1;
    for(i=0;i<ROOMCOUNT;i++){
      if(strcmp(rooms[i].name,"") !=0 && strcmp(rooms[i].name,name)==0)
       { cont=0;  }
    }
    if(cont==0)
      {write(user.socket_id,"Room already exists!",21);return;}
    sem_wait(&getin[cur_room]);
    write(user.socket_id,"Creating a Public Room",23);
    int index = rooms[cur_room].quota;
    rooms[cur_room].clients[index].socket_id = user.socket_id;
    strcpy(rooms[cur_room].clients[index].username,user.username);
    strcpy(rooms[cur_room].name,name);
    rooms[cur_room].quota = index+1;
    strcpy(rooms[cur_room].type,"PU");
    cur_room++;
    sem_post(&getin[cur_room-1]);
    sleep(1);
    inroom(rooms[cur_room-1],user,cur_room-1,index);
}

void list()
{
   int i;
   for(i=0;i<ROOMCOUNT;i++)
    {
        if(strcmp(rooms[i].type,"PU")==0)
        {
          sprintf(buffer,"%s  %s(",buffer,rooms[i].name);  // List Public Rooms
          int j;
          for(j=0;j<CLIENTCOUNT;j++)
            {
              if(rooms[i].clients[j].socket_id!=-1)
                sprintf(buffer,"%s %s ",buffer,rooms[i].clients[j].username);
            }
          sprintf(buffer,"%s) ",buffer);
        }
    }
}

void inroom(Room oda,Client user,int slctd,int cltindx)
{
    char buff[300] = {0};
    char warn[35] = {0};
    char msg[500] = {0};
    sprintf(warn,"Welcome to Room %s\n",oda.name);
    write(oda.clients[cltindx].socket_id,warn,sizeof(warn));
    memset(buff,0,300);
    read(user.socket_id,buff,sizeof(buff));
    while(1)
    {
      if(strncmp(buff,"-quit",5)==0)
         {write(user.socket_id,"Quit Selected",14);break;}  // While in room
       int i;
       sem_wait(&mutex);
       sprintf(msg," %s : %s",user.username,buff);
       for(i=0;i<CLIENTCOUNT;i++)
       {
         if(rooms[slctd].clients[i].socket_id!=-1)
         {
           write(rooms[slctd].clients[i].socket_id,msg,sizeof(msg));  // Send the message to all clients in the room.
         }
       }
       memset(buff,0,300);
       sem_post(&mutex);
       read(user.socket_id,buff,sizeof(buff));
    }
    sem_wait(&getout[slctd]);
    rooms[slctd].quota--;
    rooms[slctd].clients[cltindx].socket_id=-1;
    strcpy(rooms[slctd].clients[cltindx].username,"");  // If a client exits.
    if(slctd==5 && strcmp(rooms[5].name,"")==0)
       {slctd--; rooms[slctd].quota=0;}
    if(rooms[slctd].quota==0 && slctd>3)
      {
        if(slctd==4 && strcmp(rooms[5].name,"")!=0)  // Swap deleted one , doesn't work perfectly..
        {
         strcpy(rooms[slctd].name,rooms[5].name);
         strcpy(rooms[slctd].type,rooms[5].type);
         strcpy(rooms[slctd].passwd,rooms[5].passwd);
         rooms[slctd].quota=rooms[5].quota;
          int j;
          for(j=0;j<CLIENTCOUNT;j++)
          {
            if(rooms[5].clients[j].socket_id!=-1)
              {
                rooms[slctd].clients[j]=rooms[5].clients[j];
              }
          }
          strcpy(rooms[5].name,"");
          strcpy(rooms[5].type,"");
          strcpy(rooms[5].passwd,"");
        }
        else
        {
           strcpy(rooms[slctd].name,"");
           strcpy(rooms[slctd].type,"");
           strcpy(rooms[slctd].passwd,"");
        }
        cur_room--;
      }
    sem_post(&getout[slctd]);
}


void preparerooms()
{
   strcpy(rooms[0].name,"Antalya");
   strcpy(rooms[1].name,"Manisa");
   strcpy(rooms[2].name,"Izmir");
   strcpy(rooms[3].name,"Ankara");  // Prepare main rooms
    
   int i;
   for(i=0;i<ROOMCOUNT-2;i++)
   {
     rooms[i].quota = 0;
     strcpy(rooms[i].type,"PU");
   }
   for(i=0;i<ROOMCOUNT;i++)
   {
     int j;
     for(j=0;j<CLIENTCOUNT;j++)
       rooms[i].clients[j].socket_id=-1;
   }
}


int main(int argc, char *argv[])
{
    int socket_desc, new_socket, c, *new_sock;
    struct sockaddr_in server, client;
    int opt = 1;
     
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        puts("Could not create socket");
        return 1;
    }

    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(3205);
     
    if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
{
        puts("Binding failed");
        return 1;
    }
     
    listen(socket_desc, 3);

    preparerooms();

    sem_init(&mutex,0,1);
    int j;
    for(j=0;j<ROOMCOUNT;j++){
      sem_init(&getin[j],0,1);   // init semaphores.
      sem_init(&getout[j],0,1);
    }
     
    puts("Waiting for incoming connections...");

    c = sizeof(struct sockaddr_in);

    while((new_socket = 
           accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;
         
        if(pthread_create(&sniffer_thread, NULL, lobby,
                          (void*)new_sock) < 0)
        {
            puts("Could not create thread");
            return 1;
        }
    }
     
    return 0;
}