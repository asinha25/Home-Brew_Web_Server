// 
// A Home-Brew Web Server
//
// Name: Aditya Sinha
// NetID: asinha
// 

#include <fnmatch.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <dirent.h>

#define BACKLOG (10)

void serve_request(int);

// 
// This handles other binary files such as html, txt, jpeg, gif, png, pdf and ico.
// 

// handles html requests - (given)
char * request_str = "HTTP/1.0 200 OK\r\n"
"Content-type: text/html; charset=UTF-8\r\n\r\n";

// handles text requests 
char * request_strTXT = "HTTP/1.0 200 OK\r\n"
"Content-type: text/plain; charset=UTF-8\r\n\r\n";

// handles jpeg requests
char * request_strJPG = "HTTP/1.0 200 OK\r\n"
"Context-type: image/jpeg; charset=UTF-8\r\n\r\n";

// handles gif requests
char * request_strGIF = "HTTP/1.0 200 OK\r\n"
"Context-type: image/gif; charset=UTF-8\r\n\r\n";

// handles png requests
char * request_strPNG = "HTTP/1.0 200 OK\r\n"
"Context-type: image/png; charset=UTF-8\r\n\r\n";

// handles pdf requests
char * request_strPDF = "HTTP/1.0 200 OK\r\n"
"Context-type: application/pdf; charset=UTF-8\r\n\r\n";

// handles ico requests
char * request_strICO = "HTTP/1.0 200 OK\r\n"
"Context-type: image/x-icon; charset=UTF-8\r\n\r\n";


char * index_hdr = "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\"><html>"
"<title>Directory listing for %s</title>"
"<body>"
"<h2>Directory listing for %s</h2><hr><ul>"; 

// snprintf(output_buffer,4096,index_hdr,filename,filename);

char * index_body = "<li><a href=\"%s\">%s</a>";

char * index_ftr = "</ul><hr></body></html>";

//
// Function that returns the appropriate request based on the file extension
//
char* mime_type(char* filename)
{
	if(strstr(filename, ".html"))
		return request_str;
	else if(strstr(filename, ".txt"))
		return request_strTXT;
	else if(strstr(filename, ".jpeg") || strstr(filename, ".jpg") )
		return request_strJPG;
	else if(strstr(filename, ".gif"))
		return request_strGIF;
	else if(strstr(filename, ".png"))
		return request_strPNG;
	else if(strstr(filename, ".pdf"))
		return request_strPDF;
	else if(strstr(filename, ".ico"))
		return request_strICO;
	else
		return request_strTXT;
}

/* char* parseRequest(char* request)
* Args: HTTP request of the form "GET /path/to/resource HTTP/1.X" 
*
* Return: the resource requested "/path/to/resource"
*         0 if the request is not a valid HTTP request 
* 
* Does not modify the given request string. 
* The returned resource should be free'd by the caller function. 
*/
char* parseRequest(char* request) {
//assume file paths are no more than 256 bytes + 1 for null. 
	char *buffer = malloc(sizeof(char)*257);
	memset(buffer, 0, 257);

	if(fnmatch("GET * HTTP/1.*",  request, 0)) return 0; 

	sscanf(request, "GET %s HTTP/1.", buffer);
	return buffer; 
}

//
// Function that uses the stat call to determine if path points 
// to a regular file
//
int is_reg(const char *path) {
	struct stat stat_path;
	stat(path, &stat_path);
	return S_ISREG(stat_path.st_mode);
} 

//
// Function that uses the stat call to determine if path points 
// to a directory
//
int is_dir(const char *path) {
	struct stat stat_path;
	stat(path, &stat_path);
	return S_ISDIR(stat_path.st_mode);
}

