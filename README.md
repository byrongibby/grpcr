# gRPCr

A gRPC server and client implementation for R, powered by Rserve and C++. **Not production ready.**

## Overview

`gRPCr` enables R to act as both a gRPC server and a gRPC client. 

*   **Server**: It runs a high-performance C++ gRPC server that proxies requests to an R session via Rserve. This allows you to define gRPC services directly in R code.
*   **Client**: It provides a C++ based client to send requests to any gRPC server.

It leverages the [RProtoBuf](https://github.com/eddelbuettel/rprotobuf) package for Protocol Buffer serialization/deserialization within R.

## Features

*   **Native C++ Implementation**: Uses the official `grpcpp` library for high performance.
*   **Rserve Integration**: Maps gRPC service methods to Rserve Object Capabilities (OCAPs), allowing for flexible service definitions.
*   **Blocking & Non-Blocking**: Support for both blocking and non-blocking server execution.
*   **Client Support**: Send requests to other gRPC services from R.

## Installation

### Prerequisites

*   **System Libraries**: You need the gRPC C++ libraries installed on your system.
    *   *Linux (Arch)*: `pacman -S grpc`
    *   *Debian/Ubuntu*: `apt-get install libgrpc++-dev protobuf-compiler-grpc`
    *   *macOS*: `brew install grpc`
*   **R Packages**: `RProtoBuf`, `Rserve` (server only)

### Install from GitHub

```r
# install.packages("remotes")
remotes::install_github("byrongibby/grpcr")
```

## Usage

### 1. Define your Service

First, define your gRPC service using a `.proto` file and implement the logic in R.

**helloworld.R**:
```r
suppressPackageStartupMessages({
  library(gRPCr)
  library(RProtoBuf)
})

RProtoBuf::readProtoFiles(system.file("helloworld.proto", package = "gRPCr"))

service_defs <- list(
  helloworld.Greeter = list(
    SayHello = function(request) {
      message <- helloworld.HelloRequest$read(request)
      response <- new(helloworld.HelloReply)
      response$message <- paste0("Hello ", message$name)
      return(serialize(response, NULL))
    }
  )
)

oc.init <- grpc_ocaps(service_defs)

message("Successfully loaded gRPC services to Rserve as OCAPs.")
```

### 2. Start the Server

```r
library(gRPCr)

server <- grpc_server(service_defs = "path/to/helloworld.R",
                      port = 50051,
                      is_blocking = FALSE)
```

### 3. Create a Client and Make a Request

```r
library(gRPCr)
library(RProtoBuf)

# Initialize client
client <- grpc_client(host = "127.0.0.1", port = 50051)

# Create a request message
# You need the proto definitions loaded on the client side too
readProtoFiles(system.file("helloworld.proto", package = "gRPCr"))
request <- new(helloworld.HelloRequest, name = "World")
request_bytes <- request$serialize(NULL)

# Send the request
# The response handler is optional; here we use it to parse the reply immediately
response_text <- grpc_request(
  client  = client,
  service = "Greeter",
  method  = "SayHello",
  request = request_bytes, 
  response_handler = function(response_bytes) {
    helloworld.HelloReply$read(response_bytes)$message
  }
)

print(response_text)
# Output: "Hello World"
```

### 4. Shutdown

```r
grpc_shutdown(server)
```

## Architecture

The `gRPCr` server acts as a proxy:
1.  **C++ gRPC Server**: Listens for incoming gRPC calls.
2.  **Rserve Client**: When a call is received, the C++ server acts as an Rserve client and forwards the request to a local Rserve instance via OCAPs (Object Capabilities).
3.  **R Execution**: The Rserve instance executes the corresponding R function and returns the result.

## License

MIT
