# Useful Codes
Some useful codes in C++

## Content
1. ReadWriteFile
	Read and Write File in standard C++, using fstream, stringstream, string, etc.
2. Encryption, TestEncryptionLib
	简单的软件加密与解密，将过期时间字符映射到密文，然后写入注册表进行比较.
	Encryption生成静态lib，TestEncryptionLib为具体的调用方法
	可以升级参考的方法：AES-CBC/ECB加密.
	Ref: 
		[1] PicoSHA2 - a C++ SHA256 hash generator https://github.com/okdshin/PicoSHA2
		[2] Tiny AES in C https://github.com/kokke/tiny-AES-c