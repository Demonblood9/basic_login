#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QSysInfo>
#include <QNetworkInterface>
#include <QStorageInfo>
#include <QTimer>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialize settings (encrypted storage)
    settings = new QSettings("SecureAuth", "LicenseClient");

    // Initialize network manager
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::handleNetworkReply);

    // Load saved settings
    bool rememberKey = settings->value("rememberKey", false).toBool();
    ui->rememberCheckBox->setChecked(rememberKey);

    // Auto-login if key is saved
    if (rememberKey && hasSavedKey()) {
        QString savedKey = loadKey();
        ui->keyLineEdit->setText(savedKey);
        autoLogin();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    delete settings;
}

QString MainWindow::getHWID()
{
    QString hwid;

    // Combine multiple hardware identifiers for a unique HWID
    QStringList components;

    // 1. Machine ID (Windows)
    components << QSysInfo::machineUniqueId();

    // 2. MAC Address of first network interface
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        if (!(interface.flags() & QNetworkInterface::IsLoopBack)) {
            components << interface.hardwareAddress();
            break; // Use first non-loopback interface
        }
    }

    // 3. System information
    components << QSysInfo::productType();
    components << QSysInfo::kernelVersion();

    // 4. Storage serial (if available)
    QStorageInfo storage = QStorageInfo::root();
    if (storage.isValid()) {
        components << storage.device();
    }

    // Combine and hash
    QString combined = components.join("-");
    QByteArray hash = QCryptographicHash::hash(combined.toUtf8(), QCryptographicHash::Sha256);
    hwid = hash.toHex();

    return hwid;
}

void MainWindow::saveKey(const QString &key)
{
    // Simple XOR obfuscation (not encryption, but better than plaintext)
    QByteArray keyData = key.toUtf8();
    QByteArray obfuscated;

    const char xorKey = 0x5A; // Simple XOR key
    for (int i = 0; i < keyData.size(); ++i) {
        obfuscated.append(keyData[i] ^ xorKey);
    }

    settings->setValue("savedKey", obfuscated.toBase64());
    settings->sync();
}

QString MainWindow::loadKey()
{
    if (!settings->contains("savedKey")) {
        return QString();
    }

    QByteArray obfuscated = QByteArray::fromBase64(settings->value("savedKey").toByteArray());
    QByteArray keyData;

    const char xorKey = 0x5A;
    for (int i = 0; i < obfuscated.size(); ++i) {
        keyData.append(obfuscated[i] ^ xorKey);
    }

    return QString::fromUtf8(keyData);
}

void MainWindow::clearKey()
{
    settings->remove("savedKey");
    settings->sync();
}

bool MainWindow::hasSavedKey()
{
    return settings->contains("savedKey");
}

void MainWindow::attemptLogin(const QString &key)
{
    if (key.isEmpty()) {
        showError("Please enter your license key");
        return;
    }

    // Disable UI during login
    ui->loginButton->setEnabled(false);
    ui->keyLineEdit->setEnabled(false);
    ui->statusLabel->setText("Validating license...");
    ui->statusLabel->setStyleSheet("color: #2a82da;");

    // Get HWID
    QString hwid = getHWID();

    // Prepare request
    QNetworkRequest request(QUrl("http://localhost:5000/api/validate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Create JSON payload
    QJsonObject json;
    json["key"] = key;
    json["hwid"] = hwid;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    // Send request
    networkManager->post(request, data);
}

void MainWindow::handleNetworkReply(QNetworkReply *reply)
{
    // Re-enable UI
    ui->loginButton->setEnabled(true);
    ui->keyLineEdit->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError) {
        showError("Connection error: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    // Parse response
    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject json = doc.object();

    bool success = json["success"].toBool();
    QString message = json["message"].toString();

    if (success) {
        QString username = json["username"].toString();
        QString expires = json["expires_at"].toString();
        int daysRemaining = json["days_remaining"].toInt();
        bool isPermanent = json["is_permanent"].toBool();

        showSuccess(username, expires, daysRemaining, isPermanent);

        // Save key if remember is checked
        if (ui->rememberCheckBox->isChecked()) {
            saveKey(ui->keyLineEdit->text());
            settings->setValue("rememberKey", true);
        }
    } else {
        showError(message);

        // If HWID violation, clear saved key
        if (message.contains("HWID") || message.contains("suspended")) {
            clearKey();
            ui->rememberCheckBox->setChecked(false);
            settings->setValue("rememberKey", false);
        }
    }

    reply->deleteLater();
}

void MainWindow::showSuccess(const QString &username, const QString &expires, int daysRemaining, bool isPermanent)
{
    ui->statusLabel->setText("✓ Authentication Successful!");
    ui->statusLabel->setStyleSheet("color: #10c010; font-weight: bold;");

    QString details;
    if (isPermanent) {
        details = QString("Welcome, %1!\n\nLicense Type: Permanent\nStatus: Active").arg(username);
    } else {
        details = QString("Welcome, %1!\n\nExpires: %2\nDays Remaining: %3")
                      .arg(username)
                      .arg(expires)
                      .arg(daysRemaining);
    }

    ui->detailsLabel->setText(details);
    ui->detailsLabel->setStyleSheet("color: #e0e0e0;");

    QMessageBox::information(this, "Login Successful", details);
}

void MainWindow::showError(const QString &message)
{
    ui->statusLabel->setText("✗ Authentication Failed");
    ui->statusLabel->setStyleSheet("color: #ff3030; font-weight: bold;");

    ui->detailsLabel->setText(message);
    ui->detailsLabel->setStyleSheet("color: #ff6060;");
}

void MainWindow::autoLogin()
{
    // Small delay to show the window first
    QTimer::singleShot(500, this, [this]() {
        attemptLogin(ui->keyLineEdit->text());
    });
}

void MainWindow::on_loginButton_clicked()
{
    attemptLogin(ui->keyLineEdit->text());
}

void MainWindow::on_clearButton_clicked()
{
    ui->keyLineEdit->clear();
    ui->statusLabel->clear();
    ui->detailsLabel->clear();

    if (ui->rememberCheckBox->isChecked()) {
        int ret = QMessageBox::question(this, "Clear Saved Key",
                                        "Do you want to remove the saved license key?",
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            clearKey();
            ui->rememberCheckBox->setChecked(false);
            settings->setValue("rememberKey", false);
        }
    }
}

void MainWindow::on_rememberCheckBox_toggled(bool checked)
{
    settings->setValue("rememberKey", checked);

    if (!checked && hasSavedKey()) {
        clearKey();
    }
}
