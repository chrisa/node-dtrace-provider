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
#include "manifest_builder.h"
#include <utility>
#include <algorithm>

EtwDllManager g_dll_manager;

RealProvider::RealProvider(const GUID& guid): m_enabled_status(false), m_max_event_id(1) {
  memcpy(&m_guid, &guid, sizeof(GUID));
}

RealProvider::~RealProvider() {
  m_enabled_status = false;
  g_dll_manager.Disable(this);
}

RealProbe* RealProvider::AddProbe(const char* event_name, EVENT_DESCRIPTOR* descriptor, ProbeArgumentsTypeInfo& datatypes) {
  int result_id;

  if(descriptor) {
    if (descriptor->Id >= m_max_event_id) {
      m_max_event_id = descriptor->Id + 1;
    }
    result_id = descriptor->Id;
  } else {
    result_id = m_max_event_id;
  }

  try {
    g_manifest_builder.MakeProbeRecord(m_guid, event_name, result_id, datatypes);
  } catch(const ManifestBuilder::eRecordExists&) {
    throw eError("A probe with this name already exists");
  }

  std::unique_ptr<RealProbe> probe;

  if(descriptor) {
    probe.reset(new RealProbe(event_name, descriptor, datatypes.GetArgsNumber()));
  } else {
    probe.reset(new RealProbe(event_name, m_max_event_id, datatypes.GetArgsNumber()));   
    m_max_event_id++;
  }
  
  return probe.release();
}

bool RealProvider::RemoveProbe(RealProbe* probe) {
  probe->Unbind();
  return true;
}

void RealProvider::Enable(void) {
  try {
    g_dll_manager.Enable(&m_guid, this);
  } catch (const eError& ex) {
    throw(ex);
  }
}

void RealProvider::Disable() {
  g_dll_manager.Disable(this);
}

void RealProvider::Fire(RealProbe* probe) {
  if (!m_enabled_status) {
    return;
  }

  try {
    probe->Fire();
  } catch(const eError& ex) {
    throw(ex);
  }
}