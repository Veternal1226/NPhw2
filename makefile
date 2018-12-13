all:	
	gcc -pthread -o hw2_server hw2_server.c 
	gcc -pthread -o hw2_client hw2_client.c 
clean:
	rm -f hw2_server
	rm -f hw2_client
