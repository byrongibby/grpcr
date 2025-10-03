grpc_server <- function(service_definitions, port = 35000, ...) {
  .Call("server")
}
