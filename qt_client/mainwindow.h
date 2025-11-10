#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_loginButton_clicked();
    void on_clearButton_clicked();
    void on_rememberCheckBox_toggled(bool checked);
    void handleNetworkReply(QNetworkReply *reply);

private:
    Ui::MainWindow *ui;
    QSettings *settings;
    QNetworkAccessManager *networkManager;

    QString getHWID();
    void saveKey(const QString &key);
    QString loadKey();
    void clearKey();
    bool hasSavedKey();
    void attemptLogin(const QString &key);
    void showSuccess(const QString &username, const QString &expires, int daysRemaining, bool isPermanent);
    void showError(const QString &message);
    void autoLogin();
};

#endif // MAINWINDOW_H
