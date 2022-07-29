/**
 * SourcePawn SHA-256
 *
 * Copyright (C) 2022 SirDigbot (GitHub username)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#if defined _sha256_included
    #endinput
#endif
#define _sha256_included


#define HASH_SIZE_256BIT 65 // Number of bytes needed to fit a 256-bit hash string


//#define DEBUG_MESSAGEBLOCK
//#define DEBUG_MESSAGESCHEDULE
//#define DEBUG_EXTENDMESSAGE


/**
 * How to interpret a string literal of data.
 */
enum StrInputMode
{
    String_UTF8 = 0,    // Raw bytes (UTF-8 text)
    String_Hex,         // Hexadecimal bytes (2 hex chars per byte)
    String_Binary       // Binary bytes (8 binary chars per byte)
}


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
{
    // With thanks to https://en.wikipedia.org/wiki/SHA-2, https://sha256algorithm.com/ and Peace-Maker
    // For reference https://www.rfc-editor.org/rfc/rfc6234#section-8.2.2


    // Verify inputs
    int len = strlen(input);
    if (mode == String_Hex && len % 2 == 1)
        ThrowError("Length of hexadecimal byte string must be a mutliple of 2 (half a byte is not allowed)");
    else if (mode == String_Binary && len % 8 != 0)
        ThrowError("Length of binary byte string must be a mutliple of 8 (input is currently limited to bytes, not bits)");
    // TODO: make binary accept arbitrary amounts of bits?


    // Below here, this function contains the unedited psuedocode from
    // wikipedia so you can independently verify it works.


    // Note 1: All variables are 32 bit unsigned integers and addition is calculated modulo 2^32
    // Note 2: For each round, there is one round constant k[i] and one entry in the message schedule array w[i], 0 ≤ i ≤ 63
    // Note 3: The compression function uses 8 working variables, a through h
    // Note 4: Big-endian convention is used when expressing the constants in this pseudocode,
    //     and when parsing message block data from bytes to words, for example,
    //     the first word of the input message "abc" after padding is 0x61626380
    

    // Initialize hash values:
    // (first 32 bits of the fractional parts of the square roots of the first 8 primes 2..19):
    int hashValues[8] = 
    {
        0x6a09e667, // h0
        0xbb67ae85, // h1
        0x3c6ef372, // h2
        0xa54ff53a, // h3
        0x510e527f, // h4
        0x9b05688c, // h5
        0x1f83d9ab, // h6
        0x5be0cd19  // h7
    };

    // Initialize array of round constants:
    // (first 32 bits of the fractional parts of the cube roots of the first 64 primes 2..311):
    int roundConstants[64] = 
    {
       0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
       0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
       0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
       0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
       0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
       0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
       0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
       0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };
    
    // Pre-processing (Padding):
    // begin with the original message of length L bits
    // append a single '1' bit
    // append K '0' bits, where K is the minimum number >= 0 such that (L + 1 + K + 64) is a multiple of 512
    // append L as a 64-bit big-endian integer, making the total post-processed length a multiple of 512 bits
    // such that the bits in the message are: <original message of length L> 1 <K zeros> <L as 64 bit integer> , (the number of bits will be a multiple of 512)
    //
    // In english this means make a buffer that is a multiple of 512 that fits the input (as binary),
    // plus a single 1 bit, plus a 64 bit integer. And put the 64-bit integer last with zeros in between it and the input.
    int L;                              // L = size in bits
    if (mode == String_UTF8)
        L = len * 8;
    else if (mode == String_Hex)
        L = len * 4;
    else if (mode == String_Binary)
        L = len;

    int K_512_diff = (L + 1 + 64);
    int K = LowestMultiplePower2(K_512_diff, 512);  // Fit into lowest multiple of 512
    int K_bytes = K / 32;                           // Fit bits into 32-bit ints
    int[] messageBlock = new int[K_bytes];

    int bytesWritten = CopyStringBytes(mode, input, len, messageBlock, K_bytes);
    messageBlock[bytesWritten / 4] |= (1 << 31 - (bytesWritten * 8));  // Append the 1 bit
    
    // Append length in bits (64-bit int). Sizes in SP are 32-bit max anyway
    if (mode == String_UTF8)
        messageBlock[K_bytes - 1] = len * 8;
    else if (mode == String_Hex)
        messageBlock[K_bytes - 1] = len * 4;
    else if (mode == String_Binary)
        messageBlock[K_bytes - 1] = len;
    
#if defined DEBUG_MESSAGEBLOCK
    PrintToServer("len = %i", len);
    PrintToServer("L = %i", L);
    PrintToServer("K_512_diff = %i", K_512_diff);
    PrintToServer("K = %i", K);
    PrintToServer("K_bytes = %i", K_bytes);
    PrintToServer("bytesWritten = %i", bytesWritten);
    PrintToServer("Append 1 bit index %i, 1 bit OR'd value: %032b", bytesWritten / 4, (1 << 31 - (bytesWritten * 8)));
    PrintToServer("Length: %i bits, index %i", messageBlock[K_bytes - 1], K_bytes - 1);
    for (int i = 0; i < K_bytes; ++i)
        PrintToServer("MSGBLOCK[%i]:%032b", i, messageBlock[i]);
#endif

    // Process the message in successive 512-bit chunks:
    // break message into 512-bit chunks
    // for each chunk
    //     create a 64-entry message schedule array w[0..63] of 32-bit words
    // (continued in SHA256_ProcessChunk)
#if defined DEBUG_MESSAGESCHEDULE
    PrintToServer("SCHED offset < %i", K_bytes);
#endif
    int messageSchedule[64];
    for (int offset = 0; offset < K_bytes; offset += (512 / 32)) // Each chunk is 512 bit / 32 bit ints
    {
        SHA256_ProcessChunk(
            messageBlock[offset],
            K_bytes - offset,
            messageSchedule,
            hashValues,
            roundConstants);
#if defined DEBUG_MESSAGESCHEDULE
        for (int i = 0; i < 64; ++i)
            PrintToServer("SCHED[%i/%i] %032b", offset, i, messageSchedule[i]);
        PrintToServer("-----------");
#endif
    }

    // Produce the final hash value (big-endian):
    // digest := hash := h0 append h1 append h2 append h3 append h4 append h5 append h6 append h7
    FormatEx(output, size, "%08x%08x%08x%08x%08x%08x%08x%08x",
        hashValues[0],
        hashValues[1],
        hashValues[2],
        hashValues[3],
        hashValues[4],
        hashValues[5],
        hashValues[6],
        hashValues[7]);
}

