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
#include <evntprov.h>
#include "provider_types.h"
#include "v8_compatibility.h"
#include <vector>
#include <memory>

using namespace node;
using namespace v8;

#define PROBE_CLASS_NAME "Probe"

/*
 * Exposable implementation stores a pointer to the actual implementation for the V8 object.
 * Aggregation ensures that the pointer to this implementation will always be valid
 * even if V8 GC moves around the containing object in memory.
 * Wrapping the implementation in shared_ptr ensures that when the containing object (V8Provider) for the provider is deleted by the GC
 * its implementation will live for as long as there are probes bound to this implementation.
 * When these probes are deleted, the provider implementation will be deleted as well.
 */
template <typename T> 
class ExposableImplementation: protected ObjectWrap {
  std::shared_ptr<T> m_implementation;
public:
  ExposableImplementation(T* implementation): m_implementation(implementation) {}
  T* GetImplementation() {
    return m_implementation.get();
  }
  std::shared_ptr<T> GetSharedImplementation() {
    return m_implementation;
  }
};

class RealProbe;

class V8Probe: public ExposableImplementation<RealProbe> {
public:
  V8Probe(RealProbe* implementation, ProbeArgumentsTypeInfo& types): ExposableImplementation(implementation) {
    m_type_info = std::move(types);

    m_type_info.DoForEach([&](EventPayloadType type) {
      IArgumentValue* argument_value = nullptr;
      switch (type) {
        case EDT_JSON:
        case EDT_STRING: argument_value = new ArgumentValue<std::string>(); break;
        case EDT_WSTRING: argument_value = new ArgumentValue<std::wstring>(); break;
        case EDT_INT32: argument_value = new ArgumentValue<int32_t>(); break;
        case EDT_INT8: argument_value = new ArgumentValue<int8_t>(); break;
        case EDT_INT16: argument_value = new ArgumentValue<int16_t>(); break;						
        case EDT_INT64: argument_value = new ArgumentValue<int64_t>(); break;			
        case EDT_UINT32: argument_value = new ArgumentValue<uint32_t>(); break;	
        case EDT_UINT8: argument_value = new ArgumentValue<uint8_t>(); break;	
        case EDT_UINT16: argument_value = new ArgumentValue<uint16_t>(); break;	 
        case EDT_UINT64: argument_value = new ArgumentValue<uint64_t>(); break;	
        default: 
          break;
      }
      m_argument_values.push_back(std::unique_ptr<IArgumentValue>(argument_value));
    });
  }

  static Handle<Value> New(V8Probe* probe);

  DEFINE_V8_CALLBACK(Fire)
  /*
   * This function extracts the data fired from JS and converts it into
   * the representation WinAPI expects.
   */
  void FillArguments(const Local<Array>& input);

  class eEmptyArrayPassed {};
  class eArrayTooLarge {};
  class eArgumentTypesMismatch {
  public:
    eArgumentTypesMismatch(int argument_index): m_failed_argument_number(argument_index) {}
    int m_failed_argument_number;
  };

private:
  template <class T, class S>
  inline void SetValueHelper(IArgumentValue* argument, const S& value) {
    T val = (T) value;
    argument->SetValue(&value);
  }

  ProbeArgumentsTypeInfo m_type_info;
  std::vector<std::unique_ptr<IArgumentValue>> m_argument_values;
};

class RealProvider;

/*
RealProbe class represents a single event/probe that was added.
It knows about the arguments the probe has and contains a storage for their values.
The storage is allocated only once and remains in this state until the probe is destroyed.
*/
class RealProbe {
public:
  RealProbe(const char* event_name, EVENT_DESCRIPTOR* descriptor, int arguments_number);
  RealProbe(const char* event_name, int event_id, int arguments_number);
  ~RealProbe();

  void Fire();
  bool IsBound() const;
  void Bind(const std::shared_ptr<RealProvider>& provider);
  void Unbind();

  /*
   * Stores the data so it could be used when writing events.
   */
  void SetArgumentValue(IArgumentValue* argument, unsigned int index) {
    EVENT_DATA_DESCRIPTOR* descriptor = m_payload_descriptors.get() + index;
    EventDataDescCreate(descriptor, argument->GetValue(), argument->GetSize());
  }

private:
  int m_arguments_number;
  EVENT_DESCRIPTOR m_event_descriptor;
  std::unique_ptr<EVENT_DATA_DESCRIPTOR[]> m_payload_descriptors;
  std::string m_event_name;
  std::shared_ptr<RealProvider> m_owner;
};

/*
 * Handles JSON serialization.
 */
class JSONHandler {
  Persistent<Object> m_json_holder;
  Persistent<Function> m_stringify_holder;

public:
  JSONHandler() {
    HandleScope scope;
    Handle<Context> context = Context::GetCurrent();
    Handle<Object> global = context->Global();
    Handle<Object> l_JSON = global->Get(String::New("JSON"))->ToObject();
    Handle<Function> l_JSON_stringify = Handle<Function>::Cast(l_JSON->Get(String::New("stringify")));

    WrapInPersistent(m_json_holder, l_JSON);
    WrapInPersistent(m_stringify_holder, l_JSON_stringify);
  }

  std::string Stringify(Handle<Value> value) {
    HandleScope scope;

    Handle<Value> args[1] = {value};

#ifndef BUILD_PRE011_COMPATIBILITY
    Local<Function> js_stringify = Local<Function>::New(Isolate::GetCurrent(), m_stringify_holder); 
    Local<Object> js_object = Local<Object>::New(Isolate::GetCurrent(), m_json_holder); 
    Local<Value> j = js_stringify->Call(js_object, 1, args);
#else
    Local<Value> j = m_stringify_holder->Call(m_json_holder, 1, args);
#endif

    String::AsciiValue str(j->ToString());
    return std::string(*str);    
  }
};