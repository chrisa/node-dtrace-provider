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
#include <WinCrypt.h>
#include <string>
/*
 * This class helps to generate the guid from the names.
 */
class GuidGenerator {
  HCRYPTPROV m_provider_handle;
  HCRYPTHASH m_hash_handle;
  bool m_is_context_acquired;
  bool m_is_hash_created;

  void Md5Start() {
    if (!CryptAcquireContext(&m_provider_handle, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
      throw eError("CryptAcquireContext failed, error = " + std::to_string(GetLastError()));
    } else {
      m_is_context_acquired = true;
    }

    if (!CryptCreateHash(m_provider_handle, CALG_MD5, 0, 0, &m_hash_handle)) {
      throw eError("CryptAcquireContext failed, error = " + std::to_string(GetLastError())); 
    } else {
      m_is_hash_created = true;
    }
  }

  void Md5Update(const unsigned char* data, const int size) {
    if (!CryptHashData(m_hash_handle, data, size, 0)) {
      throw eError("CryptHashData failed, error = " + std::to_string(GetLastError())); 
    }
  }

  void Md5Finish(unsigned char* output) {
    DWORD hash_buffer_size = 16;
    if (!CryptGetHashParam(m_hash_handle, HP_HASHVAL, output, &hash_buffer_size, 0)) {
      throw eError("CryptGetHashParam failed, error = " + std::to_string(GetLastError()));
    }

    CryptDestroyHash(m_hash_handle);
    CryptReleaseContext(m_provider_handle, 0);
  }

  void GenerateMD5Hash(const unsigned char* input, int input_size, unsigned char* output) {
    m_is_context_acquired = false;
    m_is_hash_created = false;
    try {
      Md5Start();
      Md5Update(input, input_size);
      Md5Finish(output);
    } catch (const eError& ex) {
      if(m_is_hash_created) {
        CryptDestroyHash(m_hash_handle);
      }
      if(m_is_context_acquired) {
        CryptReleaseContext(m_provider_handle, 0);
      }
      throw eError("Unable to generate names from GUID: " + ex.error_string);
    }
  }

  template<typename T> T DoEndianConvert(T input) {
    static_assert(std::numeric_limits<T>::is_integer, "attempting to do endian convert on a non-integer type");

    if(sizeof(T) == 1)
      return input;

    T result;

    int8_t* result_ptr = (int8_t*) &result;
    int8_t* input_ptr = (int8_t*) &input;

    int last_byte_index = sizeof(T) - 1;
    for(int i = 0; i < sizeof(T); i++) {
      int8_t tail_index = last_byte_index - i;
      result_ptr[i] = input_ptr[tail_index];
    }

    return result;
  }

public:
  GuidGenerator(): m_provider_handle(0), m_hash_handle(0), m_is_context_acquired(false), m_is_hash_created(false) {}

  std::pair<GUID, std::string> GenerateGuidFromNames(const std::string& provider_name, const std::string& module_name) {
    static_assert(sizeof(GUID) >= 16, "sizeof(GUID) is expected to be at least 16 bytes");
    //Concat the names of the provider and the module.
    std::string input = provider_name;

    if(!module_name.empty()) {
      input += module_name;
    }

    unsigned char raw_hash[16];

    std::pair<GUID, std::string> result;

    //Generate a GUID.
    GenerateMD5Hash((const unsigned char*)input.c_str(), input.length(), raw_hash);
    memcpy(&result.first, raw_hash, sizeof(GUID));

    //Data1 is a 4-bytes long integer, Data2 and Data3 are 2-bytes long integer, Data4 is just an array of bytes, no endian conversion is required.
    result.first.Data1 = DoEndianConvert<decltype(result.first.Data1)>(result.first.Data1);
    result.first.Data2 = DoEndianConvert<decltype(result.first.Data2)>(result.first.Data2);
    result.first.Data3 = DoEndianConvert<decltype(result.first.Data3)>(result.first.Data3);

    for(int i = 0; i < sizeof(raw_hash); i++) {
      //Convert each by hex value to its text upper case representation.
      char text_byte[3];
      sprintf_s(text_byte, 3, "%02X", raw_hash[i]);
      //... and append it to the result string.
      result.second.append(text_byte, 2);
      //Use '-' to separate the groups in the GUID.
      switch(i + 1) {
        case 4:
        case 6:
        case 8:
        case 10:
          result.second += '-';
        }
    }

    return result;
  }
};