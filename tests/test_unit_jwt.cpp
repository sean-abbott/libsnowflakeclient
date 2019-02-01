/*
 * Copyright (c) 2017-2018 Snowflake Computing, Inc. All rights reserved.
 */

#include "utils/TestSetup.hpp"
#include "utils/test_setup.h"
#include <jwt/Jwt.hpp>
#include "openssl/rsa.h"
#include <openssl/pem.h>

using Snowflake::Client::Jwt::IHeader;
using Snowflake::Client::Jwt::IClaimSet;
using Snowflake::Client::Jwt::AlgorithmType;
using Snowflake::Client::Jwt::JWTObject;
using IHeaderUptr = std::unique_ptr<IHeader>;
using IClaimSetUptr = std::unique_ptr<IClaimSet>;
using IHeaderSptr = std::shared_ptr<IHeader>;
using IClaimSetSptr = std::shared_ptr<IClaimSet>;


static EVP_PKEY * getEVPFromString()
{
  const char *pkey = "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAy9QkFIGxs8oXnuUKeIzT\nNJ3l1aFIfoUuIiRtLJ1XwmyPYHnLjC0yye3smMmctx6BcXTV9E0ebf8a0sENhSDm\nThjFM62baNka23Pzo6cSSSGbT2m1NQbARKa4dNP7zkWIPHa2tuK1/jRCy6Z/ARTd\nkPgYa4Xr0br/vL3QoZ/sy2ieeT2U4Xa03jAghU9VgFYkIp3hpI6aTaDmKG8Z5mVj\novBpW8Rg0vkkwZ3GhjhAJhr6qwMoTSgkQU/Xst0X8duO/HD7bH9NYpsySMiU4+lR\nsrCC0rhiCToT36kidynajEJI6uQoTQzsPtFM+Nz0Vd1+dZfJ1H+ZyIROyVXlCKhR\nCQIDAQAB\n-----END PUBLIC KEY-----\n";

  BIO* bio = BIO_new_mem_buf(pkey, strlen(pkey));
  if (!bio)
  {
    return NULL;
  }

  return PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
}

static EVP_PKEY *generate_key()
{
  EVP_PKEY *key = nullptr;

  std::unique_ptr<EVP_PKEY_CTX, std::function<void(EVP_PKEY_CTX *)>>
    kct{EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), [](EVP_PKEY_CTX *ctx) { EVP_PKEY_CTX_free(ctx); }};

  if (EVP_PKEY_keygen_init(kct.get()) <= 0) return nullptr;

  if (EVP_PKEY_CTX_set_rsa_keygen_bits(kct.get(), 2048) <= 0) return nullptr;

  if (EVP_PKEY_keygen(kct.get(), &key) <= 0) return nullptr;

  return key;
}

static EVP_PKEY *extract_pub_key(EVP_PKEY *key_pair)
{
  std::unique_ptr<BIO, std::function<void(BIO *)>> mem
    {BIO_new(BIO_s_mem()), [](BIO *bio) { BIO_free(bio); }};

  if (mem == nullptr) return nullptr;

  if (!PEM_write_bio_PUBKEY(mem.get(), key_pair)) return nullptr;

  EVP_PKEY *pub_key = PEM_read_bio_PUBKEY(mem.get(), nullptr, nullptr, nullptr);

  return pub_key;
}

void test_header(void **)
{
  IHeaderUptr header(IHeader::buildHeader());
  header->setAlgorithm(AlgorithmType::RS256);

  assert_true(header->getAlgorithmType() == AlgorithmType::RS256);

  std::string header_str = header->serialize();

  IHeaderUptr header_cpy(IHeader::parseHeader(header_str));

  assert_true(header_str.back() != '=');

  assert_true(header->getAlgorithmType() == header_cpy->getAlgorithmType());
}

void test_claim_set(void **)
{
  // Basic get/set tests
  // Numbers
  IClaimSetUptr claim_set(IClaimSet::buildClaimSet());
  assert_false(claim_set->containsClaim("number"));
  claim_set->addClaim("number", 200L);
  assert_true(claim_set->containsClaim("number"));
  assert_int_equal(claim_set->getClaimInLong("number"), 200L);
  claim_set->addClaim("number", 300);
  assert_int_equal(claim_set->getClaimInLong("number"), 300L);

  // Strings
  claim_set->addClaim("string", "value");
  assert_true(claim_set->containsClaim("string"));
  assert_string_equal(claim_set->getClaimInString("string").c_str(), "value");
  claim_set->removeClaim("string");
  assert_false(claim_set->containsClaim("string"));
  claim_set->addClaim("string", "new_value");
  assert_string_equal(claim_set->getClaimInString("string").c_str(), "new_value");

  // Serialization
  std::string claim_set_str = claim_set->serialize();
  IClaimSetUptr claim_set_cpy(IClaimSet::parseClaimset(claim_set_str));

  assert_string_equal(claim_set->getClaimInString("string").c_str(),
                      claim_set_cpy->getClaimInString("string").c_str());

  assert_int_equal(claim_set->getClaimInLong("number"),
                   claim_set_cpy->getClaimInLong("number"));

}

void test_sign_verify(void **)
{
  JWTObject jwt;

  IHeaderSptr header(IHeader::buildHeader());
  IClaimSetSptr claim_set(IClaimSet::buildClaimSet());
  header->setAlgorithm(AlgorithmType::RS256);
  claim_set->addClaim("string", "value");
  claim_set->addClaim("number", 200L);

  jwt.setHeader(header);
  jwt.setClaimSet(claim_set);

  std::unique_ptr<EVP_PKEY, std::function<void(EVP_PKEY *)>> key
    {generate_key(), [](EVP_PKEY *k) { EVP_PKEY_free(k); }};

  if (key == nullptr)
  {
    assert_true(false);
    return;
  }

  std::unique_ptr<EVP_PKEY, std::function<void(EVP_PKEY *)>> pub_key
    {extract_pub_key(key.get()), [](EVP_PKEY *k) { EVP_PKEY_free(k); }};

  std::string result = jwt.serialize(key.get());

  assert_string_not_equal(result.c_str(), "");

  assert_true(jwt.verify(pub_key.get(), true));
}

