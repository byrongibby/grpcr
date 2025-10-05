\name{gRPCr-package}
\alias{gRPCr-package}
\docType{package}
\title{
  gRPC client and server for R
}
\description{
  gRPC client and server for R.
}
\details{
\tabular{ll}{
Package: \tab gRPCr\cr
Type: \tab Package\cr
Version: \tab 1.0.0\cr
Date: \tab 2025-10-02\cr
License: \tab MIT\cr
}
}
\author{
Byron Botha

Maintainer: Byron Botha <byron@gibbybite.com>
}
\keyword{ package }
\examples{
#NotRun
library(grpcr)

grpc_server(service_defs = system.file("helloworld.R", package = "grpcr"))
}
