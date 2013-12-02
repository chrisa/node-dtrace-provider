#include "../shared_defs.h"
#include "dtrace_provider.h"
#include <v8.h>

#include <node.h>
#include <stdio.h>

#define NAMES_FROM_GUID_FUNCTION_NAME "namesFromGuid"

namespace node {

  using namespace v8;

  DTraceProvider::DTraceProvider() : ObjectWrap() {
    provider = NULL;
  }
  
  DTraceProvider::~DTraceProvider() {
    usdt_provider_disable(provider);
    usdt_provider_free(provider);
  }

  bool DTraceProvider::GenerateNamesFromGuid(std::string guid, std::pair<std::string, std::string>& result) {
    //Remove the group separators.
    while(true) {
      std::string::iterator position = std::find(guid.begin(), guid.end(), '-');
      if(position == guid.end()) {
        break;
      }
      guid.erase(position);
    }

    //The fixed GUID string is expected to contain exactly 32 characters.
    if(guid.length() != 32) {
      return false;
    }

    //Prepare the names.
    result = std::pair<std::string, std::string>("provider_", "module_");

    //Concat the prepared names and the GUID: the first half of the GUID goes for the provider name, the second half goes for the module name.
    result.first += guid.substr(0, 16);
    result.second += guid.substr(16, 16);

    return true;
  }

  Persistent<FunctionTemplate> DTraceProvider::constructor_template;

  void DTraceProvider::Initialize(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(DTraceProvider::New);
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol(PROVIDER_CREATOR_FUNCTION_NAME));

