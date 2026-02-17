\name{gRPCr-package}
\alias{gRPCr-package}
\alias{gRPCr}
\docType{package}
\title{
  gRPC client and server for R
}
\description{
  Provides a gRPC client and server implementation for R, allowing R scripts to communicate via gRPC.
  It uses Rserve to handle the R side of the execution.
}
\details{
\tabular{ll}{
Package: \tab gRPCr\cr
Type: \tab Package\cr
Version: \tab 1.0.0\cr
Date: \tab 2025-10-02\cr
License: \tab MIT\cr
}
Key functions:
\itemize{
  \item \code{\link{grpc_server}}: Start a gRPC server.
  \item \code{\link{grpc_client}}: Start a gRPC client.
  \item \code{\link{grpc_request}}: Make a request to a gRPC server.
  \item \code{\link{grpc_shutdown}}: Shutdown a gRPC server.
}
}
\author{
Byron Botha

Maintainer: Byron Botha <byron@gibbybite.com>
}
\keyword{ package }
\examples{
\dontrun{
library(gRPCr)
# See individual function documentation for examples.
}
}
