\name{grpc_shutdown}
\alias{grpc_shutdown}
\title{Shutdown a gRPC server}
\description{
    Shutdown a running gRPC server and the associated Rserve instance.
}
\usage{
grpc_shutdown(server)
}
\arguments{
  \item{server}{
      The gRPC server instance returned by \code{\link{grpc_server}}.
  }
}
\value{
    Invisibly returns NULL.
}
\author{
    Byron Botha
}
\examples{
\dontrun{
library(gRPCr)
server <- grpc_server(service_defs = "path/to/defs.R", is_blocking = FALSE)
grpc_shutdown(server)
}
}
\keyword{grpc_shutdown}
