#include "Cryption.h"
#include <vector>
using namespace std;

const char key[11] = "GZCFAEXBYD";

// encrypt: encrypt date to string
// first convert date to string like 20170123, then encrypt the date string
std::string encryptDate(QDate & date) {
	string dateStr = date.toString("yyyyMMdd").toStdString();
	string result;
	result.resize(dateStr.size());
	for (int i = 0; i < dateStr.size(); ++i) {
		int index = dateStr[i] - '0';
		result[i] = key[index];
	}

	return result;
}


QDate decryptDate(const std::string & data) {
	string dateStr;
	dateStr.resize(data.size());
	for (int i = 0; i < dateStr.size(); ++i) {
		char ch = data[i];
		int num(0);
		while (key[num] != ch) {
			++num;
		}
		dateStr[i] = num + '0';
	}

	return QDate::fromString(QString::fromStdString(dateStr), "yyyyMMdd");
}
