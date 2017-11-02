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
#include <netdb.h>
#include <sys/poll.h>
#include <mcrypt.h>
#include <termios.h>
#include <sys/socket.h>
#include <errno.h> 
#include <sys/wait.h>
#include <signal.h>

int key_len;
int port = 0;
int err;
int client_fd;
char* log_file = NULL;
char* key_file = NULL;
int log_file_fd, key_file_fd;
struct pollfd pd[2];
MCRYPT crypt_fd, decrypt_fd;
struct termios saved_terminal_state;

void process();
void handle_IO();
void set_input_mode();
char* get_key();
void encrypt(char* msg, int len);
void decrypt(char* msg, int len);
void init_mcrypt(char * key, int key_len);
void reset_input_mode(){ tcsetattr (0 ,TCSANOW, &saved_terminal_state);}


int main(int argc, char** argv){
  int opt = -1; 

  static struct option options[] = {
    {"port", required_argument, 0, 'p'},
    {"log", required_argument, 0, 'l'},
    {"encrypt", required_argument, 0, 'e'}
  };
  // If there is CLI provided, loop through it
  while((opt = getopt_long(argc, argv, "p:l:e", options, NULL)) != -1){
    switch (opt) {
      // In the case of --shell input, add signal handlers 
    case 'p':
      port = atoi(optarg);
      //signal(SIGINT, signal_handler);
      break;
    case 'l':
      log_file_fd = creat(optarg,  S_IRWXU);
      if (log_file_fd < 0){
         err = errno;
        fprintf(stderr, "Error creating file: %s\n", strerror(err));
        exit(1);
      }
      log_file = optarg;
      break;
    case 'e':
      key_file = optarg;
      char* key = get_key();
      init_mcrypt(key, key_len);
      break;
    default: 
      printf("USAGE: ./lab1b-client --port=PORTNUM --encrypt=KEYFILE --log=LOGFILE");
      exit(1);
    }
  }
  // Set input mode after getopt() to non-canonical
  set_input_mode();
  process();
}
void process(){
  // Make connection to server on localhost
  struct hostent * server;
  struct sockaddr_in servaddr;
  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd < 0){
    perror("Socket Error");
    exit(1);
  }
  server = gethostbyname("localhost");
  servaddr.sin_family = AF_INET;
  memcpy((char *) &servaddr.sin_addr.s_addr,
   (char*) server->h_addr,
   server->h_length);
  servaddr.sin_port = htons(port);
  if(connect(client_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
  {
    fprintf(stderr, "Error connecting to server.\n");
    exit(1);
  }
  // Poll data fro the server or keyboard 
  handle_IO();

}
void handle_IO(){
  int bytes_proc;
  char buffer[256];
  int buffer_size = 256;
  char rec[18] = "RECEIVED x bytes: ";
  char sent[14] = "SENT x bytes: ";
  pd[0].fd = client_fd;
  pd[0].events = POLLIN | POLLHUP | POLLERR;
  pd[1].fd = STDIN_FILENO;
  pd[1].events = POLLIN | POLLHUP | POLLERR;
  // Keep polling until server exits or client does
  while(1){
    poll(pd, 2, 0);
    // Input from the server
    if (pd[0].revents && POLLIN){
      bytes_proc = read(client_fd, &buffer, buffer_size);
      if (bytes_proc <= 0)
       break;
      rec[9] = '1';
      for (int i = 0; i < bytes_proc; i++){
        // If logging was chosen, write received data, 
        // encrypted or not, to the log file
        if (log_file){
          write(log_file_fd, &rec, 18);
          write(log_file_fd, &buffer[i], 1);
          write(log_file_fd, "\n", 1);
        }
        if (key_file)
          decrypt(&buffer[i], 1);
        // Map <cr> or <lf> --> <cr><lf>
        if (buffer[i] == '\n' || buffer[i] == '\r'){
          char cr_lf[2] = {'\r', '\n'};
          write(1, &cr_lf, 2);
          continue;
        }
        // Write to STDOUT as client types
        write(1, &buffer[i], 1);
      }
    }
    // Input from the keyboard
    if (pd[1].revents && POLLIN){
      bytes_proc = read(STDIN_FILENO, &buffer, buffer_size);
      for (int i = 0; i < bytes_proc; i++){
        // if ^C or ^D, simply pass it to the server
        if (buffer[i] == 0x03 || buffer[i] == 0x04){
          write(client_fd, &buffer[i], 1);
          continue;
        }
        if (key_file){
          encrypt(&buffer[i], 1);
        }
        // Map <cr> or <lf> --> <cr><lf>
        if (buffer[i] == '\r' || buffer[i] == '\n'){
          char cr_lf[2] = {'\r', '\n'};
          write(1, &cr_lf, 2);
          write(client_fd, &cr_lf, 2);
          continue;
        }
        // If client chose logging, log client input
        if(log_file){
          sent[5] = '1';
          write(log_file_fd, &sent, 14);
          write(log_file_fd, &buffer[i], 1);
          write(log_file_fd, "\n", 1);
        }
        // Output input
        write(1, &buffer[i], 1);

        // Write to the server
        write(client_fd, &buffer[i], 1);
      }
    }
  }
}
// Returns the encryption key from the specified file
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

void set_input_mode(){
  struct termios config_terminal_state;
  tcgetattr(STDIN_FILENO, &saved_terminal_state);
  atexit(reset_input_mode);
  tcgetattr(STDIN_FILENO, &config_terminal_state);
  config_terminal_state.c_iflag = ISTRIP;
  config_terminal_state.c_oflag = 0;
  config_terminal_state.c_lflag = 0;
  config_terminal_state.c_cc[VMIN] = 1;
  config_terminal_state.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &config_terminal_state);
}
