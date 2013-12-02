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
#define BUILDING_NODE_EXTENSION
#include <node.h>
#include <evntprov.h>
#include <string>
#include <unordered_map>
#include "../shared_defs.h"
#include "provider_types.h"
#include "etw_probe.h"
#include "v8_compatibility.h"

using namespace node;
using namespace v8;

#define GUID_FROM_NAMES_FUNCTION_NAME "guidFromNames"

class RealProvider;
/*
An intermediary class used to abstract from V8 API differences.
It handles all V8-specific actions and calls the appropriate methods
of the implementation to handle platform-specific actions.
*/
class V8Provider: public ExposableImplementation<RealProvider> {
private:
  /*
  * PayloadMap converts string literals received as the first argument from the addProbe JS function to named constants.
  * This class is also needed to screen off all invalid (unsupported) argument types received from the addProbe function.
  */
  class PayloadMap {
    std::unordered_map<std::string, EventPayloadType> m_data_map;
  public:
    PayloadMap() {
      m_data_map["char *"] = EDT_STRING;		
      m_data_map["wchar_t *"] = EDT_WSTRING;
      m_data_map["json"] = EDT_JSON;
      m_data_map["int"] = EDT_INT32;
      m_data_map["int32"] = EDT_INT32;
      m_data_map["int8"] = EDT_INT8;
      m_data_map["int16"] = EDT_INT16;
      m_data_map["int64"] = EDT_INT64;
      m_data_map["uint"] = EDT_UINT32;
      m_data_map["uint32"] = EDT_UINT32;
      m_data_map["uint8"] = EDT_UINT8;
      m_data_map["uint16"] = EDT_UINT16;
      m_data_map["uint64"] = EDT_UINT64;
    }

    EventPayloadType ExtractPayloadType(const char* string_type) const {		
      auto iter = m_data_map.find(string_type);
      if(iter == m_data_map.end())
        return EDT_UNKNOWN;
      else
        return (*iter).second;
    }
  };

  static PayloadMap m_payload_map;
  enum {
    DESCRIPTOR_ARRAY_SIZE = 7
  };
  
public:
  DEFINE_V8_CALLBACK(New)
  DEFINE_V8_CALLBACK(AddProbe)
  DEFINE_V8_CALLBACK(RemoveProbe)
  DEFINE_V8_CALLBACK(Enable)
  DEFINE_V8_CALLBACK(Disable)
  DEFINE_V8_CALLBACK(Fire)

  V8Provider(RealProvider* implementation): ExposableImplementation(implementation) {}
  ~V8Provider() {}
};

/*
Represents one ETW provider.
The class handles all platform-specific actions 
and is completely V8-agnostic.
*/
class RealProvider {
public:
  RealProvider(const GUID& guid);
  ~RealProvider();

  RealProbe* AddProbe(const char* event_name, EVENT_DESCRIPTOR* descriptor, ProbeArgumentsTypeInfo& datatypes);
  bool RemoveProbe(RealProbe* probe);
  void Enable();
  void Disable();
  void Fire(RealProbe* probe);
  /*
   * The callback is triggered by EtwDllManager whenever the state of the provider is changed.
   */
  void OnEtwStatusChanged(bool is_enabled) {
    m_enabled_status = is_enabled;
  }

private:
  bool m_enabled_status;
  int m_max_event_id;
  GUID m_guid;
};

/*
 * This class stores templates for the creator functions.
 * It is also used to expose API to JS.
 */
class V8TemplateHolder {
public:
  static Persistent<FunctionTemplate> m_provider_creator;
  static Persistent<FunctionTemplate> m_probe_creator;

  /*
   * This method is invoked each time someone tries to create a provider from JS.
   * A probe is never created in this manner and there is no corresponding method for it.
   */
  DEFINE_V8_CALLBACK(NewProviderInstance)
  /*
  * The callback for the guidFromNames function exposed to JS to generate a GUID from the names.
  */
  DEFINE_V8_CALLBACK(GuidFromNames)
  /*
   * Prepares a template for the provider and exposes the creator function for it.
   */
  static void InitProvider(Handle<Object> target);
  /*
   * Prepares a template for the probe. This template is used when the provider creates a probe.
   */
  static void InitProbe(Handle<Object> target);
  /*
   * Exposes the methods the user can use from JS on the object returned by require().
   */
  static void ExposeModuleInterface(Handle<Object> target);
};