void test_jwt_sign_verification(void **)
{
  JWTObject jwt;
  
  IHeaderSptr header(IHeader::buildHeader());
  IClaimSetSptr claim_set(IClaimSet::buildClaimSet());
  header->setAlgorithm(AlgorithmType::RS512);
  header->setCustomHeaderEntry("ssd_iss", "dep1");
  claim_set->addClaim("keyVer", 0.2);
  claim_set->addClaim("pubKeyType", "RSA");
  claim_set->addClaim("pubKey", "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzA/Ws3OvcV0uUN+2Q+wH\n4JLGwD/gabXz7OYyw49it1bgvUMlmKssxmaWYEQ3h/uGAtKn5pahKK3a/P3e8LrD\nfwl8On5WWgSlvF3WwbtXrJDT9UYTjjakjBxngooE9gh9BJdkb/kLUL0MulERsUf5\noGBPnWK4tyr3TXCTcBis9dnU09Q71QGAIgYe/eaXda2xqOUZLkAfewTQ2AqGL1SK\nlOl5Xpk9zsBgMseYuoEAe92w9YNn0g41wRukPvCt/z6/J9b26x/+DF2Du4PpeZeX\n1Qij5VgbHmiUut9QIiymV+bSC0+yKfe5Lwt8QR4oyuEVmbHe6bHUiVtWiahiU8sR\nJwIDAQAB\n-----END PUBLIC KEY-----\n");
  
  jwt.setHeader(header);
  jwt.setClaimSet(claim_set);

  std::unique_ptr<EVP_PKEY, std::function<void(EVP_PKEY *)>> key
          {generate_key(), [](EVP_PKEY *k) { EVP_PKEY_free(k); }};

  if (key == nullptr)
  {
    assert_true(false);
    return;
  }

  std::unique_ptr<EVP_PKEY, std::function<void(EVP_PKEY *)>> pub_key
          {extract_pub_key(key.get()), [](EVP_PKEY *k) { EVP_PKEY_free(k); }};

  std::string result = jwt.serialize(key.get());

  assert_string_not_equal(result.c_str(), "");

  assert_true(jwt.verify(pub_key.get(), true));
}

void test_jwt_sign_verify_b64(void **)
{
  EVP_PKEY *pkey = getEVPFromString();
  JWTObject jwt("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiIsInNzZF9pc3MiOiJkZXAxIn0.eyJrZXlWZXIiOiIwLjIiLCJwdWJLZXlUeXAiOiJSU0EiLCJwdWJLZXkiOiItLS0tLUJFR0lOIFBVQkxJQyBLRVktLS0tLVxuTUlJQklqQU5CZ2txaGtpRzl3MEJBUUVGQUFPQ0FROEFNSUlCQ2dLQ0FRRUF6QS9XczNPdmNWMHVVTisyUSt3SFxuNEpMR3dEL2dhYlh6N09ZeXc0OWl0MWJndlVNbG1Lc3N4bWFXWUVRM2gvdUdBdEtuNXBhaEtLM2EvUDNlOExyRFxuZndsOE9uNVdXZ1NsdkYzV3didFhySkRUOVVZVGpqYWtqQnhuZ29vRTlnaDlCSmRrYi9rTFVMME11bEVSc1VmNVxub0dCUG5XSzR0eXIzVFhDVGNCaXM5ZG5VMDlRNzFRR0FJZ1llL2VhWGRhMnhxT1VaTGtBZmV3VFEyQXFHTDFTS1xubE9sNVhwazl6c0JnTXNlWXVvRUFlOTJ3OVlObjBnNDF3UnVrUHZDdC96Ni9KOWIyNngvK0RGMkR1NFBwZVplWFxuMVFpajVWZ2JIbWlVdXQ5UUlpeW1WK2JTQzAreUtmZTVMd3Q4UVI0b3l1RVZtYkhlNmJIVWlWdFdpYWhpVThzUlxuSndJREFRQUJcbi0tLS0tRU5EIFBVQkxJQyBLRVktLS0tLVxuIiwibmJmIjoxNTQ4OTI0NjMxfQ.g4ndRfk2_caUg-72dy8FQVy0h73CGnAG8vzvvSpMiaRXLiTblsvSHRxC32lqPRkcW93FrM8EAf63TxNWKHt9JUJJGbLm1nSROnZ4Ckdn0W6TaFEdyuhC-G1zhD4AAThx2LxqZ5Y_zfUKiPPbBCz4R4vVvp0e3nyYn0hdB1n0Cn3x3L3PTQCBstybS7vYacesn6H-CI6oSHGpfbiy8w2YNjdonDX2ligRTfDWi_AW7NfuL2-ZnscnYaRaoskk_ygU7mtHyVL0vTS8RdKzYebT8mqT6tNY2xg01BOY8GGMVzap2ZBJ1zGthO18MUAboj22jbXU_uhSy8bscJRwvOxqVA");
  assert_true(jwt.verify(pkey, false));
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_claim_set),
    cmocka_unit_test(test_header),
    cmocka_unit_test(test_sign_verify),
    cmocka_unit_test(test_jwt_sign_verification),
    cmocka_unit_test(test_jwt_sign_verify_b64),
  };
  return cmocka_run_group_tests(tests, nullptr, nullptr);
}
