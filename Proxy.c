// ************************************ Server Code ****************************************************
// Authors       :      Chiranjeev Ghosh (chiranjeev.ghosh@tamu.edu) 
// Organisation  :      Texas A&M University, CS for ECEN 602 Assignment 4
// Description   :      Implementation of a simple TCP HTTP Proxy Server. Establishes an IPv4/IPv6 socket
//                      connection with a client and caches incoming requests or forwards them to the 
//                      appropriate web server, eventually forwarding the requested HTTP content to the 
//                      client. Multiple clients can be supported.
// Last_Modified :      11/27/2017


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>
#include <time.h>
#include <utils.h>





struct Cache {
  char URL[256];
  char Last_Modified[50];
  char Access_Date[50]; 
  char Expires[50];
  char *body;
};

static const struct Cache Clear_Entry;
int num_cache_entries = 0;


struct Cache Proxy_Cache[MAX_CACHE_ENTRY];


int Update_Cache(char *URL, char *buf, int flag, int x) {

  int j=0;
  int p=0;

  if (flag == 1) {                    // New entry
    if (num_cache_entries==MAX_CACHE_ENTRY){
      Proxy_Cache[0] = Clear_Entry;     // Popping LRU
      for (j=0; j<MAX_CACHE_ENTRY; j++){
        if (j+1!=MAX_CACHE_ENTRY)
          Proxy_Cache[j] = Proxy_Cache[j+1];
        else {
          // Add new entry at the head (latest)
          memset(&Proxy_Cache[j], 0, sizeof(struct Cache));
          memcpy(Proxy_Cache[j].URL,URL,256);
	  Proxy_Cache[j].body = (char *) malloc(strlen(buf));
          memcpy(Proxy_Cache[j].body,buf,strlen(buf));
          parseHDR("Expires:", buf, Proxy_Cache[j].Expires);
	  parseHDR("Last-Modified:", buf, Proxy_Cache[j].Last_Modified);
	  parseHDR("Date:", buf, Proxy_Cache[j].Access_Date);
        }
      }
    }
    else {              // If cache has not reached max allowed capacity (MAX_CACHE_ENTRY)
      Proxy_Cache[num_cache_entries] = Clear_Entry;     
      memcpy(Proxy_Cache[num_cache_entries].URL,URL,256);
      parseHDR("Expires:", buf, Proxy_Cache[num_cache_entries].Expires);
      parseHDR("Last-Modified:", buf, Proxy_Cache[num_cache_entries].Last_Modified);
      parseHDR("Date:", buf, Proxy_Cache[num_cache_entries].Access_Date);
      Proxy_Cache[num_cache_entries].body = (char *) malloc(strlen(buf));
      memcpy(Proxy_Cache[num_cache_entries].body,buf,strlen(buf));
      num_cache_entries++;
    }
  }
  else {                              // Existing entry
    struct Cache tmp;
    memset(&tmp, 0, sizeof(struct Cache));
    tmp = Proxy_Cache[x];
    for (j=x; j<num_cache_entries; j++){
      if (j==num_cache_entries-1)
	break;
      Proxy_Cache[j] = Proxy_Cache[j+1];
    }
    Proxy_Cache[num_cache_entries -1] = tmp;               
    struct tm tmp_t;
    time_t nw = time(NULL);
    tmp_t = *gmtime(&nw);
    const char* op_tmp = "%a, %d %b %Y %H:%M:%S GMT";
    strftime (Proxy_Cache[num_cache_entries - 1].Access_Date, 50, op_tmp, &tmp_t);
  }
}

int Cache_Display () {
  int t = 0;
  if (num_cache_entries == 0)
    printf("Cache is unoccupied currently\n");
  else {
    printf("Cache count: %d\n", num_cache_entries);
    for (t=0; t<num_cache_entries; t++) {
      if (strcmp(Proxy_Cache[t].Expires, "") != 0 && strcmp(Proxy_Cache[t].Last_Modified, "") != 0) 
        printf("Index: %d  |  URL: %s  |  Access Date: %s  |  Expires: %s  |  Last_Modified: %s\n\n", t, Proxy_Cache[t].URL, Proxy_Cache[t].Access_Date, Proxy_Cache[t].Expires, Proxy_Cache[t].Last_Modified);
      else if (strcmp(Proxy_Cache[t].Expires, "") == 0 && strcmp(Proxy_Cache[t].Last_Modified, "") == 0) 
        printf("Index: %d  |  URL: %s  |  Access Date: %s  |  Expires: N/A  |  Last_Modified: N/A\n\n", t, Proxy_Cache[t].URL, Proxy_Cache[t].Access_Date);
      else if (strcmp(Proxy_Cache[t].Expires, "") == 0)
        printf("Index: %d  |  URL: %s  |  Access Date: %s  |  Expires: N/A  |  Last_Modified: %s\n\n", t, Proxy_Cache[t].URL, Proxy_Cache[t].Access_Date, Proxy_Cache[t].Last_Modified);
      else if (strcmp(Proxy_Cache[t].Last_Modified, "") == 0) 
        printf("Index: %d  |  URL: %s  |  Access Date: %s  |  Expires: %s  |  Last_Modified: N/A\n\n", t, Proxy_Cache[t].URL, Proxy_Cache[t].Access_Date, Proxy_Cache[t].Expires);
    }
  }
  return 0;
}


