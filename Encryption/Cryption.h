#pragma once
#include <string>
#include <QDate>

// easy algorithm to cryption and decryption date, just map the char in key
std::string encryptDate(QDate& date);

// decrypt
QDate decryptDate(const std::string& data);

