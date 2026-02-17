\name{grpc_request}
\alias{grpc_request}
\title{Make a gRPC request}
\description{
    Send a request to a gRPC server using an initialized client.
}
\usage{
grpc_request(client, service, method, request, response_handler = NULL)
}
\arguments{
  \item{client}{
      The gRPC client instance returned by \code{\link{grpc_client}}.
  }
  \item{service}{
      The name of the gRPC service.
  }
  \item{method}{
      The name of the gRPC method to call.
  }
  \item{request}{
      The request payload as a raw vector (serialized Protobuf message).
  }
  \item{response_handler}{
      Optional function to handle the response. If NULL, the raw response is returned.
  }
}
\value{
    The response from the server, either as a raw vector or the result of \code{response_handler}.
}
\author{
    Byron Botha
}
\examples{
\dontrun{
library(gRPCr)
client <- grpc_client(host = "127.0.0.1", port = 50051)
# Assuming 'Greeter' service and 'SayHello' method
# request_bytes must be a serialized Protobuf message
response <- grpc_request(client, "Greeter", "SayHello", request_bytes)
}
}
\keyword{grpc_request}
