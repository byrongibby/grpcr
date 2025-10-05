# gRPCr

A gRPC server and client for R(ProtoBuf)

### Rationale

 > gRPC is a modern open source high performance Remote Procedure Call (RPC) framework that can run in any environment.

This package hopes to extend "any environment" to include R. While there are alternatives which allow other programs to use the facilities of R, such as [Rserve](https://www.rforge.net/Rserve/), gRPC offers some distinct pros and cons that make it worth having in the R toolkit. 

gRPCr compliments the already existing [RProtoBuf](https://github.com/eddelbuettel/rprotobuf) library, without which it would not be possible.

### Design

As [envisioned](https://github.com/eddelbuettel/rprotobuf/blob/master/vignettes/pdf/RProtoBuf-intro.pdf) by the authors of RProtoBuf, gRPCr is based on the functionality of the Rserve package. It maps gRPC services to Rserve's object capabilities (OCAP), using the [Java gRPC](https://github.com/grpc/grpc-java) library to do the heavy lifting.

There is an alternative [rJava/JRI](https://www.rforge.net/rJava/) backend that exposes the gRPC services (essentially a list of R functions that take a raw vector representing a protobuf payload which is (de)serlialised by RProtoBuf) directly to the JVM. It's main limitation being that it is single threaded.

Using Rserve we can control the number of connections we would like and thereby make good use of the asynchronous abilities of gRPC. Unlike Rserve, gRPC is designed to be stateless, so it is not a good choice if the need is to manage state (have persistent sessions) on the server.

### Install

```r
library(remotes)
install_github("coderaanalytics/grpcr")
```

### Author

Byron Botha

### License

MIT
