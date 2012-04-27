#include "dtrace_provider.h"
#include <v8.h>

#include <node.h>
#include <stdio.h>

#ifdef _HAVE_DTRACE

namespace node {
  
  using namespace v8;

  Persistent<FunctionTemplate> DTraceProvider::constructor_template;
  
  void DTraceProvider::Initialize(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(DTraceProvider::New);
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("DTraceProvider"));

    NODE_SET_PROTOTYPE_METHOD(constructor_template, "addProbe", DTraceProvider::AddProbe);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "enable", DTraceProvider::Enable);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "fire", DTraceProvider::Fire);

    target->Set(String::NewSymbol("DTraceProvider"), constructor_template->GetFunction());

    DTraceProbe::Initialize(target);
  }
  
  Handle<Value> DTraceProvider::New(const Arguments& args) {
    HandleScope scope;
    DTraceProvider *p = new DTraceProvider();

    p->Wrap(args.This());

    if (args.Length() != 1 || !args[0]->IsString()) {
      return ThrowException(Exception::Error(String::New(
        "Must give provider name as argument")));
    }
    
    String::AsciiValue name(args[0]->ToString());

    if ((p->provider = usdt_create_provider(*name, "modname")) == NULL) {
      return ThrowException(Exception::Error(String::New(
        "usdt_create_provider failed")));
    }

    return args.This();
  }

  Handle<Value> DTraceProvider::AddProbe(const Arguments& args) {
    HandleScope scope;
    const char *types[USDT_ARG_MAX];
    int argc = 0;

    DTraceProvider *provider = ObjectWrap::Unwrap<DTraceProvider>(args.Holder());

    // create a DTraceProbe object - hateful, what's the right way?
    Handle<Function> klass = DTraceProbe::constructor_template->GetFunction();
    v8::Handle<v8::Value> args2[] = { 
      args[0], args[1], args[2], args[3], args[4], args[5], args[6]
    };
    Handle<Value> pd = Persistent<Value>::New(klass->NewInstance(7, args2));

    // append to probe list
    DTraceProbe *probe = ObjectWrap::Unwrap<DTraceProbe>(pd->ToObject());
    String::AsciiValue name(args[0]->ToString());

    for (int i = 0; i < USDT_ARG_MAX; i++) {
      if (i < args.Length() - 1) {
        String::AsciiValue type(args[i + 1]->ToString());
        types[i] = strdup(*type);
        argc++;
      }
    }

    probe->probedef = usdt_create_probe(*name, *name, argc, types);
    usdt_provider_add_probe(provider->provider, probe->probedef);

    if (provider->probes == NULL)
      provider->probes = probe;
    else {
      DTraceProbe *p;
      for (p = provider->probes; (p->next != NULL); p = p->next) ;
      p->next = probe;
    }
    
    return pd;
  }

  Handle<Value> DTraceProvider::Enable(const Arguments& args) {
    HandleScope scope;
    DTraceProvider *provider = ObjectWrap::Unwrap<DTraceProvider>(args.Holder());

    if (usdt_provider_enable(provider->provider) != 0) {
      return ThrowException(Exception::Error(String::New(
        "Failure to enable")));
    }      

    return Undefined();
  }

  Handle<Value> DTraceProvider::Fire(const Arguments& args) {
    HandleScope scope;
    DTraceProvider *provider = ObjectWrap::Unwrap<DTraceProvider>(args.Holder());

    if (!args[0]->IsString()) {
      return ThrowException(Exception::Error(String::New(
        "Must give probe name as first argument")));
    }

    if (!args[1]->IsFunction()) {
      return ThrowException(Exception::Error(String::New(
        "Must give probe value callback as second argument")));
    }

    String::AsciiValue probe_name(args[0]->ToString());

    // find the probe we should be firing
    DTraceProbe *p;
    for (p = provider->probes; p != NULL; p = p->next) {
      if (!strcmp(p->probedef->name, *probe_name)) {
        break;
      }
    }
    if (p == NULL) {
      return Undefined();
    }

    p->_fire(args[1]);
  }

  extern "C" void
  init(Handle<Object> target) {
    DTraceProvider::Initialize(target);
  }

} // namespace node

#endif // _HAVE_DTRACE
