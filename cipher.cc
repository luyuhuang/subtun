#include "cipher.h"

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <string>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <cassert>

using std::string;
using std::runtime_error;
using std::logic_error;
using std::to_string;

size_t aes_128_gcm::encrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap) {
	thread_local static EVP_CIPHER_CTX *ctx = nullptr;

	if (cap < len + min_cap) throw runtime_error("cap is not large enough");

	int n;
	size_t ans = 0;

	if (!ctx) {
		if (!(ctx = EVP_CIPHER_CTX_new()))
			goto error;
		if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr))
			goto error;

		assert(EVP_CIPHER_CTX_key_length(ctx) == key_size);
		assert(EVP_CIPHER_CTX_iv_length(ctx) == iv_size);
		assert(EVP_CIPHER_CTX_block_size(ctx) == block_size);
	}

	if (key_size != key.size())
		throw runtime_error("key length is wrong. expect " + to_string(key_size) + " got " + to_string(key.size()));

	RAND_bytes(encrypted, iv_size);
	if (!EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.c_str(), encrypted))
		goto error;

	encrypted += iv_size;
	ans += iv_size;

	if (!EVP_EncryptUpdate(ctx, encrypted, &n, data, len))
		goto error;

	encrypted += n;
	ans += n;

	if (!EVP_EncryptFinal(ctx, encrypted, &n))
		goto error;

	encrypted += n;
	ans += n;

	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag_size, encrypted))
		goto error;

	return ans + tag_size;

error:
	auto err = ERR_get_error();
	thread_local static char buf[256];
	ERR_error_string_n(err, buf, sizeof(buf));
	throw runtime_error(buf);
}

size_t aes_128_gcm::decrypt(const key_type& key, const uint8_t* data, size_t len, uint8_t* decrypted, size_t cap) {
	thread_local static EVP_CIPHER_CTX* ctx = nullptr;
	thread_local static uint8_t tag[tag_size];

	int n0, n1;

	if (!ctx) {
		if (!(ctx = EVP_CIPHER_CTX_new()))
			goto error;
		if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr))
			goto error;

		assert(EVP_CIPHER_CTX_key_length(ctx) == key_size);
		assert(EVP_CIPHER_CTX_iv_length(ctx) == iv_size);
		assert(EVP_CIPHER_CTX_block_size(ctx) == block_size);
	}

	if (key_size != key.size())
		throw runtime_error("key length is wrong. expect " + to_string(key_size) + " got " + to_string(key.size()));

	if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.c_str(), data))
		goto error;

	data += iv_size;
	len -= iv_size + tag_size;

	if (!EVP_DecryptUpdate(ctx, decrypted, &n0, data, len))
		goto error;

	memcpy(tag, data + len, tag_size);
	if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_size, tag))
		goto error;

	if (!EVP_DecryptFinal(ctx, decrypted + n0, &n1))
		goto error;

	return n0 + n1;
error:
	auto err = ERR_get_error();
	thread_local static char buf[256];
	ERR_error_string_n(err, buf, sizeof(buf));
	throw runtime_error(buf);
}

size_t chacha20_poly1305::encrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap) {
	thread_local static EVP_CIPHER_CTX *ctx = nullptr;

	if (cap < len + min_cap) throw runtime_error("cap is not large enough");

	int n;
	size_t ans = 0;

	if (!ctx) {
		if (!(ctx = EVP_CIPHER_CTX_new()))
			goto error;
		if (!EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr))
			goto error;

		assert(EVP_CIPHER_CTX_key_length(ctx) == key_size);
		assert(EVP_CIPHER_CTX_iv_length(ctx) == iv_size);
		assert(EVP_CIPHER_CTX_block_size(ctx) == block_size);
	}

	if (key_size != key.size())
		throw runtime_error("key length is wrong. expect " + to_string(key_size) + " got " + to_string(key.size()));

	RAND_bytes(encrypted, iv_size);
	if (!EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.c_str(), encrypted))
		goto error;

	encrypted += iv_size;
	ans += iv_size;

	if (!EVP_EncryptUpdate(ctx, encrypted, &n, data, len))
		goto error;

	encrypted += n;
	ans += n;

	if (!EVP_EncryptFinal(ctx, encrypted, &n))
		goto error;

	encrypted += n;
	ans += n;

	if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, tag_size, encrypted))
		goto error;

	return ans + tag_size;

error:
	auto err = ERR_get_error();
	thread_local static char buf[256];
	ERR_error_string_n(err, buf, sizeof(buf));
	throw runtime_error(buf);
}

size_t chacha20_poly1305::decrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap) {
	thread_local static EVP_CIPHER_CTX *ctx = nullptr;
	thread_local static uint8_t tag[tag_size];

	int n0, n1;

	if (!ctx) {
		if (!(ctx = EVP_CIPHER_CTX_new()))
			goto error;
		if (!EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr))
			goto error;

		assert(EVP_CIPHER_CTX_key_length(ctx) == key_size);
		assert(EVP_CIPHER_CTX_iv_length(ctx) == iv_size);
		assert(EVP_CIPHER_CTX_block_size(ctx) == block_size);
	}

	if (key_size != key.size())
		throw runtime_error("key length is wrong. expect " + to_string(key_size) + " got " + to_string(key.size()));

	if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.c_str(), data))
		goto error;

	data += iv_size;
	len -= iv_size + tag_size;

	if (!EVP_DecryptUpdate(ctx, decrypted, &n0, data, len))
		goto error;

	memcpy(tag, data + len, tag_size);
	if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, tag_size, tag))
		goto error;

	if (!EVP_DecryptFinal(ctx, decrypted + n0, &n1))
		goto error;

	return n0 + n1;
error:
	auto err = ERR_get_error();
	thread_local static char buf[256];
	ERR_error_string_n(err, buf, sizeof(buf));
	throw runtime_error(buf);
}
