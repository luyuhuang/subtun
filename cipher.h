#pragma once

#include <openssl/evp.h>

#include <string>

typedef std::basic_string<unsigned char> ustring;

ustring cmac(const ustring &key, const uint8_t *data, size_t len);
ustring cmac(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len);

struct aes_128_cbc_cmac {
	using key_type = ustring;

	static const size_t iv_size = 16;
	static const size_t mac_size = 16;
	static const size_t min_cap = iv_size + mac_size + EVP_MAX_BLOCK_LENGTH;

	size_t encrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap);
	size_t decrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap);
};

struct aes_128_gcm {
	using key_type = ustring;

	static const size_t iv_size = 16;
	static const size_t tag_size = 16;
	static const size_t min_cap = iv_size + tag_size + EVP_MAX_BLOCK_LENGTH;

	size_t encrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap);
	size_t decrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap);
};
