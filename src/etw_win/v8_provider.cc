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
#include "guid_generator.h"
#include "manifest_builder.h"
#include <limits>
#include <sstream>
#include <algorithm>
#include <stdio.h>

V8Provider::PayloadMap V8Provider::m_payload_map;
GuidGenerator g_guid_generator;

Handle<Value> V8Provider::New(const V8Arguments& args) {
  HandleScope scope;

  bool has_object = false;
  bool is_guid_generation_required = false;

  if(!args[0]->IsString()) { //If the first argument is not a string.
    if(args[0]->IsObject() && args.Length() == 1) { // Then it should be an object and only one.
      has_object = true; //Indicate it..
    } else { //..or generate an exception.
      throw eError("Invalid type passed, one object is expected");
    }
  } else if(args.Length() > 2) { //If first argument is a string, make sure that there is only one additional string argument.
    throw eError("The function can take only two string arguments");
  }

  std::string provider_name;
  std::string module_name;
  GUID provider_guid;

  if(has_object) { //If we have been given an object, handle it fields and extract the data.
    Local<String> guid_property_key = String::New(PROPERTY_GUID);
    Local<String> provider_name_property_key = String::New(PROPERTY_PROVIDER_NAME);
    Local<String> module_name_property_key = String::New(PROPERTY_MODULE_NAME);
    Handle<String> property_value;

    Local<Object> argument = args[0]->ToObject();

    bool has_guid = false;
    bool has_name = false;

    //Extract the GUID if we have it.
    if(argument->Has(guid_property_key)) {
      //If the property is not a string, throw an exception.
      if(!argument->Get(guid_property_key)->IsString()) {
        throw eError("Property value must be a string");
      } else {
        //Extract the string.
        property_value = Handle<String>::Cast(argument->Get(guid_property_key));
        String::AsciiValue string_guid(property_value);

        //And convert it into a GUID.
        long rc = UuidFromString((unsigned char*)*string_guid, &provider_guid);
        if (rc != RPC_S_OK) {
          throw eError("Incorrect GUID format");
        }
        has_guid = true;
      }
    }

    //Now try to extract the names.
    if(argument->Has(provider_name_property_key)) { 
      if(!argument->Get(provider_name_property_key)->IsString()) { 
        throw eError("Property value must be a string");
      } else {
        //Extract the provider name.
        property_value = Handle<String>::Cast(argument->Get(provider_name_property_key));
        String::AsciiValue provider(property_value);
        provider_name = *provider;

        //Check is the user passed the optional module name.
        if(argument->Has(module_name_property_key)) {
          if(!argument->Get(module_name_property_key)->IsString()) {
            throw eError("Property value must be a string");
          } else {
            //Extract the module name string.
            property_value = Handle<String>::Cast(argument->Get(module_name_property_key));
            String::AsciiValue module(property_value);
            module_name = *module;
          }
        }
        has_name = true;
      }
    }

    //If we have no GUID and no names, it is an error.
    if(!has_guid) { 
      if(!has_name) {
        throw eError("Specify name(s) or a GUID to create a provider");
      } else { //If we have no GUID, but have the names (or at lease one) - indicate that we can and need generate the guid.
        is_guid_generation_required = true;
      }      
    }

  } else { //If we have been given the string names...
    String::AsciiValue provider(args[0]->ToString()); //Get the provider name first.
    provider_name = *provider;

    //Check if the module name exists and it is valid.
    if (args.Length() == 2) {
      if (!args[1]->IsString()) {
        throw eError("Argument 2 is expected to be a string");
      } else { //If it is, extract it.
        String::AsciiValue module(args[1]->ToString());
        module_name = *module;
      }
    }
    is_guid_generation_required = true;
  }

  if(is_guid_generation_required) {
    std::pair<GUID, std::string> result = g_guid_generator.GenerateGuidFromNames(provider_name, module_name);
    provider_guid = result.first;
  }

  try {
    g_manifest_builder.MakeProviderRecord(provider_guid, provider_name);
  } catch (const ManifestBuilder::eRecordExists&) {
    throw eError("Provider with this GUID has already been created");
  }

  V8Provider* v8_provider = new V8Provider(new RealProvider(provider_guid));
  //And expose it to JavaScript.
  v8_provider->Wrap(args.This());
  return args.This();
}

