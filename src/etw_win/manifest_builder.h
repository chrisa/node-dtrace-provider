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
#include <string>
#include <memory>
#include "provider_types.h"
#include <xmllite.h>
#include "v8_compatibility.h"

#include <atlbase.h>

#pragma comment(lib, "XmlLite.lib")

/*
 * This class keeps information about providers and their probes which is used to generate the manifest.
 * It is completely independent of the V8 garbage collector and the lifetimes of the providers/probes registered.
 * It is expected to be a unique global object.
 */
class ManifestBuilder { 
  class eNotSingle{};

  class ProbeInfo {
  public:
    std::string m_name;
    int m_id;
    ProbeArgumentsTypeInfo m_type_info;
    ProbeInfo(const std::string name, int id, const ProbeArgumentsTypeInfo& type_info): m_name(name), m_id(id), m_type_info(type_info) {}
  };

  class ProviderInfo {
  public:
    GUID m_guid;
    std::string m_name;
    std::list<ProbeInfo> m_associated_probes;
    ProviderInfo(const GUID& guid, const std::string& name): m_guid(guid), m_name(name) {}
  };

  /*
   * Handles binary->text GUID conversion imposing RAII safety.
   */
  class GuidConverter {
    UCHAR* guid_string;
    std::string converted_guid;
  public:
    GuidConverter(const GUID& guid) {
      UuidToString(&guid, &guid_string);
    }
    ~GuidConverter() {
      RpcStringFree(&guid_string);
    }
    std::string GetResult() {
      return std::string((char*)guid_string);
    }
  };

  class XmlDocument;

  /*
   * Represents a node, linked with a specific XmlDocument.
   * Stores information about its attributes and child nodes.
   */
  class XmlNode {
  public:
    std::string m_name;
    std::list<std::pair<std::string, std::string>> m_attributes;
    XmlDocument* m_owner;
    std::list<std::unique_ptr<XmlNode>> m_children;

    XmlNode(XmlDocument* owner, const std::string& node_name): m_owner(owner), m_name(node_name) {}
    void SetAttribute(const std::string& key, const std::string& value) {
      m_attributes.push_back(std::pair<std::string, std::string> (key, value));
    }

    void AddString(const std::string& id, const std::string& value) {
      XmlNode* string = AddNewNode("string");
      string->SetAttribute("id", id);
      string->SetAttribute("value", value);
    }

    XmlNode* AddNewNode(const std::string& node_name) {
      m_children.push_back(std::unique_ptr<XmlNode>(new XmlNode(m_owner, node_name)));
      return m_children.back().get();
    }
  };

  /*
   * Implements an xml file with nodes.
   * The node data is stored in RAM and is actually written when the Generate() function is called.
   */
  class XmlDocument {
    CComPtr<IXmlWriter> m_xml_writer;
    CComPtr<IStream> m_output_stream;
    std::string m_manifest_filename;
    std::unique_ptr<XmlNode> m_root_node;
    bool m_is_first;

    std::wstring MakeWString(const std::string& string) {
      std::wstring result(string.length(), L'#');
      mbstowcs(&result[0], string.c_str(), string.length());
      return result;
    }

    std::string MakeErrorStringFromCode(const std::string& function_name, HRESULT result) {
      char format[128];
      sprintf_s(format, (function_name + " failed with code %08.8lx").c_str(), result);
      return format;
    }

    void WriteNamespace(const std::string& prefix, const std::string& name, const std::string& uri) {
      HRESULT result = m_xml_writer->WriteAttributeString(MakeWString(prefix).c_str(), MakeWString(name).c_str(), nullptr, MakeWString(uri).c_str());
      if(S_OK != result) {
        throw eError(MakeErrorStringFromCode("WriteAttributeString", result));
      }
    }

    void WriteAllAttributes(XmlNode* node) {
      for(auto attribute : node->m_attributes) {
        std::wstring wstr_key = MakeWString(attribute.first);
        std::wstring wstr_val = MakeWString(attribute.second);
        HRESULT result = m_xml_writer->WriteAttributeString(0, wstr_key.c_str(), 0, wstr_val.c_str());
        if(S_OK != result) {
          throw eError(MakeErrorStringFromCode("WriteAttributeString", result));
        }
      }
    }

