# ********************** Makefile *******************************************
# Author        :      Chiranjeev Ghosh (chiranjeev.ghosh@tamu.edu)
# Organization  :      Texas A&M University, CS for ECEN 602 Assignment 4
# Description   :      Compiles Client & Proxy Server source code
# Last_Modified :      11/27/2017


# 'make all' for compiling all code in package
all: proxy client

# 'make server' for compiling Server.c
proxy: Proxy.c
	gcc -I . -pthread Proxy.c -o proxy

# 'make client' for compiling Client.c
client: Client.c
	gcc -I . Client.c -o client

# 'make clean' for discarding all previously created object files
clean:
	$(RM) client server
	
	
	





