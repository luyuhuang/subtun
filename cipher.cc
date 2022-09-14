#include "cipher.h"

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/params.h>
#include <openssl/rand.h>

#include <string>
#include <stdexcept>
#include <algorithm>
#include <string.h>

using std::string;
using std::runtime_error;
using std::logic_error;
using std::to_string;

ustring cmac(const ustring &key, const uint8_t *data, size_t len) {
	return cmac(key.c_str(), key.size(), data, len);
}

ustring cmac(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len) {
	thread_local static EVP_MAC *mac = EVP_MAC_fetch(NULL, "cmac", NULL);
	thread_local static char cipher[] = "aes-128-cbc";
	thread_local static OSSL_PARAM params[] = {
		OSSL_PARAM_construct_utf8_string("cipher", cipher, 0),
		OSSL_PARAM_construct_end(),
	};
	thread_local static EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);

	ustring ans;
	size_t n;

	if (ctx == nullptr)
		goto error;
	if (!EVP_MAC_init(ctx, key, key_len, params))
		goto error;
	if (!EVP_MAC_update(ctx, data, data_len))
		goto error;

	if (!EVP_MAC_final(ctx, nullptr, &n, 0))
		goto error;
	ans.resize(n);
	if (!EVP_MAC_final(ctx, ans.data(), &n, ans.size()))
		goto error;

	return ans;

error:
	auto err = ERR_get_error();
	thread_local static char buf[256];
	ERR_error_string_n(err, buf, sizeof(buf));
	throw runtime_error(buf);
}

size_t aes_128_cbc_cmac::encrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap) {
	thread_local static EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

	if (cap < len + min_cap) throw runtime_error("cap is not large enough");

	int n;
	size_t ans = 0;
	auto mac = cmac(key, data, len);

	if (!ctx)
		goto error;
	if (!EVP_EncryptInit_ex2(ctx, EVP_aes_128_cbc(), nullptr, nullptr, nullptr))
		goto error;
	if (auto k = EVP_CIPHER_CTX_get_key_length(ctx); k != key.size())
		throw runtime_error("key length is wrong. expect " + to_string(k) + " got " + to_string(key.size()));
	if (EVP_CIPHER_CTX_get_iv_length(ctx) != iv_size)
		throw logic_error("iv length is wrong!");

	RAND_bytes(encrypted, iv_size);
	if (!EVP_EncryptInit_ex2(ctx, nullptr, key.c_str(), encrypted, nullptr))
		goto error;

	encrypted += iv_size;
	ans += iv_size;

	if (!EVP_EncryptUpdate(ctx, encrypted, &n, data, len))
		goto error;

	encrypted += n;
	ans += n;

	if (!EVP_EncryptUpdate(ctx, encrypted, &n, mac.c_str(), mac.size()))
		goto error;

	encrypted += n;
	ans += n;

	if (!EVP_EncryptFinal(ctx, encrypted, &n))
		goto error;

	return ans + n;

error:
	auto err = ERR_get_error();
	thread_local static char buf[256];
	ERR_error_string_n(err, buf, sizeof(buf));
	throw runtime_error(buf);
}

size_t aes_128_cbc_cmac::decrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap) {
	thread_local static EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

	if (cap < len + min_cap) throw runtime_error("cap is not large enough");

	int n0, n1;
	size_t ans;
	ustring mac;

	if (!ctx)
		goto error;
	if (!EVP_DecryptInit_ex2(ctx, EVP_aes_128_cbc(), nullptr, nullptr, nullptr))
		goto error;

	if (auto k = EVP_CIPHER_CTX_get_key_length(ctx); k != key.size())
		throw runtime_error("key length is wrong. expect " + to_string(k) + " got " + to_string(key.size()));
	if (EVP_CIPHER_CTX_get_iv_length(ctx) != iv_size)
		throw logic_error("iv length is wrong!");

	if (!EVP_DecryptInit_ex2(ctx, nullptr, key.c_str(), data, nullptr))
		goto error;

	data += iv_size;
	len -= iv_size;

	if (!EVP_DecryptUpdate(ctx, decrypted, &n0, data, len))
		goto error;

	if (!EVP_DecryptFinal(ctx, decrypted + n0, &n1))
		goto error;

	ans = n0 + n1 - mac_size;
	mac = cmac(key, decrypted, ans);
	if (!std::equal(mac.begin(), mac.end(), decrypted + ans))
		throw runtime_error("cmac check fails");

	return ans;

error:
	auto err = ERR_get_error();
	thread_local static char buf[256];
	ERR_error_string_n(err, buf, sizeof(buf));
	throw runtime_error(buf);
}

size_t aes_128_gcm::encrypt(const key_type &key, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap) {
	thread_local static EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

	if (cap < len + min_cap) throw runtime_error("cap is not large enough");

	int n;
	size_t ans = 0;

	if (!ctx)
		goto error;
	if (!EVP_EncryptInit_ex2(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr))
		goto error;
	if (auto k = EVP_CIPHER_CTX_get_key_length(ctx); k != key.size())
		throw runtime_error("key length is wrong. expect " + to_string(k) + " got " + to_string(key.size()));
	if (EVP_CIPHER_CTX_get_iv_length(ctx) != iv_size)
		throw logic_error("iv length is wrong!");

	RAND_bytes(encrypted, iv_size);
	if (!EVP_EncryptInit_ex2(ctx, nullptr, key.c_str(), encrypted, nullptr))
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
	thread_local static EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	thread_local static uint8_t tag[tag_size];

	int n0, n1;

	if (!ctx)
		goto error;
	if (!EVP_DecryptInit_ex2(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr))
		goto error;
	if (auto k = EVP_CIPHER_CTX_get_key_length(ctx); k != key.size())
		throw runtime_error("key length is wrong. expect " + to_string(k) + " got " + to_string(key.size()));
	if (EVP_CIPHER_CTX_get_iv_length(ctx) != iv_size)
		throw logic_error("iv length is wrong!");

	if (!EVP_EncryptInit_ex2(ctx, nullptr, key.c_str(), data, nullptr))
		goto error;

	data += iv_size;
	len -= iv_size + tag_size;

	if (!EVP_DecryptUpdate(ctx, decrypted, &n0, data, len))
		goto error;

	memcpy(tag, data + len, tag_size);
	if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_size, tag))
		goto error;

	if (EVP_DecryptFinal(ctx, decrypted + n0, &n1))
		goto error;

	return n0 + n1;
error:
	auto err = ERR_get_error();
	thread_local static char buf[256];
	ERR_error_string_n(err, buf, sizeof(buf));
	throw runtime_error(buf);
}
