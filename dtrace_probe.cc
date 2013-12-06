#include "dtrace_provider.h"
#include <nan.h>

namespace node {

  using namespace v8;

  DTraceProbe::DTraceProbe() : ObjectWrap() {
    argc = 0;
    probedef = NULL;
  }

  DTraceProbe::~DTraceProbe() {
    for (size_t i = 0; i < argc; i++)
      delete(this->arguments[i]);
    usdt_probe_release(probedef);
  }

  Persistent<FunctionTemplate> DTraceProbe::constructor_template;

  void DTraceProbe::Initialize(Handle<Object> target) {
    NanScope();

    Local<FunctionTemplate> t = FunctionTemplate::New(DTraceProbe::New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(NanSymbol("DTraceProbe"));
    NanAssignPersistent(FunctionTemplate, constructor_template, t);

    NODE_SET_PROTOTYPE_METHOD(t, "fire", DTraceProbe::Fire);

    target->Set(NanSymbol("DTraceProbe"), t->GetFunction());
  }

  NAN_METHOD(DTraceProbe::New) {
    NanScope();
    DTraceProbe *probe = new DTraceProbe();
    probe->Wrap(args.This());
    NanReturnValue(args.This());
  }

  NAN_METHOD(DTraceProbe::Fire) {
    NanScope();
    DTraceProbe *pd = ObjectWrap::Unwrap<DTraceProbe>(args.Holder());
    NanReturnValue(pd->_fire(args[0]));
  }

  Handle<Value> DTraceProbe::_fire(v8::Local<v8::Value> argsfn) {
    NanScope();

    if (usdt_is_enabled(this->probedef->probe) == 0) {
      return Undefined();
    }

    // invoke fire callback
    TryCatch try_catch;

    if (!argsfn->IsFunction()) {
      return ThrowException(Exception::Error(String::New(
        "Must give probe value callback as argument")));
    }

    Local<Function> cb = Local<Function>::Cast(argsfn);
    Local<Value> probe_args = cb->Call(NanObjectWrapHandle(this), 0, NULL);

    // exception in args callback?
    if (try_catch.HasCaught()) {
      FatalException(try_catch);
      return Undefined();
    }

    // check return
    if (!probe_args->IsArray()) {
      return Undefined();
    }

    Local<Array> a = Local<Array>::Cast(probe_args);
    void *argv[USDT_ARG_MAX];

    // convert each argument value
    for (size_t i = 0; i < argc; i++) {
      argv[i] = this->arguments[i]->ArgumentValue(a->Get(i));
    }

    // finally fire the probe
    usdt_fire_probe(this->probedef->probe, argc, argv);

    // free argument values
    for (size_t i = 0; i < argc; i++) {
      this->arguments[i]->FreeArgument(argv[i]);
    }

    return True();
  }

} // namespace node
