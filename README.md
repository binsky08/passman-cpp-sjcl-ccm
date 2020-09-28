# Passman optimized AES-CCM c++ implementation based on the SJCL.js implementation

This is a very similar implementation as in the Passman Android project.

It is helpful to test and debug encryption stuff on your local computer instead of using the C implementation of Android apps with JNI.

## Install dependencies

- Clone the repo
- Setup the git submodules with `git submodule update --init --recursive`
- Make openssl (for your AMD64 machine):
```bash
cd libs/openssl
./Configure
make
cd -
```
- Make SimpleJSON (for your AMD64 machine):
```bash
cd libs/SimpleJSON
make depend
make all
cd -
```

## Usage

- Use the CLion IDE to easily run the project

- Run manually from command line

```bash
g++ -o main libs/base64.cpp libs/json.hpp libs/openssl/libcrypto.a libs/openssl/libssl.a libs/SimpleJSON/src/JSON.cpp libs/SimpleJSON/src/JSONValue.cpp main.cpp -lcrypto -lssl
./main
```
