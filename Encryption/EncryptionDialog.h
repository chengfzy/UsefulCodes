#pragma once
#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>

class EncryptionDialog : public QDialog {
	Q_OBJECT

public:
	EncryptionDialog(QString reg, QString key);
	~EncryptionDialog();

signals:
	void licensePass(bool pass);

public:
	// check license and show dialog if license is expired
	void checkLicense();


private:
	bool isLicenseValid(const QString& license);
	void checkInput();

private:
	QString reg_;
	QString key_;
	QLabel* infoLabel_;
	QTextEdit* licenseEdit_;
};
