#include "mainWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QPixmap>
#include <QUrl>
#include <QNetworkRequest>
#include <filesystem>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_process(new QProcess(this)),
      m_pollTimer(new QTimer(this)),
      m_netManager(new QNetworkAccessManager(this)) {
    
    setWindowTitle("Universal Console Disc Reader");
    resize(780, 620);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 1. Top Drive & Settings Bar
    QGroupBox* driveGroup = new QGroupBox("Drive & Extraction Options", this);
    QHBoxLayout* driveLayout = new QHBoxLayout(driveGroup);

    driveLayout->addWidget(new QLabel("Drive:", this));
    m_driveCombo = new QComboBox(this);
    for (char c = 'D'; c <= 'Z'; ++c) {
        m_driveCombo->addItem(QString("%1:").arg(c));
    }
    m_driveCombo->setCurrentText("E:");
    driveLayout->addWidget(m_driveCombo);

    m_checkDiscBtn = new QPushButton("Rescan Drive", this);
    driveLayout->addWidget(m_checkDiscBtn);

    driveLayout->addSpacing(20);
    m_autoCleanupCheck = new QCheckBox("Delete temp files (.scram, .state, .subcode) after dump", this);
    m_autoCleanupCheck->setChecked(true);
    driveLayout->addWidget(m_autoCleanupCheck, 1);

    mainLayout->addWidget(driveGroup);

    // 2. Pre-Dump Metadata & Box Art Panel
    QGroupBox* infoGroup = new QGroupBox("Disc Metadata & Cover Preview", this);
    QHBoxLayout* infoLayout = new QHBoxLayout(infoGroup);

    // Left: Box Art Image
    m_coverArtLabel = new QLabel("No Disc\nInserted", this);
    m_coverArtLabel->setFixedSize(160, 220);
    m_coverArtLabel->setAlignment(Qt::AlignCenter);
    m_coverArtLabel->setStyleSheet(
        "background-color: #252526; color: #888888; "
        "border: 1px solid #3e3e42; border-radius: 4px; font-weight: bold;"
    );
    infoLayout->addWidget(m_coverArtLabel);
    infoLayout->addSpacing(15);

    // Right: Game Metadata Details
    QFormLayout* metaForm = new QFormLayout();
    metaForm->setVerticalSpacing(10);

    m_titleValueLabel   = new QLabel("Waiting for PlayStation 1 or 2 disc...", this);
    m_titleValueLabel->setStyleSheet("font-size: 13pt; font-weight: bold; color: #ffffff;");
    m_consoleValueLabel = new QLabel("—", this);
    m_serialValueLabel  = new QLabel("—", this);
    m_regionValueLabel  = new QLabel("—", this);
    m_statusBadgeLabel  = new QLabel("Insert a disc to inspect automatically", this);
    m_statusBadgeLabel->setStyleSheet("font-weight: bold; color: #aaaaaa;");

    metaForm->addRow("Game Title:", m_titleValueLabel);
    metaForm->addRow("Platform:", m_consoleValueLabel);
    metaForm->addRow("Serial / Ver:", m_serialValueLabel);
    metaForm->addRow("Region:", m_regionValueLabel);
    metaForm->addRow("Dump Status:", m_statusBadgeLabel);

    infoLayout->addLayout(metaForm, 1);
    mainLayout->addWidget(infoGroup);

    // 3. Action Buttons & Progress Bar
    QHBoxLayout* actionLayout = new QHBoxLayout();
    m_startDumpBtn = new QPushButton("Start Disc Extraction", this);
    m_startDumpBtn->setMinimumHeight(38);
    
    m_cancelDumpBtn = new QPushButton("Cancel", this);
    m_cancelDumpBtn->setMinimumHeight(38);
    m_cancelDumpBtn->setEnabled(false);

    actionLayout->addWidget(m_startDumpBtn);
    actionLayout->addWidget(m_cancelDumpBtn);
    mainLayout->addLayout(actionLayout);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);

    // 4. Live Console Output Box
    m_logOutput = new QPlainTextEdit(this);
    m_logOutput->setReadOnly(true);
    m_logOutput->setStyleSheet(
        "background-color: #1e1e1e; color: #d4d4d4; "
        "font-family: Consolas, monospace; font-size: 9.5pt;"
    );
    mainLayout->addWidget(m_logOutput, 1);

    // 5. Connect Signals
    connect(m_checkDiscBtn, &QPushButton::clicked, this, &MainWindow::onCheckDiscClicked);
    connect(m_startDumpBtn, &QPushButton::clicked, this, &MainWindow::onStartDumpClicked);
    connect(m_cancelDumpBtn, &QPushButton::clicked, this, &MainWindow::onCancelDumpClicked);
    connect(m_netManager, &QNetworkAccessManager::finished, this, &MainWindow::onCoverDownloaded);

    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::onProcessReadyRead);
    connect(m_process, &QProcess::finished, this, &MainWindow::onProcessFinished);

    // Load Game Databases
    m_gameDb.loadFromDirectory(findResourcesDir().toStdString());
    m_logOutput->appendPlainText(QString("[INFO] Loaded %1 PlayStation titles into database.")
                                 .arg(m_gameDb.count()));

    // Start Auto-Detection Timer
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::pollDrives);
    m_pollTimer->start(2000);
    pollDrives(); // Run an immediate check on launch
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

