# Distributed File Transfer

This repository contains the first project for the INE5418 - Distributed Computing course at the Federal University of Santa Catarina (UFSC).

### What does this project do?
This project is a system for uploading files to a server, which stores them. It follows a client-server architecture: the client sends files directly to the server through a command line executable.

The system was written in C and uses Berkeley Sockets over UDP. Since UDP doesn't guarantee packet delivery, the application uses the Stop-and-Wait protocol to handle lost packets.

There is also a web server, written in Python with FastAPI, that shows which files are available on the server and which are still being transferred.

## How to configure the application?

First, set up the environment to run the application. Create a Python virtual environment and install the required dependencies:

```console
user@pc: python3 -m venv .venv  # Creates the virtual environment
user@pc: source .venv/bin/activate  # Activates the virtual environment
user@pc: pip install -r requirements.txt  # Installs the required dependencies
```

Next, compile the client and the server with make:
```console
user@pc: make all  # Compiles the client and the server code
```

This creates two executables in the project root:
- server
- client

## How to run the server and the client?

### Local
To start the server, run:
```console
user@pc: ./server
```
The server will use the default port (6000) and store the received files in the default directory (root-project/results).

To send a file to the server, run the client:
```console
user@pc: ./client filename.extension
```
The client will send "filename.extension" to the server at localhost (127.0.0.1), using the default port (6000).

To use a different port, pass it as an argument:
```console
user@pc: ./server port
```
To save the files in a different directory, pass the directory name:
```console
user@pc: ./server directory
```
You can also pass both:
```console
user@pc: ./server port directory
```

If you change the server port, remember to pass the same port to the client:
```console
user@pc: ./client port filename.extension
```
Otherwise, the client won't be able to reach the server.

### LAN
Running on a LAN is almost the same. The only difference is that the client needs the IP address of the server machine instead of 127.0.0.1.

On the server machine, start the server as usual and find out its IP address:
```console
user@server: ./server
user@server: hostname -I  # Shows the IP addresses of this machine (e.g. 192.168.0.10)
```

On the client machine, pass the server IP before the file name:
```console
user@client: ./client 192.168.0.10 filename.extension
```
This sends the file to the server at 192.168.0.10 on the default port (6000).

If the server uses a different port, pass the IP first and then the port:
```console
user@client: ./client 192.168.0.10 port filename.extension
```

### How to run the web server?
The web server reads `status_file.txt`, which the server creates in the directory it was started from. So run the web server from that same directory (the project root, if you followed the steps above) with the virtual environment activated:
```console
user@pc: source .venv/bin/activate  # Activates the virtual environment, if it isn't already
user@pc: fastapi run web_server.py  # Starts the web server on port 8000
```

Then open http://localhost:8000/files in the browser. It returns a JSON listing every file sent to the server and its current state:
```json
{
  "example1.txt": "Disponível",
  "example2.bin": "Em transferência"
}
```
- **Disponível**: the file was fully received and can be used.
- **Em transferência**: the file is still being sent, or the transfer stopped halfway.

If no file has been sent yet, the endpoint returns an empty JSON (`{}`).

`fastapi run` listens on all network interfaces, so other machines on the LAN can also check the files at `http://<server-ip>:8000/files`. To use another port, pass the `--port` option:
```console
user@pc: fastapi run web_server.py --port 8080
```
FastAPI also generates a documentation page automatically, available at `http://localhost:<port>/docs`.
