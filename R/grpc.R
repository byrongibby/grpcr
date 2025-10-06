grpc_ocaps <- function(service_defs) {
  ocaps <- list()
  for (service in names(service_defs)) {
    stopifnot(exists(service))
    stopifnot(isa(get(service), "ServiceDescriptor"))
    for (method in names(service_defs[[service]])) {
      stopifnot(exists(paste(service, method, sep = ".")))
      stopifnot(isa(get(paste(service, method, sep = ".")), "MethodDescriptor"))
      ocaps[[paste0("/", service, "/", method)]] <-
        ocap(service_defs[[service]][[method]])
    }
  }
  return(function() ocap(function() ocaps))
}

grpc_server <- function(service_defs, port = 50051, ...) {
  tryCatch({
    dots <- list(...)
    num_connections <- if (!is.null(dots$num_connections)) {
      dots$num_connections
    } else {
      10
    }
    Rserve(debug = FALSE,
           port = 6311L,
           args = paste0("--no-save ",
                         "--slave ",
                         "--RS-set shutdown=1 ",
                         "--RS-set qap.oc=1 ",
                         "--RS-set source=", service_defs),
           wait = TRUE)
    server <- .Call("grpcr_server_start", port)
    Sys.sleep(Inf)
  }, error = function(e) {
    stop("Failed to start gRPC server: ", conditionMessage(e), "\n")
  }, interrupt = function(i) {
    .Call("grpcr_server_shutdown", server)
    server <- NULL
    system("pkill Rserve")
    return(invisible(server))
  })
}