QString MainWindow::findResourcesDir() const {
    if (QFileInfo::exists("./resources/GameIndex.yaml")) {
        return QDir::cleanPath(QFileInfo("./resources").absoluteFilePath());
    }
    return QDir::cleanPath(QFileInfo("./build/resources").absoluteFilePath());
}

void MainWindow::pollDrives() {
    // Do not poll optical drives while redumper is actively reading sectors!
    if (m_process->state() != QProcess::NotRunning) return;

    // 1. Check the currently selected drive first
    std::string currentDrive = m_driveCombo->currentText().toStdString();
    DiscInfo info = DiscInspector::inspectDrive(currentDrive);

    // 2. If empty, scan D: through Z: automatically to see if a disc was inserted elsewhere
    if (!info.isPlayStation) {
        for (char c = 'D'; c <= 'Z'; ++c) {
            std::string testDrive = std::string(1, c) + ":";
            DiscInfo scanned = DiscInspector::inspectDrive(testDrive);
            if (scanned.isPlayStation) {
                m_driveCombo->setCurrentText(QString::fromStdString(testDrive));
                info = scanned;
                break;
            }
        }
    }

    // Only update UI & fetch cover art if the disc state changed
    if (info.cleanSerial != m_lastDetectedSerial) {
        m_lastDetectedSerial = info.cleanSerial;
        onCheckDiscClicked();
    }
}

void MainWindow::onCheckDiscClicked() {
    std::string drive = m_driveCombo->currentText().toStdString();
    m_currentDisc = DiscInspector::inspectDrive(drive);

    if (!m_currentDisc.isPlayStation) {
        m_titleValueLabel->setText("No PS1/PS2 Disc Detected");
        m_consoleValueLabel->setText("—");
        m_serialValueLabel->setText("—");
        m_regionValueLabel->setText("—");
        m_statusBadgeLabel->setText("Waiting for disc in " + m_driveCombo->currentText());
        m_statusBadgeLabel->setStyleSheet("color: #d9534f; font-weight: bold;");
        m_coverArtLabel->setPixmap(QPixmap());
        m_coverArtLabel->setText("No Disc\nInserted");
        return;
    }

    m_currentDisc.gameTitle = m_gameDb.getGameTitle(m_currentDisc.cleanSerial);

    QString title   = QString::fromStdString(m_currentDisc.gameTitle);
    QString serial  = QString::fromStdString(m_currentDisc.cleanSerial);
    QString console = QString::fromStdString(m_currentDisc.consoleType);
    QString media   = QString::fromStdString(m_currentDisc.mediaType);
    QString region  = QString::fromStdString(m_currentDisc.region);
    QString version = QString::fromStdString(m_currentDisc.version);

    m_titleValueLabel->setText(title);
    m_consoleValueLabel->setText(QString("%1  •  %2").arg(console, media));
    m_serialValueLabel->setText(QString("%1  (v%2)").arg(serial, version));
    m_regionValueLabel->setText(region);

    // Check if dump already exists
    std::string safeName = GameDatabase::sanitizeForFilename(m_currentDisc.gameTitle);
    std::string dumpFolder = "./dumps/" + safeName;

    bool alreadyDumped = std::filesystem::exists(dumpFolder + "/" + safeName + ".iso") ||
                         std::filesystem::exists(dumpFolder + "/" + safeName + ".cue");

    if (alreadyDumped) {
        m_statusBadgeLabel->setText("Already Dumped in Library!");
        m_statusBadgeLabel->setStyleSheet("color: #f0ad4e; font-weight: bold;");
    } else {
        m_statusBadgeLabel->setText("Ready to Extract");
        m_statusBadgeLabel->setStyleSheet("color: #5cb85c; font-weight: bold;");
    }

    // Trigger immediate Cover Art lookup
    fetchCoverArt(m_currentDisc);
}

