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
#include <regex>
using namespace std;

#define ERROR_MSG_NO_CONNECTION "No Connection Established"
#define MSG_RECONNECTING "Connection Closed, reconnecting..."
#define CRLF "\r\n"
#define SERVER_ADDR "ftp.ietf.org"

void connect();
string sendMessageToServer(string snd_data);
void setupSockets(int *client_socket, sockaddr_in *server_socket, string server_addr, int server_port);


string connected_server = "", email = "e0032087@u.nus.edu";
int client_socket = -1;

string requestInput(string msg) {
	if (client_socket > -1) {
		return "(" + connected_server + ") " + msg; 
	} else return msg;
}

void error(string error_msg) {
	cout << error_msg << endl;
}

void getResponseCode(const char *rcv_data, char *response_code) {
	strncpy(response_code, rcv_data, 3);
}

int hasResponseCode(const char *rcv_data, string response_code_match) {
	char response_code[3] = ""; getResponseCode(rcv_data, response_code);
	return (!strncmp(response_code, response_code_match.c_str(), 3)) ? 1 : 0;
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
int connectToServerSocket(int client_socket, sockaddr_in server_sock) {
	int connection = connect(client_socket, (struct sockaddr *)&server_sock, sizeof(struct sockaddr));
	if (connection < 0) {
		perror("Connect");
		return 0;
	}
	return 1;
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
	if(!connectToServerSocket(client_socket, server_socket)) return 0;
	char rcv_data[1024] = "";
	recv(client_socket, rcv_data, 1024, 0);

	if (hasResponseCode(rcv_data, "220")) {
		cout << "Successfully Connected to FTP Server! Response: " << rcv_data << endl;
		return 1;
	} else {
		cout << "Failed to connect to FTP Server! Response: " << rcv_data << endl;
		return 0;
	}
}

void handleRcvData(string rcv_data, string snd_data) {
	cout << requestInput(string("ftp >> ") + rcv_data);
	if(hasResponseCode(rcv_data.c_str(), "421")) {
		// connection closed
		cout << MSG_RECONNECTING << endl;
		connect();
		sendMessageToServer(snd_data);
	}

}

string receiveMessageFromServer(int client_socket) {
	char rcv_data[1024] = "";
	recv(client_socket, rcv_data, 1024, 0);
	return string(rcv_data);
}

string sendMessageToServer(string snd_data) {
	if (client_socket < 0) {
		error(ERROR_MSG_NO_CONNECTION);
		return "";
	} else {
		snd_data += CRLF;
		int bytes_sent = send(client_socket, snd_data.c_str(), snd_data.length(), MSG_DONTWAIT);
		string rcv_data = receiveMessageFromServer(client_socket);
		handleRcvData(rcv_data, snd_data);
		return rcv_data;
	}
	
}

string extractEpsvPort(string response) {
	if (!hasResponseCode(response.c_str(), "229")) return "";
	regex reg("229 Entering Extended Passive Mode \\(\\|\\|\\|([0-9]+)\\|\\)");
	smatch match;
	string psv_port = "";
	if(regex_search(response, match, reg)) {
		psv_port = match[1];
	}
	return psv_port;
}

int epsv() {
	string response = sendMessageToServer("EPSV");
	string psv_port = extractEpsvPort(response);
	if (psv_port.empty()) return 0;
	int client_psv_socket;
	sockaddr_in server_psv_socket;
	string psv_server_addr = SERVER_ADDR;
	int psv_server_port = stoi(psv_port);
	setupSockets(&client_psv_socket, &server_psv_socket, psv_server_addr, psv_server_port);
	return (connectToServerSocket(client_psv_socket, server_psv_socket)) ? client_psv_socket : 0;
}

void pwd() {
	string response = sendMessageToServer("PWD");
}

void cwd() {

}

void list() {
	int client_psv_socket = epsv();
	if (client_psv_socket) {
		string response = sendMessageToServer("LIST");
		if (hasResponseCode(response.c_str(), "150")) {
			string rcv_data = receiveMessageFromServer(client_psv_socket);
			cout << "Received Data: " << endl << rcv_data << endl;
			close(client_psv_socket);

			string trans_complete = receiveMessageFromServer(client_socket);
			cout << "Trans Complete: " << trans_complete << endl;
		}
	}
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

void setupSockets(int *client_socket, sockaddr_in *server_socket, string server_addr, int server_port) {
	string server_ip; server_ip = getServerIp(server_addr);
	*client_socket = createMySocket();
	*server_socket = setupServerSocket(server_port, server_ip);
}

void connect() {
	string server_addr = SERVER_ADDR;
	sockaddr_in server_socket;
	setupSockets(&client_socket, &server_socket, server_addr, 21);
	if(connectToFtpServer(client_socket, server_socket)) {
		connected_server = server_addr;
		sendMessageToServer("USER anonymous");
		if(email.empty()) {
			cout << requestInput("NUS Email: ");
			cin >> email;
		}
		sendMessageToServer("PASS " + email);
	} else {
		cout << "Failed to connect to FTP Server" << endl;
	}
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