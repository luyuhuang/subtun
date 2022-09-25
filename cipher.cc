#include "cipher.h"

#include <openssl/evp.h>
#include <openssl/err.h>

#include <string>
#include <stdexcept>
#include <cassert>

using std::string;
using std::runtime_error;
using std::logic_error;
using std::to_string;

size_t aes_128_gcm::encrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap, uint8_t *tag) {
	thread_local static EVP_CIPHER_CTX *ctx = nullptr;

	if (cap < len + padding_size) throw runtime_error("cap is not large enough");

	int n0, n1;

	if (!ctx) {
		if (!(ctx = EVP_CIPHER_CTX_new()))
			goto error;
		if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr))
			goto error;

		assert(EVP_CIPHER_CTX_key_length(ctx) == key_size);
		assert(EVP_CIPHER_CTX_iv_length(ctx) == iv_size);
		assert(EVP_CIPHER_CTX_block_size(ctx) == block_size);
	}

	if (!EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, iv))
		goto error;

	if (!EVP_EncryptUpdate(ctx, encrypted, &n0, data, len))
		goto error;

	if (!EVP_EncryptFinal(ctx, encrypted + n0, &n1))
		goto error;

	if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag_size, tag))
		goto error;

	return n0 + n1;

error:
	auto err = ERR_get_error();
	thread_local static char buf[256];
	ERR_error_string_n(err, buf, sizeof(buf));
	throw runtime_error(buf);
}

size_t aes_128_gcm::decrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap, uint8_t *tag) {
	thread_local static EVP_CIPHER_CTX *ctx = nullptr;

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

	if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, key, iv))
		goto error;

	if (!EVP_DecryptUpdate(ctx, decrypted, &n0, data, len))
		goto error;

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

size_t chacha20_poly1305::encrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap, uint8_t *tag) {
	thread_local static EVP_CIPHER_CTX *ctx = nullptr;

	if (cap < len + padding_size) throw runtime_error("cap is not large enough");

	int n0, n1;

	if (!ctx) {
		if (!(ctx = EVP_CIPHER_CTX_new()))
			goto error;
		if (!EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr))
			goto error;

		assert(EVP_CIPHER_CTX_key_length(ctx) == key_size);
		assert(EVP_CIPHER_CTX_iv_length(ctx) == iv_size);
		assert(EVP_CIPHER_CTX_block_size(ctx) == block_size);
	}

	if (!EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, iv))
		goto error;

	if (!EVP_EncryptUpdate(ctx, encrypted, &n0, data, len))
		goto error;

	if (!EVP_EncryptFinal(ctx, encrypted + n0, &n1))
		goto error;

	if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, tag_size, tag))
		goto error;

	return n0 + n1;

error:
	auto err = ERR_get_error();
	thread_local static char buf[256];
	ERR_error_string_n(err, buf, sizeof(buf));
	throw runtime_error(buf);
}

size_t chacha20_poly1305::decrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap, uint8_t *tag) {
	thread_local static EVP_CIPHER_CTX *ctx = nullptr;

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

	if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, key, iv))
		goto error;

	if (!EVP_DecryptUpdate(ctx, decrypted, &n0, data, len))
		goto error;

	if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, tag_size, tag))
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
