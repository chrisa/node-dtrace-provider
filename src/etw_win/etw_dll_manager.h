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
#include <evntprov.h>
#define MAKE_ERROR_MESSAGE(error_msg) "Failed to write ETW event: "##error_msg
/*
 * Handles all interactions with WinAPI.
 */
class EtwDllManager {
  typedef ULONG (NTAPI *EventRegisterFunc)(LPCGUID ProviderId, PENABLECALLBACK EnableCallback, PVOID CallbackContext, PREGHANDLE RegHandle);
  typedef ULONG (NTAPI *EventUnregisterFunc)(REGHANDLE RegHandle);
  typedef ULONG (NTAPI *EventWriteFunc)(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount,PEVENT_DATA_DESCRIPTOR UserData);

  std::unordered_map<RealProvider*, REGHANDLE> m_provider_storage;

  HMODULE m_dll_handle;

  EventRegisterFunc m_event_register_ptr;
  EventUnregisterFunc m_event_unregister_ptr;
  EventWriteFunc m_event_write_ptr;

  // This callback is called by ETW when the consumers of our provider are enabled or disabled.
  // The callback is dispatched on the ETW thread.
  static void NTAPI EtwEnableCallback(
    LPCGUID SourceId,
    ULONG IsEnabled,
    UCHAR Level,
    ULONGLONG MatchAnyKeyword,
    ULONGLONG MatchAllKeywords,
    PEVENT_FILTER_DESCRIPTOR FilterData,
    PVOID CallbackContext) {
      if (CallbackContext) {
        RealProvider* provider = (RealProvider*)CallbackContext;
        provider->OnEtwStatusChanged(IsEnabled);
      }
  }

  bool InitLibrary() {
    m_dll_handle = LoadLibrary("advapi32.dll");
    if (m_dll_handle) {
      m_event_register_ptr = (EventRegisterFunc) GetProcAddress(m_dll_handle, "EventRegister");
      m_event_unregister_ptr = (EventUnregisterFunc) GetProcAddress(m_dll_handle, "EventUnregister");
      m_event_write_ptr = (EventWriteFunc) GetProcAddress(m_dll_handle, "EventWrite");
    }

    if (!m_dll_handle || !m_event_register_ptr || !m_event_unregister_ptr || !m_event_write_ptr) {
      throw eError("Failed trying to load Windows ETW API");
    }
    return true;
  }

public:
  EtwDllManager(): m_dll_handle(nullptr), m_event_register_ptr(nullptr), m_event_unregister_ptr(nullptr), m_event_write_ptr(nullptr) {}

  bool IsRegistered(RealProvider* provider) {
    auto iter = m_provider_storage.find(provider);
    if(iter == m_provider_storage.end()) { 
      return false; //Most likely, the provider has been disabled.
    } else {
      return true;
    }
  }

  void Release() {
    if (m_dll_handle) {
      FreeLibrary(m_dll_handle);
      m_dll_handle = 0;
      m_event_register_ptr = nullptr;
      m_event_unregister_ptr = nullptr;
      m_event_write_ptr = nullptr;
    }
  }

  void Enable(UUID* guid, RealProvider* provider) {
    //Already registered. 
    if (!m_dll_handle) {
      if(!InitLibrary()) {
        throw eError("Failed to init the library");
      }
    }

    auto iter = m_provider_storage.find(provider);
    if(iter != m_provider_storage.end()) {
      return;
    }

    REGHANDLE handle;
    DWORD status = m_event_register_ptr(guid, EtwEnableCallback, provider, &handle);
    if (status != ERROR_SUCCESS) {
      throw eError("Failed trying to register ETW event provider");
    }

    m_provider_storage.insert(std::make_pair(provider, handle));
    return;
  }

  void Disable(RealProvider* provider) {
    auto iter = m_provider_storage.find(provider);
    if(iter == m_provider_storage.end()) {
      return; //If the specified provider is not in the storage, it has already been disabled, do nothing.
    }

    if (m_dll_handle && m_event_unregister_ptr) {
      m_event_unregister_ptr(iter->second);
      iter->second = 0;
    }

    m_provider_storage.erase(provider); //Just remove the pointer and let the V8 garbage collector deal with the object.
  }

  void Write(RealProvider* provider, PCEVENT_DESCRIPTOR descriptor, ULONG user_data_count, PEVENT_DATA_DESCRIPTOR user_data) {
    auto iter = m_provider_storage.find(provider);
    if(iter == m_provider_storage.end()) {
      throw eError("provider is not found");
    }

    ULONG result = m_event_write_ptr(iter->second, descriptor, user_data_count, user_data);
    const char* error;
    switch(result) {
      case ERROR_SUCCESS:
        return;
      case ERROR_INVALID_PARAMETER:
        error = MAKE_ERROR_MESSAGE("one or more of the parameters is not valid");;
        break;
      case ERROR_INVALID_HANDLE:
        error = MAKE_ERROR_MESSAGE("the registration handle of the provider is not valid");
        break;
      case ERROR_ARITHMETIC_OVERFLOW:
        error = MAKE_ERROR_MESSAGE("the event size is larger than the allowed maximum (64k - header)");
        break;
      case ERROR_MORE_DATA:
        error = MAKE_ERROR_MESSAGE("the session buffer size is too small for the event");
        break;
      case ERROR_NOT_ENOUGH_MEMORY:
        error = MAKE_ERROR_MESSAGE("Occurs when filled buffers are trying to flush to disk, but disk IOs are not happening fast enough. This happens when the disk is slow and event traffic is heavy. Eventually, there are no more free (empty) buffers and the event is dropped.");
        break;
      case ERROR_LOG_FILE_FULL:
        error = MAKE_ERROR_MESSAGE("The real-time playback file is full. Events are not logged to the session until a real-time consumer consumes the events from the playback file.");
        break;
    }

    throw eError(error);
  }
};