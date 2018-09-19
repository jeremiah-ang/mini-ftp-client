#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h> 
#include <netdb.h>
#include <unistd.h>

#define ERROR_MSG_NO_CONNECTION "No Connection Established"
#define CRLF "\r\n"
#define SERVER_ADDR "ftp.ietf.org"

using namespace std;

string connected_server = "";
int client_socket = -1;

string requestInput(string msg) {
	if (client_socket > -1) {
		return "(" + connected_server + ") " + msg; 
	} else return msg;
}

void error(string error_msg) {
	cout << error_msg << endl;
}

// Create client socket
int createMySocket() {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		perror("Socket");
		exit(1);
	}
	return sock;
}

// Set parameters of server port
sockaddr_in setupServerSocket(int server_port, string server_ip) {
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	inet_pton(AF_INET, server_ip.c_str(), &(server_addr.sin_addr));
	return server_addr;
}

// Connect to server's socket
void connectToServerSocket(int client_socket, sockaddr_in server_sock) {
	int connection = connect(client_socket, (struct sockaddr *)&server_sock, sizeof(struct sockaddr));
	if (connection < 0) {
		perror("Connect");
		exit(1);
	}
}

// Get IP Address of Server
string getServerIp(string server_addr) {
	struct addrinfo hints, *infoptr;
	hints.ai_family = AF_INET;
	memset(&hints, 0, sizeof hints);
	
	int result = getaddrinfo(server_addr.c_str(), NULL, &hints, &infoptr);
	if (result) { // If failed to get IP Address
		cout << "Error Getting " << server_addr << " Info: " << gai_strerror(result) << endl;
		exit(1);
	}

	char server_ip[256];
	getnameinfo(infoptr->ai_addr, infoptr->ai_addrlen,
				server_ip, sizeof(server_ip),
				NULL, 0, NI_NUMERICHOST);

	return string(server_ip);
}

int connectToFtpServer(int client_socket, sockaddr_in server_socket) {
	connectToServerSocket(client_socket, server_socket);
	char rcv_data[1024];
	recv(client_socket, rcv_data, 1024, 0);

	char connection_status[3];
	strncpy(connection_status, rcv_data, 3);
	if (strcmp(connection_status, "220")) {
		cout << "Successfully Connected to FTP Server! Response: " << rcv_data << endl;
		return 1;
	} else {
		cout << "Failed to connect to FTP Server! Error Code: " << connection_status << endl;
		return 0;
	}
}

string sendMessageToServer(string snd_data) {
	if (client_socket < 0) {
		error(ERROR_MSG_NO_CONNECTION);
		return "";
	} else {
		snd_data += CRLF;
		int bytes_sent = send(client_socket, snd_data.c_str(), snd_data.length(), MSG_DONTWAIT);
		char rcv_data[1024] = "";
		recv(client_socket, rcv_data, 1024, 0);
		cout << requestInput(string("ftp >> ") + string(rcv_data));
		return string(rcv_data);
	}
	
}

void pwd() {
	string response = sendMessageToServer("PWD");
	
}

void cwd() {

}

void list() {

}

void retr() {
	cout << "Enter filename: ";
	string filename; cin >> filename;
}

void stor() {
	cout << "Enter filename: ";
	string filename; cin >> filename;
}

void quit() {
	cout << "Exiting FTP Client..." << endl;
	exit(0);
}	

void connect() {
	cout << "Enter FTP Server Address: ";
	string server_addr = SERVER_ADDR;
	string server_ip; server_ip = getServerIp(server_addr);
	int server_port = 21;
	cout << "Connecting to " << server_addr;
	cout << " at IP: " << server_ip; 
	cout << " Port: " << server_port << endl;
	client_socket = createMySocket();
	sockaddr_in server_socket = setupServerSocket(server_port, server_ip);
	if(connectToFtpServer(client_socket, server_socket)) {
		connected_server = server_addr;
		sendMessageToServer("USER anonymous");
		cout << requestInput("NUS Email: ");
		string email; cin >> email;
		sendMessageToServer("PASS " + email);
	};
}

void invalid(int command) {
	cout << "INVALID COMMAND: " << command << endl;
}

void displayOptions() {
	cout << "*******************************************" << endl;
	cout << "*******Mini-ftp.ietf.org-FTP client********" << endl;
	cout << "*******************************************" << endl;
   	cout << "1. Connect to FTP server" << endl;
   	cout << "2. Print working directory" << endl;
    cout << "3. Change working directory" << endl;
   	cout << "4. List all files" << endl;
   	cout << "5. Upload file" << endl;
   	cout << "6. Download file" << endl;
   	cout << "7. Quit" << endl;
   	cout << "Enter option (1-7): ";
}

int main(int argc, char const *argv[]) {
	int command;
	displayOptions();

	while(cin >> command) {
		switch(command) {
			case 1: connect(); break;
			case 2: pwd(); break;
			case 3: cwd(); break;
			case 4: list(); break;
			case 5: stor(); break;
			case 6: retr(); break;
			case 7: quit(); break;
			default: invalid(command);
		}

		cout << requestInput("Enter option (1-7): ");
	}
	return 0;
}