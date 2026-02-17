#include <cstdlib>
#include <iostream>
#include <future>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub_callback.h>  

#include <R.h>
#include <Rdefines.h>

typedef struct GRPCRClient {
  grpc::GenericStubCallback *stub;
} GRPCRClient;

typedef struct Buffer {
  char *data;
  size_t size;
} Buffer;

using grpc::ByteBuffer;
using grpc::ClientContext;
using grpc::ClientUnaryReactor;
using grpc::GenericStubCallback;
using grpc::InsecureChannelCredentials;
using grpc::Slice;
using grpc::Status;
using grpc::StubOptions;

using std::exception;
using std::future;
using std::promise;
using std::string;

extern "C" {

class ClientReactor : public ClientUnaryReactor {
 public:
  ClientReactor(
      GenericStubCallback *stub,
      const char *method,
      Buffer *request,
      Buffer *response,
      promise<Status> *promise
  ) : m_response_buf(response), m_promise(promise) {

    StubOptions options;

    string method_str(method);
    Slice slice(request->data, request->size);
    ByteBuffer request_grpc_buf = ByteBuffer(&slice, 1);

    stub->PrepareUnaryCall(
      &m_context,
      method_str,
      options,
      &request_grpc_buf,
      &m_response_grpc_buf,
      this
    );

    StartCall();
  }

  void OnDone(const Status& s) override {
    Slice slice;
    Status status;

    if (s.ok()) {
      status = m_response_grpc_buf.DumpToSingleSlice(&slice);
      m_response_buf->data = (char *)malloc(slice.size());
      memcpy(m_response_buf->data, slice.begin(), slice.size());
      m_response_buf->size = slice.size();
      m_promise->set_value(s);
    } else {
      m_promise->set_value(s);
    }

    delete this;
  }

 private:
  ClientContext m_context;
  ByteBuffer m_response_grpc_buf;
  Buffer *m_response_buf;
  promise<Status> *m_promise;
};

void grpcr_client_finalize(SEXP ptr)
{
  if (TYPEOF(ptr) != EXTPTRSXP) return;

  GRPCRClient *p = (GRPCRClient *)R_ExternalPtrAddr(ptr);

  if (p) {
    Rprintf("Finalizing grpc_client at address %p\n", p);
    delete p->stub;
    free(p);
    R_ClearExternalPtr(ptr);
  }
}

SEXP grpcr_client_init(SEXP host, SEXP port) {
  SEXP result  = PROTECT(allocVector(VECSXP, 3));
  SEXP success = PROTECT(allocVector(LGLSXP, 1));
  SEXP message = PROTECT(allocVector(STRSXP, 1));
  int *success_ptr = LOGICAL(success);

  if ((TYPEOF(host) != STRSXP && length(host) == 1) ||
      TYPEOF(port) != REALSXP) {

    success_ptr[0] = 0;

    SET_STRING_ELT(
      message,
      0,
      mkChar("Failed to initialise gRPC client: Parameter mismatch.")
    );

    SET_VECTOR_ELT(result,  0, success);
    SET_VECTOR_ELT(result,  1, message);
    SET_VECTOR_ELT(result,  2, R_NilValue);

    UNPROTECT(3);

    return result;
  }

  char address[200];
  snprintf(address, 200, "%s:%d", CHAR(STRING_ELT(host, 0)), asInteger(port));
  string server_address(address);

  auto channel = CreateChannel(server_address, InsecureChannelCredentials());
  GRPCRClient *p = (GRPCRClient *)malloc(sizeof(GRPCRClient));

  p->stub = new GenericStubCallback(channel);

  // Create an external pointer, setting the tag and finalizer
  SEXP ptr = PROTECT(R_MakeExternalPtr(p, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(ptr, grpcr_client_finalize);
  setAttrib(ptr, R_ClassSymbol, mkString("grpc_client_extptr"));


  success_ptr[0] = 1;

  SET_VECTOR_ELT(result,  0, success);
  SET_VECTOR_ELT(result,  1, R_NilValue);
  SET_VECTOR_ELT(result,  2, ptr);

  UNPROTECT(4);

  return result;
}

// Sync-Over-Async pattern - reduce async unary call to a sync call
// in the context of R
SEXP grpcr_client_call(SEXP ptr, SEXP method, SEXP request)
{
  SEXP result  = PROTECT(allocVector(VECSXP, 3));
  SEXP success = PROTECT(allocVector(LGLSXP, 1));
  SEXP message = PROTECT(allocVector(STRSXP, 1));
  int *success_ptr = LOGICAL(success);

  if (TYPEOF(ptr) != EXTPTRSXP ||
      (TYPEOF(method) != STRSXP && length(method) == 1) ||
      TYPEOF(request) != RAWSXP) {

    success_ptr[0] = 0;

    char error[2000];
    SET_STRING_ELT(message, 0, mkChar("gRPC call failed. Parameter mismatch."));

    SET_VECTOR_ELT(result,  0, success);
    SET_VECTOR_ELT(result,  1, message);
    SET_VECTOR_ELT(result,  2, R_NilValue);

    UNPROTECT(3);

    return result;
  }

  GRPCRClient *p = (GRPCRClient *)R_ExternalPtrAddr(ptr);
  const char *method_ptr = CHAR(STRING_ELT(method, 0));
  Buffer request_buffer, response_buffer = { NULL, 0 };
  promise<Status> promise;
  auto future = promise.get_future();

  request_buffer.data = (char *)RAW(request);
  request_buffer.size = length(request);

  new ClientReactor(
    p->stub,
    method_ptr,
    &request_buffer,
    &response_buffer,
    &promise
  );

  Status status;
  SEXP response;

  // BLOCK HERE until ClientReactor.OnDone is called
  status = future.get();

  if (status.ok()) {
    success_ptr[0] = 1;

    PROTECT(response = allocVector(RAWSXP, response_buffer.size));
    memcpy(RAW(response), response_buffer.data, response_buffer.size);
    free(response_buffer.data);

    SET_VECTOR_ELT(result,  0, success);
    SET_VECTOR_ELT(result,  1, R_NilValue);
    SET_VECTOR_ELT(result,  2, response);

    UNPROTECT(4);

    return result;
  } else {
    if (response_buffer.data) free(response_buffer.data);

    success_ptr[0] = 0;

    char error[2000];
    const char *status_message = status.error_message().c_str() ?
      "no message available" :
      status.error_message().c_str();
    const char *status_details = status.error_details().c_str() ?
      "no details available" : 
      status.error_details().c_str();
    snprintf(
      error,
      2000, 
      "gRPC call failed. Error code %d: %s, %s.",
      status.error_code(),
      status_message,
      status_details
    );
    SET_STRING_ELT(message, 0, mkChar(error));

    SET_VECTOR_ELT(result,  0, success);
    SET_VECTOR_ELT(result,  1, message);
    SET_VECTOR_ELT(result,  2, R_NilValue);

    UNPROTECT(3);

    return result;
  }

  return R_NilValue; // Should not be reached
}

} // END_EXTERN_C