    NODE_SET_PROTOTYPE_METHOD(constructor_template, ADD_PROBE_FUNCTION_NAME, DTraceProvider::AddProbe);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, REMOVE_PROBE_FUNCTION_NAME, DTraceProvider::RemoveProbe);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, ENABLE_FUNCTION_NAME, DTraceProvider::Enable);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, DISABLE_FUNCTION_NAME, DTraceProvider::Disable);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, FIRE_FUNCTION_NAME, DTraceProvider::Fire);

    target->Set(String::NewSymbol(PROVIDER_CREATOR_FUNCTION_NAME), constructor_template->GetFunction());
    target->Set(String::NewSymbol(NAMES_FROM_GUID_FUNCTION_NAME), FunctionTemplate::New(NamesFromGuid)->GetFunction());

    DTraceProbe::Initialize(target);
  }

  Handle<Value> DTraceProvider::NamesFromGuid(const v8::Arguments& args) {
    HandleScope scope;
    if(args.Length() != 1 || !args[0]->IsString()) {
      return ThrowException(Exception::Error(String::New(
        "Expected only one string argument")));
    }

    String::AsciiValue guid(args[0]->ToString());
    std::string guid_name(*guid);

    std::pair<std::string, std::string> result;
    if(!GenerateNamesFromGuid(guid_name, result)) {
      return ThrowException(Exception::Error(String::New(
        "Incorrect GUID format given!")));
    }

    Handle<Array> result_array = Array::New(2);
    result_array->Set(0, String::New(result.first.c_str()));
    result_array->Set(1, String::New(result.second.c_str()));

    return scope.Close(result_array);
  }

  Handle<Value> DTraceProvider::New(const Arguments& args) {
    HandleScope scope;
    DTraceProvider *p = new DTraceProvider();
    char module[128];

    bool has_object = false;

    p->Wrap(args.This());

    if (args.Length() < 1) {
      return ThrowException(Exception::Error(String::New(
        "Must give provider name or object with the guid, provider_name and module_name properties as argument")));
    }

    if(!args[0]->IsString()) {
      has_object = true;
    }

    std::string provider_name;
    std::string module_name;

    //There may be 3 different cases here.

    //CASE 1: We have not been given an object with properties, handle the string names as before.
    if(!has_object) { 
      String::AsciiValue name(args[0]->ToString());

      if (args.Length() == 2) {
        if (!args[1]->IsString()) {
          return ThrowException(Exception::Error(String::New(
            "Must give module name as argument")));
        }

        String::AsciiValue mod(args[1]->ToString());
        (void) snprintf(module, sizeof (module), "%s", *mod);
      } else if (args.Length() == 1) {
        // If no module name is provided, develop a synthetic module name based
        // on our address
        (void) snprintf(module, sizeof (module), "mod-%p", p);
      } else {
        return ThrowException(Exception::Error(String::New(
          "Expected only provider name and module as arguments")));
      }

      provider_name = *name;
      module_name = module;
    } else { //Otherwise, extract data from the object.     
      Local<String> guid_property_key = String::New(PROPERTY_GUID);
      Local<String> provider_name_property_key = String::New(PROPERTY_PROVIDER_NAME);
      Local<String> module_name_property_key = String::New(PROPERTY_MODULE_NAME);
      Handle<String> property_value;

      Local<Object> argument = args[0]->ToObject();

      //CASE 2: We have been given an object with the names and maybe a GUID. Just extract the names and ignore the GUID in this case.
      if(argument->Has(provider_name_property_key)) { 
        if(!argument->Get(provider_name_property_key)->IsString()) { 
          return ThrowException(Exception::Error(String::New(
            "Property value must be a string")));
        } else {
          //Extract the provider name.
          property_value = Handle<String>::Cast(argument->Get(provider_name_property_key));
          String::AsciiValue provider(property_value);
          provider_name = *provider;

          //Check is the user passed the optional module name.
          if(argument->Has(module_name_property_key)) {
            if(!argument->Get(module_name_property_key)->IsString()) {
              return ThrowException(Exception::Error(String::New(
                "Property value must be a string")));
            } else {
              //Extract the module name string.
              property_value = Handle<String>::Cast(argument->Get(module_name_property_key));
              String::AsciiValue module(property_value);
              module_name = *module;
            }
          }
        }
      } else { //CASE 3: We have been given an object, but it doesn't contain the properties for the names.
        //Check if we have the GUID first.
        if(argument->Has(guid_property_key)) {
          //If the property is not a string, throw an exception.
          if(!argument->Get(guid_property_key)->IsString()) {
            return ThrowException(Exception::Error(String::New(
              "Property value must be a string")));
          } else {
            //Extract the string.
            property_value = Handle<String>::Cast(argument->Get(guid_property_key));
            String::AsciiValue string_guid(property_value);
            std::string guid(*string_guid);
            std::pair<std::string, std::string> result;

            if(!GenerateNamesFromGuid(guid, result)) {
              return ThrowException(Exception::Error(String::New(
                "Incorrect GUID format given!")));
            }
          
            provider_name = result.first;
            module_name = result.second;
          }
        }
      }
    }

    if ((p->provider = usdt_create_provider(provider_name.c_str(), module_name.c_str())) == NULL) {
      return ThrowException(Exception::Error(String::New(
        "usdt_create_provider failed")));
    }

    return args.This();
  }

  Handle<Value> DTraceProvider::AddProbe(const Arguments& args) {
    HandleScope scope;
    const char *types[USDT_ARG_MAX];

    Handle<Object> obj = args.Holder();
    DTraceProvider *provider = ObjectWrap::Unwrap<DTraceProvider>(obj);

    // create a DTraceProbe object
    Handle<Function> klass = DTraceProbe::constructor_template->GetFunction();
    Handle<Object> pd = Local<Object>::New(klass->NewInstance());

    // store in provider object
    DTraceProbe *probe = ObjectWrap::Unwrap<DTraceProbe>(pd->ToObject());
    obj->Set(args[0]->ToString(), pd);

    int index_offset = 1;

    //If we have received a descriptor array for ETW - ignore it and adjust the index.
    if(args.Length() >= 2 && args[1]->IsArray()) {
      index_offset = 2;
    }

    // add probe to provider
    for (int i = 0; i < USDT_ARG_MAX; i++) {
      if (i < args.Length() - index_offset) {
        String::AsciiValue type(args[i + index_offset]->ToString());

        if (strncmp("json", *type, 4) == 0)
          probe->arguments[i] = new DTraceJsonArgument();
        else if (strncmp("char *", *type, 6) == 0 || 
                 strncmp("wchar_t *", *type, 9) == 0)
          probe->arguments[i] = new DTraceStringArgument();
        else if (strncmp("int", *type, 3) == 0 || 
                 strncmp("int8", *type, 4) == 0 ||
                 strncmp("int16", *type, 5) == 0 ||
                 strncmp("int32", *type, 5) == 0 ||
                 strncmp("int64", *type, 5) == 0 ||
                 strncmp("uint", *type, 4) == 0 ||
                 strncmp("uint8", *type, 5) == 0 ||
                 strncmp("uint16", *type, 6) == 0 ||
                 strncmp("uint32", *type, 6) == 0 ||
                 strncmp("uint64", *type, 6) == 0)
          probe->arguments[i] = new DTraceIntegerArgument();
        else
          probe->arguments[i] = new DTraceStringArgument();

        types[i] = strdup(probe->arguments[i]->Type());
        probe->argc++;
      }
    }

    String::AsciiValue name(args[0]->ToString());
    probe->probedef = usdt_create_probe(*name, *name, probe->argc, types);
    usdt_provider_add_probe(provider->provider, probe->probedef);

    for (size_t i = 0; i < probe->argc; i++) {
      free((char *)types[i]);
    }

    return pd;
  }

  Handle<Value> DTraceProvider::RemoveProbe(const Arguments& args) {
    HandleScope scope;

    Handle<Object> provider_obj = args.Holder();
    DTraceProvider *provider = ObjectWrap::Unwrap<DTraceProvider>(provider_obj);

    Handle<Object> probe_obj = Local<Object>::Cast(args[0]);
    DTraceProbe *probe = ObjectWrap::Unwrap<DTraceProbe>(probe_obj);

    Handle<String> name = String::New(probe->probedef->name);
    provider_obj->Delete(name);

    if (usdt_provider_remove_probe(provider->provider, probe->probedef) != 0)
      return ThrowException(Exception::Error(String::New(usdt_errstr(provider->provider))));

    return True();
  }

  Handle<Value> DTraceProvider::Enable(const Arguments& args) {
    HandleScope scope;
    DTraceProvider *provider = ObjectWrap::Unwrap<DTraceProvider>(args.Holder());

    if (usdt_provider_enable(provider->provider) != 0)
      return ThrowException(Exception::Error(String::New(usdt_errstr(provider->provider))));

    return Undefined();
  }

  Handle<Value> DTraceProvider::Disable(const Arguments& args) {
    HandleScope scope;
    DTraceProvider *provider = ObjectWrap::Unwrap<DTraceProvider>(args.Holder());

    if (usdt_provider_disable(provider->provider) != 0)
      return ThrowException(Exception::Error(String::New(usdt_errstr(provider->provider))));

    return Undefined();
  }

  Handle<Value> DTraceProvider::Fire(const Arguments& args) {
    HandleScope scope;

    if (!args[0]->IsString()) {
      return ThrowException(Exception::Error(String::New(
        "Must give probe name as first argument")));
    }

    if (!args[1]->IsFunction()) {
      return ThrowException(Exception::Error(String::New(
        "Must give probe value callback as second argument")));
    }

    Handle<Object> provider = args.Holder();
    Handle<Object> probe = Local<Object>::Cast(provider->Get(args[0]));

    DTraceProbe *p = ObjectWrap::Unwrap<DTraceProbe>(probe);
    if (p == NULL)
      return Undefined();

    p->_fire(args[1]);

    return True();
  }

  extern "C" void
  init(Handle<Object> target) {
    DTraceProvider::Initialize(target);
  }

  NODE_MODULE(TraceProviderBindings, init)
} // namespace node
