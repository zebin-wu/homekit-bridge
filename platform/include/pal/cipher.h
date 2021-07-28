// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_CIPHER_H_
#define PLATFORM_INCLUDE_PAL_CIPHER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pal/types.h>

/**
 * @brief Supported {cipher type, cipher mode} pairs.
 */
typedef enum {
    PAL_CIPHER_TYPE_AES_128_ECB,          /**< AES cipher with 128-bit ECB mode. */
    PAL_CIPHER_TYPE_AES_192_ECB,          /**< AES cipher with 192-bit ECB mode. */
    PAL_CIPHER_TYPE_AES_256_ECB,          /**< AES cipher with 256-bit ECB mode. */
    PAL_CIPHER_TYPE_AES_128_CBC,          /**< AES cipher with 128-bit CBC mode. */
    PAL_CIPHER_TYPE_AES_192_CBC,          /**< AES cipher with 192-bit CBC mode. */
    PAL_CIPHER_TYPE_AES_256_CBC,          /**< AES cipher with 256-bit CBC mode. */
    PAL_CIPHER_TYPE_AES_128_CFB128,       /**< AES cipher with 128-bit CFB128 mode. */
    PAL_CIPHER_TYPE_AES_192_CFB128,       /**< AES cipher with 192-bit CFB128 mode. */
    PAL_CIPHER_TYPE_AES_256_CFB128,       /**< AES cipher with 256-bit CFB128 mode. */
    PAL_CIPHER_TYPE_AES_128_CTR,          /**< AES cipher with 128-bit CTR mode. */
    PAL_CIPHER_TYPE_AES_192_CTR,          /**< AES cipher with 192-bit CTR mode. */
    PAL_CIPHER_TYPE_AES_256_CTR,          /**< AES cipher with 256-bit CTR mode. */
    PAL_CIPHER_TYPE_AES_128_GCM,          /**< AES cipher with 128-bit GCM mode. */
    PAL_CIPHER_TYPE_AES_192_GCM,          /**< AES cipher with 192-bit GCM mode. */
    PAL_CIPHER_TYPE_AES_256_GCM,          /**< AES cipher with 256-bit GCM mode. */
    PAL_CIPHER_TYPE_CAMELLIA_128_ECB,     /**< Camellia cipher with 128-bit ECB mode. */
    PAL_CIPHER_TYPE_CAMELLIA_192_ECB,     /**< Camellia cipher with 192-bit ECB mode. */
    PAL_CIPHER_TYPE_CAMELLIA_256_ECB,     /**< Camellia cipher with 256-bit ECB mode. */
    PAL_CIPHER_TYPE_CAMELLIA_128_CBC,     /**< Camellia cipher with 128-bit CBC mode. */
    PAL_CIPHER_TYPE_CAMELLIA_192_CBC,     /**< Camellia cipher with 192-bit CBC mode. */
    PAL_CIPHER_TYPE_CAMELLIA_256_CBC,     /**< Camellia cipher with 256-bit CBC mode. */
    PAL_CIPHER_TYPE_CAMELLIA_128_CFB128,  /**< Camellia cipher with 128-bit CFB128 mode. */
    PAL_CIPHER_TYPE_CAMELLIA_192_CFB128,  /**< Camellia cipher with 192-bit CFB128 mode. */
    PAL_CIPHER_TYPE_CAMELLIA_256_CFB128,  /**< Camellia cipher with 256-bit CFB128 mode. */
    PAL_CIPHER_TYPE_CAMELLIA_128_CTR,     /**< Camellia cipher with 128-bit CTR mode. */
    PAL_CIPHER_TYPE_CAMELLIA_192_CTR,     /**< Camellia cipher with 192-bit CTR mode. */
    PAL_CIPHER_TYPE_CAMELLIA_256_CTR,     /**< Camellia cipher with 256-bit CTR mode. */
    PAL_CIPHER_TYPE_CAMELLIA_128_GCM,     /**< Camellia cipher with 128-bit GCM mode. */
    PAL_CIPHER_TYPE_CAMELLIA_192_GCM,     /**< Camellia cipher with 192-bit GCM mode. */
    PAL_CIPHER_TYPE_CAMELLIA_256_GCM,     /**< Camellia cipher with 256-bit GCM mode. */
    PAL_CIPHER_TYPE_DES_ECB,              /**< DES cipher with ECB mode. */
    PAL_CIPHER_TYPE_DES_CBC,              /**< DES cipher with CBC mode. */
    PAL_CIPHER_TYPE_DES_EDE_ECB,          /**< DES cipher with EDE ECB mode. */
    PAL_CIPHER_TYPE_DES_EDE_CBC,          /**< DES cipher with EDE CBC mode. */
    PAL_CIPHER_TYPE_DES_EDE3_ECB,         /**< DES cipher with EDE3 ECB mode. */
    PAL_CIPHER_TYPE_DES_EDE3_CBC,         /**< DES cipher with EDE3 CBC mode. */
    PAL_CIPHER_TYPE_BLOWFISH_ECB,         /**< Blowfish cipher with ECB mode. */
    PAL_CIPHER_TYPE_BLOWFISH_CBC,         /**< Blowfish cipher with CBC mode. */
    PAL_CIPHER_TYPE_BLOWFISH_CFB64,       /**< Blowfish cipher with CFB64 mode. */
    PAL_CIPHER_TYPE_BLOWFISH_CTR,         /**< Blowfish cipher with CTR mode. */
    PAL_CIPHER_TYPE_ARC4_128,             /**< RC4 cipher with 128-bit mode. */
    PAL_CIPHER_TYPE_AES_128_CCM,          /**< AES cipher with 128-bit CCM mode. */
    PAL_CIPHER_TYPE_AES_192_CCM,          /**< AES cipher with 192-bit CCM mode. */
    PAL_CIPHER_TYPE_AES_256_CCM,          /**< AES cipher with 256-bit CCM mode. */
    PAL_CIPHER_TYPE_CAMELLIA_128_CCM,     /**< Camellia cipher with 128-bit CCM mode. */
    PAL_CIPHER_TYPE_CAMELLIA_192_CCM,     /**< Camellia cipher with 192-bit CCM mode. */
    PAL_CIPHER_TYPE_CAMELLIA_256_CCM,     /**< Camellia cipher with 256-bit CCM mode. */
    PAL_CIPHER_TYPE_ARIA_128_ECB,         /**< Aria cipher with 128-bit key and ECB mode. */
    PAL_CIPHER_TYPE_ARIA_192_ECB,         /**< Aria cipher with 192-bit key and ECB mode. */
    PAL_CIPHER_TYPE_ARIA_256_ECB,         /**< Aria cipher with 256-bit key and ECB mode. */
    PAL_CIPHER_TYPE_ARIA_128_CBC,         /**< Aria cipher with 128-bit key and CBC mode. */
    PAL_CIPHER_TYPE_ARIA_192_CBC,         /**< Aria cipher with 192-bit key and CBC mode. */
    PAL_CIPHER_TYPE_ARIA_256_CBC,         /**< Aria cipher with 256-bit key and CBC mode. */
    PAL_CIPHER_TYPE_ARIA_128_CFB128,      /**< Aria cipher with 128-bit key and CFB-128 mode. */
    PAL_CIPHER_TYPE_ARIA_192_CFB128,      /**< Aria cipher with 192-bit key and CFB-128 mode. */
    PAL_CIPHER_TYPE_ARIA_256_CFB128,      /**< Aria cipher with 256-bit key and CFB-128 mode. */
    PAL_CIPHER_TYPE_ARIA_128_CTR,         /**< Aria cipher with 128-bit key and CTR mode. */
    PAL_CIPHER_TYPE_ARIA_192_CTR,         /**< Aria cipher with 192-bit key and CTR mode. */
    PAL_CIPHER_TYPE_ARIA_256_CTR,         /**< Aria cipher with 256-bit key and CTR mode. */
    PAL_CIPHER_TYPE_ARIA_128_GCM,         /**< Aria cipher with 128-bit key and GCM mode. */
    PAL_CIPHER_TYPE_ARIA_192_GCM,         /**< Aria cipher with 192-bit key and GCM mode. */
    PAL_CIPHER_TYPE_ARIA_256_GCM,         /**< Aria cipher with 256-bit key and GCM mode. */
    PAL_CIPHER_TYPE_ARIA_128_CCM,         /**< Aria cipher with 128-bit key and CCM mode. */
    PAL_CIPHER_TYPE_ARIA_192_CCM,         /**< Aria cipher with 192-bit key and CCM mode. */
    PAL_CIPHER_TYPE_ARIA_256_CCM,         /**< Aria cipher with 256-bit key and CCM mode. */
    PAL_CIPHER_TYPE_AES_128_OFB,          /**< AES 128-bit cipher in OFB mode. */
    PAL_CIPHER_TYPE_AES_192_OFB,          /**< AES 192-bit cipher in OFB mode. */
    PAL_CIPHER_TYPE_AES_256_OFB,          /**< AES 256-bit cipher in OFB mode. */
    PAL_CIPHER_TYPE_AES_128_XTS,          /**< AES 128-bit cipher in XTS block mode. */
    PAL_CIPHER_TYPE_AES_256_XTS,          /**< AES 256-bit cipher in XTS block mode. */
    PAL_CIPHER_TYPE_CHACHA20,             /**< ChaCha20 stream cipher. */
    PAL_CIPHER_TYPE_CHACHA20_POLY1305,    /**< ChaCha20-Poly1305 AEAD cipher. */
    PAL_CIPHER_TYPE_MAX,
} pal_cipher_type;