int Fresh (int cache_ptr) {         
  struct tm tmp_t;
  time_t nw  = time(NULL);	
  tmp_t = *gmtime(&nw);
  struct tm EXPIRES; 
  if (strcmp(Proxy_Cache[cache_ptr].Expires, "") != 0) {
    strptime(Proxy_Cache[cache_ptr].Expires, "%a, %d %b %Y %H:%M:%S %Z", &EXPIRES); 
    time_t EXP = mktime(&EXPIRES);	
    time_t NOW = mktime(&tmp_t);	
    if (difftime (NOW, EXP) < 0)
      return 1;
    else
      return -1;
  }  
  else
    return -1;
}

int Cache_Element(char *URL) {
  int b=0;
  for (b=0; b<MAX_CACHE_ENTRY; b++) {
    if (strcmp(Proxy_Cache[b].URL, URL)==0) {
      return b;
    }
  }
  return -1;
}

int WebS_Socket (char *host) {
  
  struct addrinfo dynamic_addr, *ai, *p;
  int ret_val = 0;
  int webs_sockfd = 0;


  memset(&dynamic_addr, 0, sizeof dynamic_addr);
  dynamic_addr.ai_family = AF_INET;
  dynamic_addr.ai_socktype = SOCK_STREAM;

  if ((ret_val = getaddrinfo(host, "http", &dynamic_addr, &ai)) != 0) {
    fprintf(stderr, "SERVER: %s\n", gai_strerror(ret_val));
    exit(1);
  }
  for(p = ai; p != NULL; p = p->ai_next) {
    webs_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (webs_sockfd >= 0 && (connect(webs_sockfd, p->ai_addr, p->ai_addrlen) >= 0)) 
      break;
  }

  if (p == NULL)
    webs_sockfd = -1;
  
  freeaddrinfo(ai);
  return webs_sockfd;
  
}