static void SHA256_ProcessChunk(
    int[] messageBlock,
    int messageSize,
    int messageSchedule[64],
    int hashValues[8],
    int roundConstants[64])
{
    // Process the message in successive 512-bit chunks:
    // break message into 512-bit chunks
    // for each chunk
    //     create a 64-entry message schedule array w[0..63] of 32-bit words
    //     (The initial values in w[0..63] don't matter, so many implementations zero them here)
    //     copy chunk into first 16 words w[0..15] of the message schedule array
    for (int i = 0; i < 16 && i < messageSize; ++i)
    {
        messageSchedule[i] = messageBlock[i];
        messageBlock[i] = 0; // Erase potentially sensitive data after we're done with it. (RFC 6234 SHA224_256Finalize)
    }

    // Extend the first 16 words into the remaining 48 words w[16..63] of the message schedule array:
    // for i from 16 to 63
    //     s0 := (w[i-15] rightrotate  7) xor (w[i-15] rightrotate 18) xor (w[i-15] rightshift  3)
    //     s1 := (w[i-2] rightrotate 17) xor (w[i-2] rightrotate 19) xor (w[i-2] rightshift 10)
    //     w[i] := w[i-16] + s0 + w[i-7] + s1
    for (int i = 16; i < 64; ++i)
    {
        int s0 = RightRot(messageSchedule[i-15], 7) ^ RightRot(messageSchedule[i-15], 18) ^ (messageSchedule[i-15] >>> 3);
        int s1 = RightRot(messageSchedule[i-2], 17) ^ RightRot(messageSchedule[i-2], 19) ^ (messageSchedule[i-2] >>> 10);
        messageSchedule[i] = messageSchedule[i-16] + s0 + messageSchedule[i-7] + s1; // Modulus addition (mod 2^32)

#if defined DEBUG_EXTENDMESSAGE
        PrintToServer(
            "EXTEND[%i]\n"
            ... "     %032b\n"
            ... "     %032b XOR\n"
            ... "     %032b XOR\n"
            ... "s0 = %032b\n"
            ... "     %032b\n"
            ... "     %032b XOR\n"
            ... "     %032b XOR\n"
            ... "s1 = %032b\n"
            ... "messageSchedule[%i] = %032b",
            i,
            RightRot(messageSchedule[i-15], 7),
            RightRot(messageSchedule[i-15], 18),
            (messageSchedule[i-15] >>> 3),
            s0,
            RightRot(messageSchedule[i-2], 17),
            RightRot(messageSchedule[i-2], 19),
            (messageSchedule[i-2] >>> 10),
            s1,
            i,
            messageSchedule[i]);
#endif
    }
    
    // Initialize working variables to current hash value:
    int a = hashValues[0];
    int b = hashValues[1];
    int c = hashValues[2];
    int d = hashValues[3];
    int e = hashValues[4];
    int f = hashValues[5];
    int g = hashValues[6];
    int h = hashValues[7];
    
    // Compression function main loop:
    // for i from 0 to 63
    //     S1 := (e rightrotate 6) xor (e rightrotate 11) xor (e rightrotate 25)
    //     ch := (e and f) xor ((not e) and g)
    //     temp1 := h + S1 + ch + k[i] + w[i]
    //     S0 := (a rightrotate 2) xor (a rightrotate 13) xor (a rightrotate 22)
    //     maj := (a and b) xor (a and c) xor (b and c)
    //     temp2 := S0 + maj
    // 
    //     h := g
    //     g := f
    //     f := e
    //     e := d + temp1
    //     d := c
    //     c := b
    //     b := a
    //     a := temp1 + temp2
    //
    // TODO: "The computation of the ch and maj values can be optimized the same way as described for SHA-1."

    for (int i = 0; i < 64; ++i)
    {
        int s1 = RightRot(e, 6) ^ RightRot(e, 11) ^ RightRot(e, 25);
        int ch = (e & f) ^ (~e & g);
        int temp1 = h + s1 + ch + roundConstants[i] + messageSchedule[i]; // Modulus addition (mod 2^32)
        int s0 = RightRot(a, 2) ^ RightRot(a, 13) ^ RightRot(a, 22);
        int maj = (a & b) ^ (a & c) ^ (b & c);
        int temp2 = s0 + maj;
        
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    
    // Add the compressed chunk to the current hash value:
    hashValues[0] += a;
    hashValues[1] += b;
    hashValues[2] += c;
    hashValues[3] += d;
    hashValues[4] += e;
    hashValues[5] += f;
    hashValues[6] += g;
    hashValues[7] += h;
}


/**
 * StringToInt for a single byte in hexadecimal (2 chars of 0 to F)
 */
static int ByteFromHex(char l, char r)
{
    char left[2]; // zero-initialised for the null terminator
    char right[2];
    left[0] = l;
    right[0] = r;
    return (StringToInt(left, 16) & 0x0F) * 16 | (StringToInt(right, 16) & 0x0F);
}


/**
 * StringToInt but only for a single 'byte' of binary text (8 chars of 1/0)
 */
static int ByteFromBitChars(const char[] input)
{
    int val = 0;
    for (int i = 0; i < 8; ++i)
        val |= (input[i] & 1) << (7 - i);
    return val;
}


/**
 * Find the smallest multiple of a number (which must be a power of 2)
 * that can contain another number.
 */
static int LowestMultiplePower2(int x, int powerOf2Multiple)
{
    // For non powers of 2 use x + ((512 - (x mod 512)) mod 512)
    // https://en.wikipedia.org/wiki/Data_structure_alignment#Computing_padding
    return (x + powerOf2Multiple - 1) &~ (powerOf2Multiple - 1);
}


/**
 * Binary rotate the bits of a 32-bit integer.
 * e.g. 00010011 --- Rotate right by 1 ---> 10001001
 */
static int RightRot(int x, int bits)
{
    // >> Doesn't work correctly because of signed ints
    return (x >>> bits)|(x << (32 - bits));
}


/**
 * Copy the individual bytes out of a string (made of 32-bit chars with 8-bit values) and pack them
 * into a 32-bit integer array.
 *
 * Additionally, specify the way the input bytes are interpreted:
 *  - String_UTF8: Each char is 1 byte
 *  - String_Hex: Each pair of chars is 1 byte (in hexadecimal)
 *  - String_Binary: Each group of 8 chars is 1 byte (in binary)
 */
static int CopyStringBytes(StrInputMode mode, const char[] source, int sourceLen, int[] dest, int destSize)
{
    if (mode == String_Hex && sourceLen % 2 == 1)
        ThrowError("Length of hexadecimal byte string must be a mutliple of 2 (half a byte is not allowed)");
    else if (mode == String_Binary && sourceLen % 8 != 0)
        ThrowError("Length of binary byte string must be a mutliple of 8 (input is currently limited to bytes, not bits)");

    int byteOffset = 3;
    int destIdx = 0;
    int sourceIdx = 0;
    int written = 0;
    int ch = 0;

    int increment;
    switch (mode)
    {
        case String_UTF8:
        {
            increment = 1;
        }
        
        case String_Hex:
        {
            increment = 2;
        }
        
        case String_Binary:
        {
            increment = 8;
        }
    }


    for ( ; sourceIdx < sourceLen && destIdx < destSize - 1; )
    {
        // Zero out entire int before use
        if (byteOffset == 3)
            dest[destIdx] = 0;

        // Mask source to 8 bits, shift into next free byte in dest
        switch (mode)
        {
            case String_UTF8:
            {
                ch = source[sourceIdx];
            }
            
            case String_Hex:
            {
                ch = ByteFromHex(source[sourceIdx], source[sourceIdx + 1]);
            }
            
            case String_Binary:
            {
                ch = ByteFromBitChars(source[sourceIdx]);
            }
        }
        dest[destIdx] |= ((ch & 0xFF) << (byteOffset * 8));
        ++written;

        if (byteOffset != 0)
            --byteOffset;
        else
        {
            byteOffset = 3;
            ++destIdx;
        }

        sourceIdx += increment;
    }
    return written;
}
