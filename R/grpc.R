grpc_ocaps <- function(service_defs) {
  ocaps <- list()
  for (service in names(service_defs)) {
    for (method in names(service_defs[[service]])) {
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
    system("pkill Rserve")
    .Call("grpcr_server_shutdown", server)
    server <- NULL
    return(invisible(server))
  })
}