/**
 * @brief Supported cipher padding types.
 */
typedef enum {
    PAL_CIPHER_PADDING_NONE,        /**< Never pad (full blocks only).   */
    PAL_CIPHER_PADDING_PKCS7,       /**< PKCS7 padding (default).        */
    PAL_CIPHER_PADDING_ISO7816_4,   /**< ISO/IEC 7816-4 padding.         */
    PAL_CIPHER_PADDING_ANSI923,     /**< ANSI X.923 padding.             */
    PAL_CIPHER_PADDING_ZERO,        /**< Zero padding (not reversible).  */
    PAL_CIPHER_PADDING_MAX,
} pal_cipher_padding;

/**
 * @brief Type of operation.
 */
typedef enum {
    PAL_CIPHER_OP_NONE = -1,
    PAL_CIPHER_OP_ENCRYPT,
    PAL_CIPHER_OP_DECRYPT,
    PAL_CIPHER_OP_MAX,
} pal_cipher_operation;

/**
 * @brief Generic cipher context.
 */
typedef struct pal_cipher_ctx pal_cipher_ctx;

/**
 * @brief New a cipher context.
 *
 * @param type Type of the cipher.
 *
 * @return the cipher context on success.
 * @return NULL on failure.
 */
pal_cipher_ctx *pal_cipher_new(pal_cipher_type type);

