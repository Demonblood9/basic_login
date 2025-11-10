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
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QProgressDialog>
#include <QNetworkRequest>
#include <QTemporaryFile>
#include <QStandardPaths>
#include "version.h"

// QuaZip for handling ZIP files
#ifdef Q_OS_WIN
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentVersion(APP_VERSION_STRING)  // Auto-generated from VERSION file
    , downloadReply(nullptr)
{
    ui->setupUi(this);

    // Initialize settings (encrypted storage)
    settings = new QSettings("SecureAuth", "LicenseClient");

    // Initialize network manager for login
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::handleNetworkReply);

    // Initialize separate network manager for updates (to avoid signal conflicts)
    updateNetworkManager = new QNetworkAccessManager(this);

    // Load saved settings
    bool rememberKey = settings->value("rememberKey", false).toBool();
    ui->rememberCheckBox->setChecked(rememberKey);

    // Check for updates on startup
    checkForUpdates();

    // Auto-login if key is saved (with delay to allow update check)
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
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    QList<QNetworkInterface>::const_iterator it;
    for (it = interfaces.constBegin(); it != interfaces.constEnd(); ++it) {
        if (!(it->flags() & QNetworkInterface::IsLoopBack)) {
            components << it->hardwareAddress();
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

// ============================================================================
// AUTO-UPDATE FUNCTIONS
// ============================================================================

void MainWindow::checkForUpdates()
{
    QUrl url("http://localhost:5000/api/version/check");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = updateNetworkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleUpdateCheckReply(reply);
    });
}

void MainWindow::handleUpdateCheckReply(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Update check failed:" << reply->errorString();
        return; // Silently fail
    }

    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject obj = doc.object();

    bool updateAvailable = obj["update_available"].toBool();
    QString serverVersion = obj["version"].toString();
    qint64 fileSize = obj["file_size"].toInteger();
    QString releaseNotes = obj["release_notes"].toString();
    QString downloadUrl = obj["download_url"].toString();

    // Only proceed with update if there's a version mismatch
    if (!updateAvailable || serverVersion.isEmpty()) {
        return;
    }

    if (serverVersion == currentVersion) {
        return;
    }

    // Show mandatory update dialog
    QString updateMessage = QString("A mandatory update is required!\n\n"
                             "Current version: %1\n"
                             "New version: %2\n"
                             "Size: %3 MB\n\n")
                        .arg(currentVersion)
                        .arg(serverVersion)
                        .arg(fileSize / 1024.0 / 1024.0, 0, 'f', 2);

    if (!releaseNotes.isEmpty()) {
        updateMessage += "Release Notes:\n" + releaseNotes + "\n\n";
    }

    updateMessage += "The application will now download and install the update.\n"
               "This is required to continue using the application.";

    QMessageBox::information(this, "Mandatory Update", updateMessage);

    // Force download - no choice
    downloadUpdate(downloadUrl, serverVersion);
}

void MainWindow::downloadUpdate(const QString &downloadUrl, const QString &version)
{
    QUrl url(downloadUrl);
    QNetworkRequest request(url);

    QNetworkReply *reply = updateNetworkManager->get(request);
    downloadReply = reply;

    // Create progress dialog (no cancel button - mandatory update)
    QProgressDialog *progress = new QProgressDialog("Downloading mandatory update...", QString(), 0, 100, this);
    progress->setWindowModality(Qt::ApplicationModal);
    progress->setMinimumDuration(0);
    progress->setValue(0);
    progress->setCancelButton(nullptr);  // Remove cancel button
    progress->setWindowFlags(progress->windowFlags() & ~Qt::WindowCloseButtonHint);  // Disable close button

    // Connect download progress
    connect(reply, &QNetworkReply::downloadProgress, this, [progress](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            int percent = (int)((bytesReceived * 100) / bytesTotal);
            progress->setValue(percent);
        }
    });

    // Handle download completion
    connect(reply, &QNetworkReply::finished, this, [this, reply, progress, version]() {
        progress->close();
        progress->deleteLater();
        reply->deleteLater();
        downloadReply = nullptr;

        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::critical(this, "Mandatory Update Failed",
                                 "Failed to download mandatory update:\n" + reply->errorString() +
                                 "\n\nThe application cannot continue without this update.\n"
                                 "The application will now close.");
            QApplication::quit();
            return;
        }

        // Save update file
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString updateFilePath = tempDir + "/update_v" + version + ".zip";

        QFile file(updateFilePath);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Mandatory Update Failed",
                                 "Failed to save update file:\n" + file.errorString() +
                                 "\n\nThe application cannot continue without this update.\n"
                                 "The application will now close.");
            QApplication::quit();
            return;
        }

        file.write(reply->readAll());
        file.close();

        // Apply update
        if (applyUpdate(updateFilePath)) {
            QMessageBox::information(this, "Update Complete",
                                   "Update has been downloaded successfully.\n"
                                   "The application will now restart to apply the update.");

            // Batch script will restart the application, just quit here
            QApplication::quit();
        } else {
            QMessageBox::critical(this, "Mandatory Update Failed",
                                "Failed to apply the mandatory update.\n\n"
                                "The application cannot continue without this update.\n"
                                "The application will now close.\n\n"
                                "Please contact support or reinstall the application.");
            QApplication::quit();
        }
    });
}

bool MainWindow::applyUpdate(const QString &updateFilePath)
{
    // For Windows, we'll create a batch script to replace files after app closes
    #ifdef Q_OS_WIN
    QString appDir = QCoreApplication::applicationDirPath();
    QString batchPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/update.bat";

    QFile batchFile(batchPath);
    if (!batchFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&batchFile);
    out << "@echo off\n";
    out << "echo Applying update...\n";
    out << "timeout /t 2 /nobreak >nul\n";  // Wait for app to close
    
    // Use PowerShell to extract ZIP
    out << "powershell -Command \"Expand-Archive -Path '" << updateFilePath << "' -DestinationPath '" << appDir << "' -Force\"\n";
    
    out << "if %errorlevel% neq 0 (\n";
    out << "    echo Update failed!\n";
    out << "    pause\n";
    out << "    exit /b 1\n";
    out << ")\n";
    out << "echo Update complete!\n";
    
    // Restart the application
    out << "start \"\" \"" << QCoreApplication::applicationFilePath() << "\"\n";
    
    // Delete the batch file and update zip
    out << "del \"" << updateFilePath << "\"\n";
    out << "del \"%~f0\"\n";  // Delete self
    
    batchFile.close();

    // Execute the batch file
    QProcess::startDetached("cmd.exe", QStringList() << "/c" << batchPath);
    
    return true;
    #else
    // For Linux/Mac, you'd need to implement similar logic
    // This is a simplified version
    return false;
    #endif
}
