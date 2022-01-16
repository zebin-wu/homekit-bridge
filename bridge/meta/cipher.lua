---@class cipherlib
local cipher = {}

---@alias CipherType
---| '"AES-128-ECB"'            # AES cipher with 128-bit ECB mode.
---| '"AES-192-ECB"'            # AES cipher with 192-bit ECB mode.
---| '"AES-256-ECB"'            # AES cipher with 256-bit ECB mode.
---| '"AES-128-CBC"'            # AES cipher with 128-bit CBC mode.
---| '"AES-192-CBC"'            # AES cipher with 192-bit CBC mode.
---| '"AES-256-CBC"'            # AES cipher with 256-bit CBC mode.
---| '"AES-128-CFB128"'         # AES cipher with 128-bit CFB128 mode.
---| '"AES-192-CFB128"'         # AES cipher with 192-bit CFB128 mode.
---| '"AES-256-CFB128"'         # AES cipher with 256-bit CFB128 mode.
---| '"AES-128-CTR"'            # AES cipher with 128-bit CTR mode.
---| '"AES-192-CTR"'            # AES cipher with 192-bit CTR mode.
---| '"AES-256-CTR"'            # AES cipher with 256-bit CTR mode.
---| '"AES-128-GCM"'            # AES cipher with 128-bit GCM mode.
---| '"AES-192-GCM"'            # AES cipher with 192-bit GCM mode.
---| '"AES-256-GCM"'            # AES cipher with 256-bit GCM mode.
---| '"CAMELLIA-128-ECB"'       # Camellia cipher with 128-bit ECB mode.
---| '"CAMELLIA-192-ECB"'       # Camellia cipher with 192-bit ECB mode.
---| '"CAMELLIA-256-ECB"'       # Camellia cipher with 256-bit ECB mode.
---| '"CAMELLIA-128-CBC"'       # Camellia cipher with 128-bit CBC mode.
---| '"CAMELLIA-192-CBC"'       # Camellia cipher with 192-bit CBC mode.
---| '"CAMELLIA-256-CBC"'       # Camellia cipher with 256-bit CBC mode.
---| '"CAMELLIA-128-CFB128"'    # Camellia cipher with 128-bit CFB128 mode.
---| '"CAMELLIA-192-CFB128"'    # Camellia cipher with 192-bit CFB128 mode.
---| '"CAMELLIA-256-CFB128"'    # Camellia cipher with 256-bit CFB128 mode.
---| '"CAMELLIA-128-CTR"'       # Camellia cipher with 128-bit CTR mode.
---| '"CAMELLIA-192-CTR"'       # Camellia cipher with 192-bit CTR mode.
---| '"CAMELLIA-256-CTR"'       # Camellia cipher with 256-bit CTR mode.
---| '"CAMELLIA-128-GCM"'       # Camellia cipher with 128-bit GCM mode.
---| '"CAMELLIA-192-GCM"'       # Camellia cipher with 192-bit GCM mode.
---| '"CAMELLIA-256-GCM"'       # Camellia cipher with 256-bit GCM mode.
---| '"DES-ECB"'                # DES cipher with ECB mode.
---| '"DES-CBC"'                # DES cipher with CBC mode.
---| '"DES-EDE-ECB"'            # DES cipher with EDE ECB mode.
---| '"DES-EDE-CBC"'            # DES cipher with EDE CBC mode.
---| '"DES-EDE3-ECB"'           # DES cipher with EDE3 ECB mode.
---| '"DES-EDE3-CBC"'           # DES cipher with EDE3 CBC mode.
---| '"BLOWFISH-ECB"'           # Blowfish cipher with ECB mode.
---| '"BLOWFISH-CBC"'           # Blowfish cipher with CBC mode.
---| '"BLOWFISH-CFB64"'         # Blowfish cipher with CFB64 mode.
---| '"BLOWFISH-CTR"'           # Blowfish cipher with CTR mode.
---| '"ARC4-128"'               # RC4 cipher with 128-bit mode.
---| '"AES-128-CCM"'            # AES cipher with 128-bit CCM mode.
---| '"AES-192-CCM"'            # AES cipher with 192-bit CCM mode.
---| '"AES-256-CCM"'            # AES cipher with 256-bit CCM mode.
---| '"CAMELLIA-128-CCM"'       # Camellia cipher with 128-bit CCM mode.
---| '"CAMELLIA-192-CCM"'       # Camellia cipher with 192-bit CCM mode.
---| '"CAMELLIA-256-CCM"'       # Camellia cipher with 256-bit CCM mode.
---| '"ARIA-128-ECB"'           # Aria cipher with 128-bit key and ECB mode.
---| '"ARIA-192-ECB"'           # Aria cipher with 192-bit key and ECB mode.
---| '"ARIA-256-ECB"'           # Aria cipher with 256-bit key and ECB mode.
---| '"ARIA-128-CBC"'           # Aria cipher with 128-bit key and CBC mode.
---| '"ARIA-192-CBC"'           # Aria cipher with 192-bit key and CBC mode.
---| '"ARIA-256-CBC"'           # Aria cipher with 256-bit key and CBC mode.
---| '"ARIA-128-CFB128"'        # Aria cipher with 128-bit key and CFB-128 mode.
---| '"ARIA-192-CFB128"'        # Aria cipher with 192-bit key and CFB-128 mode.
---| '"ARIA-256-CFB128"'        # Aria cipher with 256-bit key and CFB-128 mode.
---| '"ARIA-128-CTR"'           # Aria cipher with 128-bit key and CTR mode.
---| '"ARIA-192-CTR"'           # Aria cipher with 192-bit key and CTR mode.
---| '"ARIA-256-CTR"'           # Aria cipher with 256-bit key and CTR mode.
---| '"ARIA-128-GCM"'           # Aria cipher with 128-bit key and GCM mode.
---| '"ARIA-192-GCM"'           # Aria cipher with 192-bit key and GCM mode.
---| '"ARIA-256-GCM"'           # Aria cipher with 256-bit key and GCM mode.
---| '"ARIA-128-CCM"'           # Aria cipher with 128-bit key and CCM mode.
---| '"ARIA-192-CCM"'           # Aria cipher with 192-bit key and CCM mode.
---| '"ARIA-256-CCM"'           # Aria cipher with 256-bit key and CCM mode.
---| '"AES-128-OFB"'            # AES 128-bit cipher in OFB mode.
---| '"AES-192-OFB"'            # AES 192-bit cipher in OFB mode.
---| '"AES-256-OFB"'            # AES 256-bit cipher in OFB mode.
---| '"AES-128-XTS"'            # AES 128-bit cipher in XTS block mode.
---| '"AES-256-XTS"'            # AES 256-bit cipher in XTS block mode.
---| '"CHACHA20"'               # ChaCha20 stream cipher.
---| '"CHACHA20-POLY1305"'      # ChaCha20-Poly1305 AEAD cipher.

