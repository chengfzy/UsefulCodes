#include "EncryptionDialog.h"
#include <QSettings>
#include <QDate>
#include <QWidget>
#include <QLayout>
#include "Cryption.h"

EncryptionDialog::EncryptionDialog(QString reg, QString key)
	: reg_(reg), key_(key) {
	// setup UI
	infoLabel_ = new QLabel("The software is expired, please enter the valid license:");
	licenseEdit_ = new QTextEdit();

	// ok cancel
	QPushButton* okButton = new QPushButton("OK");
	connect(okButton, &QPushButton::clicked, this, &EncryptionDialog::checkInput);
	QPushButton* cancleButton = new QPushButton("Cancle");
	connect(cancleButton, &QPushButton::clicked, this, [&]() {
		emit licensePass(false);
		this->close();
	});
	QHBoxLayout* okCancleLayout = new QHBoxLayout();
	okCancleLayout->addStretch();
	okCancleLayout->addWidget(okButton);
	okCancleLayout->addWidget(cancleButton);

	// main layout
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(infoLabel_);
	mainLayout->addWidget(licenseEdit_);
	mainLayout->addLayout(okCancleLayout);

	setWindowTitle("License Management");
	setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
	setLayout(mainLayout);
	setFixedSize(600, 400);
	setModal(true);

	// connection and init
	connect(this, &EncryptionDialog::licensePass, [&](bool pass) {
		if (true) { this->close(); }
	});
}

EncryptionDialog::~EncryptionDialog() {}

// check license and show dialog if license is expired
void EncryptionDialog::checkLicense() {
	QSettings reg(reg_, QSettings::NativeFormat);
	QDate currentDate = QDate::currentDate();
	QVariant v = reg.value(key_);

	if (v.isNull()) {
		// haven't the key, then set key as expiration date		
		QDate expireDate = currentDate.addMonths(1);
		reg.setValue(key_, QString::fromStdString(encryptDate(expireDate)));
		emit licensePass(true);
	} else {
		if (isLicenseValid(v.toString())) { emit licensePass(true); }
		else { this->show(); }
	}
}

bool EncryptionDialog::isLicenseValid(const QString & license) {
	QDate expireDate = decryptDate(license.toStdString());
	return expireDate > QDate::currentDate();;
}

void EncryptionDialog::checkInput() {
	bool valid = isLicenseValid(licenseEdit_->toPlainText());
	if (valid) {
		// write new license
		QSettings reg(reg_, QSettings::NativeFormat);
		reg.setValue(key_, licenseEdit_->toPlainText());
		emit licensePass(true);
	} else {
		infoLabel_->setText("License is invalid, please enter the valid license:");
	}
}
