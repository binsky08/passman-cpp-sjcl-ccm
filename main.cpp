//custom execution command: g++ -o main libs/base64.cpp libs/json.hpp libs/openssl/build/x86_64/lib/libcrypto.a libs/openssl/build/x86_64/lib/libssl.a libs/SimpleJSON/src/JSON.cpp libs/SimpleJSON/src/JSONValue.cpp main.cpp -lcrypto -lssl && ./main

#include <openssl/evp.h>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include "libs/json.hpp"
#include "libs/base64.h"
#include "libs/SimpleJSON/src/JSON.h"


using namespace std;


void handleErrors(const char* error){
    printf("FUCK THIS SHIT GOT AN ERROR: %s\n", error);
}

string get_random_string(int n){
    const int MAX_SIZE = 62;

    char letters[MAX_SIZE] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
                              'r','s','t','u','v','w','x','y','z',
                              'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
                              'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                              '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    string ran = "";
    for (int i=0;i<n;i++)
        ran = ran + letters[rand() % MAX_SIZE];
    return ran;
}

int decryptccm(unsigned char *ciphertext, int ciphertext_len, unsigned char *aad,
               int aad_len, unsigned char *tag, unsigned char *key, unsigned char *iv,
               unsigned char *plaintext
) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors("Error initializing context");

    /* Initialise the decryption operation. */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_ccm(), NULL, NULL, NULL)) handleErrors("Error setting crypto mode");

    int lol = 2;
    if (ciphertext_len >= 1<<16) lol++;
    if (ciphertext_len >= 1<<24) lol++;

    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, 15-lol, NULL)) handleErrors("Error setting IV Length");

    /* Set expected tag value. */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, 8, tag)) handleErrors("Error setting TAG value");

    /* Initialise key and IV */
    if(1 != EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors("Error setting KEY and IV");

    /* Provide the total ciphertext length
     */
    if(1 != EVP_DecryptUpdate(ctx, NULL, &len, NULL, ciphertext_len)) handleErrors("Error setting cyphertext length");

    /* Provide any AAD data. This can be called zero or more times as
     * required
     */
    if(1 != EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len)) handleErrors("Error setting AAD data");

    /* Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    ret = EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);

    plaintext_len = len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    if(ret > 0)
    {
        /* Success */
        return plaintext_len;
    }
    else
    {
        /* Verify failed */
        return ret;
    }
}

int encryptccm(unsigned char *plaintext, int plaintext_len,
               unsigned char *aad, int aad_len,
               unsigned char *key,
               unsigned char *iv,
               int iv_len,
               unsigned char *ciphertext,
               unsigned char *tag,
               int tag_len)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;


    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors("Error initializing context");

    /* Initialise the encryption operation. */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_ccm(), NULL, NULL, NULL))
        handleErrors("Error setting crypto mode");

    /*
     * Setting IV len to 7. Not strictly necessary as this is the default
     * but shown here for the purposes of this example.
     */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, iv_len, NULL))
        handleErrors("Error setting IV Length");

    /* Set tag length */
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, tag_len, NULL);

    /* Initialise key and IV */
    if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
        handleErrors("Error setting KEY and IV");

    /* Provide the total plaintext length */
    if(1 != EVP_EncryptUpdate(ctx, NULL, &len, NULL, plaintext_len))
        handleErrors("Error setting plaintext length");

    /* Provide any AAD data. This can be called zero or one times as required */
    if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
        handleErrors("Error setting AAD data");

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can only be called once for this.
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors("Error obtaining the encrypted output");
    ciphertext_len = len;

    /*
     * Finalise the encryption. Normally ciphertext bytes may be written at
     * this stage, but this does not occur in CCM mode.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors("Error finalizing the encryption");
    ciphertext_len += len;

    /* Get the tag */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_GET_TAG, tag_len, tag))
        handleErrors("Error getting the encryption tag");

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

/**
 * Casts an WString to an standard char array, beware, it does not care about encoding!
 * It just discards the wide part of the chars!
 */
char* wstring_to_char(wstring str) {
    char* c = (char *) malloc(sizeof(char) * str.length() + 1);
    const wchar_t* data = str.c_str();
    for (int i = 0; i <= str.length(); i++) {
        c[i] = (char) data[i];
    }
    return c;
}

