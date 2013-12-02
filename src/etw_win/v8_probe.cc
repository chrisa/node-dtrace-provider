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
#include "etw_provider.h"
#include "etw_dll_manager.h"
#include "provider_types.h"
#include <sstream>

extern EtwDllManager g_dll_manager;
JSONHandler g_json_handler;

Handle<Value> V8Probe::New(V8Probe* probe) {
  HandleScope scope;
  Local<FunctionTemplate> pers_obj = ExtractFromPersistent(V8TemplateHolder::m_probe_creator);
  Local<Object> instance = pers_obj->GetFunction()->NewInstance();
  probe->Wrap(instance);
  return scope.Close(instance);
}

Handle<Value> V8Probe::Fire(const V8Arguments& args) {
  HandleScope scope;

  V8Probe* probe = ObjectWrap::Unwrap<V8Probe>(args.Holder());

  if(!probe->GetImplementation()->IsBound()) {
    throw eError("Calling fire on a removed probe");
  }

  if (!args[0]->IsFunction()) {
    throw eError("Must give event value callback as second argument");
  }

  Local<Function> cb = Local<Function>::Cast(args[0]);

  Local<Value> event_args;
  {
    TryCatch try_catch;

    event_args = cb->Call(GetObjectHandle(probe), 0, nullptr);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
      return scope.Close(Undefined());
    }
  }

  if (event_args->IsArray()) {
    Local<Array> a = Local<Array>::Cast(event_args);

    try {
      probe->FillArguments(a);
    } catch (const eArgumentTypesMismatch& ex) {
      std::stringstream formatter;
      formatter << "Invalid argument type passed in array index " << ex.m_failed_argument_number;
      throw eError(formatter.str());
    } catch (const eArrayTooLarge&) {
      throw eError("The number of entries in the array returned exceeds the number of arguments the probe expects");
    } catch (const eEmptyArrayPassed&) {
      return scope.Close(Undefined());
    }

    try {
      probe->GetImplementation()->Fire();
    } catch(const eError& ex) {
      throw(ex);
    }
  }

  return scope.Close(Undefined());
}

void V8Probe::FillArguments(const Local<Array>& input) {
  uint32_t arguments_number = input->Length();
  //Uncomment this code to generate an exception when the user fires an empty array or an array with more arguments than the probe expects.
  /*
  if(!arguments_number) {
    throw eEmptyArrayPassed();
  }

  if(arguments_number > m_type_info.GetArgsNumber()) {
    throw eArrayTooLarge();
  }
  */
  int current_index = 0;
  const int last_argument_index = arguments_number - 1;

  m_type_info.DoForEach([&](EventPayloadType type) {
    IArgumentValue* arg_val = m_argument_values[current_index].get();

    //If the user has passed less values in the array than the probe expects - do nothing, these values will be default-initialized.
    if(current_index <= last_argument_index) { 
      Local<Value> argument_value = input->Get(current_index);
      bool is_type_check_passed = true;

      switch (type) {
        case EDT_STRING:
        case EDT_WSTRING:
          if(!argument_value->IsString()) is_type_check_passed = false;
          break;
        case EDT_JSON:
          if(!argument_value->IsObject()) is_type_check_passed = false;
          break;
        case EDT_INT32:
        case EDT_INT8:
        case EDT_INT16:
          if(!argument_value->IsInt32()) is_type_check_passed = false;
          break;
        case EDT_UINT32:
        case EDT_UINT8:
        case EDT_UINT16:
          if(!argument_value->IsUint32()) is_type_check_passed = false;
          break;
        case EDT_INT64:
        case EDT_UINT64:
          if(!argument_value->IsNumber()) is_type_check_passed = false; //There is no IsInteger() method, this one is the best we can do.
          break;
        default:
          break;
      }

      if(!is_type_check_passed) {
        throw eArgumentTypesMismatch(current_index);
      }

      switch (type) {
        case EDT_STRING: {
          String::AsciiValue val(argument_value->ToString());
          arg_val->SetValue(*val);
          break;
        }
        case EDT_WSTRING: {
          String::Value val(argument_value->ToString());
          arg_val->SetValue(*val);
          break;
        }
        case EDT_JSON: {
          std::string val = g_json_handler.Stringify(argument_value);
          arg_val->SetValue(val.c_str());
          break;
        }
        case EDT_INT32: SetValueHelper<int32_t>(arg_val, argument_value->ToInt32()->Value()); break;
        case EDT_INT8: SetValueHelper<int8_t>(arg_val, argument_value->ToInt32()->Value()); break;	
        case EDT_INT16: SetValueHelper<int16_t>(arg_val, argument_value->ToInt32()->Value()); break;	
        case EDT_INT64: SetValueHelper<int64_t>(arg_val, argument_value->ToInteger()->Value()); break;	
        case EDT_UINT32: SetValueHelper<uint32_t>(arg_val, argument_value->ToUint32()->Value()); break;
        case EDT_UINT8: SetValueHelper<uint8_t>(arg_val, argument_value->ToUint32()->Value()); break;		
        case EDT_UINT16: SetValueHelper<uint16_t>(arg_val, argument_value->ToUint32()->Value()); break;		
        case EDT_UINT64: SetValueHelper<uint64_t>(arg_val, argument_value->ToInteger()->Value()); break;	
        default:
          break;
      }
    } else {
      //Normally we don't get here. This branch is executed when few or no values have been returned by fire().
      //Updating the internal storage with default values is necessary to clean up the values stored from previous fires with the right number of values.
      arg_val->SetDefaultValue();
    }

    GetImplementation()->SetArgumentValue(arg_val, current_index);
    current_index++;
  });

  m_type_info.SetFilledArgumentsNumber(current_index);
}


//////////////////////////////////////////////////////////////////////////

RealProbe::RealProbe(const char* event_name, EVENT_DESCRIPTOR* desc, int arguments_number): m_owner(nullptr), m_event_name(event_name), m_payload_descriptors(new EVENT_DATA_DESCRIPTOR[arguments_number]), m_arguments_number(arguments_number) {
  memcpy(&m_event_descriptor, desc, sizeof(EVENT_DESCRIPTOR));
}

RealProbe::RealProbe(const char* event_name, int event_id, int arguments_number): m_owner(nullptr), m_event_name(event_name), m_payload_descriptors(new EVENT_DATA_DESCRIPTOR[arguments_number]), m_arguments_number(arguments_number) {
  ZeroMemory(&m_event_descriptor, sizeof(EVENT_DESCRIPTOR));
  m_event_descriptor.Id = event_id;
}

bool RealProbe::IsBound() const {
  return m_owner ? true : false;
}

void RealProbe::Bind(const std::shared_ptr<RealProvider>& provider) {
  m_owner = provider;
}

void RealProbe::Unbind() {
  m_owner.reset();
}
//The probe implementation is not shared. When the containing object is deleted it will 
//trigger the RealProbe destructor which will allow the owning provider implementation to be deleted.
//It is safe for as long as the containing object doesn't have any data required by the implementation.
RealProbe::~RealProbe() {
  Unbind();
}

void RealProbe::Fire() {
  if (!g_dll_manager.IsRegistered(m_owner.get())) {
    return;
  }

  try {
    g_dll_manager.Write(m_owner.get(), &m_event_descriptor, m_arguments_number, m_payload_descriptors.get());
  } catch (const eError& ex) {
    throw(ex);
  }
}