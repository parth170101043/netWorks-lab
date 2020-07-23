First run the server cod in terminal .
$gcc server.c -o server
$./server 8080
(8080 is port number.)
then input a error probability between 0 and 1
Now the server is running....
now run the client code in another terminal
$gcc client.c -o client
$./client 127.0.0.1 8080

then input a error probability between 0 and 1
Now the client is running....
now start sending message.
(in server code server ip is set to 127.0.0.1,it can be changed there.Then "./client 127.0.0.1 8080"
should also be changed to  "$./client new serverip 8080"
