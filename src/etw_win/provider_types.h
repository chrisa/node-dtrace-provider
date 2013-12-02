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
#include <list>
#include <functional>
/*
A set of named constants for argument types.
*/
enum EventPayloadType {
  EDT_STRING,
  EDT_WSTRING,
  EDT_JSON,
  EDT_INT8,
  EDT_INT32,
  EDT_INT16,
  EDT_INT64,
  EDT_UINT8,
  EDT_UINT32,
  EDT_UINT16,
  EDT_UINT64,
  EDT_UNKNOWN
};

/*
A helper class used as a map for probe arguments. 
Each probe may have different sets of payload arguments 
and in order to reflect this difference each probe has 
its own instance of this class.
*/
class ProbeArgumentsTypeInfo {
  int m_filled_arguments_number;
  std::list<EventPayloadType> m_type_info;
public:
  ProbeArgumentsTypeInfo(): m_filled_arguments_number(0) {}
  ProbeArgumentsTypeInfo(ProbeArgumentsTypeInfo&& op): m_filled_arguments_number(0) {
    m_type_info = std::move(op.m_type_info);
  }
  void operator = (ProbeArgumentsTypeInfo&& op) {
    m_type_info = std::move(op.m_type_info);
  }

  void AddType(EventPayloadType type) {
    m_type_info.push_back(type);
  }

  void DoForEach(std::function<void(EventPayloadType)> action) const {
    for(auto type : m_type_info) {
      action(type);
    }
  }

  int GetArgsNumber() const {
    return m_type_info.size();
  }

  int GetFilledArgumentsNumber() const {
    return m_filled_arguments_number;
  }

  void SetFilledArgumentsNumber(int filled_number) {
    m_filled_arguments_number = filled_number;
  }
};

/*
A storage interface for the values received from JS payload arguments.
It is type-agnostic and can be used for any argument types.
*/
class IArgumentValue {
public:
  virtual ULONG GetSize() = 0;
  virtual void* GetValue() = 0;
  virtual void SetValue(const void*) = 0;
  virtual void SetDefaultValue() = 0;
  virtual ~IArgumentValue() {}
};

/*
This class is used to store arguments of different types.
If a type requires any special handling, template specification must be used.
*/
template <typename T> class ArgumentValue: public IArgumentValue {
  T m_value;
  bool m_is_dirty;
public:
  ArgumentValue(const T& value = T()): m_is_dirty(false) { //m_value will be value-initialized by default.
    m_value = value;
  }
  ~ArgumentValue() {}
  void SetValue(const void* value);
  void SetDefaultValue();
  void* GetValue();
  ULONG GetSize();
};

template <typename T>
inline void* ArgumentValue<T>::GetValue() {
  return static_cast<void*>(&m_value);
}

template <>
inline void* ArgumentValue<std::wstring>::GetValue() {
  return (void*)(m_value.c_str());
}

template <>
inline void* ArgumentValue<std::string>::GetValue() {
  return (void*)(m_value.c_str());
}

template <typename T>
inline ULONG ArgumentValue<T>::GetSize() {
  return sizeof(T);
}

template <>
inline ULONG ArgumentValue<std::string>::GetSize() {
  return (m_value.length() + 1) * sizeof(char);
}

template <>
inline ULONG ArgumentValue<std::wstring>::GetSize() {
  return (m_value.length() + 1) * sizeof(wchar_t);
}

template <typename T>
inline void ArgumentValue<T>::SetValue(const void* value) {
  m_value = *((T*)value);
  m_is_dirty = true;
}

template <>
inline void ArgumentValue<std::string>::SetValue(const void* value) {
  m_value = (char*)value;
  m_is_dirty = true;
}

template <>
inline void ArgumentValue<std::wstring>::SetValue(const void* value) {
  m_value = (wchar_t*)value;
  m_is_dirty = true;
}

template <typename T>
inline void ArgumentValue<T>::SetDefaultValue() {
  if(m_is_dirty) {
    m_value = T();
    m_is_dirty = false;
  } 
}