test_that("Greeter service: client and server can communicate", {
  library(RProtoBuf)
  library(gRPCr)
  library(zoo)

  pkg <- system.file(package = "gRPCr")

  server <- grpc_server(service_defs = file.path(pkg, "helloworld.R"),
                        port = 50051,
                        is_blocking = FALSE)

  RProtoBuf::readProtoFiles(file.path(pkg, "helloworld.proto"))

  # Return HelloRequest message to send to the server
  say_hello <- function(name) {
    request <- new(helloworld.HelloRequest, name = name)
    return(request$serialize(NULL))
  }

  # Return function that will create HelloReply message
  # from the server response
  response_handler <- function() {
    function(response) {
      message <- helloworld.HelloReply$read(response)
      return(message$message)
    }
  }

  client <- grpc_client(port = 50051)

  response <- grpc_request(client  = client,
                           service = "helloworld.Greeter",
                           method  = "SayHello",
                           request = say_hello("gRPCr"),
                           response_handler = response_handler())

  grpc_shutdown(server)

  expect_equal("Hello gRPCr", response)
})
