# SourcePawn SHA-256
An single-include file that implements a SHA-256 hashing function in native SourcePawn.  


## Disclaimer
I make no guarantees that this code is even fully functional, let alone bug free.  
Any use of this code for security purposes is entirely at your own risk.  

Also, *do not use SHA-256 for security*, doofus. Use a password-specific hashing algorithm like bcrypt.


## How to use
```sourcepawn
/**
 * Generate a SHA-256 hash of a string of input data.
 * Input data can be in UTF-8, hexadecimal or binary.
 *
 * @param input     Input data (UTF-8 text, hexadecimal bytes or binary bytes).
 * @param output    Output buffer.
 * @param size      Size of output buffer.
 * @param mode      How to interpret input data.
 */
stock void SHA256(const char[] input, char[] output, int size, StrInputMode mode = String_UTF8)

char output[HASH_SIZE_256BIT]; // size 65

/* Text (UTF-8) */
SHA256("The quick brown fox jumps over the lazy dog", output, sizeof(output));
// output = "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"
SHA256("The quick brown fox jumps over the lazy dog?", output, sizeof(output));
// output = "f77bc0c0d0779a6b329c7b9e847acb3797734ae531f08cc9d44cb4124d6e2b16"


/* Hexadecimal bytes (Case-insensitive) */
SHA256("AA0066FFDEADBEEF", output, sizeof(output), String_Hex);
// output = "1a61db0b7dc6ba6812d73eb6acdd2e84271c1124b0ec684192aca6f0933045a8"


/* Binary bytes */
SHA256("011000010110001001100011", output, sizeof(output), String_Binary);
// or SHA256("abc", output, sizeof(output), String_UTF8);
// output = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
```

<hr />

### How do I know it works?
To test the code, I used some of the tests made freely available by the National Institute of Standards and Technology (NIST--the organisation that originally created the algorithm) via their [Cryptographic Algorithm Validation Program](https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/Secure-Hashing "Cryptographic Algorithm Validation Program") (no that doesn\'t mean this code has been validated).  

Included in the repo is a python script `generate_nist_tests_sha256.py` which can process the `SHA256LongMsg.rsp` and `SHA256ShortMsg.rsp` files available under [SHA Test Vectors for Hashing Byte-Oriented Messages](https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/documents/shs/shabytetestvectors.zip "SHA Test Vectors for Hashing Byte-Oriented Messages").  

You can just drop the files onto the script and it generates a plugin (called `tests_SHA256LongMsg.sp` or `tests_SHA256ShortMsg.sp`) that executes the tests.  
*Double-check the tests actually generated correctly because my python code is janky.*  


### Hey the \'00\' (null string) test is failing! Your code sucks!
No, the test is wrong.  
You heard me.  
That specific test assumes that a null string (length of 0) is the same as a *null byte* (0x00, length of 1).  
I will die on this hill. Null means no data, null byte is just a 0.  
Other SHA-256 implementations also have this same quirk (7-Zip for example).  