    void WriteStartDocument() {
      HRESULT result;
      result = SHCreateStreamOnFile(m_manifest_filename.c_str(), STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_WRITE, &m_output_stream);
      if(S_OK != result) {
        throw eError(MakeErrorStringFromCode("SHCreateStreamOnFile", result));
      }

      if(S_OK != m_xml_writer->SetOutput(m_output_stream)) {
        throw eError(MakeErrorStringFromCode("SetOutput", result));
      }

      if(S_OK != m_xml_writer->WriteStartDocument(XmlStandalone_Omit)) {
        throw eError(MakeErrorStringFromCode("WriteStartDocument", result));
      }
    }

    void WriteEndDocument() {
      HRESULT result = m_xml_writer->WriteEndDocument();
      if(S_OK != result) {
        throw eError(MakeErrorStringFromCode("WriteEndDocument", result));
      }
    }

    void WriteStartNode(XmlNode* node) {
      wchar_t* nspace = nullptr;

      if(m_is_first) {
        nspace = L"http://schemas.microsoft.com/win/2004/08/events";      
      }

      HRESULT result = m_xml_writer->WriteStartElement(0, MakeWString(node->m_name).c_str(), nspace);
      if(S_OK != result) {
        throw eError(MakeErrorStringFromCode("WriteStartElement", result));
      }

      if(m_is_first) {
        m_is_first = false;
        WriteNamespace("xmlns", "xsi", "http://www.w3.org/2001/XMLSchema-instance");
        WriteNamespace("xmlns", "xs", "http://www.w3.org/2001/XMLSchema");
        WriteNamespace("xmlns", "win", "http://manifests.microsoft.com/win/2004/08/windows/events");
        WriteNamespace("xmlns", "trace", "http://schemas.microsoft.com/win/2004/08/events/trace");
        WriteNamespace("xsi", "schemaLocation", "http://schemas.microsoft.com/win/2004/08/events eventman.xsd");
      }
    }

    void WriteEndNode() {
      HRESULT result = m_xml_writer->WriteEndElement();
      if(S_OK != result) {
        throw eError(MakeErrorStringFromCode("WriteEndElement", result));
      }
    }

    void WriteNode(XmlNode* node) {
      WriteStartNode(node);
      WriteAllAttributes(node);
      for(auto iter = node->m_children.begin(); iter != node->m_children.end(); ++iter) {
        WriteNode((*iter).get());
      }
      WriteEndNode();
    }

  public:
    XmlDocument(CComPtr<IXmlWriter>& writer, const std::string& filename, const std::string& root_node_name): m_xml_writer(writer), m_manifest_filename(filename), m_is_first(true), m_root_node(new XmlNode(this, root_node_name)) {}

    XmlNode* GetRootNode() {
      return m_root_node.get();
    }
    /*
     * Writes the data into a file.
     */
    void Generate() {
      WriteStartDocument();
      WriteNode(m_root_node.get());
      WriteEndDocument();
    }
  };

  std::list<ProviderInfo> m_all_providers;
  CComPtr<IXmlWriter> m_xml_writer;
  static ManifestBuilder* m_instance;

  void UpdateManifestForProvider(const ProviderInfo& provider);
public:
  ManifestBuilder() {
    if(m_instance) {
      throw eNotSingle();
    } else {
      m_instance = this;
    }

    if(S_OK != CreateXmlWriter(__uuidof(IXmlWriter), (void**)&m_xml_writer, nullptr)) {
      printf("CreateXmlWriter failed\n");
    }

    if(S_OK != m_xml_writer->SetProperty(XmlWriterProperty_Indent, TRUE)) {
      printf("SetProperty failed\n");
    }
  }

  class eRecordExists {};

  void MakeProviderRecord(const GUID& guid, const std::string& name="");
  void MakeProbeRecord(const GUID&, const std::string&, int, const ProbeArgumentsTypeInfo&);
};

extern ManifestBuilder g_manifest_builder;