int Proxy_Server(int client_fd) {
  int webs_sockfd;
  char *msg;
  char forward_client_msg[MAX_LEN] = {0};
  int ret;
  int cache_el = 0;
  //char resp[1024] = {0};
  char *resp = NULL;
  //char to_client[10240] = {0};
  char *to_client = NULL;
  //string Method;
  //string Protocol;
  char path[256];
  char hostname[64];
  int port = 80;
  char URL[256] = {0};
  char Method[8] = {0};
  char Protocol[16] = {0};
  char cond_msg[256] = {0};
  char url_parse[256] = {0};
  int check = 0;


  //memset(&url, 0, sizeof url);
  msg = (char *) malloc (MAX_LEN);
  ret = read(client_fd, msg, MAX_LEN);
  printf("SERVER: Request retrieved from client: \n%s", msg);
  if (ret < 0) 
    err_sys ("SERVER: Error in extracting message request from client");
  sscanf(msg, "%s %s %s", Method, URL, Protocol);
  //free (msg);
  //printf("SERVER: URL extracted: %s\n", URL);
  if ((cache_el = Cache_Element (URL)) != -1 && (Fresh (cache_el) == 1)) {             
    //printf("Cache_el: %d\n", cache_el);
    printf ("SERVER: Requested URL: %s is in cache and is fresh\n", URL);
    Update_Cache(URL, NULL, 0, cache_el);
    to_client = (char *) malloc(strlen(Proxy_Cache[cache_el].body));
    memcpy(to_client, Proxy_Cache[cache_el].body, strlen(Proxy_Cache[cache_el].body)); 
  }
  else {        // Either URL is not cached or it is stale
    memset(hostname, 0, 64);  
    memset(path, 0, 256);  
    memcpy(&url_parse[0], &URL[0], 256);
    parse_URL (url_parse, hostname, &port, path);
    if ((webs_sockfd = WebS_Socket (hostname)) == -1)
      err_sys ("SERVER: Error in connecting with web server");

    printf ("SERVER: Successfully connected to web server %d\n", webs_sockfd);
    if (cache_el != -1) {                        // If cache entry exists but has expired
      printf ("SERVER: Requested URL: %s is in cache but is expired\n", URL);
      //split_URL (URL, split_url);
      if (strcmp(Proxy_Cache[cache_el].Expires, "") != 0 && strcmp(Proxy_Cache[cache_el].Last_Modified, "") != 0) 
        snprintf(cond_msg, MAX_LEN, "%s %s %s\r\nHost: %s\r\nUser-Agent: HTTPTool/1.0\r\nIf-Modified_Since: %s\r\n\r\n", Method, path, Protocol, hostname, Proxy_Cache[cache_el].Expires);
      else if (strcmp(Proxy_Cache[cache_el].Expires, "") == 0 && strcmp(Proxy_Cache[cache_el].Last_Modified, "") == 0) 
        snprintf(cond_msg, MAX_LEN, "%s %s %s\r\nHost: %s\r\nUser-Agent: HTTPTool/1.0\r\nIf-Modified_Since: %s\r\n\r\n", Method, path, Protocol, hostname, Proxy_Cache[cache_el].Access_Date);
      else if (strcmp(Proxy_Cache[cache_el].Expires, "") == 0)
        snprintf(cond_msg, MAX_LEN, "%s %s %s\r\nHost: %s\r\nUser-Agent: HTTPTool/1.0\r\nIf-Modified_Since: %s\r\n\r\n", Method, path, Protocol, hostname, Proxy_Cache[cache_el].Last_Modified);
      else if (strcmp(Proxy_Cache[cache_el].Last_Modified, "") == 0) 
        snprintf(cond_msg, MAX_LEN, "%s %s %s\r\nHost: %s\r\nUser-Agent: HTTPTool/1.0\r\nIf-Modified_Since: %s\r\n\r\n", Method, path, Protocol, hostname, Proxy_Cache[cache_el].Expires);
      printf("Conditional GET Generated: \n%s", cond_msg);
      write(webs_sockfd, cond_msg, MAX_LEN); 
      //resp = malloc (10240);    // FIXME: May be needed to increase allocation
      //memset(resp, 0, 1024);  
      resp = (char *) malloc (100000);
      check = Extract_Read(webs_sockfd, resp);
      //printf("Checking: %d\n", check);
      //Extract_Read(webs_sockfd, resp);
      to_client = (char *) malloc(strlen(resp));
      if (strstr(resp, "304 Not Modified") != NULL) {
        printf("'304 Not Modified' received. Sending file in cache\n");
        memcpy(to_client, Proxy_Cache[cache_el].body, strlen(Proxy_Cache[cache_el].body)); 
	Update_Cache(URL, NULL, 0, cache_el);
      }
      else {
        printf("SERVER: File was modified\n");
        memcpy(to_client, resp, strlen(resp)); 
        Update_Cache(URL, NULL, 0, cache_el);         // move to head (LRU) of the queue
        Proxy_Cache[--num_cache_entries] = Clear_Entry;                 // Popping LRU
        Update_Cache(URL, resp, 1, 0);     // treat like a new entry as it was modified
      }
    }
    else {             // document is not cached
      printf ("SERVER: Requested URL is not in cache\n");
      memset(forward_client_msg, 0, MAX_LEN);  
      snprintf(forward_client_msg, MAX_LEN, "%s %s %s\r\nHost: %s\r\nUser-Agent: HTTPTool/1.0\r\n\r\n", Method, path, Protocol, hostname);
      printf("SERVER: Request generated: \n%s", forward_client_msg);
      write(webs_sockfd, forward_client_msg, MAX_LEN); 
      resp = (char *) malloc (100000);
      check = Extract_Read(webs_sockfd, resp);
      to_client = (char *) malloc(strlen(resp));
      memcpy(to_client, resp, strlen(resp)); 
      Update_Cache(URL, resp, 1, 0);
    }
  }
  Cache_Display();
  write(client_fd, to_client, strlen(to_client) + 1); 
}




int Extract_Read(int fd, char *msg) {            // Extracts message body from read socket 
  int total = 0;
  char buffer[MAX_LEN] = {0};
  int cnt = 1;
  int h;
  while(cnt>0) {
    memset(buffer, 0, sizeof(buffer));
    cnt = read(fd, buffer, MAX_LEN);
    if (cnt == 0) break;
    strcat(msg, buffer);
    total = total + cnt;
    if (buffer[cnt - 1] == EOF) {
      strncpy(msg,msg,(strlen(msg)-1));
      total--;
      break;
    }
  }
  return total;
}


int main (int argc, char *argv[])
{
  int sockfd, comm_fd, bind_fd, listen_fd;	
  int port_number ;
  struct sockaddr_storage remoteaddr; 
  socklen_t addrlen;
  
  if (argc != 3){
    err_sys ("USAGE: ./proxy <Server IP Address> <Port_Number>");
    return 0;
  }
  
  port_number = atoi(argv[2]);
  struct sockaddr_in servaddr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0)
    err_sys ("ERR: Socket Error");

  bzero( &servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
  servaddr.sin_port = htons(port_number);
  bind_fd = bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  if (bind_fd < 0)
    err_sys ("ERR: Bind Error");
  listen_fd = listen(sockfd, 10);
  if (listen_fd < 0)
    err_sys ("ERR: Listen Error");
  memset(Proxy_Cache,0,MAX_CACHE_ENTRY*sizeof(struct Cache));

  addrlen = sizeof remoteaddr;
  pthread_t x;
  printf("\nPROXY SERVER is online\n\n");
  while(1)
  {
    comm_fd = accept(sockfd, (struct sockaddr*)&remoteaddr,&addrlen);
    pthread_create(&x, NULL, (void *)(&Proxy_Server), (void *)(intptr_t)comm_fd);
  }
  
}