using namespace WLF::Crypto;

char* decrypt(string sjcl_json, string key) {
    handleErrors("json to decode:");
    handleErrors(sjcl_json.c_str());
    JSONValue *data = JSON::Parse(sjcl_json.c_str());

    if (data == NULL || ! data->IsObject()) {
        handleErrors("Error parsing the SJCL JSON");
        return NULL;
    }

    JSONObject food = data->AsObject();

    int iter        = 0,
            key_size    = 0,
            tag_size    = 0;
    char *iv_64, *salt_64, *cyphertext, *adata;

    // Extract the requiered values from the JSON
    iv_64       = wstring_to_char(food[L"iv"]->AsString());
    adata       = wstring_to_char(food[L"adata"]->AsString());
    salt_64     = wstring_to_char(food[L"salt"]->AsString());
    cyphertext  = wstring_to_char(food[L"ct"]->AsString());

    iter        = (int) food[L"iter"]->AsNumber();
    key_size    = (int) food[L"ks"]->AsNumber();
    tag_size    = (int) food[L"ts"]->AsNumber();

    tag_size /= 8; // Make it bytes!
    key_size /= 8; // Make it bytes

    // The actual cryptogram includes the tag size, so we need to take this into account later on!
    Datagram* cryptogram = BASE64::decode((unsigned char *) cyphertext, strlen(cyphertext));
    int cyphertext_data_length = cryptogram->length - tag_size;

    handleErrors(to_string(cyphertext_data_length).c_str());
    handleErrors(to_string(cryptogram->length).c_str());

    Datagram* salt = BASE64::decode((unsigned char *) salt_64, strlen(salt_64));
    Datagram* iv_raw = BASE64::decode((unsigned char *) iv_64, strlen(iv_64));
//    Datagram* aadata = BASE64::decode((const unsigned char *) "", 0); // Not sure if this is required since we don't use adata

    unsigned char* derived_key = (unsigned char*) malloc(sizeof(unsigned char) * key_size);

    // Assuming plaintext will always be smaller than the sjcl cyphertext which includes the tag and padding and stuff
    unsigned char* plaintext = (unsigned char*) malloc(sizeof(unsigned char) * strlen(cyphertext));
    // Ensure plaintext ends up null terminated
    for (int i = 0; i < strlen(cyphertext); i++) plaintext[i] = '\0';
    string s = string("The allocated string is: ") + string((char*)plaintext);

    /* PBKDF2 Key derivation with SHA256 as SJCL does by default */
    PKCS5_PBKDF2_HMAC(key.c_str(), key.length(), salt->data, salt->length, iter, EVP_sha256(), key_size, derived_key);

    char* ret = NULL;
    // Decrypt the data
    unsigned char *tag = &cryptogram->data[cyphertext_data_length];
    int plaintext_len = decryptccm(cryptogram->data, cyphertext_data_length, (unsigned char *) adata, strlen(adata),
                                   tag, (unsigned char *) key.c_str(), iv_raw->data, plaintext);

    if (0 < plaintext_len) {
        // Try to make strings strings instead of json encoded strings
        JSONValue *result = JSON::Parse((char *) plaintext);
        if (result->IsString()) {
            ret =  wstring_to_char(result->AsString());
            free(plaintext);
            free(result);
        }
        else {
            ret = (char *) plaintext;
        }
    }

    // Free up resources
    free(iv_64);
    free(adata);
    free(salt_64);
    free(cyphertext);
    free(data);
    free(derived_key);
//    free(food);

    return ret;
}

