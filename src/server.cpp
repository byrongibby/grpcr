#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/generic/callback_generic_service.h>

#include <R.h>
#include <Rdefines.h>

extern "C" {

#include "rserve-client/rexp.h"
#include "rserve-client/rlist.h"
#include "rserve-client/rserve.h"
#include "rserve-client/utilities.h"

}

using grpc::ByteBuffer;
using grpc::CallbackGenericService;
using grpc::CallbackServerContext;
using grpc::GenericCallbackServerContext;
using grpc::SerializationTraits;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerGenericBidiReactor;
using grpc::Status;
using grpc::StatusCode;

typedef struct RServe {
    RConnection conn;
    RList *methods;
} RServe;

// Thread-safe queue of Rserve connection objects
// https://www.geeksforgeeks.org/dsa/implement-thread-safe-queue-in-c/
class RServeQueue {
private:
    // Underlying queue
    std::queue<RServe> m_queue;

    // mutex for thread synchronization
    std::mutex m_mutex;

    // Condition variable for signaling
    std::condition_variable m_cond;

public:
    // Pushes an element to the queue
    void push(RServe item)
    {
        // Acquire lock
        std::unique_lock<std::mutex> lock(m_mutex);

        // Add item
        m_queue.push(item);

        // Notify one thread that
        // is waiting
        m_cond.notify_one();
    }

    // Pops an element off the queue
    RServe pop()
    {
        // acquire lock
        std::unique_lock<std::mutex> lock(m_mutex);

        // wait until queue is not empty
        m_cond.wait(lock,
                    [this]() { return !m_queue.empty(); });

        // retrieve item
        RServe item = m_queue.front();
        m_queue.pop();

        // return item
        return item;
    }
};

template <>
class grpc::SerializationTraits<REXP> {
  public:
    // Deserialize a ByteBuffer into a XT_RAW REXP
    static Status Deserialize(ByteBuffer* buffer, REXP* msg) {
      if (!buffer || !msg) {
        return Status(StatusCode::INVALID_ARGUMENT, "Null buffer or REXP pointer.");
      }

      grpc::Slice slice;

      Status status = buffer->DumpToSingleSlice(&slice);
      if (!status.ok()) {
        return status;
      }

      rawrexp_init(msg, (char *)slice.begin(), (char *)slice.end());
      
      return Status::OK;
    }

    // Serialize XT_RAW REXP into a ByteBuffer
    static Status Serialize(const REXP& msg, ByteBuffer* buffer, bool* own_buffer) {

      grpc::Slice slice((char *)msg.data, rawrexp_size(&msg));

      *buffer = grpc::ByteBuffer(&slice, 1);
      *own_buffer = true; // Indicate that the caller owns the ByteBuffer

      return Status::OK;
    }
};