void serve_request(int client_fd){
  int read_fd;
  int bytes_read;
  int file_offset = 0;
  char client_buf[4096];
  char send_buf[4096];
  char filename[4096];
  char filename_2[4096];
  char * requested_file;
  memset(client_buf,0,4096);
  memset(filename,0,4096);
  while(1){

    file_offset += recv(client_fd,&client_buf[file_offset],4096,0);
    if(strstr(client_buf,"\r\n\r\n"))
      break;
  }
  requested_file = parseRequest(client_buf);
  filename[0] = '.';
  strncpy(&filename[1],requested_file,4095);
	

	//
	// Check if the requested file is in the directory
	//
	if(is_dir(filename) > 0) {
		strcpy(filename_2, filename);
		strcat(filename_2, "/index.html"); 

		// if the requested file exists...
		if(is_reg(filename_2) > 0) {
			send(client_fd,request_str,strlen(request_str),0);
			// open the file
			read_fd = open(filename_2,0,0);
			// while true
			while(1){
				// read the file	
				bytes_read = read(read_fd,send_buf,4096);
				// if the file is empty or non-existent, break
				if(bytes_read == 0)
					break;
				// sends the requested file if the file is valid
				send(client_fd,send_buf,bytes_read,0);
			}
			close(read_fd); // close the file
			close(client_fd); // close the connection			
		}

		// if requested file is not found, list all the files in the directory
		else {
			// initialize vars for replacing header and body
			char* index_bodyRep = (char*) malloc(sizeof(char)*4096);
			char* index_headerRep = (char*) malloc(sizeof(char)*4096);
			// open the directory path
			DIR* path = opendir(filename);
			// replace header
			snprintf(index_headerRep, 4096, index_hdr, filename, filename);
			// send request
			send(client_fd, request_str, strlen(request_str), 0);
			// send header
			send(client_fd, index_headerRep, strlen(index_headerRep), 0);

			//check if the opening the directory was successful
			if(path != NULL) {
			// stores underlying info of files and sub_directories of directory_path
				struct dirent* underlying_file = NULL;
      		// while no more files, send body
				while((underlying_file = readdir(path)) != NULL) {
      			// replace body
					//printf("[%s]\n", underlying_file->d_name);
					printf("/");
					// replace body
					snprintf(index_bodyRep, 4096, index_body, underlying_file->d_name, underlying_file->d_name);
					// send index_body
					send(client_fd, index_bodyRep, strlen(index_bodyRep), 0);
				}
			// send footer
				send(client_fd, index_ftr, strlen(index_ftr), 0);
			// close the directory
				closedir(path); 
			// close the connection
				close(client_fd); 
			}
		}
	} 
	// checks if the requested file is a regular file
	else if (is_reg(filename)) {
		// sends the string to the client to identify file type
		send(client_fd,mime_type(filename),strlen(mime_type(filename)),0);
		// opens the file
		read_fd = open(filename,0,0);
		// while true
		while(1){
			// read the file		
			bytes_read = read(read_fd,send_buf,4096);
			// checks if file is valid, if not - break and close file and connection
			if(bytes_read == 0)
				break;
			// if file is valid, send the requested file
			send(client_fd,send_buf,bytes_read,0);
		}
		close(read_fd); // close the file 
		close(client_fd); // close the connection
	}

	// display 404 page if file/dir doesn't exist
	else {
		// store the error page string
			char *error = "<!DOCTYPE html>"
				"<html lang=\"en\">"
				"<head>"
				"<meta charset=\"utf-8\">"
				"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
				"<meta name=\"description\" content=\"404 Not Found\">"
				"<meta name=\"author\" content=\"\">"
				"<title>Page Not Found :(</title>"
				"<!-- Bootstrap core CSS -->"
				"<link rel=\"stylesheet\" href=\"//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css\">"
				"<link rel=\"stylesheet\" href=\"//maxcdn.bootstrapcdn.com/font-awesome/4.3.0/css/font-awesome.min.css\">"
				"<style>"
				"/* Error Page Inline Styles */"
				"body {"
				"  padding-top: 20px;"
				"  background-color: #ebefe8;"
				"  padding: 0px;"
				"  margin: 0px;"
				"}"
				"/* Layout */"
				".jumbotron {"
				"  font-size: 21px;"
				"  font-weight: 300;"
				"  line-height: 2.1428571435;"
				"  color: #325da3;"
				" padding: 10px 0px;"
				"}"
				"/* Everything but the jumbotron gets side spacing for mobile-first views */"
				".masthead, .body-content, {"
				"  padding-left: 15px;"
				"  padding-right: 15px;"
				"}"
				"/* Main marketing message and sign up button */"
				".jumbotron {"
				"  text-align: center;"
				"  background-color: transparent;"
				"}"
				".jumbotron .btn {"
				"  font-size: 21px;"
				"  padding: 14px 24px;"
				"}"
				"/*Align*/"
				".center {"
				"    text-align: center;"
				"}"
				"/* Colors */"
				".green {color:#408e08;}"
				".orange {color:#f0ad4e;}"
				".red {color:#bc3232;}"
				"/*.custom1 {color:#;}*/"
				"</style>"
				"<script type=\"text/javascript\">"
				"  function loadDomain() {"
				"    var display = document.getElementById(\"display-domain\");"
				"    display.innerHTML = document.domain;"
				"  }"
				" "
				"function myFunction() {"
				"    window.open(\"https://slither.io\");"
				"}"
				"</script>"
				"</head>"
				"<body onload=\"javascript:loadDomain();\">"
				"<!-- Error Page Content -->"
				"<div class=\"container\">"
				"  <!-- Jumbotron -->"
				"  <div class=\"jumbotron\">"
				"    <h1><i class=\"fa fa-puzzle-piece red\"></i> Error 404: Page Not Found <i class=\"fa fa-puzzle-piece red\"></i></h1>"
				"    <p class=\"lead\">We couldn't find what you're looking for.</p>"
				"    <p><a onclick=myFunction(); class=\"btn btn-default btn-lg\"><span class=\"green\">Click me for fun!</span></a>"
				"        <script type=\"text/javascript\">"
				"            function checkSite(){"
				"              var currentSite = window.location.hostname;"
				"                window.location = \"http://\" + currentSite;"
				"            }"
				"        </script>"
				"    </p>"
				"  </div>"
				"</div>"
				"<div class=\"container\">"
				"  <div class=\"body-content\">"
				"    <div class=\"row\">"
				"      <div class=\"center\">"
				"        <h2>What happened?</h2>"
				"        <p class=\"lead\">A 404 error status implies that the file or page that you're looking for could not be found.</p>"
				"      </div>"
				"    </div>"
				"  </div>"
				"</div>"
				"<!-- End Error Page Content -->"
				"<!--Scripts-->"
				"<!-- jQuery library -->"
				"<script src=\"//ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js\"></script>"
				"<script src=\"//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js\"></script>"
				"</body>"
				"</html>";

		// specify the request and context type = text/html since that's the 404 page	
		char * request_strREQ404 = "HTTP/1.0 404 BAD\r\n"
		"Context-type: text/html; charset=UTF-8\r\n\r\n";
		// sends the client the 404 to let client know that its a bad request
		send(client_fd,request_strREQ404,strlen(request_strREQ404),0);
		// sends the actual error string
		send(client_fd,error,strlen(error),0);
		// close the connection
		close(client_fd);  
	}
}


