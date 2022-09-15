#pragma once

#include <openssl/evp.h>

#include <string>

typedef std::basic_string<unsigned char> ustring;

struct aes_128_gcm {
	using key_type = ustring;

	static const size_t key_size = 16;
	static const size_t iv_size = 12;
	static const size_t tag_size = 16;
	static const size_t block_size = 1;
	static const size_t min_cap = iv_size + tag_size + block_size - 1;

	size_t encrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap);
	size_t decrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap);
};

struct chacha20_poly1305 {
	using key_type = ustring;

	static const size_t key_size = 32;
	static const size_t iv_size = 12;
	static const size_t tag_size = 16;
	static const size_t block_size = 1;
	static const size_t min_cap = iv_size + tag_size + block_size - 1;

	size_t encrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap);
	size_t decrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap);
};