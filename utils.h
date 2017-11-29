// ********************** Utilities Code ************************************
// Author        :      Chiranjeev Ghosh (chiranjeev.ghosh@tamu.edu)
// Organization  :      Texas A&M University, CS for ECEN 602 Assignment 4
// Description   :      Contains utility function definitions
// Last_Modified :      11/26/2017

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CACHE_ENTRY 10
#define MAX_LEN 1024


int parseHDR(const char* hdr, char* buf, char* op) {
  char *st = strstr(buf, hdr);
  if(!st) { 
    return 0;
  }
  char *end = strstr(st, "\r\n");
  st += strlen(hdr);
  while(*st == ' ') 
    ++st;
  while(*(end - 1) == ' ') 
    --end;
  strncpy(op, st, end - st);
  op[end - st] = '\0';
  return 1;

}

int parse_URL (char* URL, char *hostname, int *port, char *path) {
  char *token;
  char *host_temp, *path_temp;
  char *tmp1, *tmp2;
  int num = 0;
  char s[16];
  if (strstr(URL,"http") != NULL){
    token = strtok(URL, ":");
    tmp1 = token + 7;
  }
  else{
    tmp1 = URL;
  }
  tmp2 = malloc (64);
  memcpy(tmp2, tmp1, 64);
  if(strstr(tmp1, ":") != NULL){
    host_temp = strtok(tmp1, ":");
    *port = atoi(tmp1 + strlen(host_temp) + 1);
    sprintf(s, "%d", *port);
    path_temp = tmp1 + strlen(host_temp) + strlen(s) + 1;
  }
  else{
    host_temp = strtok(tmp1, "/");   
    *port = 80;
    path_temp = tmp2 + strlen(host_temp);
  }
  if (strcmp(path_temp, "") == 0)
    strcpy(path_temp, "/");
  memcpy(hostname, host_temp, 64);
  memcpy(path, path_temp, 256);
  return(0);
}




int err_sys(const char* x)    // Error display source code
{ 
  perror(x); 
  exit(1); 
}