void *thread(void *varg) {
	serve_request(*(int *)varg);
}

int main(int argc, char** argv) {
    /* For checking return values. */
    int retval;

    chdir(argv[2]);

    /* Read the port number from the first command line argument. */
    int port = atoi(argv[1]);

    /* Create a socket to which clients will connect. */
    int server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if(server_sock < 0) {
        perror("Creating socket failed");
        exit(1);
    }

    /* A server socket is bound to a port, which it will listen on for incoming
     * connections.  By default, when a bound socket is closed, the OS waits a
     * couple of minutes before allowing the port to be re-used.  This is
     * inconvenient when you're developing an application, since it means that
     * you have to wait a minute or two after you run to try things again, so
     * we can disable the wait time by setting a socket option called
     * SO_REUSEADDR, which tells the OS that we want to be able to immediately
     * re-bind to that same port. See the socket(7) man page ("man 7 socket")
     * and setsockopt(2) pages for more details about socket options. */
    int reuse_true = 1;
    retval = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_true,
                        sizeof(reuse_true));
    if (retval < 0) {
        perror("Setting socket option failed");
        exit(1);
    }

    /* Create an address structure.  This is very similar to what we saw on the
     * client side, only this time, we're not telling the OS where to connect,
     * we're telling it to bind to a particular address and port to receive
     * incoming connections.  Like the client side, we must use htons() to put
     * the port number in network byte order.  When specifying the IP address,
     * we use a special constant, INADDR_ANY, which tells the OS to bind to all
     * of the system's addresses.  If your machine has multiple network
     * interfaces, and you only wanted to accept connections from one of them,
     * you could supply the address of the interface you wanted to use here. */
    
   
    struct sockaddr_in6 addr;   // internet socket address data structure
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port); // byte order is significant
    addr.sin6_addr = in6addr_any; // listen to all interfaces

    
    /* As its name implies, this system call asks the OS to bind the socket to
     * address and port specified above. */
    retval = bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    if(retval < 0) {
        perror("Error binding to port");
        exit(1);
    }

    /* Now that we've bound to an address and port, we tell the OS that we're
     * ready to start listening for client connections.  This effectively
     * activates the server socket.  BACKLOG (#defined above) tells the OS how
     * much space to reserve for incoming connections that have not yet been
     * accepted. */
    retval = listen(server_sock, BACKLOG);
    if(retval < 0) {
        perror("Error listening for connections");
        exit(1);
    }

    while(1) {
        /* Declare a socket for the client connection. */
        int sock;
        char buffer[256];

        /* Another address structure.  This time, the system will automatically
         * fill it in, when we accept a connection, to tell us where the
         * connection came from. */
        struct sockaddr_in remote_addr;
        unsigned int socklen = sizeof(remote_addr); 

        /* Accept the first waiting connection from the server socket and
         * populate the address information.  The result (sock) is a socket
         * descriptor for the conversation with the newly connected client.  If
         * there are no pending connections in the back log, this function will
         * block indefinitely while waiting for a client connection to be made.
         * */
        sock = accept(server_sock, (struct sockaddr*) &remote_addr, &socklen);
        if(sock < 0) {
            perror("Error accepting connection");
            exit(1);
        }

        pthread_t tid;
        pthread_create(&tid, NULL, thread, &sock);
        pthread_detach(tid);

        /* At this point, you have a connected socket (named sock) that you can
         * use to send() and recv(). */

        /* ALWAYS check the return value of send().  Also, don't hardcode
         * values.  This is just an example.  Do as I say, not as I do, etc. */
        serve_request(sock);

        /* Tell the OS to clean up the resources associated with that client
         * connection, now that we're done with it. */
        close(sock);
    }
    close(server_sock);
}