Handle<Value> V8Provider::AddProbe(const V8Arguments& args) {
  HandleScope scope;
  EVENT_DESCRIPTOR descriptor;

  bool is_descriptor_passed = false;

  if (args.Length() < 2) {
    throw eError("Must have at least 2 arguments");
  }

  if (!args[0]->IsString()) {
    throw eError("Argument 1 must be event name as string");
  }

  if (!args[1]->IsArray()) {
    if (!args[1]->IsString()) {
      throw eError("Argument 2 must be descriptor array or string");
    }
  } else {
    Local<Array> descriptor_array = Local<Array>::Cast(args[1]);
    if (descriptor_array->Length() != DESCRIPTOR_ARRAY_SIZE) {
      throw eError("Event descriptor argument must be array of 7 int");
    }

    //TODO: Add check for integer type in the array.
    descriptor.Id = descriptor_array->Get(0)->ToInt32()->Value();
    descriptor.Version = descriptor_array->Get(1)->ToInt32()->Value();
    descriptor.Channel = descriptor_array->Get(2)->ToInt32()->Value();
    descriptor.Level = descriptor_array->Get(3)->ToInt32()->Value();
    descriptor.Opcode = descriptor_array->Get(4)->ToInt32()->Value();
    descriptor.Task = descriptor_array->Get(5)->ToInt32()->Value();
    descriptor.Keyword = descriptor_array->Get(6)->ToInteger()->Value();

    is_descriptor_passed = true;
  }

  int utility_arguments_number;
  if(is_descriptor_passed) {
    utility_arguments_number = 2;
  } else {
    utility_arguments_number = 1;
  }

  //Get the actual number of types passed minus 1 for the probe name and 1 for the descriptor (if exists).
  int number_of_types = args.Length() - utility_arguments_number;

  ProbeArgumentsTypeInfo payload_type;

  if(number_of_types <= 0) {
    throw eError("The probe must have at least one type argument");
  }

  //Recognize the types.
  for (int i = 0; i < number_of_types; i++) {
    if (!args[i + utility_arguments_number]->IsString()) {
      throw eError("Data types arguments must be strings");
    }

    String::AsciiValue string_type(args[i + utility_arguments_number]->ToString());
    EventPayloadType type = m_payload_map.ExtractPayloadType((char*)*string_type);

    if (type == EDT_UNKNOWN) {
      throw eError("Data types argument must be of a recognized type");
    }
    payload_type.AddType(type);
  }

  String::AsciiValue str(args[0]->ToString());
  const char* probe_name = *str;

  Handle<Object> wrapped_provider = args.Holder();
  V8Provider* provider = ObjectWrap::Unwrap<V8Provider>(wrapped_provider);

  std::unique_ptr<RealProbe> probe;
  EVENT_DESCRIPTOR* p_descriptor = nullptr;

  if(is_descriptor_passed) {
    p_descriptor = &descriptor;
  } 

  probe.reset(provider->GetImplementation()->AddProbe(probe_name, p_descriptor, payload_type));
  probe->Bind(provider->GetSharedImplementation());

  Handle<Value> wrapped_probe = V8Probe::New(new V8Probe(probe.release(), payload_type));

  //Associate the probe with the provider using its name as the key.
  wrapped_provider->Set(args[0]->ToString(), wrapped_probe);

  return scope.Close(wrapped_probe);
}

Handle<Value> V8Provider::RemoveProbe(const V8Arguments& args) {
  HandleScope scope;

  if (!args[0]->IsString()) {
    throw eError("Argument 1 must be event name as string");
  }

  Handle<Object> wrapped_provider = args.Holder();
  V8Provider* provider = ObjectWrap::Unwrap<V8Provider>(wrapped_provider);

  Handle<Object> wrapped_probe = Local<Object>::Cast(wrapped_provider->Get(args[0]));
  V8Probe* probe = ObjectWrap::Unwrap<V8Probe>(wrapped_probe);

  provider->GetImplementation()->RemoveProbe(probe->GetImplementation());
  wrapped_provider->Delete(args[0]->ToString());

  return scope.Close(Undefined());
}

Handle<Value> V8Provider::Enable(const V8Arguments& args) {
  V8Provider* provider = ObjectWrap::Unwrap<V8Provider>(args.Holder());

  try {
    provider->GetImplementation()->Enable();
  } catch(const eError& ex) {
    throw(ex);
  }

  return Undefined();
}

Handle<Value> V8Provider::Disable(const V8Arguments& args) {
  V8Provider* provider = ObjectWrap::Unwrap<V8Provider>(args.Holder());
  provider->GetImplementation()->Disable();
  return Undefined();
}

Handle<Value> V8Provider::Fire(const V8Arguments& args) {
  HandleScope scope;

  if (!args[0]->IsString()) {
    throw eError("Must give event name as first argument");
  }

  if (!args[1]->IsFunction()) {
    throw eError("Must give event value callback as second argument");
  }

  Handle<Object> wrapped_provider = args.Holder();
  V8Provider* provider = ObjectWrap::Unwrap<V8Provider>(wrapped_provider);

  if(!wrapped_provider->Has(args[0]->ToString())) {
    throw eError("Probe does not exist");
  }

  Handle<Object> wrapped_probe = Local<Object>::Cast(wrapped_provider->Get(args[0]));
  V8Probe* probe = ObjectWrap::Unwrap<V8Probe>(wrapped_probe);

  if(!probe) {
    throw eError("Failed to unwrap the probe");
  }

  Local<Function> js_callback = Local<Function>::Cast(args[1]);

  Local<Value> event_args;
  {
    TryCatch try_catch;

    event_args = js_callback->Call(GetObjectHandle(provider), 0, nullptr);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
      return scope.Close(Undefined());
    }
  }

  if (event_args->IsArray()) {
    Local<Array> argument_values = Local<Array>::Cast(event_args);

    try {
      probe->FillArguments(argument_values);
    } catch(const V8Probe::eArgumentTypesMismatch& ex) {
      std::stringstream formatter;
      formatter << "Invalid argument type passed in array index " << ex.m_failed_argument_number;
      throw eError(formatter.str());
    } catch (const V8Probe::eArrayTooLarge&) {
      throw eError("The number of entries in the array returned exceeds the number of arguments the probe expects");
    } catch (const V8Probe::eEmptyArrayPassed&) {
      return scope.Close(Undefined());
    }
  } else {
    throw eError("The callback must return an array");
  }

  try {
    provider->GetImplementation()->Fire(probe->GetImplementation());
  } catch (const eError& ex) {
    throw(ex);
  }

  return scope.Close(Undefined());
}