extern "C" {

// Logic and data behind the server's behavior.
class RserveServiceImpl final : public CallbackGenericService {

  public:
  RserveServiceImpl(const char *host, const int port) {
    RServe rserve;
    REXP call = { XT_NULL, 0, 0 }, ocaps = { XT_NULL, 0, 0 };
    char *method;
    int i, ret;

    for (i = 0; i < 1; ++i) {
      if ((ret = rserve_connect(&rserve.conn, host, port)) != 0) {
        printf("Rserve error: %s\n", rserve_error(ret));
        printf("Failed to estabilish connection\n");
      }

      // Create call that will return the gRPC methods
      assign_call(&call, &rserve.conn.capabilities, NULL, 0);

      // Query gRPC service methods
      if ((ret = rserve_callocap(&rserve.conn, &call, &ocaps)) != 0) {
        printf("Rserve error: %s\n", rserve_error(ret));
        printf("Failed to return gRPC methods (OCAPs)\n");
      }

      rexp_clear(&call);

      // Extract list of methods
      rserve.methods = (RList *)malloc(sizeof(RList));
      *rserve.methods = *(RList *)ocaps.data;

      if (i == 0) {
        for(int j = 0; j < rlist_size(rserve.methods); ++j) {
          method = rlist_name_at(rserve.methods, j);
          m_rserve_methods.push_back(std::string(method));
        }
      }

      m_rserve_queue.push(rserve);
    }
  }

  ~RserveServiceImpl() {
    RServe rserve;
    for (int i = 0; i < 1; ++i) {
      rserve = m_rserve_queue.pop();
      rserve_disconnect(&rserve.conn);
      rlist_free(rserve.methods);
      printf("Successfully disconnected from Rserve\n");
    }
  }

  private:
  ServerGenericBidiReactor* CreateReactor(
      GenericCallbackServerContext* context) override {

    for (int i = 0; i < m_rserve_methods.size(); ++i) {
      if(context->method() == m_rserve_methods.at(i)) {
        return new RserveReactor(this, i);
      }
    }
    // Forward this to the implementation of the base class returning
    // UNIMPLEMENTED.
    return CallbackGenericService::CreateReactor(context);
  }

  class RserveReactor : public ServerGenericBidiReactor {
   public:
    RserveReactor(RserveServiceImpl *service, int method_index) { 
      m_method_index = method_index;
      m_service = service;
      StartRead(&m_request); 
    }

   private:
    // Call Rserve
    Status RserveRequest(REXP& request, REXP* reply) {
      int ret;
      Status result;
      REXP rx, *ocap, call = { XT_NULL, NULL, NULL };
      RServe rserve = m_service->m_rserve_queue.pop();

      // Create gRPC method call
      ocap = rlist_at(rserve.methods, m_method_index);
      assign_call(&call, ocap, &request, 1);

      // Call gRPC method
      if ((ret = rserve_callocap(&rserve.conn, &call, &rx)) != 0) {
        printf("Rserve error: %s\n", rserve_error(ret));
        printf("Failed to return call to OCAP\n");
      }

      rexp_clear(&call);

      *reply = rx;

      m_service->m_rserve_queue.push(rserve);

      return Status::OK;
    }

    // Handle (de)serialization and communication with Rserve
    void ProcessRequest() {
      Status result;

      // Deserialize a request message
      REXP request = { XT_NULL, 0, 0 };
      result =  grpc::SerializationTraits<REXP>::Deserialize(
          &m_request, &request);
      if (!result.ok()) {
        Finish(result);
        return;
      }

      // Call the Rserve handler
      REXP reply = { XT_NULL, 0, 0 };
      result = RserveRequest(request, &reply);
      if (!result.ok()) {
        Finish(result);
        return;
      }

      // Serialize a reply message
      bool own_buffer;
      result =  grpc::SerializationTraits<REXP>::Serialize(
          reply, &m_response, &own_buffer);
      if (!result.ok()) {
        Finish(result);
        return;
      }

      StartWrite(&m_response);
    }

    void OnDone() override {
      delete this;
    }

    void OnReadDone(bool ok) override {
      if (!ok) {
        return;
      }
      // Push blocking operations onto its own thread
      std::thread t(&RserveReactor::ProcessRequest, this);
      t.detach();
    }

    void OnWriteDone(bool ok) override {
      Finish(ok ? Status::OK : Status(StatusCode::UNKNOWN, "Unexpected failure"));
    }

    ByteBuffer m_request;
    ByteBuffer m_response;
    int m_method_index;
    RserveServiceImpl *m_service;
  };

 private:
  std::vector<std::string> m_rserve_methods;
  RServeQueue m_rserve_queue;
};

typedef struct GRPCRServer {
  RserveServiceImpl *service;
  grpc::Server *server;
} GRPCRServer;

void grpcr_server_finalize(SEXP ptr)
{
  if (TYPEOF(ptr) != EXTPTRSXP) return;

  GRPCRServer *p = (GRPCRServer *)R_ExternalPtrAddr(ptr);

  if (p) {
    Rprintf("Finalizing grpc_server at address %p\n", p);
    delete p->service;
    delete p->server;
    free(p);
    R_ClearExternalPtr(ptr);
  }
}

SEXP grpcr_server_start(SEXP port)
{
  char server_address[250], rserve_address[250];
  snprintf(server_address, 250, "0.0.0.0:%d", asInteger(port));
  snprintf(rserve_address, 250, "127.0.0.1");
  int rserve_port = 6311;

  RserveServiceImpl *service = new RserveServiceImpl(rserve_address, rserve_port);
  grpc::EnableDefaultHealthCheckService(true);
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(std::string(server_address), grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterCallbackGenericService(service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  printf("Server listening on %s\n", server_address);

  GRPCRServer *p = (GRPCRServer *)malloc(sizeof(GRPCRServer));
  p->service = service;
  p->server = server.release();

  // Create an external pointer, setting the tag and finalizer
  SEXP ptr = PROTECT(R_MakeExternalPtr(p, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(ptr, grpcr_server_finalize);

  // Set a custom class for easier identification in R
  setAttrib(ptr, R_ClassSymbol, mkString("grpc_server_extptr"));

  UNPROTECT(1);

  return ptr;
}

SEXP grpcr_server_shutdown(SEXP ptr)
{
  if (TYPEOF(ptr) != EXTPTRSXP)  R_NilValue;

  GRPCRServer *p = (GRPCRServer *)R_ExternalPtrAddr(ptr);

  if (p) {
    Rprintf("Shutting down grpc_server at address %p\n", p);
    p->server->Shutdown();
  }

  return R_NilValue;
}

} // END_EXTERN_C
