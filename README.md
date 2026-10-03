# Distributed File Transfer
Authors: Rodrigo Schwartz (R0drigoSchwartz) and Vinicius Henrique Ribeiro (vini-ribeiro)

This repository contains the first project for the INE5418 - Distributed Computing course at the Federal University of Santa Catarina (UFSC).

## What does this project do?
This project is a system for uploading files to a server, which stores them. It follows a client-server architecture: the client sends files directly to the server through a command line executable.

The system was written in C and uses Berkeley Sockets over UDP. Since UDP doesn't guarantee packet delivery, the application uses the Stop-and-Wait protocol to handle lost packets.

There is also a web server, written in Python with FastAPI, that shows which files are available on the server and which are still being transferred.

## Requirements
- A Unix-like operating system (e.g. Linux or WSL)
- A C compiler (gcc) and make
- OpenSSL development headers (libcrypto)
- Python 3 with the venv module

On Ubuntu/Debian, you can install everything with:
```console
user@pc: sudo apt install build-essential libssl-dev python3 python3-venv
```

## How to configure the application?

First, use make to compile the client and the server, create the virtual environment and install the dependencies:
```console
user@pc: make all 
```

This creates two executables in `bin/`:
- `bin/server`
- `bin/client`

To compile only the C programs, run `make server client`.

After, activate the virtual environment:

```console
user@pc: source .venv/bin/activate  # Activates the virtual environment
```

## How to run the server and the client?

### Local
To start the server, run:
```console
user@pc: ./bin/server
```
Run these commands from the project root. The server uses port 6000 and automatically creates `results/` if it does not exist. You can pass a different destination directory; its parent directory must already exist.

To send a file to the server, run the client:
```console
user@pc: ./bin/client filename.extension
```
The client will send "filename.extension" to the server at localhost (127.0.0.1), using the default port (6000).

To use a different port, pass it as an argument:
```console
user@pc: ./bin/server port
```
To save the files in a different directory, pass the directory name:
```console
user@pc: ./bin/server directory
```
You can also pass both:
```console
user@pc: ./bin/server port directory
```

If you change the server port, remember to pass the same port to the client:
```console
user@pc: ./bin/client port filename.extension
```
Otherwise, the client won't be able to reach the server.

### LAN
Running on a LAN is almost the same. The only difference is that the client needs the IP address of the server machine instead of 127.0.0.1.

On the server machine, start the server as usual and find out its IP address:
```console
user@server: ./bin/server
user@server: hostname -I  # Shows the IP addresses of this machine (e.g. 192.168.0.10)
```

On the client machine, pass the server IP before the file name:
```console
user@client: ./bin/client 192.168.0.10 filename.extension
```
This sends the file to the server at 192.168.0.10 on the default port (6000).

If the server uses a different port, pass the IP first and then the port:
```console
user@client: ./bin/client 192.168.0.10 port filename.extension
```

### How to run the web server?
The web server reads `status_file.txt`, which the server creates in the directory it was started from. So run the web server from that same directory (the project root, if you followed the steps above) with the virtual environment activated:
```console
user@pc: source .venv/bin/activate  # Activates the virtual environment, if it isn't already
user@pc: fastapi run web/web_server.py  # Starts the web server on port 8000
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
user@pc: fastapi run web/web_server.py --port 8080
```
FastAPI also generates a documentation page automatically, available at `http://localhost:<port>/docs`.

## Project structure

- `src/`: C sources and headers for the client, server, utilities and hashmap.
- `web/`: FastAPI server and Python requirements.
- `tests/`: automated integration tests.
- `scripts/`: the interactive transfer script, run with `./scripts/test.sh <file>`.
- `build/`: generated object files.
- `bin/`: generated executables.

`results/` and `status_file.txt` are runtime data, created when needed and ignored by Git. Custom destination paths are still accepted. Build output and Python environments are also ignored.

## How to run the tests?

From the project root:

```sh
make server client
python3 tests/test_server_hashmap.py
```
