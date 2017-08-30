/**
 * \file SM4_MCU.h
 */
#ifndef SM4_MCU_H__
#define SM4_MCU_H__


#include <stdint.h>

#define SM4_ENCRYPT     1
#define SM4_DECRYPT     0

/**
 * \brief          SM4 context structure
 * // 用于保存加密模式和各轮子密钥的结构体
 */
struct sm4_context
{
    uint32_t mode;					/*!<  encrypt/decrypt   */
    uint32_t sk[32];					/*!<  SM4 subkeys       */
};


#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          SM4 key schedule (128-bit, encryption)
 *
 * \param ctx      SM4 context to be initialized
 * \param key      16-byte secret key
 */
void sm4_setkey_enc( struct sm4_context *ctx, unsigned char *key);

/**
 * \brief          SM4 key schedule (128-bit, decryption)
 *
 * \param ctx      SM4 context to be initialized
 * \param key      16-byte secret key
 */
void sm4_setkey_dec(struct sm4_context *ctx, unsigned char *key);

/**
 * \brief          SM4-ECB block encryption/decryption
 * \param ctx      SM4 context
 * \param mode     SM4_ENCRYPT or SM4_DECRYPT
 * \param length   length of the input data
 * \param input    input block
 * \param output   output block
 */
void sm4_crypt_ecb( struct sm4_context *ctx, \
				     int mode,
					 int length,
                     unsigned char *input,
                     unsigned char *output);

/**
 * \brief          SM4-CBC buffer encryption/decryption
 * \param ctx      SM4 context
 * \param mode     SM4_ENCRYPT or SM4_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 */
void sm4_crypt_cbc( struct sm4_context *ctx,
                     int mode,
                     int length,
                     unsigned char *iv,
                     unsigned char *input,
                     unsigned char *output );

#ifdef __cplusplus
}
#endif

#endif /* sm4.h */