---@alias CipherPadding
---|>'"NONE"'           # Never pad (full blocks only).
---| '"PKCS7"'          # PKCS7 padding (default).
---| '"ISO7816_4"'      # ISO/IEC 7816-4 padding.
---| '"ANSI923"'        # ANSI X.923 padding.
---| '"ZERO"'           # Zero padding (not reversible).

---@class _cipher:userdata Cipher context.
local _cipher = {}

---Create a cipher context.
---@param type CipherType Cipher type.
---@return _cipher context Cipher context.
---@nodiscard
function cipher.create(type) end

---Return the length of the key in bytes.
---@return integer bytes The key length.
---@nodiscard
function _cipher:getKeyLen() end

---Return the length of the initialization vector (IV) in bytes.
---@return integer bytes The IV length.
---@nodiscard
function _cipher:getIVLen() end

---Set the padding mode, for cipher modes that use padding.
---@param padding CipherPadding The padding mode.
function _cipher:setPadding(padding) end

---Begin a encryption/decryption process.
---@param op '"encrypt"'|'"decrypt"'   Operation.
---@param key string The key to use.
---@param iv? string The initialization vector (IV).
function _cipher:begin(op, key, iv) end

---The generic cipher update function.
---@param input string Input binary data.
---@return string output Output binary data.
---@nodiscard
function _cipher:update(input) end

---Finsh the encryption/decryption process.
---@return string output Output binary data.
---@nodiscard
function _cipher:finsh() end

return cipher
