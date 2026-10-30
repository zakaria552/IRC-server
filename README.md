_This project has been created as part of the 42 curriculum by losypenk and zfarah._

# IRC-server

IRC-server is an Internet Relay Chat (IRC) server implemented in C++20 as part of the 42 School curriculum.

The project focuses on building a functional IRC server capable of handling multiple simultaneous client connections, allowing users to communicate through public channels and private messages.

Network communication is handled over TCP/IP using non-blocking sockets. The server relies on **I/O multiplexing with `poll()`**, where a single thread monitors multiple file descriptors for incoming data, outgoing data, and connection events. This event-driven approach allows the server to manage multiple clients concurrently without blocking on individual I/O operations or relying on multiple threads or processes.

## Demo
<img width="572" height="720" alt="ezgif-cropped" src="https://github.com/user-attachments/assets/d6dbc9df-f35f-4de7-bf9d-77187abe788f" />

## Features

- Non-blocking sockets and single-threaded I/O multiplexing with poll().
- Concurrent handling of multiple client connections.
- Queued message delivery with automatic retry handling for non-blocking socket operations.
- Password-based client authentication and user registration.
- Channel creation, joining, and leaving.
- Channel operators and regular users.
- Channel topics, passwords, invite-only mode, and user limits.
- Private messaging between users.

## Supported IRC Commands

| Command | Description |
|---|---|
| `PASS` | Authenticate with the server password. |
| `NICK` | Set or change your nickname. |
| `USER` | Register your username. |
| `JOIN` | Join or create a channel. |
| `PART` | Leave a channel. |
| `PRIVMSG` | Send a message to a user or channel. |
| `KICK` | Remove a user from a channel. |
| `INVITE` | Invite a user to a channel. |
| `TOPIC` | View or change a channel's topic. |
| `MODE` | View or modify channel modes. |
| `QUIT` | Disconnect from the server. |

### Channel Modes

Channel operators can manage channels using the following modes:

| Mode | Description |
|---|---|
| `+i` / `-i` | Enable / disable invite-only mode. |
| `+t` / `-t` | Restrict topic changes to operators. |
| `+k` / `-k` | Set / remove a channel password. |
| `+o` / `-o` | Give / remove operator privileges. |
| `+l` / `-l` | Set / remove the channel user limit. |

## Getting Started

### Requirements

- Unix-like operating system (Linux or macOS).
- C++ compiler with C++20 support.
- GNU Make.
- [Irssi](https://irssi.org/) IRC client.

### Compilation

Clone the repository and navigate into the project directory:

```bash
git clone <repository-url>
cd ft_irc
```

Compile the server:

```bash
make
```

This will generate the `ircserv` executable.

Available Makefile commands:

```bash
make        # Compile the project
make clean  # Remove object files
make fclean # Remove object files and executable
make re     # Recompile everything
```

## Starting the Server

The server takes two arguments:

```bash
./ircserv <port> <password>
```

- `port`: The TCP port on which the server listens for incoming connections.
- `password`: The password required by clients to connect.

For example:

```bash
./ircserv 6667 mypassword
```

The server is now listening for incoming connections on port `6667`.

## Connecting with Irssi

[Irssi](https://irssi.org/) is a terminal-based IRC client that can be used to connect to the server and test its functionality.

Open another terminal and start Irssi:

```bash
irssi
```

### 1. Connect to the server

```irc
/connect 127.0.0.1 6667 mypassword
```

This connects to the local IRC server using the specified port and password.

### 2. Set your nickname

```irc
/nick alice
```

### 3. Join a channel

```irc
/join #general
```

If the channel does not exist, it will be created automatically.

### 4. Send messages

Send a message to everyone in the current channel:

```irc
Hello everyone!
```

Send a private message to another user:

```irc
/msg bob Hey, how are you?
```

### 5. Manage a channel

Channel operators can use the following commands:

```irc
/topic #general Welcome to the channel!
/kick #general bob
/invite bob #general
/mode #general +i
/mode #general +k secret
/mode #general +o bob
/mode #general +l 10
```

### 6. Disconnect

```irc
/quit
```

## Resources

- [IRC specification](https://modern.ircdocs.horse/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [Sockets and Network Programming in C](https://www.codequoi.com/en/sockets-and-network-programming-in-c/)
