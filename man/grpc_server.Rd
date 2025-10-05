\name{grpc_server}
\alias{grpc_server}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{Start a gRPC server}
\description{
    A gRPC server that uses gRPC service definitions to communicate with Rserve.
}
\usage{
grpc_server(service_defs, host)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{service_defs}{
      Path to R script containing gRPC service definitions.
  }
  \item{port}{
      Port on which the server is reachable.
  }
}
\value{
    Void.
}
\author{
    Byron Botha
}

\examples{
#NotRun
library(gRPCr)

grpc_server(service_defs = system.file("helloworld.R", package = "gRPCr"))
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{grpc_server}
