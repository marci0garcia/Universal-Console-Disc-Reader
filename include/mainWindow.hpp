#pragma once
#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QProcess>
#include "discInspector.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onCheckDiscClicked();
    void onStartDumpClicked();
    void onCancelDumpClicked();
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    // ui widgets
    QComboBox* m_driveCombo;
    QPushButton* m_checkDiscBtn;
    QLabel* m_discStatusLabel;
    QPushButton* m_startDumpBtn;
    QPushButton* m_cancelDumpBtn;
    QProgressBar* m_progressBar;
    QPlainTextEdit* m_logOutput;

    // backend process and state
    QProcess* m_process;
    DiscInfo m_currentDisc;
    QString findRedumperBinary() const;
};