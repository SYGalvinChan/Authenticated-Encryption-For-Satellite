from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import json
import random

def main():
    # Parameters to play around with
    json_file_name = "testcases.json"
    c_code_file_name = "c_code.txt"
    text_file_name = "output.txt"

    num_testcases = 10
    min_payload_length = 1
    max_payload_length = 100

    seed = 1

    random.seed(seed)

    testcases_json = []
    testcases_c_code = []
    testcases_text = []

    for i in range(num_testcases):
        # Create random Crypto Packet
        payload_length = random.randrange(min_payload_length, max_payload_length)        
        crypto_header = generate_crypto_header(payload_length)
        plaintext = generate_random_hex(payload_length)

        # Create random Key
        key = generate_random_hex(16)

        # Encrypt Payload
        iv = crypto_header
        aad = "00000000" + crypto_header

        ciphertext, tag = encrypt(key, plaintext, iv, aad)

        # Decrypt for sanity check
        plaintext_d = decrypt(key, ciphertext + tag, iv, aad)
        if (plaintext_d != plaintext):
            print("ERROR: DECRYPTED PLAINTEXT DIFFERENT FROM ORIGINAL PLAINTEXT")
            return

        testcase = {}
        testcase["key"] = key
        testcase["crypto_header"] = crypto_header
        testcase["plaintext"] = plaintext
        testcase["ciphertext"] = ciphertext
        testcase["tag"] = tag        
        testcases_json.append(testcase)

        c_code = f"Testcase {i}:\n"
        c_code += format_c_code("key", key)
        c_code += format_c_code("crypto_header", crypto_header)
        c_code += format_c_code("plaintext", plaintext)
        c_code += format_c_code("ciphertext", ciphertext)
        c_code += format_c_code("tag", tag)
        testcases_c_code.append(c_code)

        text = f"Testcase {i}:\n"
        text += format_text("key", key)
        text += format_text("crypto_header", crypto_header)
        text += format_text("plaintext", plaintext)
        text += format_text("ciphertext", ciphertext)
        text += format_text("tag", tag)
        testcases_text.append(text)
    
    json_file = open(json_file_name, "w")
    json_file.write(json.dumps(testcases_json, indent=2))
    json_file.close()

    c_code_file = open(c_code_file_name, "w")
    c_code_file.write("\n".join(testcases_c_code))
    c_code_file.close()

    text_file = open(text_file_name, "w")
    text_file.write("\n".join(testcases_text))
    text_file.close()

def format_text(var_name, value):
    chunks = [value[i:i+32] for i in range(0, len(value), 32)]
    out = (var_name + ":").ljust(20) + ' '.join(chunks) + "\n"
    return out

def format_c_code(var_name, value):
    string = ("char " + var_name + "[]").ljust(20) + " = {"
    blocks = [value[i:i+32] for i in range(0, len(value), 32)]
    blocks = [", ".join(["0x" + block[i:i+2] for i in range(0, len(block), 2)]) for block in blocks]
    string += (",\n" + " " * len(string)).join(blocks)
    string += "};\n"
    return string
    
def generate_crypto_header(payload_length):
    possible_directions = ["11", "12", "13", "11", "21", "31"]

    direction = ''.join(random.choices(possible_directions))
    key_index = generate_random_hex(1)
    payload_length = hex(payload_length)[2:].zfill(4)
    replay_counters = generate_random_hex(8)    

    return direction + key_index + payload_length + replay_counters

def generate_random_hex(num_bytes):
    return ''.join(random.choices("0123456789abcdef", k=num_bytes * 2))

def encrypt(key_s: str, plaintext_s: str, iv_s: str, aad_s: str):
    iv = bytes.fromhex(iv_s)
    key = bytes.fromhex(key_s)
    aad = bytes.fromhex(aad_s)
    plaintext = bytes.fromhex(plaintext_s)
    aesgcm = AESGCM(key)
    output = aesgcm.encrypt(iv, plaintext, aad)
    cipher = output[:-16]
    tag = output[-16:]
    return cipher.hex(), tag.hex()

def decrypt(key_s: str, cipher_s: str, iv_s: str, aad_s: str):
    iv = bytes.fromhex(iv_s)
    key = bytes.fromhex(key_s)
    aad = bytes.fromhex(aad_s)
    cipher = bytes.fromhex(cipher_s)
    aesgcm = AESGCM(key)
    return aesgcm.decrypt(iv, cipher, aad).hex()

if __name__ == "__main__":
    main()
