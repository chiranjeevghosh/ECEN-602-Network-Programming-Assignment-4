# ECEN-602-Network-Programming-Assignment 4: 

This project implements a simple client - (proxy) server system wherein the client issues HTTP requests to access data across the network. The proxy server understands the requests from the client and "proxies" them to the appropriate web servers using HTTP. The request is subsequently cached for future reference. In this implementation, the proxy server can cache up to 10 items (This can be changed by parameter "MAX_CACHE_ENTRY" in the utils.h file).

Package content:

1. Proxy.c
2. Client.c
3. utils.h
4. Makefile

Usage:

1. 'make clean' to remove all previously created object files.
2. 'make all' to compile the complete source code in the package. 'make proxy' OR 'make client' if you want to compile individual files.
3. ./proxy *Proxy Server IP* *Port*
4. ./client *Proxy Server IP* *Port* *URL*
  
Tests:

1. All LRU (Least Recently Used) based cache manipulation experiments tested OK. LRU entry is popped from the cache if maximum capacity of cache is reached.
2. All uncached (new) entity access from client tested OK.
3. Access to cached entries tested OK.
4. All 'conditional GET' experiments tested OK.
5. Data received in client dumped into file with hostname. The HTML content was opened with Firefox and expected data could be viewed.
