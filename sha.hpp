#pragma once

#include <string>
#include <openssl/sha.h>

std::string sha256_string(std::string to_hash) {
  char outputBuffer[65];
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, to_hash.c_str(), to_hash.length());
  SHA256_Final(hash, &sha256);
  int i = 0;
  for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
  }
  return std::string(outputBuffer);
}
