#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include "socketutil.h"

void startListeningAndPrintMessagesOnNewThread(int fd);

void listenAndPrint(int socketFD);

void readConsoleEntriesAndSendToServer(int socketFD);

int getParity(int n);

char *name;

int main() {

    int socketFD = createTCPIpv4Socket();
    struct sockaddr_in *address = createIPv4Address("127.0.0.1", 2000);


    int result = connect(socketFD,address,sizeof (*address));
    if(result == 0)
        printf("Connection was successful\n");

    name = NULL;
    size_t nameSize= 0;
    printf("Please enter your name\n");
    ssize_t  nameCount = getline(&name,&nameSize,stdin);
    name[nameCount-1]=0;

    ssize_t amountWasSent =  send(socketFD,
                                  name,
                                  strlen(name), 0);


    startListeningAndPrintMessagesOnNewThread(socketFD);

    readConsoleEntriesAndSendToServer(socketFD);


    close(socketFD);

    return 0;
}

void readConsoleEntriesAndSendToServer(int socketFD) {


    printf("Which user do you want to send message?\n");
// recieve()

    //for (int i = 0; i < ; ++i) {
    //
    //}

    char *recieverName =NULL;
    size_t recieverNameSize =0;
    ssize_t  recieverNameCount = getline(&recieverName,&recieverNameSize,stdin);
    recieverName[recieverNameCount-1]=0;



    char *line = NULL;
    size_t lineSize= 0;
    printf("Enter your message (type /help for commands)...\n");
    // and use "@" before the reciever name

    char buffer[1024];

    while(true)
    {
        ssize_t  charCount = getline(&line,&lineSize,stdin);
        line[charCount-1]=0;

        sprintf(buffer,"%s:%s",name,line);

        int parity= 0;
        for (int i = 0; i < strlen(buffer); ++i)
            parity += getParity(buffer[i]);
        parity = getParity(parity);

        sprintf(buffer,"%s%d%c",buffer, parity,'\0');

        if(charCount>0)
        {
            if(strcmp(line,"/exit")==0)
                break;
            else if(strcmp(line,"/help") == 0){
                printf("\"/exit\" -> exit the application\n\"/list\" -> list of online people\n");
                continue;
            }
            else if(!strcmp(line, "/list")){
                send(socketFD,buffer,strlen(buffer),0);
                while (1){
                    char personList[1024];
                    ssize_t  amt = recv(socketFD,personList,1024,0);
                    if  (amt > 0){
                        printf("%s", personList);
                        break;
                    }
                }
                continue;
            }

            // @eray mrb ========= buffer = mrb@eray ======= for @
            strcat(buffer, "@");
            strcat(buffer,recieverName);
            //strcat(buffer, "\0");

            ssize_t amountWasSent =  send(socketFD,
                                          buffer,
                                          strlen(buffer), 0);
        }
    }
}

void startListeningAndPrintMessagesOnNewThread(int socketFD) {

    pthread_t id ;
    pthread_create(&id,NULL,listenAndPrint,socketFD);
}

void listenAndPrint(int socketFD) {
    char buffer[1024];

    while (true)
    {
        ssize_t  amountReceived = recv(socketFD,buffer,1024,0);
        char type[10];

        int n = amountReceived;
        int i = (int)n; i--;
//        while (buffer[i+1] != '\0')
//            i++;

//        char checkbuffer[1024];
//        int j = 0;
//        while (buffer[j] != ':')
//            j++;
//
//        int k = 0;
//        while (buffer[j+1] != '\0')
//        {
//            checkbuffer[k] = buffer[j+1];
//            k++; j++;
//        }
//        checkbuffer[k] = '\0';

        int typenumber=0;
        time_t t;
        srand((unsigned) time(&t));
        if ((rand() % 2) == 0)
            typenumber = 1;
        else
            typenumber = 0;


        // LIST
        if ( strstr(buffer,"\n") != NULL )
        {
            if(amountReceived>0)
            {
                buffer[amountReceived] = 0;
//                int c = 0;
//                while (buffer[c+2] != '\0')
//                    c++;
//                buffer[c] = '\0';
//                buffer[c+1] = '\0';
                printf("%s\n ",buffer);
            }

            if(amountReceived==0)
                break;
        }

        // PARITY
        else if (typenumber==0)
        {
            int cal_parity= 0;
            for (int j = 0; j < strlen(buffer)-1; ++j){
                if(buffer[j] == '\0' || buffer[j] == '\n') continue;

                cal_parity += getParity(buffer[j]);
            }

            cal_parity = getParity(cal_parity);

            if (buffer[i]-48 == cal_parity)
            {
                if(amountReceived>0)
                {
                    buffer[amountReceived] = 0;
                    printf("%s\n ",buffer);
                }

                if(amountReceived==0)
                    break;
            }
            else
            {
                if(amountReceived>0)
                {
                    buffer[amountReceived] = 0;
                    printf("%s",type);
                    printf("You got a corrupted message!\n");

                    char errmesage[10];
                    strcpy(errmesage, "!err!");
                    ssize_t amountWasSent =  send(socketFD,
                                                  errmesage,
                                                  strlen(errmesage), 0);


//                  char errmesage[100];
//                  strcpy(errmesage, name);
//
    //                char errmesageSender[10];
    //
    //                int j=0;
    //                while (buffer[j] != ':')
    //                {
    //                    errmesageSender[j] = buffer[j];
    //                    j++;
    //                }
    //                errmesageSender[j] = '\0';
    //
    //                strcat(errmesage, ":Your message has been corrupted!@");
    //                strcat(errmesage,errmesageSender);
    //                ssize_t amountWasSent =  send(socketFD,
    //                                              errmesage,
    //                                              strlen(errmesage), 0);
                }
                if(amountReceived==0)
                    break;
            }

        }

        // CRC
        else
        {

        }
    }

    close(socketFD);
}

int getParity(int n)
{
    int parity = 0;
    while (n)
    {
        parity = !parity;
        n = n & (n - 1);
    }
    return parity;
}