char* encrypt(char* plaintext, const string& key) {
    /*
        int lol = 2;
        if (ciphertext_len >= 1<<16) lol++;
        if (ciphertext_len >= 1<<24) lol++;
     */
    int iv_len = 13;    //8? (see 3.1 at https://tools.ietf.org/html/rfc4309#page-4)
    int salt_len = 12;  //can I use a random number here?
    int iter = 1000;
    int key_size = 256;
    int tag_size = 64;
    int ciphertext_allocation_multiplicator = 30;

    int ks = key_size / 8;  // Make it bytes
    int ts = tag_size / 8;  // Make it bytes
    unsigned char *ciphertext;
    unsigned char *derived_key;
    unsigned char tag[ts];  // Make it bytes
    unsigned char *iv = nullptr;
    unsigned char *salt = nullptr;
    unsigned char *additional = (unsigned char *)"";
    char *ret = NULL;

    srand(time(NULL));

    //iv = (unsigned char *) get_random_string(iv_len).c_str();
    std::string tmpiv = get_random_string(iv_len);
    iv = (unsigned char *) tmpiv.c_str();
    iv_len = tmpiv.length();

    //salt = (unsigned char *) get_random_string(salt_len).c_str();
    std::string tmpsalt = get_random_string(salt_len);
    salt = (unsigned char *) tmpsalt.c_str();
    salt_len = tmpsalt.length();

    derived_key = (unsigned char*) malloc(sizeof(unsigned char) * ks);

    // PBKDF2 Key derivation with SHA256 as SJCL does by default
    PKCS5_PBKDF2_HMAC(key.c_str(), key.length(), salt, salt_len, iter, EVP_sha256(), ks, derived_key);

    // Assuming ciphertext will not be bigger that the plaintext length * ciphertext_allocation_multiplicator
    ciphertext = (unsigned char *) malloc(sizeof(unsigned char) * strlen(plaintext) * ciphertext_allocation_multiplicator);

    // Ensure ciphertext ends up null terminated (do I need this?)
    for (int i = 0; i < (sizeof(unsigned char) * strlen(plaintext) * ciphertext_allocation_multiplicator); i++) ciphertext[i] = '\0';

    int ciphertext_len = encryptccm(reinterpret_cast<unsigned char *>(plaintext), strlen(plaintext), additional, strlen ((char *)additional),
                                    derived_key, iv, iv_len, ciphertext, tag, ts);
    if (0 < ciphertext_len) {
        //char ciphertext_with_tag[ciphertext_len + ts];   // array to hold the result.
        /*char *ciphertext_with_tag = (char *)malloc(sizeof(char *) * (ciphertext_len + ts));

        //strcpy(ciphertext_with_tag, reinterpret_cast<const char *>(ciphertext));
        strncpy(ciphertext_with_tag, reinterpret_cast<const char *>(ciphertext), ciphertext_len);
        //strcat(ciphertext_with_tag, reinterpret_cast<const char *>(tag));
        strncat(ciphertext_with_tag, reinterpret_cast<const char *>(tag), ts);*/

        //char *ciphertext_with_tag = (char *)malloc(sizeof(char *) * (ciphertext_len + ts));
        uint8_t *ciphertext_with_tag = static_cast<uint8_t *>(malloc(sizeof(char *) * (ciphertext_len + ts)));
        memcpy(ciphertext_with_tag, ciphertext, ciphertext_len);
        memcpy(ciphertext_with_tag + ciphertext_len, tag, ts);

        Datagram* ciphertext_base64 = BASE64::encode(reinterpret_cast<const unsigned char *>(ciphertext_with_tag), ciphertext_len + ts);
        char *tmpct64 = reinterpret_cast<char*>(ciphertext_base64->data);

        Datagram* salt_base64 = BASE64::encode(salt, salt_len);
        char *tmpsalt64 = reinterpret_cast<char*>(salt_base64->data);

        Datagram* iv_base64 = BASE64::encode(iv, iv_len);
        char *tmpiv64 = reinterpret_cast<char *>(iv_base64->data);

        nlohmann::json food;

        food["ct"] = tmpct64;
        food["salt"] = tmpsalt64;
        food["iv"] = tmpiv64;

        food["v"] = 1;
        food["iter"] = iter;
        food["ks"] = key_size;
        food["ts"] = tag_size;
        food["mode"] = "ccm";
        food["adata"] = "";
        food["cipher"] = "aes";

        string retrn = food.dump();
        ret = (char *) malloc(sizeof(char) * retrn.length());
        strcpy(ret, retrn.c_str());
    }

    // Free up resources
    free(ciphertext);

    return ret;
}

int main(void) {
    string key = "mykey123456";
    char *plaintext = (char *) "test123";
    char *ciphertext = encrypt(plaintext, key);
    handleErrors(ciphertext);

    //char *recreated_plaintext = decrypt(ciphertext, key);
    //handleErrors(recreated_plaintext);
    return 0;
}
