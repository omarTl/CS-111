#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h> 
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <mcrypt.h>
#include <sys/socket.h>
#include <errno.h> 
#include <sys/poll.h>
#include <sys/wait.h>
#include <signal.h>


void process();
void _pipe(int fd[2]);
void handle_IO();
char* get_key();
void encrypt(char* plain, int crypt_len);
void signal_handler(int signal);
void decrypt(char* plain, int crypt_len);
void init_mcrypt(char* key, int key_len);
void exit_prog();

int socket_fd, cont_fd;
MCRYPT crypt_fd, decrypt_fd;
int fd_in[2], fd_out[2];
int err;
int status;
struct pollfd pd[2];
char* key_file = NULL;
int key_len;
int child_id;

int port = 0;
int main(int argc, char** argv){
  int opt = -1; 
  static struct option options[] = {
    {"port", required_argument, 0, 'p'},
    {"encrypt", required_argument, 0, 'e'}
  };
  // If there is CLI provided, loop through it
  while((opt = getopt_long(argc, argv, "p:e", options, NULL)) != -1){
    switch (opt) {
    case 'p':
      port = atoi(optarg);
      break;
    case 'e':
      key_file = optarg;
      char* key = get_key();
      init_mcrypt(key, key_len);
      break;
    default: 
      printf("USAGE: ./lab1b-sever --p=[PORT] --encrypt=KEYFILE\n");
      exit(1);
    }
  }
  // in case a SIGPIPE occurs, handle it gracefully
  signal(SIGPIPE, signal_handler);
  process();
  exit(0);
}

void process(){
  // Set up server connection to the specified port
  struct sockaddr_in server_addr, client_addr;
  socklen_t con_len;
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0){
    perror("Socket Error");
    exit(1);
  }
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);
  if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
    perror("Error binding socket");
    exit(1);
  } 
  listen(socket_fd, 4);
  con_len = sizeof(client_addr);
  cont_fd = accept(socket_fd, (struct sockaddr *) &client_addr, &con_len);
  if (cont_fd < 0){
    perror("Error while accepting socket");
    exit(1);
  }
  _pipe(fd_in);
  _pipe(fd_out);
  // fork --> exec
  pid_t proc_id = fork();
  if (proc_id < 0){
    perror("Error forking");
    exit(1);
  }
  // child process
  if (proc_id == 0){
    close(fd_in[1]);
    close(fd_out[0]);
    dup2(fd_in[0], 0);
    close(fd_in[0]);
    dup2(fd_out[1], 1);
    dup2(fd_out[1], 2);
    close(fd_out[1]);
    execlp("/bin/bash", "/bin/bash", NULL);
  }
  // parent process
  else {
    // Record child id
    child_id = proc_id;
    close(fd_in[0]);
    close(fd_out[1]);
    // Poll for input from client or shell
    handle_IO();
  }
  exit(0);
}


void _pipe(int fd[2]){
  if (pipe(fd) < 0){
    perror("Error piping");
    exit(1);
  }
}

void handle_IO(){
  int bytes_proc;
  char buffer[256];
  int buffer_size = 256;
  pd[0].fd = fd_out[0];
  pd[0].events = POLLIN | POLLHUP | POLLERR;
  pd[1].fd = cont_fd;
  pd[1].events = POLLIN | POLLHUP | POLLERR;
  while(1){
    poll(pd, 2, 0);
    // Output from shell
    if (pd[0].revents && POLLIN){
      bytes_proc = read(fd_out[0], &buffer, buffer_size);
      if (bytes_proc == 0){
        exit_prog();
      }
      // Process output from the shell and write back 
      // to the client
      for (int i = 0; i < bytes_proc; i++){
        if (key_file)
          encrypt(&buffer[i], 1);
        write(cont_fd, &buffer[i], 1);
      }
    }
    // Input from client
    if (pd[1].revents && POLLIN){
      bytes_proc = read(cont_fd, &buffer, buffer_size);
      for (int i = 0; i < bytes_proc; i++){
        if (key_file)
           decrypt(&buffer[i], 1);

        if (buffer[i] != '\r'){
          // If ^C received, kill program 
          if (buffer[i] == 0x03){
            kill(child_id, SIGINT);
            int status;
            waitpid(child_id, &status, 0);
            exit(0);
          }
          // If ^D, close write pipe to shell
          if (buffer[i] == 0x04){
            close(fd_in[1]);
            continue;
          }
          // Write client input to shell
          write(fd_in[1], &buffer[i], 1);
        }
      }
    }      
  }
}
// returns key from specified file
char *get_key(){
  struct stat key_stat;
  int key_fd = open(key_file, O_RDONLY);
  if(fstat(key_fd, &key_stat) < 0){ 
    err = errno;
    fprintf(stderr, "fstat error%s\n", strerror(err));
    exit(1);
  }
  char* key = (char*) malloc(key_stat.st_size * sizeof(char));
  if(read(key_fd, key, key_stat.st_size) < 0) { 
    err = errno;
    fprintf(stderr, "Read error%s\n", strerror(err));
    exit(1);
  }
  key_len = key_stat.st_size;

  return key;
}
void encrypt(char* plain,int crypt_len)
{
  if(mcrypt_generic(crypt_fd, plain, crypt_len) != 0) { 
    err = errno;
    fprintf(stderr, "Error Encrypting%s\n", strerror(err));
    exit(1);
  }
}

//Runs the corresponding decryption algorithm on the specified buffer for the size
void decrypt(char * enc_msg,int decrypt_len)
{
  if(mdecrypt_generic(decrypt_fd, enc_msg, decrypt_len) != 0) { 
    err = errno;
    fprintf(stderr, "Error Decrypting%s\n", strerror(err));
    exit(1);
  }
}
void init_mcrypt(char* key, int key_len){
  // Use cfb byte by bytes encryption
  crypt_fd = mcrypt_module_open("blowfish", NULL, "cfb", NULL);
  if(crypt_fd == MCRYPT_FAILED) { 
    err = errno;
    fprintf(stderr, "Error opening encryption module%s\n", strerror(err));
    exit(1);
  }
  if(mcrypt_generic_init(crypt_fd, key, key_len, NULL) < 0) { 
    err = errno;
    fprintf(stderr, "Error initializing encryption%s\n", strerror(err));
    exit(1);
  }

  decrypt_fd = mcrypt_module_open("blowfish", NULL, "cfb", NULL);
  if(decrypt_fd == MCRYPT_FAILED) { 
    err = errno;
    fprintf(stderr, "Error opening decryption module%s\n", strerror(err));
    exit(1);
  }
  if(mcrypt_generic_init(decrypt_fd, key, key_len, NULL) < 0) { 
    err = errno;
    fprintf(stderr, "Error initializing decryption%s\n", strerror(err));
    exit(1);
  }
}

void signal_handler(int signal){
  if (signal == SIGPIPE){
    exit_prog();
  }
}
// exits the program, collecting exit status
void exit_prog(){
  close(fd_out[0]);
  close(fd_in[0]);
  close(fd_out[1]);
  close(fd_in[1]);
  waitpid(child_id, &status, 0);
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d", WTERMSIG(status), WEXITSTATUS(status));
  exit(0);
}