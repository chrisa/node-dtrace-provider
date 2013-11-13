/*
Copyright (c) Microsoft Open Technologies, Inc.
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
disclaimer in the documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once
#include <node.h>
#include "../shared_defs.h"

class eError {
public:
  std::string error_string;
  eError(const char* error): error_string(error) {}
  eError(const std::string& error): error_string(error) {}
};

/*
 * Uncomment to build the addon for a node release below 0.11.
 */
//#define BUILD_PRE011_COMPATIBILITY

#ifndef BUILD_PRE011_COMPATIBILITY
class V8Arguments: public v8::FunctionCallbackInfo<v8::Value> {};

inline auto GetObjectHandle(node::ObjectWrap* object) -> decltype(object->handle()) {
  return object->handle();
}

#define DEFINE_V8_CALLBACK(name) \
static Handle<Value> name(const V8Arguments& args); \
static void name##Native(const v8::FunctionCallbackInfo<v8::Value>& args) { \
  args.GetReturnValue().Set(DispatchCall(args, name)); \
}
#else
class V8Arguments: public v8::Arguments {};

inline auto GetObjectHandle(node::ObjectWrap* object) -> decltype(object->handle_) {
  return object->handle_;
}

#define DEFINE_V8_CALLBACK(name) \
  static Handle<Value> name(const V8Arguments& args); \
  static v8::Handle<v8::Value> name##Native(const v8::Arguments& args) { \
  return DispatchCall(args, name); \
}
#endif

#define GET_V8_CALLBACK(cls, callback) cls::callback##Native

typedef v8::Handle<v8::Value> (*AbstractionCallback)(const V8Arguments& args);

#ifndef BUILD_PRE011_COMPATIBILITY
inline v8::Handle<v8::Value> DispatchCall(const v8::FunctionCallbackInfo<v8::Value>& args, AbstractionCallback callback) {
#else
inline v8::Handle<v8::Value> DispatchCall(const v8::Arguments& args, AbstractionCallback callback) {
#endif
  v8::Handle<v8::Value> result;
  V8Arguments& arguments = (V8Arguments&) args;
  try {
    result = callback(arguments);
  } catch (const eError& ex) {
    ThrowException(v8::Exception::Error(v8::String::New(ex.error_string.c_str())));
    return v8::Undefined();
  }
  return result;
}

template <typename T>
inline v8::Local<T> ExtractFromPersistent(const v8::Persistent<T>& persitent) {
#ifndef BUILD_PRE011_COMPATIBILITY
  Local<T> object = Local<T>::New(Isolate::GetCurrent(), persitent); 
#else 
  Local<T> object = Local<T>::New(persitent);  
#endif
  return object;
}

template <typename T>
inline void WrapInPersistent(v8::Persistent<T>& persistent, const v8::Handle<T> object) {
#ifndef BUILD_PRE011_COMPATIBILITY
  persistent.Reset(Isolate::GetCurrent(), object);
#else
  persistent = Persistent<T>::New(object);
#endif
}

#ifndef BUILD_PRE011_COMPATIBILITY
typedef void (*V8Callback)(const v8::FunctionCallbackInfo<v8::Value>& args);
#else
typedef v8::Handle<v8::Value> (*V8Callback)(const v8::Arguments& args);
#endif

inline void ExposeMethod(v8::Handle<v8::FunctionTemplate>& t, const char* callback_name, V8Callback callback) {
#ifndef BUILD_PRE011_COMPATIBILITY
  t->PrototypeTemplate()->Set(v8::String::New(callback_name), v8::FunctionTemplate::New(callback)->GetFunction());
#else
  NODE_SET_PROTOTYPE_METHOD(t, callback_name, callback);
#endif
}