void MainWindow::fetchCoverArt(const DiscInfo& disc) {
    QString coversDir = findResourcesDir() + "/covers";
    QDir().mkpath(coversDir);

    QString serial = QString::fromStdString(disc.cleanSerial);
    QString localPath = coversDir + "/" + serial + ".jpg";

    // 1. Check local cache first so we don't re-download covers we already have
    if (QFileInfo::exists(localPath)) {
        QPixmap pix(localPath);
        if (!pix.isNull()) {
            m_coverArtLabel->setPixmap(pix.scaled(m_coverArtLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }
    }

    m_coverArtLabel->setPixmap(QPixmap());
    m_coverArtLabel->setText("Loading\nBox Art...");

    // 2. Fetch from xlenore's open-source PS1 / PS2 cover repositories
    QString repo = (disc.consoleType == "PS2") ? "ps2-covers" : "psx-covers";
    QString urlStr = QString("https://raw.githubusercontent.com/xlenore/%1/main/covers/default/%2.jpg")
                     .arg(repo, serial);

    QNetworkRequest request{QUrl(urlStr)};
    request.setAttribute(QNetworkRequest::User, localPath); // Pass save path to callback
    m_netManager->get(request);
}

void MainWindow::onCoverDownloaded(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_coverArtLabel->setText("No Cover\nFound");
        return;
    }

    QByteArray data = reply->readAll();
    QPixmap pix;
    if (pix.loadFromData(data)) {
        // Save to local cache for instant loading next time
        QString savePath = reply->request().attribute(QNetworkRequest::User).toString();
        if (!savePath.isEmpty()) {
            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
            }
        }
        m_coverArtLabel->setPixmap(pix.scaled(m_coverArtLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_coverArtLabel->setText("Cover Error");
    }
}

void MainWindow::onStartDumpClicked() {
    onCheckDiscClicked();

    QString exePath = findRedumperBinary();
    if (!QFileInfo::exists(exePath)) {
        m_logOutput->appendPlainText("[ERROR] Could not find redumper.exe at: " + exePath);
        return;
    }

    std::string safeName = m_currentDisc.isPlayStation 
                           ? GameDatabase::sanitizeForFilename(m_currentDisc.gameTitle)
                           : "disc_dump";

    QString imageName = QString::fromStdString(safeName);
    m_activeDumpDir = "./dumps/" + imageName;
    QDir().mkpath(m_activeDumpDir);

    // Optimize speed specifically for LG SP80NB80 (24x max for CD, 8x max for DVD)
    QString maxDriveSpeed = (m_currentDisc.consoleType == "PS2") ? "8" : "24";

    QStringList args;
    args << "disc"
         << "--drive=" + m_driveCombo->currentText()
         << "--speed=" + maxDriveSpeed
         << "--retries=10"
         << "--image-path=" + m_activeDumpDir
         << "--image-name=" + imageName
         << "--overwrite";

    m_logOutput->clear();
    m_logOutput->appendPlainText(">>> Launching: " + exePath + " " + args.join(" ") + "\n");

    m_startDumpBtn->setEnabled(false);
    m_cancelDumpBtn->setEnabled(true);
    m_driveCombo->setEnabled(false);
    m_progressBar->setValue(0);
    m_statusBadgeLabel->setText("Extracting Disc...");
    m_statusBadgeLabel->setStyleSheet("color: #5bc0de; font-weight: bold;");

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
        m_statusBadgeLabel->setText("Dump Aborted");
        m_statusBadgeLabel->setStyleSheet("color: #d9534f; font-weight: bold;");
    } else if (exitCode == 0) {
        m_progressBar->setValue(100);
        m_logOutput->appendPlainText("\n[SUCCESS] Disc dump completed successfully!");

        // Automatic Temp File Cleanup
        if (m_autoCleanupCheck->isChecked() && !m_activeDumpDir.isEmpty()) {
            cleanupTempFiles(m_activeDumpDir);
        }

        onCheckDiscClicked();
    } else {
        m_logOutput->appendPlainText(QString("\n[FINISHED] Redumper exited with code %1").arg(exitCode));
    }
}

void MainWindow::cleanupTempFiles(const QString& dumpDir) {
    QDir dir(dumpDir);
    // Remove intermediate redumper workbench files while keeping final .bin, .cue, .iso, and .log
    QStringList tempFilters = {"*.scram", "*.state", "*.subcode", "*.fulltoc", "*.cdtext", "*.toc"};
    QFileInfoList tempFiles = dir.entryInfoList(tempFilters, QDir::Files);

    qint64 bytesFreed = 0;
    int removedCount = 0;

    for (const QFileInfo& fileInfo : tempFiles) {
        bytesFreed += fileInfo.size();
        if (QFile::remove(fileInfo.absoluteFilePath())) {
            removedCount++;
        }
    }

    if (removedCount > 0) {
        double mbFreed = static_cast<double>(bytesFreed) / (1024.0 * 1024.0);
        m_logOutput->appendPlainText(
            QString("[CLEANUP] Deleted %1 temporary workbench files (freed %2 MB).")
            .arg(removedCount)
            .arg(mbFreed, 0, 'f', 1)
        );
    }
}