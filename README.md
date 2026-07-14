<h1 align="center">42_WEBSERV_1337</h1>

<p align="center">
  <a href="https://github.com/haytham-hammioui/42_WEBSERV_1337">
    <img src="https://raw.githubusercontent.com/ayogun/42-project-badges/refs/heads/main/badges/webservm.png" alt="42 Webserv Badge">
  </a>
</p>

# Webserv

`Webserv` is a small HTTP/1.1 server written in C++98 as part of the 42 Network curriculum. It is designed to handle real web-server behavior, including request parsing, routing, static file serving, CGI execution, uploads, redirects, and custom error pages.

This project focuses on low-level networking and protocol handling, with an event-driven architecture based on polling and multiple listening sockets.

---

## Features

- HTTP server implementation with support for multiple virtual servers
- Request parsing for `GET`, `POST`, and `DELETE`
- Static file serving with custom root and index configuration
- Directory listing support
- File upload handling
- CGI execution for scripts such as Python and PHP
- Location-based routing and redirections
- Custom error page handling
- Configurable client body size limits
- Timeout handling for inactive clients
- Multiple server blocks in a single configuration file

---

## Project Overview

This project has been created as part of the 42 curriculum by **[smaksiss](https://github.com/M4KSS1S)**, **[hhammiou](https://github.com/haytham-hammioui)** and **[oelhasso](https://github.com/alemdaar)**.


### Main Goals

- Parse and validate a custom configuration file
- Create and manage listening sockets
- Accept and serve multiple clients
- Dispatch requests to the right handler depending on method and route
- Support CGI execution for dynamic content
- Return correct HTTP responses and error pages

### Architecture

- `core/` handles the server loop and main entry point
- `http_layer/` contains request, response, and method handlers
- `src/` contains configuration, routing, and CGI-related components
- `www/` contains sample web content, CGI scripts, and error pages

---

## Getting Started

### Prerequisites

- OS: Linux / macOS
- Compiler: `c++`
- Standard: `C++98`
- Tools: `make`

### Installation

1. Clone the repository:

```bash
git clone https://github.com/haytham-hammioui/42_WEBSERV_1337.git webserv
cd webserv
```

2. Build the project:

```bash
make
```

3. Remove build artifacts if needed:

```bash
make clean
make fclean
make re
```

---

## Usage

Run the server with a valid configuration file:

```bash
./webserv config/default.conf
```

The program expects exactly one argument: the path to the configuration file.

---

## Configuration Example

```txt
server {
    listen 8080;
    host 127.0.0.1;
    client_max_body_size 20M;
    error_page 404 www/errors/404.html;

    location / {
        methods GET POST;
        root www;
        index index.html;
        directory_listing on;
    }

    location /cgi-bin {
        methods GET POST;
        root www/cgi-bin;
        cgi_pass .py /usr/bin/python3;
        cgi_pass .php /usr/bin/php;
    }
}
```

### Supported Concepts

- `listen` and `host`
- `client_max_body_size`
- `error_page`
- `location` blocks
- `methods`
- `root`
- `index`
- `redirect`
- `directory_listing`
- `upload_store`
- `cgi_pass`

---

## Technical Details

- Language: C++98
- Networking: sockets + `poll`
- Request handling: custom HTTP parser and dispatcher
- Routing: location matching and path resolution
- CGI: script execution through configured interpreters
- Memory: manual management

---

## Resources

- HTTP/1.1 overview: https://developer.mozilla.org/en-US/docs/Web/HTTP
- CGI basics: https://en.wikipedia.org/wiki/Common_Gateway_Interface

---

## Developed By

- **[smaksiss](https://github.com/M4KSS1S)**  
- **[hhammiou](https://github.com/haytham-hammioui)**
- **[oelhasso](https://github.com/alemdaar)**

## Contributing

Contributions to `WebServ` are welcome! Whether you've found a bug, have a feature request, or want to contribute code:

1. Fork the repository.
2. Create a new branch for your changes.
3. Add your contributions.
4. Push your branch and open a pull request against the `WebServ` repository.


