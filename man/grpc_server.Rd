\name{grpc_server}
\alias{grpc_server}
\title{Start a gRPC server}
\description{
    Start a gRPC server that uses gRPC service definitions to communicate with Rserve.
}
\usage{
grpc_server(service_defs, port = 50051, ...)
}
\arguments{
  \item{service_defs}{
      Path to R script containing gRPC service definitions.
  }
  \item{port}{
      Port on which the server is reachable. Defaults to 50051.
  }
  \item{\dots}{
      Additional arguments:
      \itemize{
        \item \code{num_connections}: Number of concurrent connections to handle. Defaults to 10.
        \item \code{rserve_port}: Port for Rserve instance. Defaults to 6311L.
        \item \code{is_blocking}: Logical, if TRUE the server will block execution. Defaults to TRUE.
      }
  }
}
\value{
    An external pointer to the gRPC server instance if \code{is_blocking} is FALSE. If \code{is_blocking} is TRUE, the function does not return until interrupted.
}
\author{
    Byron Botha
}
\examples{
\dontrun{
library(gRPCr)
# Assuming 'helloworld.R' contains service definitions
server <- grpc_server(service_defs = system.file("helloworld.R", package = "gRPCr"),
                      port = 50051,
                      is_blocking = FALSE)
}
}
\keyword{grpc_server}
