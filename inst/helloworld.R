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