/**
 * @brief Free a cipher context.
 *
 * @param ctx The cipher context to be freed.
 *            If this is NULL, the function has no effect.
 */
void pal_cipher_free(pal_cipher_ctx *ctx);

/**
 * @brief Reset the cipher context.
 *
 * @param ctx The cipher context.
 *
 * @return true on success.
 * @return false on failure.
 */
bool pal_cipher_reset(pal_cipher_ctx *ctx);

/**
 * @brief Return the block size of the given cipher in bytes.
 *
 * @param ctx The cipher context.
 *
 * @return The block size of the underlying cipher in bytes.
 */
size_t pal_cipher_get_block_size(pal_cipher_ctx *ctx);

/**
 * @brief Return the key length of the cipher in bytes.
 *
 * @param ctx The cipher context.
 *
 * @return The key length of the cipher in bytes.
 */
size_t pal_cipher_get_key_len(pal_cipher_ctx *ctx);

/**
 * @brief Return the length of the IV or nonce of the cipher in bytes.
 *
 * @param ctx The cipher context.
 *
 * @return The length of the IV.
 * @return 0 for ciphers not using an IV or a nonce.
 */
size_t pal_cipher_get_iv_len(pal_cipher_ctx *ctx);

/**
 * @brief Set the padding mode, for cipher modes that use padding.
 *        The default padding mode is PKCS7 padding.
 *
 * @param ctx The cipher context.
 * @param padding The padding mode.
 *
 * @return true on success.
 * @return false on failure.
 */
bool pal_cipher_set_padding(pal_cipher_ctx *ctx, pal_cipher_padding padding);

/**
 * @brief Begin a encryption/decryption process.
 *
 * @param ctx The cipher context.
 * @param op The operation mode of this process.
 * @param key The key to use.
 * @param iv The IV to use. NULL for ciphers not using an IV or nonce.
 *
 * @return true on success.
 * @return false on failure.
 */
bool pal_cipher_begin(pal_cipher_ctx *ctx, pal_cipher_operation op, const uint8_t *key, const uint8_t *iv);

/**
 * @brief Update data to the cipher context.
 *
 * @param ctx The cipher context.
 * @param in The buffer holding the input data.
 * @param ilen The length of the input data.
 * @param out The buffer holding the output data.
 *            This must be able to hold at least ilen + block_size.
 *            This must not be the same buffer as input.
 * @param olen Thg length of the output data, to be updated with the
 *             actual number of Bytes written. This must not be NULL.
 *
 * @return true on success.
 * @return false on failure.
 */
bool pal_cipher_update(pal_cipher_ctx *ctx, const void *in, size_t ilen, void *out, size_t *olen);

/**
 * @brief Finsh the encryption/decryption process.
 *
 * @param ctx The cipher context.
 * @param out The buffer holding the output data.
 *            This must be able to hold at block_size Bytes.
 * @param olen Thg length of the output data, to be updated with the
 *             actual number of Bytes written. This must not be NULL.
 *
 * @return true on success.
 * @return false on failure.
 */
bool pal_cipher_finsh(pal_cipher_ctx *ctx, void *out, size_t *olen);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_CIPHER_H_
