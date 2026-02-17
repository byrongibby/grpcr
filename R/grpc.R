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
    rserve_port <- if (!is.null(dots$rserve_port)) {
      dots$rserve_port
    } else {
      6311L
    }
    is_blocking <- if (!is.null(dots$is_blocking)) {
      dots$is_blocking
    } else {
      TRUE
    }
    Rserve(debug = FALSE,
           port = rserve_port,
           args = paste0("--no-save ",
                         "--slave ",
                         "--RS-set shutdown=1 ",
                         "--RS-set qap.oc=1 ",
                         "--RS-set source=", service_defs),
           wait = TRUE)
    server <- .Call("grpcr_server_start", port)
    if (is_blocking) Sys.sleep(Inf)
    return(server)
  }, error = function(e) {
    stop("Failed to initialise gRPC server: ", conditionMessage(e), "\n")
  }, interrupt = function(i) {
    .Call("grpcr_server_shutdown", server)
    server <- NULL
    system("pkill Rserve")
    return(invisible(server))
  })
}

grpc_shutdown <- function(server) {
  stopifnot(isa(server, "grpc_server_extptr"))
  .Call("grpcr_server_shutdown", server)
  server <- NULL
  system("pkill Rserve")
  return(invisible(server))
}

grpc_client <- function(host = "127.0.0.1", port = 50051) {
  tryCatch({
    stopifnot(is.character(host) && length(host) == 1)
    stopifnot(is.numeric(port))

    client <- .Call("grpcr_client_init", host, port)

    if (is.null(client)) {
      stop("Client is NULL")
    } else if (!client[[1]]) {
      stop(client[[2]])
    } else {
      return(client[[3]])
    }
  }, error = function(e) {
    stop("Failed to initialise gRPC client: ", conditionMessage(e), "\n")
  })
}

grpc_request <- function(
  client,
  service,
  method,
  request,
  response_handler = NULL
) {
  tryCatch({
    stopifnot(isa(client, "grpc_client_extptr"))
    stopifnot(is.character(service) && length(service) == 1)
    stopifnot(is.character(method) && length(method) == 1)
    stopifnot(is.raw(request))
    stopifnot(is.null(response_handler) || is.function(response_handler))

    response <- .Call("grpcr_client_call",
                      client,
                      paste0("/", service, "/", method),
                      request)

    if (is.null(response)) {
      stop("Response is NULL")
    } else if (!response[[1]]) {
      stop(response[[2]])
    } else {
      if (is.null(response_handler)) {
        return(response[[3]])
      } else {
        return(response_handler(response[[3]]))
      }
    }
  }, error = function(e) {
    stop("Request to gRPC server failed: ", conditionMessage(e), "\n")
  })
}
