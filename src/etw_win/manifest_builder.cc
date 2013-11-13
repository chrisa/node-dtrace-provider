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
#include "manifest_builder.h"
#include <algorithm>

ManifestBuilder g_manifest_builder;
ManifestBuilder* ManifestBuilder::m_instance = nullptr;

void ManifestBuilder::MakeProviderRecord(const GUID& guid, const std::string& name) {
  //Check if this GUID has already been recorded.
  auto provider = std::find_if(m_all_providers.begin(), m_all_providers.end(), [&](const ProviderInfo& info) {
    if(info.m_guid == guid) {
      return true;
    } else {
      return false;
    }
  });
  //If it's not been - make the record.
  if(provider == m_all_providers.end()) {
    m_all_providers.push_back(ProviderInfo(guid, name));
  } else { 
    throw eRecordExists(); //Otherwise, throw an exception: each provider is expected to have its own unique GUID.
  }
}

void ManifestBuilder::MakeProbeRecord(const GUID& owning_provider_guid, const std::string& name, int id, const ProbeArgumentsTypeInfo& arguments) {
  //Using the GUID, find the provider to which this probe belongs.
  auto provider = std::find_if(m_all_providers.begin(), m_all_providers.end(), [&](const ProviderInfo& info) {
    if(info.m_guid == owning_provider_guid) {
      return true;
    } else {
      return false;
    }
  });

  //If the provider has not been found, return.
  if(provider == m_all_providers.end()) {
    return;
  }

  //Check if there is a probe with this name...
  auto probe = std::find_if(provider->m_associated_probes.begin(), provider->m_associated_probes.end(), [&](const ProbeInfo& info) {
    if(info.m_name == name) {
      return true;
    } else {
      return false;
    }
  });

  //If there is no such a probe, make the record; throw an exception otherwise.
  if(probe == provider->m_associated_probes.end()) {
    provider->m_associated_probes.push_back(ProbeInfo(name, id, arguments));
  } else {
    throw eRecordExists();
  }
  //Now update the manifest for this provider.
  UpdateManifestForProvider(*provider);
}

void ManifestBuilder::UpdateManifestForProvider(const ProviderInfo& provider) {
  GuidConverter guid_converter(provider.m_guid);
  std::string guid = guid_converter.GetResult();

  XmlDocument provider_manifest(m_xml_writer, "provider_" + guid + ".man", "instrumentationManifest");
  XmlNode* provider_node = provider_manifest.GetRootNode()->AddNewNode("instrumentation")->AddNewNode("events")->AddNewNode("provider");

  //Add the provider attributes.
  if(provider.m_name.empty()) {
    provider_node->SetAttribute("name", "Node-ETWProvider");
  } else {
    provider_node->SetAttribute("name", provider.m_name);
  }

  provider_node->SetAttribute("guid", std::string("{" + guid + "}"));
  provider_node->SetAttribute("symbol", "ProviderGuid");
  provider_node->SetAttribute("resourceFileName", "");
  provider_node->SetAttribute("messageFileName", "");

  //Create child nodes inside <provider> 
  XmlNode* templates_node = provider_node->AddNewNode("templates");
  XmlNode* events_node = provider_node->AddNewNode("events");
  XmlNode* keywords_node = provider_node->AddNewNode("keywords");
  XmlNode* tasks_node = provider_node->AddNewNode("tasks");
  XmlNode* opcodes_node = provider_node->AddNewNode("opcodes");

  //Go through each probe that belongs to this provider
  for(auto probe : provider.m_associated_probes) {
    //Create <template> for each probe, make the tid attribute equal to the name of the probe.    
    XmlNode* template_node = templates_node->AddNewNode("template");
    std::string tid = probe.m_name + "_tid";
    template_node->SetAttribute("tid", tid);
    int argument_index = 1;
    //Go through each argument data type the probe has...
    probe.m_type_info.DoForEach([&](EventPayloadType payload_type) {
      //And create the according <data> element for each argument type in the <template> element of the probe.
      XmlNode* data_node = template_node->AddNewNode("data");
      data_node->SetAttribute("name", "Argument" + std::to_string(argument_index++));

      const char* string_type = nullptr;
      //Based on the argument data type, set according attribute value in the <data> element.
      switch(payload_type) {
        case EDT_JSON:
        case EDT_STRING: string_type = "win:AnsiString"; break;
        case EDT_WSTRING: string_type = "win:UnicodeString"; break;
        case EDT_INT32: string_type = "win:Int32"; break;
        case EDT_INT8: string_type = "win:Int8"; break;
        case EDT_INT16: string_type = "win:Int16"; break;
        case EDT_INT64: string_type = "win:Int64"; break;
        case EDT_UINT32: string_type = "win:UInt32"; break;
        case EDT_UINT8: string_type = "win:UInt8"; break;
        case EDT_UINT16: string_type = "win:UInt16"; break;
        case EDT_UINT64: string_type = "win:UInt64"; break;
      }

      if(string_type)
        data_node->SetAttribute("inType", string_type);
    });

    std::string string_id = std::to_string(probe.m_id);

    //Add <event> inside <events> and fill it with values.
    XmlNode* event_node = events_node->AddNewNode("event");
    event_node->SetAttribute("value", string_id);
    event_node->SetAttribute("version", "0");
    event_node->SetAttribute("template", tid);
    event_node->SetAttribute("opcode", "opcode_probe_" + string_id);
    event_node->SetAttribute("level", "win:Informational");
    event_node->SetAttribute("task", probe.m_name);
    event_node->SetAttribute("keywords", "probe_keyword");
    event_node->SetAttribute("message", "$(string.event_message)");

    //Add <task> node inside <tasks>.
    XmlNode* task_node = tasks_node->AddNewNode("task");
    task_node->SetAttribute("value", string_id);
    task_node->SetAttribute("name", probe.m_name);
    task_node->SetAttribute("message", "$(string.probe_" + string_id + "_task_message)");

    //Attach <opcode>.
    XmlNode* opcode_node = opcodes_node->AddNewNode("opcode");
    opcode_node->SetAttribute("value", string_id);
    opcode_node->SetAttribute("name", "opcode_probe_" + string_id);
    opcode_node->SetAttribute("message", "$(string.probe_" + string_id + "_opcode_message)");
  }

  //Fill <keywords>.
  XmlNode* keyword_node = keywords_node->AddNewNode("keyword");
  keyword_node->SetAttribute("name", "probe_keyword");
  keyword_node->SetAttribute("mask", "0x2");
  keyword_node->SetAttribute("message", "$(string.probe_keyword_message)");

  //Create <localization> inside <instrumentationManifest> and <resources> inside <localization> to store strings.
  XmlNode* resources_node = provider_manifest.GetRootNode()->AddNewNode("localization")->AddNewNode("resources");
  resources_node->SetAttribute("culture", "en-US");
  XmlNode* string_table_node = resources_node->AddNewNode("stringTable");
  //Add opcode messages for tasks and opcodes.
  for(auto probe : provider.m_associated_probes) {
    std::string string_id = std::to_string(probe.m_id);
    string_table_node->AddString("probe_" + string_id + "_opcode_message", probe.m_name);
    string_table_node->AddString("probe_" + string_id + "_task_message", probe.m_name);
  }
  //Add event and keyword messages. 
  string_table_node->AddString("event_message", "Node.js probe");
  string_table_node->AddString("probe_keyword_message", "Node.js probe");

  //Finally, write the file.
  provider_manifest.Generate();
}