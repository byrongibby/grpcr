\name{grpc_client}
\alias{grpc_client}
\title{Start a gRPC client}
\description{
    Start a gRPC client that connects to a gRPC server.
}
\usage{
grpc_client(host = "127.0.0.1", port = 50051)
}
\arguments{
  \item{host}{
      The host or IP address of the gRPC server. Defaults to "127.0.0.1".
  }
  \item{port}{
      The port on which the gRPC server is listening. Defaults to 50051.
  }
}
\value{
    An external pointer to the gRPC client instance.
}
\author{
    Byron Botha
}
\examples{
\dontrun{
library(gRPCr)
client <- grpc_client(host = "127.0.0.1", port = 50051)
}
}
\keyword{grpc_client}
