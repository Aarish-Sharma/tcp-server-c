# Multi-Client TCP Server in C

A concurrent TCP server written in C from scratch, demonstrating core Linux systems programming concepts including socket programming, process forking, signal handling, and file descriptor management.

## How it works

When a client connects, the server forks a child process to handle that client exclusively while the parent immediately goes back to accepting new connections. This allows multiple clients to be served simultaneously without blocking.
```
Client 1 ──┐
Client 2 ──┤──► server_fd (accept loop)
Client 3 ──┘         │
                      ├── fork() ──► Child 1 handles Client 1
                      ├── fork() ──► Child 2 handles Client 2
                      └── fork() ──► Child 3 handles Client 3
```

## Concepts demonstrated

- **BSD Sockets API** — `socket()`, `bind()`, `listen()`, `accept()`, `read()`, `write()`
- **Process lifecycle** — `fork()`, parent/child separation, `exit()`
- **Signal handling** — `SIGCHLD` with `SIG_IGN` to reap zombie processes automatically
- **File descriptor management** — closing unused fd's in parent and child after fork
- **Connection logging** — timestamped logs persisted to `/tmp/tcp-server.log`
- **Socket options** — `SO_REUSEADDR` to avoid "Address already in use" on restart

## Why file descriptors matter after fork()

After `fork()`, both parent and child inherit copies of all open file descriptors. If the parent doesn't close `client_fd` and the child doesn't close `server_fd`:

- The socket never fully closes — client hangs waiting for EOF
- File descriptors leak on every connection — server eventually crashes

## Build and run
```bash
gcc server.c -o server
./server
```

## Test with multiple simultaneous clients

Open 3 terminals and run simultaneously:
```bash
echo "Hello from terminal 1" | nc localhost 8080
echo "Hello from terminal 2" | nc localhost 8080
echo "Hello from terminal 3" | nc localhost 8080
```

## Check the logs
```bash
cat /tmp/tcp-server.log
```

## Environment

- Ubuntu 22.04
- GCC 13.3.0
