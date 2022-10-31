# file_server_mp
multi-platform file server for Windows/Linux
 
## About
it is a simple multi-platform file server, it can send and receive single files between different platforms (Windows/Linux).
 
## Building
### Windows
```
cl.exe client.c -o client.exe
cl.exe server.c -o server.exe
```
### Linux
```
gcc.exe client.c -o client.out
gcc.exe server.c -o server.out
```
## Usage
### Server
```
server <port>
```
`port` - port for accepting a remote connections

### Client
```
client <ip> <port> [r/t] <from> <to>
```
`ip`   - remote server ip for connection  
`port` - remote server port for connection  
`r/t`  - file transfer direction (receive - receiving a file from a server/transmit - transmitting a file to a server)  
`from` - source directory from a server on receive or source directory from a client on transmit  
`to`   - destination directory for a client on receive or destination directory for a server on transmit  

`from` and `to` paths are relative by default
