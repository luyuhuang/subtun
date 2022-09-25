#pragma once

#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

#include <cstring>
#include <string>
#include <stdexcept>

typedef std::basic_string<unsigned char> ustring;

struct aes_128_gcm {
	static const size_t key_size = 16;
	static const size_t iv_size = 12;
	static const size_t tag_size = 16;
	static const size_t block_size = 1;
	static const size_t padding_size = block_size - 1;

	size_t encrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap, uint8_t *tag);
	size_t decrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap, uint8_t *tag);
};

struct chacha20_poly1305 {
	static const size_t key_size = 32;
	static const size_t iv_size = 12;
	static const size_t tag_size = 16;
	static const size_t block_size = 1;
	static const size_t padding_size = block_size - 1;

	size_t encrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap, uint8_t *tag);
	size_t decrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap, uint8_t *tag);
};

template <typename Aead>
struct aead_indep : public Aead {
	static const size_t min_cap = Aead::iv_size + Aead::tag_size + Aead::padding_size;

	size_t encrypt(const uint8_t *key, const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap) {
		thread_local static uint8_t tag[Aead::tag_size];

		if (cap < len + min_cap) throw std::runtime_error("cap is not large enough");

		// iv load tag
		RAND_bytes(encrypted, Aead::iv_size);
		size_t n = Aead::encrypt(key, encrypted, data, len, encrypted + Aead::iv_size, cap - Aead::iv_size - Aead::tag_size, tag);
		memcpy(encrypted + Aead::iv_size + n, tag, Aead::tag_size);
		return Aead::iv_size + n + Aead::tag_size;
	}
	size_t decrypt(const uint8_t *key, const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap) {
		thread_local static uint8_t tag[Aead::tag_size];

		// iv load tag
		memcpy(tag, data + len - Aead::tag_size, Aead::tag_size);
		return Aead::decrypt(key, data, data + Aead::iv_size, len - Aead::iv_size - Aead::tag_size, decrypted, cap, tag);

	}
};

using aes_128_gcm_indep = aead_indep<aes_128_gcm>;
using chacha20_poly1305_indep = aead_indep<chacha20_poly1305>;

template <typename Aead>
struct aead_iter : public Aead {
	static const size_t min_cap = Aead::tag_size + Aead::padding_size;

	aead_iter(const aead_iter &a) : m_enc_iv(BN_dup(a.m_enc_iv)), m_dec_iv(BN_dup(a.m_dec_iv)) {
		memcpy(m_key, a.m_key, Aead::key_size);
	}
	aead_iter(aead_iter &&a) : m_enc_iv(a.m_enc_iv), m_dec_iv(a.m_dec_iv) {
		memcpy(m_key, a.m_key, Aead::key_size);
		a.m_enc_iv = a.m_dec_iv = nullptr;
	}

	aead_iter &operator=(const aead_iter &a) {
		if (&a == this) return *this;
		free();
		memcpy(m_key, a.m_key, Aead::key_size);
		m_enc_iv = BN_dup(a.m_enc_iv);
		m_dec_iv = BN_dup(a.m_dec_iv);
		return *this;
	}
	aead_iter &operator=(aead_iter &&a) {
		if (&a == this) return *this;
		free();
		memcpy(m_key, a.m_key, Aead::key_size);
		m_enc_iv = a.m_enc_iv;
		m_dec_iv = a.m_dec_iv;
		a.m_enc_iv = a.m_dec_iv = nullptr;
		return *this;
	}

	~aead_iter() {
		free();
	}

	void init(const uint8_t *key, const uint8_t *enc_iv, const uint8_t *dec_iv) {
		if (key) memcpy(m_key, key, Aead::key_size);
		if (enc_iv) m_enc_iv = BN_bin2bn(enc_iv, Aead::iv_size, nullptr);
		if (dec_iv) m_dec_iv = BN_bin2bn(dec_iv, Aead::iv_size, nullptr);
	}

	size_t encrypt(const uint8_t *data, size_t len, uint8_t *encrypted, size_t cap) {
		thread_local static uint8_t tag[Aead::tag_size];
		thread_local static uint8_t iv[Aead::iv_size];

		if (cap < len + min_cap) throw std::runtime_error("cap is not large enough");
		if (!m_enc_iv) throw std::runtime_error("encrypt iv has not been initialized");

		BN_bn2binpad(m_enc_iv, iv, Aead::iv_size);
		BN_add_word(m_enc_iv, 1);

		size_t n = Aead::encrypt(m_key, iv, data, len, encrypted, cap - Aead::tag_size, tag);
		memcpy(encrypted + n, tag, Aead::tag_size);
		return n + Aead::tag_size;
	}
	size_t decrypt(const uint8_t *data, size_t len, uint8_t *decrypted, size_t cap) {
		thread_local static uint8_t tag[Aead::tag_size];
		thread_local static uint8_t iv[Aead::iv_size];

		if (!m_dec_iv) throw std::runtime_error("encrypt iv has not been initialized");

		BN_bn2binpad(m_dec_iv, iv, Aead::iv_size);
		BN_add_word(m_dec_iv, 1);

		memcpy(tag, data + len - Aead::tag_size, Aead::tag_size);
		return Aead::decrypt(m_key, iv, data, len - Aead::tag_size, decrypted, cap, tag);
	}

private:
	uint8_t m_key[Aead::key_size] = {};
	BIGNUM *m_enc_iv = nullptr, *m_dec_iv = nullptr;

	void free() {
		BN_free(m_enc_iv), BN_free(m_dec_iv);
	}
};

using aes_128_gcm_iter = aead_iter<aes_128_gcm>;
using chacha20_poly1305_iter = aead_iter<chacha20_poly1305>;
