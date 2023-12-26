#include "mbedtls/base64.h"
#include "mbedtls/sha256.h"
#include "mbedtls/entropy.h"
#include "mbedtls/md.h"
#include "mbedtls/pk.h"
#include "mbedtls/ctr_drbg.h"

#include <string.h>
#include <stdio.h>

#include <time.h>
#include <sys/time.h>

#include "config.h"

static const unsigned char* private_key = (unsigned char*)PRIVATE_KEY;

static const unsigned char* payload_format = (unsigned char*)"{"
    "\"iss\":\"%s\","
    "\"sub\":\"%s\","
    "\"scope\":\"https://www.googleapis.com/auth/pubsub\","
    "\"aud\":\"https://oauth2.googleapis.com/token\","
    "\"iat\":\"%ld\","
    "\"exp\":\"%ld\""
    "}";

static const unsigned char* header_format = (unsigned char*)"{"
    "\"alg\":\"RS256\","
    "\"typ\":\"JWT\","
    "\"kid\":\"%s\""
    "}";

static unsigned char    payload_base64[512];
static size_t           payload_base64_len;

static unsigned char    header_base64[512];
static size_t           header_base64_len;

static unsigned char    concatenated[1024];

static unsigned char    signature[MBEDTLS_PK_SIGNATURE_MAX_SIZE];
static size_t           signature_len;

static unsigned char    signature_base64[1024];
static size_t           signature_base64_len;

unsigned char           jwt[1024];

// Function to generate the payload (adds the issued at time and expiration time)
static int generate_payload(unsigned char* output, size_t size, const unsigned char* input)
{
    time_t seconds = time(NULL);
    return snprintf((char*)output, size, (char*)input, ISS, SUB, (long int)seconds, (long int)(seconds + TOKEN_EXPIRATION_TIME_IN_SECONDS));    // Token expires in an hour
}

// Function to encode a string to base64 URL SAFE
static void base64_url_encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen)
{
    unsigned char temp[dlen];
    size_t temp_len;
    mbedtls_base64_encode(temp, dlen, &temp_len, src, slen);
    size_t dst_i = 0;
    for (size_t i = 0; i < temp_len; i++)
    {
        if ((temp[i] == (unsigned char)'=')) continue;
        if (temp[i] == (unsigned char)'+') dst[dst_i++] = (unsigned char)'-';
        else if (temp[i] == (unsigned char)'/') dst[dst_i++] = (unsigned char)'_';
        else dst[dst_i++] = temp[i];
    }

    dst[dst_i + 1] = (unsigned char)'\0';
    *olen = dst_i + 1;
}

// Function to generate the JWT
static void generate_jwt()
{
    unsigned char header[256];
    snprintf((char*)header, 256, (char*)header_format, KEY_ID);

    unsigned char payload[512];
    generate_payload(payload, 512, payload_format);
    
    base64_url_encode(payload_base64, 512, &payload_base64_len, payload, strlen((char*)payload));   // Base64 URL encode for payload

    base64_url_encode(header_base64, 512, &header_base64_len, header, strlen((char*)header));       // Base64 URL encode for header

    snprintf((char*)concatenated, 1024, "%s.%s", header_base64, payload_base64);                    // Concatenate header + payload to format of <header>.<payload>

    // RS256 signing
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, MBEDTLS_CTR_DRBG_MAX_SEED_INPUT);

    mbedtls_pk_parse_key(&pk, private_key, (strlen((char*)private_key) + 1), NULL, 0, mbedtls_ctr_drbg_random, &ctr_drbg);

    unsigned char hash[32];
    mbedtls_sha256(concatenated, strlen((char*)concatenated), hash, 0);

    mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, 32, signature, MBEDTLS_PK_SIGNATURE_MAX_SIZE, &signature_len, mbedtls_ctr_drbg_random, &ctr_drbg);

    base64_url_encode(signature_base64, 1024, &signature_base64_len, signature, signature_len);     // Base64 URL encode for signature

    snprintf((char*)jwt, 2048, "%s.%s.%s", header_base64, payload_base64, signature_base64);        // Form JWT of format <header>.<payload>.<signature>


    printf("\n%s\n", jwt);                                                                          // Print JWT

}