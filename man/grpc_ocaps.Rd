\name{grpc_ocaps}
\alias{grpc_ocaps}
\title{Generate gRPC OCAPs}
\description{
    Generate Object Capabilities (OCAPs) for the gRPC services defined in `service_defs`.
    This function is primarily used internally by the gRPC server to expose services to Rserve.
}
\usage{
grpc_ocaps(service_defs)
}
\arguments{
  \item{service_defs}{
      A list structure defining the services and methods to be exposed.
      The structure is expected to be a named list of services, where each service is a named list of methods.
      The function expects `ServiceDescriptor` and `MethodDescriptor` objects to exist in the environment matching the names.
  }
}
\value{
    A function that returns an OCAP, which in turn returns the list of OCAPs for the services.
}
\author{
    Byron Botha
}
\keyword{grpc_ocaps}