Persistent<FunctionTemplate> V8TemplateHolder::m_provider_creator;
Persistent<FunctionTemplate> V8TemplateHolder::m_probe_creator;

void V8TemplateHolder::InitProvider(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(GET_V8_CALLBACK(V8Provider, New));
  t->SetClassName(String::New(PROVIDER_CREATOR_FUNCTION_NAME));
  t->InstanceTemplate()->SetInternalFieldCount(1);

  WrapInPersistent(m_provider_creator, t);

  ExposeMethod(t, ADD_PROBE_FUNCTION_NAME, GET_V8_CALLBACK(V8Provider, AddProbe));
  ExposeMethod(t, REMOVE_PROBE_FUNCTION_NAME, GET_V8_CALLBACK(V8Provider, RemoveProbe));
  ExposeMethod(t, ENABLE_FUNCTION_NAME, GET_V8_CALLBACK(V8Provider, Enable));
  ExposeMethod(t, DISABLE_FUNCTION_NAME, GET_V8_CALLBACK(V8Provider, Disable));
  ExposeMethod(t, FIRE_FUNCTION_NAME, GET_V8_CALLBACK(V8Provider, Fire));
}

void V8TemplateHolder::InitProbe(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New();
  t->SetClassName(String::New(PROBE_CLASS_NAME));
  t->InstanceTemplate()->SetInternalFieldCount(1);

  WrapInPersistent(m_probe_creator, t);
  ExposeMethod(t, FIRE_FUNCTION_NAME, GET_V8_CALLBACK(V8Probe, Fire));
}

void V8TemplateHolder::ExposeModuleInterface(Handle<Object> target) {
  target->Set(String::New(PROVIDER_CREATOR_FUNCTION_NAME), FunctionTemplate::New(GET_V8_CALLBACK(V8TemplateHolder, NewProviderInstance))->GetFunction());
  target->Set(String::New(GUID_FROM_NAMES_FUNCTION_NAME), FunctionTemplate::New(GET_V8_CALLBACK(V8TemplateHolder, GuidFromNames))->GetFunction());
}

Handle<Value> V8TemplateHolder::NewProviderInstance(const V8Arguments& args) {
  HandleScope scope;

  const int argc = args.Length();

  std::vector<Handle<Value>> argv(argc);
  for(int i = 0; i < args.Length(); i++) {
    argv[i] = args[i];
  }

  Local<v8::FunctionTemplate> pers_obj = ExtractFromPersistent(m_provider_creator);
  Local<Object> instance = pers_obj->GetFunction()->NewInstance(argc, &argv[0]);

  return scope.Close(instance);
}

Handle<Value> V8TemplateHolder::GuidFromNames(const V8Arguments& args) {
  HandleScope scope;

  bool has_module_name = false;
  switch(args.Length()) {
    case 0:
      throw eError("Expected at least one string for the provider name");
    case 1:
      has_module_name = false; 
      break;
    case 2:
      has_module_name = true;
      break;
    default:
      throw eError("Function doesn't take more than 2 arguments");
  }

  if(!args[0]->IsString()) {
    throw eError("Invalid argument passed, a string is expected");
  }

  String::AsciiValue provider(args[0]->ToString());
  std::string provider_name(*provider);
  std::string module_name;

  if(has_module_name) {
    if(!args[1]->IsString()) {
      throw eError("Invalid argument passed, a string is expected");
    }
    String::AsciiValue module(args[1]->ToString());
    module_name = *module;
  }

  std::pair<GUID, std::string> result = g_guid_generator.GenerateGuidFromNames(provider_name, module_name);

  return scope.Close(String::New(result.second.c_str()));
}

//////////////////////////////////////////////////////////////////////////
//Init method called when the module is loaded.
//Need to declare as a dll export.
extern "C" {
  __declspec(dllexport)
    void init (Handle<Object> target) {
      V8TemplateHolder::InitProvider(target);
      V8TemplateHolder::InitProbe(target);
      V8TemplateHolder::ExposeModuleInterface(target);
  }

  NODE_MODULE(TraceProviderBindings, init);
};
