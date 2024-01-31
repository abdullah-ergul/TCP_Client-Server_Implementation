#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include "socketutil.h"

struct AcceptedSocket
{
    int acceptedSocketFD;
    char nickname[10];
    struct sockaddr_in address;
    int error;
    bool acceptedSuccessfully;
};

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD);
void acceptNewConnectionAndReceiveAndPrintItsData(int serverSocketFD);
void receiveAndPrintIncomingData(int socketFD);

void startAcceptingIncomingConnections(int serverSocketFD);

void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket);

void sendReceivedMessageToTheOtherClients(char *buffer,int socketFD);
char *GetRecieverName(char *buffer);
int getParity(int n);

struct AcceptedSocket acceptedSockets[10] ;
int acceptedSocketsCount = 0;
char lastmessage[1024];


void startAcceptingIncomingConnections(int serverSocketFD) {

    while(true)
    {
        struct AcceptedSocket* clientSocket  = acceptIncomingConnection(serverSocketFD);
        acceptedSockets[acceptedSocketsCount++] = *clientSocket;

        FILE *file = fopen("log.txt", "a+");
        fprintf(file, "login > %d\t%s\t%d\t%d\n", acceptedSockets[acceptedSocketsCount-1].acceptedSocketFD, acceptedSockets[acceptedSocketsCount-1].nickname, acceptedSockets[acceptedSocketsCount-1].error, acceptedSockets[acceptedSocketsCount-1].acceptedSuccessfully);

        receiveAndPrintIncomingDataOnSeparateThread(clientSocket);
        fclose(file);
    }
}



void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket) {

    pthread_t id;
    pthread_create(&id,NULL,receiveAndPrintIncomingData,pSocket->acceptedSocketFD);
}

void receiveAndPrintIncomingData(int socketFD) {
    char buffer[1024];

    while (true)
    {
        ssize_t  amountReceived = recv(socketFD,buffer,1024,0);

        if(amountReceived>0)
        {
            buffer[amountReceived] = 0;
            if (strstr(buffer,"/list") != NULL)
            {
                char personList[1024] = {0};
                for (int i = 0; i < acceptedSocketsCount; ++i) {
                    strcat(personList, acceptedSockets[i].nickname);
                    strcat(personList, "\n");
                }

                int parity= 0;
                for (int i = 0; i < strlen(personList); ++i)
                    parity += getParity(personList[i]);
                parity = getParity(parity);

                sprintf(personList,"%s%d%s",personList,parity,"\n\0");

                sendReceivedMessageToTheOtherClients(personList,socketFD);

                continue;
            }

            if (strstr(buffer,"!err!") != NULL)
            {
                sendReceivedMessageToTheOtherClients(lastmessage,socketFD);
                continue;
            }


            //printf("%s\n",buffer);
            char *reciever = (char *)malloc(10 * sizeof(char));

            strcpy(reciever, GetRecieverName(buffer));
            int recievcerSocketFD;

            for (int i = 0; i < acceptedSocketsCount; ++i)
            {
                if (!strcmp(acceptedSockets[i].nickname, reciever))
                {
                    recievcerSocketFD = acceptedSockets[i].acceptedSocketFD;
                    break;
                }
            }

            // !!!!!!!!!!!!!!!!!!!
            char newbuffer[1024];
            strcpy(newbuffer, buffer);

            int j = 0;
            while (buffer[j] != ':')
                j++;

            strcpy(lastmessage, newbuffer);

            time_t t;
            srand((unsigned) time(&t));
            if ((rand() % 2) == 0)
                newbuffer[j+1] += 1;

            FILE *file = fopen("log.txt", "a+");
            fprintf(file, "msg   > %s -> %s\n", buffer, reciever);
            fclose(file);

            sendReceivedMessageToTheOtherClients(newbuffer,recievcerSocketFD);

        }

        if(amountReceived==0)
            break;
    }

    close(socketFD);
}

void sendReceivedMessageToTheOtherClients(char *buffer,int socketFD) {
    for(int i = 0 ; i<acceptedSocketsCount ; i++) {
        if (acceptedSockets[i].acceptedSocketFD == socketFD) {
            send(acceptedSockets[i].acceptedSocketFD, buffer, strlen(buffer), 0);
        }
    }
}

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in  clientAddress ;
    int clientAddressSize = sizeof (struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD,&clientAddress,&clientAddressSize);

    struct AcceptedSocket* acceptedSocket = malloc(sizeof (struct AcceptedSocket));
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD>0;

    if(!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->error = clientSocketFD;


    char nicknameBuffer[10];
    while (true)
    {
        ssize_t amountReceived = recv(clientSocketFD, nicknameBuffer, 10, 0);
        if (amountReceived > 0)
        {
            nicknameBuffer[amountReceived] = 0;
            strcpy(acceptedSocket->nickname, nicknameBuffer);
            break;
        }
    }

    printf("%s is joined.\n",acceptedSocket->nickname);
    return acceptedSocket;
}

char *GetRecieverName(char *buffers) {
    char *buffer = (char *)malloc(1024*sizeof(char));
    strcpy(buffer,buffers);
    char *recieverName = (char *)malloc(10*sizeof(char));
    int j=0;

    for (int i = 0; i < strlen(buffer); ++i)
    {
        if (buffer[i] == '@')
        {
            while (buffer[i] != '\0')
                recieverName[j++] = buffer[++i];
            break;
        }
    }

    for (int i = 0; i < strlen(buffer); ++i) {
        if (buffer[i] == '@'){
            buffer[i] = 0;
            break;
        }
    }

    //char *deneme = (char *) malloc(10*sizeof(char));

    strcpy(buffers, buffer);

    //printf("\n===target-> %s | msg-> %s===\n", recieverName, buffers);


    return recieverName;
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
int main() {
    int serverSocketFD = createTCPIpv4Socket();
    struct sockaddr_in *serverAddress = createIPv4Address("",2000);

    int result = bind(serverSocketFD,serverAddress, sizeof(*serverAddress));
    if(result == 0)
        printf("Socket was bound successfully\n");

    int listenResult = listen(serverSocketFD,10);

    startAcceptingIncomingConnections(serverSocketFD);

    shutdown(serverSocketFD,SHUT_RDWR);

    return 0;
}