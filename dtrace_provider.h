#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>

#ifdef _HAVE_DTRACE
extern "C" {
#include <usdt.h>
}

#include <sys/dtrace.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef __APPLE__
#include <stdlib.h>
#include <malloc.h>
#endif
    
namespace node {

  using namespace v8;

  class DTraceProbe : ObjectWrap {

  public:
    static void Initialize(v8::Handle<v8::Object> target);
    usdt_probedef_t *probedef;
    DTraceProbe *next;

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> Fire(const v8::Arguments& args);

    Handle<Value> _fire(v8::Local<v8::Value>);

    static Persistent<FunctionTemplate> constructor_template;

    DTraceProbe() : ObjectWrap() {
      next = NULL;
    }

    uint8_t Argc();

    ~DTraceProbe() {
      delete next;
    }

  private:
  };

  class DTraceProvider : ObjectWrap {

  public:
    static void Initialize(v8::Handle<v8::Object> target);
    usdt_provider_t *provider;
    DTraceProbe *probes;

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> AddProbe(const v8::Arguments& args);
    static v8::Handle<v8::Value> Enable(const v8::Arguments& args);
    static v8::Handle<v8::Value> Fire(const v8::Arguments& args);

    DTraceProvider() : ObjectWrap() {
      probes = NULL;
      provider = NULL;
    }

    ~DTraceProvider() {
      // XXX disable provider here
      delete probes;
    }
    
  private:
    static Persistent<FunctionTemplate> constructor_template;
  };  
  
  void InitDTraceProvider(v8::Handle<v8::Object> target);
}

#endif // _HAVE_DTRACE
