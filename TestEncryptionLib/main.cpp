#include "EncryptionDialog.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);

	EncryptionDialog* encrypDlg = new EncryptionDialog("HKEY_CURRENT_USER\\SOFTWARE\\VehicleRoutingProblem", "Expiration");
	QWidget* mainWidget = new QWidget;
	mainWidget->setWindowTitle("Hello World");
	QObject::connect(encrypDlg, &EncryptionDialog::licensePass, mainWidget, [&](bool pass) {
		if (pass)
			mainWidget->show();
		else
			mainWidget->close();
	});
	encrypDlg->checkLicense();

	return a.exec();
}
