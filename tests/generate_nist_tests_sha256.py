#!/usr/bin/python

import os
import sys

def get_file_name(path):
    base = os.path.basename(path)
    return os.path.splitext(base)[0]

def main():
    if len(sys.argv) != 2:
        print("Drag a .rsp file onto this script.\nOne that contains the structure\n\tLen = ...\n\tMsg = ...\n\tMD = ...\nProvided by the NIST:\nhttps://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/Secure-Hashing\nhttps://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/documents/shs/shabytetestvectors.zip")
        return
    file = open(sys.argv[1], 'r')
    outfile_name = f'tests_{get_file_name(sys.argv[1])}.sp'
    outfile = open(outfile_name, 'w')

    maxlen = 0      # Length of message in bits
    msg = ''        # Message bytes (in hex)
    digest = ''     # Expected Hash
    count = 0
    
    outfile.write('#include <sourcemod>\n#include <sha256>\n\n')
    
    for line in file.readlines():
        if line.startswith('#') or line.startswith('['):
            continue
        elif line.startswith('Len'):
            x = int(line[5:].strip())       # 'Len = '
            maxlen = x if x > maxlen else maxlen
        elif line.startswith('Msg'):
            msg = line[5:].strip()          # 'Msg = '
        elif line.startswith('MD'):
            digest = line[4:].strip()       # 'MD = '
        else:
            # Check we have a valid test before generating it
            if len(msg) < 1 or len(digest) < 1:
                msg = ''
                digest = ''
                continue

            # Compiler explodes if a line is longer than 4096 chars
            msg_split_lines = [msg[idx:idx + 1000] for idx in range(0, len(msg), 1000)]

            outfile.write('void SHATest_{0}(char[] output, int size)\n'.format(count))
            outfile.write('{\n')
            outfile.write('    SHA256(\"\"\n')

            for line in msg_split_lines:
                outfile.write('        ... \"{0}\"\n'.format(line))

            outfile.write('        ,\n{0}output,\n{1}size,\n{2}String_Hex);\n'.format(8 * ' ', 8 * ' ', 8 * ' '))
            outfile.write('    if (StrEqual(output, \"{0}\"))\n'.format(digest))
            outfile.write('    {\n')
            outfile.write('        PrintToServer("PASSED: SHATest_{0}");\n'.format(count))
            outfile.write('    }\n')
            outfile.write('    else\n')
            outfile.write('    {\n')
            outfile.write('        PrintToServer("FAILED: SHATest_{0}\\n    Expected: {1}\\n    Actual: %s", output);\n'.format(count, digest))
            outfile.write('    }\n')
            outfile.write('}\n')

            count += 1

            # After printing, clear variables
            msg = ''
            digest = ''
            
    # Create OnPluginStart to execute each test
    # They're all placed in separate scopes to prevent memory issues
    outfile.write('public void OnPluginStart()\n{\n')
    outfile.write(f'    char output[{(maxlen // 8) + 1}];\n')
    for i in range(0, count):
        outfile.write(f'    SHATest_{i}(output, sizeof(output));\n')
    outfile.write('}\n')

    file.close()
    outfile.close()

    print(f'Maxlen = {maxlen} bits')
    print(f'Wrote tests to file {outfile_name}')
    input('Press enter to close.')


if __name__ == "__main__":
    main()