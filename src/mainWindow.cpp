#include "mainWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <filesystem>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_process(new QProcess(this)) {
    
    setWindowTitle("Universal Console Disc Reader");
    resize(700, 500);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 1. Top Bar: Drive Selection & Disc Detection
    QGroupBox* driveGroup = new QGroupBox("Disc Drive & Inspection", this);
    QHBoxLayout* driveLayout = new QHBoxLayout(driveGroup);

    driveLayout->addWidget(new QLabel("Drive:", this));
    m_driveCombo = new QComboBox(this);
    // Populate common optical drive letters
    for (char c = 'D'; c <= 'Z'; ++c) {
        m_driveCombo->addItem(QString("%1:").arg(c));
    }
    m_driveCombo->setCurrentText("E:"); // Default to E:
    driveLayout->addWidget(m_driveCombo);

    m_checkDiscBtn = new QPushButton("Check Disc", this);
    driveLayout->addWidget(m_checkDiscBtn);

    m_discStatusLabel = new QLabel("No disc checked yet.", this);
    m_discStatusLabel->setStyleSheet("font-weight: bold; margin-left: 10px;");
    driveLayout->addWidget(m_discStatusLabel, 1);

    mainLayout->addWidget(driveGroup);

    // 2. Action Buttons & Progress Bar
    QHBoxLayout* actionLayout = new QHBoxLayout();
    m_startDumpBtn = new QPushButton("Start Dump", this);
    m_startDumpBtn->setMinimumHeight(36);
    
    m_cancelDumpBtn = new QPushButton("Cancel", this);
    m_cancelDumpBtn->setMinimumHeight(36);
    m_cancelDumpBtn->setEnabled(false);

    actionLayout->addWidget(m_startDumpBtn);
    actionLayout->addWidget(m_cancelDumpBtn);
    mainLayout->addLayout(actionLayout);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);

    // 3. Live Console Output Box
    m_logOutput = new QPlainTextEdit(this);
    m_logOutput->setReadOnly(true);
    m_logOutput->setStyleSheet(
        "background-color: #1e1e1e; color: #d4d4d4; "
        "font-family: Consolas, monospace; font-size: 10pt;"
    );
    mainLayout->addWidget(m_logOutput, 1);

    // 4. Connect UI & Process Signals
    connect(m_checkDiscBtn, &QPushButton::clicked, this, &MainWindow::onCheckDiscClicked);
    connect(m_startDumpBtn, &QPushButton::clicked, this, &MainWindow::onStartDumpClicked);
    connect(m_cancelDumpBtn, &QPushButton::clicked, this, &MainWindow::onCancelDumpClicked);

    // Merge stdout and stderr just like '2>&1' did
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::onProcessReadyRead);
    connect(m_process, &QProcess::finished, this, &MainWindow::onProcessFinished);
}

MainWindow::~MainWindow() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished();
    }
}

QString MainWindow::findRedumperBinary() const {
    if (QFileInfo::exists("./bin/redumper.exe")) {
        return QDir::cleanPath(QFileInfo("./bin/redumper.exe").absoluteFilePath());
    }
    return QDir::cleanPath(QFileInfo("./build/bin/redumper.exe").absoluteFilePath());
}

void MainWindow::onCheckDiscClicked() {
    std::string drive = m_driveCombo->currentText().toStdString();
    m_currentDisc = DiscInspector::inspectDrive(drive);

    if (!m_currentDisc.isPlayStation) {
        m_discStatusLabel->setText("No PS1/PS2 disc found in " + m_driveCombo->currentText());
        m_discStatusLabel->setStyleSheet("color: #d9534f; font-weight: bold; margin-left: 10px;");
        return;
    }

    QString serial = QString::fromStdString(m_currentDisc.cleanSerial);
    QString console = QString::fromStdString(m_currentDisc.consoleType);

    // Check if dump already exists
    std::string dumpFolder = "./dumps/" + m_currentDisc.cleanSerial;
    bool alreadyDumped = std::filesystem::exists(dumpFolder + "/" + m_currentDisc.cleanSerial + ".iso") ||
                         std::filesystem::exists(dumpFolder + "/" + m_currentDisc.cleanSerial + ".cue");

    if (alreadyDumped) {
        m_discStatusLabel->setText(QString("[%1] %2 — Already Dumped!").arg(console, serial));
        m_discStatusLabel->setStyleSheet("color: #f0ad4e; font-weight: bold; margin-left: 10px;");
        m_logOutput->appendPlainText("[INFO] Existing dump detected in " + QString::fromStdString(dumpFolder));
    } else {
        m_discStatusLabel->setText(QString("[%1] %2 — Ready to Dump").arg(console, serial));
        m_discStatusLabel->setStyleSheet("color: #5cb85c; font-weight: bold; margin-left: 10px;");
    }
}

void MainWindow::onStartDumpClicked() {
    // Auto-check the disc first in case the user didn't click Check Disc
    onCheckDiscClicked();

    QString exePath = findRedumperBinary();
    if (!QFileInfo::exists(exePath)) {
        m_logOutput->appendPlainText("[ERROR] Could not find redumper.exe at: " + exePath);
        return;
    }

    // Determine folder and image name, fallback to disc_dump if non-PS disc
    QString imageName = m_currentDisc.isPlayStation 
                        ? QString::fromStdString(m_currentDisc.cleanSerial) 
                        : "disc_dump";
    QString dumpDir = "./dumps/" + imageName;
    QDir().mkpath(dumpDir);

    QStringList args;
    args << "disc"
         << "--drive=" + m_driveCombo->currentText()
         << "--retries=50"
         << "--image-path=" + dumpDir
         << "--image-name=" + imageName
         << "--overwrite";

    m_logOutput->clear();
    m_logOutput->appendPlainText(">>> Launching: " + exePath + " " + args.join(" ") + "\n");

    m_startDumpBtn->setEnabled(false);
    m_cancelDumpBtn->setEnabled(true);
    m_driveCombo->setEnabled(false);
    m_progressBar->setValue(0);

    // Start redumper asynchronously
    m_process->start(exePath, args);
}

void MainWindow::onCancelDumpClicked() {
    if (m_process->state() != QProcess::NotRunning) {
        m_logOutput->appendPlainText("\n[WARN] Canceling disc dump...");
        m_process->kill();
    }
}

void MainWindow::onProcessReadyRead() {
    QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());

    output.replace("\r\n", "\n");
    output.replace('\r', '\n');

    static QRegularExpression percentRegex(R"((\d{1,3})%)");
    QRegularExpressionMatchIterator i = percentRegex.globalMatch(output);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        int pct = match.captured(1).toInt();
        if (pct >= 0 && pct <= 100) {
            m_progressBar->setValue(pct);
        }
    }

    m_logOutput->insertPlainText(output);
    m_logOutput->ensureCursorVisible();
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_startDumpBtn->setEnabled(true);
    m_cancelDumpBtn->setEnabled(false);
    m_driveCombo->setEnabled(true);

    if (exitStatus == QProcess::CrashExit) {
        m_logOutput->appendPlainText("\n[ABORTED] Dump process was terminated.");
    } else if (exitCode == 0) {
        m_progressBar->setValue(100);
        m_logOutput->appendPlainText("\n[SUCCESS] Disc dump completed successfully!");
        onCheckDiscClicked(); // Refresh the label to say already dumped
    } else {
        m_logOutput->appendPlainText(QString("\n[FINISHED] Redumper exited with code %1").arg(exitCode));
    }
}