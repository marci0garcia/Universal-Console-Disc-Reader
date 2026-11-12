#pragma once
#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QCheckBox>
#include <QProcess>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "discInspector.hpp"
#include "gameDatabase.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void pollDrives();
    void onCheckDiscClicked();
    void onStartDumpClicked();
    void onCancelDumpClicked();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCoverDownloaded(QNetworkReply* reply);

private:
    // Top Controls
    QComboBox* m_driveCombo;
    QPushButton* m_checkDiscBtn;
    QCheckBox* m_autoCleanupCheck;

    // Pre-Dump Metadata & Cover Art Panel
    QLabel* m_coverArtLabel;
    QLabel* m_titleValueLabel;
    QLabel* m_consoleValueLabel;
    QLabel* m_serialValueLabel;
    QLabel* m_regionValueLabel;
    QLabel* m_statusBadgeLabel;

    // Action & Progress
    QPushButton* m_startDumpBtn;
    QPushButton* m_cancelDumpBtn;
    QProgressBar* m_progressBar;
    QPlainTextEdit* m_logOutput;

    // Backend & Services
    QProcess* m_process;
    QTimer* m_pollTimer;
    QNetworkAccessManager* m_netManager;
    GameDatabase m_gameDb;
    DiscInfo m_currentDisc;
    std::string m_lastDetectedSerial;
    QString m_activeDumpDir;

    QString findRedumperBinary() const;
    QString findResourcesDir() const;
    void fetchCoverArt(const DiscInfo& disc);
    void cleanupTempFiles(const QString& dumpDir);
};