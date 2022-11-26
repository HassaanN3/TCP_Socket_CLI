#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> //for bool
#include <unistd.h> //for socket reading and writing
#include <netdb.h>  //for gethostname and hostent
#include <netinet/tcp.h>    //for setsockopt -> reduce connection timeout

#define MAX_SIZE 32

int get_input(int min, int max) {   //scanf but better B)
    char temp_input[3];
    int output;
    while(1) {
        fgets(temp_input, 3, stdin);
        output = atoi(temp_input);
        if (output >= min && output <= max)
            break;
        else
            printf("Enter valid option between %d and %d\n: ", min, max);
    }
    return output;
}

void remove_garbage(int size) { //Removes any garbage values at the end of the file
    FILE *fileptr1, *fileptr2;
    char ch;
    int count = 0;
    fileptr1 = fopen("Result.txt", "r");
    fileptr2 = fopen("Temp.txt", "w");
    ch = getc(fileptr1);
    while (ch != EOF) {
        count++;
        putc(ch, fileptr2);
        ch = getc(fileptr1);
        if (count == size) {
            break;
        }
    }
    fclose(fileptr1);
    fclose(fileptr2);
    remove("Result.txt");
    rename("Temp.txt", "Result.txt");
}

void display() {    //Displays file content
    FILE *fp;
	char c;
	fp = fopen("Result.txt", "r");
	if (fp == NULL) {
		printf("Cannot open file \n");
		exit(0);
	}
	c = fgetc(fp);
	while (c != EOF) {
		printf ("%c", c);
		c = fgetc(fp);
    }
	fclose(fp);
}

int main() {
    int socketfd, portno = 8080, n, input;
    //char buffer[MAX_SIZE], ip[MAX_SIZE] = "10.7.80.68", port[MAX_SIZE] = "8080";
    char buffer[MAX_SIZE], ip[MAX_SIZE], port[MAX_SIZE];
    FILE *output;
    struct sockaddr_in server_addr;
    struct hostent *server;
    bool ipchange = true;   //flag to set new ip if changed.
    
    printf("Enter Host IP: ");
    fgets(ip, MAX_SIZE, stdin);
    ip[strcspn(ip, "\n")] = 0;  //removes trailing new line that fgets creates
    printf("Enter Server Port: ");
    fgets(port, MAX_SIZE, stdin);
    port[strcspn(port, "\n")] = 0;  //removes trailing new line that fgets creates
    portno = atoi(port);
    printf("Server Address: %s:%d\n\n", ip, portno);
    
    while (1) {
        puts("1. Connect to Socket\n2. Enter new IP\n3. Exit");
        printf(": ");
        input = get_input(1,3);
        if (input == 1) {   //Connect to Server
            server = gethostbyname(ip);
            socketfd = socket(AF_INET, SOCK_STREAM, 0);   //IPv4 protocols & 2-way reliable connection based
            if (socketfd < 0) {
                perror("Error while opening socket");
            }
            else {
                if (ipchange) { //if ip has been changed, save new ip address in sockaddr_in
                    bzero((char *) &server_addr, sizeof(server_addr));  //erasing sizeof(server_addr) bits starting at &server_addr
                    server_addr.sin_family = AF_INET; //server address protocol
                    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);    //Server IP = Host IP
                    server_addr.sin_port = htons(portno);     //Host byte order
                    ipchange = false;
                }
                //To speed-up connect timeoutput
                int synRetries = 2; // Send a total of 3 SYN packets => Timeoutput ~7s
                setsockopt(socketfd, IPPROTO_TCP, TCP_SYNCNT, &synRetries, sizeof(synRetries));
                //Reference https://stackoverflow.com/a/46473173
                if (connect(socketfd,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
                    perror("Error connecting");
                }
                else {
                    puts("\nSuccessfully connected to Server");
                    while(1) {
                            puts("1. Enter Command\n2. Close Socket");
                            printf(": ");
                            input = get_input(1,2);
                        if (input == 1) {    //Command
                            printf("\nEnter the command: ");
                            memset(buffer, 0, MAX_SIZE);    //clearing buffer cuz its C duh -> no risk
                            fgets(buffer, MAX_SIZE - 1, stdin);
                            buffer[strlen(buffer) - 1] = '\0';
                            n = write(socketfd, buffer, strlen(buffer));  //Sending command to server
                            if (n < 0) {
                                perror("Error while writing to socket");
                            }
                            else {
                                output = fopen("Result.txt", "wb");
                                int size = 0;
                                read(socketfd, &size, sizeof(size));    //Reading number of total characters that are being sent
                                size = ntohl(size);
                                while (1) { 
                                    bzero(buffer, MAX_SIZE);    //Clearing Buffer cuz its C duh                                       
                                    n = read(socketfd, buffer, MAX_SIZE);
                                    if(strstr(buffer, "EOF")) {  //check for EOF -> Custom repeating string that denotes end of string
                                        explicit_bzero(strstr(buffer, "EOF"), MAX_SIZE); //Clear n bytes of memory from the point where the first EOF identified
                                        fwrite(buffer, 1, n, output);  //Write final transferred buffer
                                        break;
                                    }
                                    if (n < 0)
                                        perror("Error while reading from socket");
                                    else if (n == 0) // Socket closed -> Transfer completed
                                        break;
                                    fwrite(buffer, 1, n, output);
                                }                                
                                if (output != NULL) {
                                    fclose(output);
                                }
                                puts("Output:\n");
                                remove_garbage(size);   //Removing any trailing garbage characters that might have been saved in file
                                display();
                                remove("Result.txt");
                                close(socketfd);
                                puts("\n\nDisconnected from Socket");
                                break;  //server closes socket after file transfer hence breaking the loop
                            }
                        }
                        else if (input == 2){
                            char msg[6] = "exit";
                            write(socketfd, msg, strlen(msg));
                            close(socketfd);
                            puts("\nDisconnected from Socket");
                            break;
                        }
                        puts("");
                    }
                }
            }
        }
        else if (input == 2){
            printf("\nEnter New Host IP: ");
            fgets(ip, MAX_SIZE, stdin);
            ip[strcspn(ip, "\n")] = 0;  //removes trailing new line that fgets creates

            printf("Enter Server Port: ");
            fgets(port, MAX_SIZE, stdin);
            port[strcspn(port, "\n")] = 0;  //removes trailing new line that fgets creates
            portno = atoi(port);

            printf("Server Address: %s:%d\n\n", ip, portno);
            ipchange = true;
        }
        else if (input == 3){
            exit(0);
        }
        puts("");   //new line
    }
    return 0;
}