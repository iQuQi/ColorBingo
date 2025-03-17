#include "ui/widgets/multigamewidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QSizePolicy>
#include <QThread>
#include <QFile>
#include <QSocketNotifier>
#include <linux/input.h>
#include "hardwareInterface/webcambutton.h"
#include <QShowEvent>
#include <QHideEvent>

MultiGameWidget::MultiGameWidget(QWidget *parent, const QList<QColor> &initialColors) : QWidget(parent),
    isCapturing(false),
    showCircle(true),
    circleRadius(10),
    avgRed(0),
    avgGreen(0),
    avgBlue(0),
    showRgbValues(true),
    selectedCell(-1, -1),
    bingoCount(0),
    remainingSeconds(120), // 2분 = 120초 타이머 초기화
    sliderWidget(nullptr)  // 추가된 부분
{

    // 네트워크
    network = new P2PNetwork(this);
    connect(network, &P2PNetwork::opponentScoreUpdated, this, &MultiGameWidget::updateOpponentScore);
    connect(network, &P2PNetwork::gameOverReceived, this, &MultiGameWidget::showFailureMessage);

    qDebug() << "MultiGameWidget constructor started";

    // Initialize main variables (prevent null pointers)
    cameraView = nullptr;
    rgbValueLabel = nullptr;
    circleSlider = nullptr;
    rgbCheckBox = nullptr;
    camera = nullptr;
    circleValueLabel = nullptr;
    circleCheckBox = nullptr; // Initialize to nullptr even though we won't use it


    // 메인 레이아웃 생성 (가로 분할)
    mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(50, 20, 50, 20); // Equal left and right margins
    mainLayout->setSpacing(50); // Space between bingo area and camera area

    // 왼쪽 부분: 빙고 영역 (오렌지색 네모)
    bingoArea = new QWidget(this);
    QVBoxLayout* bingoVLayout = new QVBoxLayout(bingoArea);
    // bingoLayout = new QVBoxLayout(bingoArea);
    bingoVLayout->setContentsMargins(0, 0, 0, 0);

    // 빙고 점수 표시 레이블
    bingoScoreLabel = new QLabel("Bingo: 0", bingoArea);
    bingoScoreLabel->setAlignment(Qt::AlignCenter);
    QFont scoreFont = bingoScoreLabel->font();
    scoreFont.setPointSize(12);
    scoreFont.setBold(true);
    bingoScoreLabel->setFont(scoreFont);
    bingoScoreLabel->setMinimumHeight(30);

    // Status message label for game instructions
    statusMessageLabel = new QLabel("Please select a cell to match colors", bingoArea);
    statusMessageLabel->setAlignment(Qt::AlignCenter);
    QFont messageFont = statusMessageLabel->font();
    messageFont.setPointSize(11);
    statusMessageLabel->setFont(messageFont);
    statusMessageLabel->setMinimumHeight(30);
    statusMessageLabel->setStyleSheet("color: red; font-weight: bold;"); // 빨간색으로 변경

    // Add stretch to center bingo section vertically
    bingoVLayout->addStretch(1);

    // Add score label above the grid
    bingoVLayout->addWidget(bingoScoreLabel);

    // 빙고 그리드를 담을 컨테이너 위젯
    QWidget* gridWidget = new QWidget(bingoArea);
    gridWidget->setFixedSize(300, 300);

    // 그리드 레이아웃 생성 (이 레이아웃은 gridWidget의 자식)
    QGridLayout* gridLayout = new QGridLayout(gridWidget);
    gridLayout->setSpacing(0);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            bingoCells[row][col] = new QLabel(gridWidget);
            bingoCells[row][col]->setFixedSize(100, 100);
            bingoCells[row][col]->setAutoFillBackground(true);
            bingoCells[row][col]->setAlignment(Qt::AlignCenter);

            // 경계선 스타일 설정
            QString borderStyle = "border-top: 1px solid black; border-left: 1px solid black;";

            if (row == 2) {
                borderStyle += " border-bottom: 1px solid black;";
            }
            if (col == 2) {
                borderStyle += " border-right: 1px solid black;";
            }

            bingoCells[row][col]->setStyleSheet(
                QString("background-color: lightgray; %1").arg(borderStyle));

            // 마우스 클릭 이벤트 활성화
            bingoCells[row][col]->installEventFilter(this);

            // 빙고 상태 초기화 (체크 안됨)
            bingoStatus[row][col] = false;

            gridLayout->addWidget(bingoCells[row][col], row, col);
        }
    }

    // 그리드 위젯을 VBox 레이아웃에 추가
    bingoVLayout->addWidget(gridWidget, 0, Qt::AlignCenter);

    // Add the status message label BELOW the grid
    bingoVLayout->addWidget(statusMessageLabel, 0, Qt::AlignCenter);

    // Add stretch to center bingo section vertically
    bingoVLayout->addStretch(1);

    // 성공 메시지 레이블 초기화
    successLabel = new QLabel("SUCCESS", this);
    successLabel->setAlignment(Qt::AlignCenter);
    successLabel->setStyleSheet("background-color: rgba(0, 0, 0, 150); color: white; font-weight: bold; font-size: 72px;");
    successLabel->hide(); // 초기에는 숨김

    // 성공 메시지 타이머 초기화
    successTimer = new QTimer(this);
    successTimer->setSingleShot(true);
    connect(successTimer, &QTimer::timeout, this, &MultiGameWidget::hideSuccessAndReset);

    // 실패 메시지 레이블 초기화
    failureLabel = new QLabel("YOU LOST!", this);
    failureLabel->setAlignment(Qt::AlignCenter);
    failureLabel->setStyleSheet("background-color: rgba(255, 255, 255, 120); color: red; font-weight: bold; font-size: 72px;");
    failureLabel->hide(); // 초기에는 숨김

    // 실패 메시지 타이머 초기화
    failureTimer = new QTimer(this);
    failureTimer->setSingleShot(true);
    connect(failureTimer, &QTimer::timeout, this, &MultiGameWidget::hideFailureAndReset);

    // 오른쪽 부분: 카메라 영역
    cameraArea = new QWidget(this);
    QVBoxLayout* cameraVLayout = new QVBoxLayout(cameraArea);
    cameraVLayout->setAlignment(Qt::AlignTop); // Align to top
    cameraVLayout->setContentsMargins(0, 0, 0, 0);
    cameraVLayout->setSpacing(10); // Add spacing between elements

    // Add stretch for vertical centering
    cameraVLayout->addStretch(1);

    // 1. RGB 값 표시 레이블 (맨 위에 배치)
    rgbValueLabel = new QLabel("R: 0  G: 0  B: 0");
    rgbValueLabel->setFrameShape(QFrame::Box);
    rgbValueLabel->setFixedHeight(30);
    rgbValueLabel->setFixedWidth(300); // Same width as camera
    rgbValueLabel->setAlignment(Qt::AlignCenter);
    QFont rgbFont = rgbValueLabel->font();
    rgbFont.setPointSize(12);
    rgbValueLabel->setFont(rgbFont);
    cameraVLayout->addWidget(rgbValueLabel, 0, Qt::AlignCenter);

    // 2. 카메라 뷰 (중간에 배치)
    cameraView = new QLabel(cameraArea);
    cameraView->setFixedSize(300, 300);
    cameraView->setAlignment(Qt::AlignCenter);
    cameraView->setText(""); // 문구 제거
    cameraView->setFrameShape(QFrame::Box);
    cameraVLayout->addWidget(cameraView, 0, Qt::AlignCenter);

    // 3. 원 크기 조절 슬라이더 (맨 아래에 배치)
    sliderWidget = new QWidget(cameraArea);
    sliderWidget->setFixedWidth(300); // Same width as camera
    QHBoxLayout *circleSliderLayout = new QHBoxLayout(sliderWidget);
    circleSliderLayout->setContentsMargins(0, 0, 0, 0);

    // 초기에는 슬라이더 위젯 숨김
    sliderWidget->hide();

    QLabel *circleLabel = new QLabel("Circle Size:");
    circleSlider = new QSlider(Qt::Horizontal);
    circleSlider->setMinimum(5);
    circleSlider->setMaximum(20);
    circleSlider->setValue(circleRadius);
    circleValueLabel = new QLabel(QString::number(circleRadius) + "%");

    circleSliderLayout->addWidget(circleLabel);
    circleSliderLayout->addWidget(circleSlider);
    circleSliderLayout->addWidget(circleValueLabel);

    cameraVLayout->addWidget(sliderWidget, 0, Qt::AlignCenter);

    // Add stretch for vertical centering
    cameraVLayout->addStretch(1);

    // 원 표시 관련 초기화 - 항상 표시되도록 수정
    showCircle = true;  // 항상 원 표시
    circleRadius = 10;  // 기본 반지름 10%

    // 메인 레이아웃에 두 영역 추가
    mainLayout->addWidget(bingoArea);
    mainLayout->addWidget(cameraArea);

    // 분할 비율 고정 (왼쪽:오른쪽 = 1:1)
    mainLayout->setStretchFactor(bingoArea, 1);
    mainLayout->setStretchFactor(cameraArea, 1);

    // X 표시 사라지는 타이머 초기화
    fadeXTimer = new QTimer(this);
    fadeXTimer->setSingleShot(true);
    connect(fadeXTimer, &QTimer::timeout, this, &MultiGameWidget::clearXMark);

    // 빙고 셀에 낮은 채도의 랜덤 색상 생성 대신 initialColors 사용
    if (initialColors.size() >= 9) {
        setCustomColors(initialColors);
    } else {
        generateRandomColors();
    }

    // 카메라 초기화
    camera = new V4L2Camera(this);

    // 카메라 신호 연결
    connect(camera, &V4L2Camera::newFrameAvailable, this, &MultiGameWidget::updateCameraFrame);
    connect(camera, &V4L2Camera::deviceDisconnected, this, &MultiGameWidget::handleCameraDisconnect);

    // 위젯 컨트롤 신호 연결 - remove RGB checkbox connection
    connect(circleSlider, &QSlider::valueChanged, this, &MultiGameWidget::onCircleSliderValueChanged);

    // 카메라 재시작 타이머 설정 (유지)
    cameraRestartTimer = new QTimer(this);
    cameraRestartTimer->setInterval(30 * 60 * 1000); // 30분마다 재시작
    connect(cameraRestartTimer, &QTimer::timeout, this, &MultiGameWidget::restartCamera);

    // 카메라 열기만 하고 자동 시작은 하지 않음
    if (!camera->openCamera()) {
        cameraView->setText("Camera connection failed");
    }

    // 슬라이더 설정
    circleSlider->setMinimumHeight(30);

    // 픽셀 스타일 곰돌이 이미지 생성 (별도 함수로 분리)
    bearImage = createBearImage();
    xImage = createXImage();
    qDebug() << "Basic variable initialization completed";

    // Back 버튼 설정
    backButton = new QPushButton("Back", this);
    backButton->setFixedSize(80, 30); // 버튼 크기 고정
    connect(backButton, &QPushButton::clicked, this, &MultiGameWidget::onBackButtonClicked);

    // Restart 버튼 추가
    restartButton = new QPushButton("Restart", this);
    restartButton->setFixedSize(80, 30); // 버튼 크기 고정
    connect(restartButton, &QPushButton::clicked, this, &MultiGameWidget::onRestartButtonClicked);

    // 버튼 스타일 변경 - 진회색 배경으로 수정
    QString buttonStyle = "QPushButton { background-color: rgba(50, 50, 50, 200); color: white; "
                         "border-radius: 4px; font-weight: bold; } "
                         "QPushButton:hover { background-color: rgba(70, 70, 70, 220); }";
    backButton->setStyleSheet(buttonStyle);
    restartButton->setStyleSheet(buttonStyle);

    // 초기 위치 설정
    updateBackButtonPosition();

    // 타이머 초기화 - 이 부분이 빠져 있었습니다
    gameTimer = new QTimer(this);
    gameTimer->setInterval(1000); // 1초마다 업데이트
    connect(gameTimer, &QTimer::timeout, this, &MultiGameWidget::onTimerTick);

    // 타이머 레이블 초기화
    timerLabel = new QLabel(this);
    timerLabel->setAlignment(Qt::AlignCenter);
    timerLabel->setStyleSheet("QLabel { background-color: rgba(50, 50, 50, 200); color: white; "
                             "border-radius: 12px; padding: 8px 15px; font-weight: bold; font-size: 24px; }");

    // 실패 메시지 레이블 초기화
    failLabel = new QLabel("FAIL", this);
    failLabel->setAlignment(Qt::AlignCenter);
    failLabel->setStyleSheet("QLabel { background-color: rgba(0, 0, 0, 150); color: red; "
                           "font-weight: bold; font-size: 72px; }");
    failLabel->hide(); // 초기에는 숨김

    // 타이머 디스플레이 초기화 및 시작
    updateTimerDisplay();
    gameTimer->start();
    // 웹캠 물리 버튼 초기화 코드 변경
    webcamButton = new WebcamButton(this);
    if (webcamButton->initialize()) {
        // 버튼 신호 연결
        connect(webcamButton, &WebcamButton::captureButtonPressed, this, &MultiGameWidget::onCaptureButtonClicked);
    }
}

MultiGameWidget::~MultiGameWidget() {
    // 물리 버튼 정리
    if (cameraRestartTimer) {
        cameraRestartTimer->stop();
        delete cameraRestartTimer;
    }

    if (fadeXTimer) {
        fadeXTimer->stop();
        delete fadeXTimer;
    }

    if (successTimer) {
        successTimer->stop();
        delete successTimer;
    }

    if (camera) {
        camera->stopCapturing();
        camera->closeCamera();
    }

    if (gameTimer) {
        gameTimer->stop();
        delete gameTimer;
    }
}

bool MultiGameWidget::eventFilter(QObject *obj, QEvent *event) {
    // 빙고 셀 클릭 감지
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (obj == bingoCells[row][col] && event->type() == QEvent::MouseButtonPress) {
                // 이미 O로 표시된 칸이라면 무시
                if (bingoStatus[row][col]) {
                    return true; // 이벤트 처리됨으로 표시하고 무시
                }

                // 이전에 선택된 셀이 있으면 선택 해제
                if (selectedCell.first >= 0 && selectedCell.second >= 0) {
                    deselectCell();
                }

                // 새 셀 선택 및 카메라 시작
                selectCell(row, col);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

// 셀 선택 및 카메라 시작 함수
void MultiGameWidget::selectCell(int row, int col) {
    qDebug() << "selectCell called: (" << row << "," << col << ")";

    // Range check
    if (row < 0 || row >= 3 || col < 0 || col >= 3) {
        qDebug() << "Invalid cell coordinates.";
        return;
    }

    selectedCell = qMakePair(row, col);

    // Safety check: verify bingoCells array
    if (!bingoCells[row][col]) {
        qDebug() << "bingoCells is null at row:" << row << ", col:" << col;
        return;
    }

    // 경계선 스타일 생성
    QString borderStyle = "border-top: 1px solid black; border-left: 1px solid black;";

    if (row == 2) {
        borderStyle += " border-bottom: 1px solid black;";
    }
    if (col == 2) {
        borderStyle += " border-right: 1px solid black;";
    }

    // 선택된 셀 강조 표시 (빨간색 테두리 3px)
    QString style = QString("background-color: %1; %2 border: 3px solid red;")
                   .arg(cellColors[row][col].name())
                   .arg(borderStyle);
    bingoCells[row][col]->setStyleSheet(style);

    // 카메라 시작
    startCamera();

    // 셀이 선택되었으니 상태 메시지 업데이트
    statusMessageLabel->setText(QString("Selected cell (%1,%2) - Press camera button to match colors").arg(row+1).arg(col+1));
}

// 셀 선택 해제 및 카메라 중지 함수
void MultiGameWidget::deselectCell() {
    if (selectedCell.first >= 0 && selectedCell.second >= 0) {
        // 이미 빙고 처리된 셀이 아니라면 스타일 원래대로
        if (!bingoStatus[selectedCell.first][selectedCell.second]) {
            updateCellStyle(selectedCell.first, selectedCell.second);
        }

        // 선택 상태 초기화
        selectedCell = qMakePair(-1, -1);

        // Stop camera
        stopCamera();

        // Reset status message
        statusMessageLabel->setText("Please select a cell to match colors");
    }
}

// 카메라 시작/중지 함수
void MultiGameWidget::startCamera() {
    qDebug() << "startCamera called";

    if (!isCapturing) {
        // Safety check: verify camera object
        if (!camera) {
            qDebug() << "camera object is null.";
            return;
        }

        if (camera->startCapturing()) {
            // Reset camera view stylesheet
            if (cameraView) {
                cameraView->setStyleSheet("");
                cameraView->clear();
            }

            isCapturing = true;

            // Safety check: verify cameraRestartTimer
            if (cameraRestartTimer) {
                cameraRestartTimer->start();
                qDebug() << "Camera restart timer started";
            }

            // 카메라가 켜졌으므로 슬라이더 위젯 표시
            if (sliderWidget) {
                sliderWidget->show();
                qDebug() << "Circle slider widget shown";
            }

            qDebug() << "Camera capture started successfully";
        } else {
            qDebug() << "Failed to start camera capture";
            QMessageBox::critical(this, "Error", "Failed to start camera");
        }
    } else {
        qDebug() << "Camera is already capturing.";
    }
}

void MultiGameWidget::stopCamera() {
    qDebug() << "stopCamera called";

    if (isCapturing) {
        // Safety check: verify camera object
        if (!camera) {
            qDebug() << "camera object is null.";
            return;
        }

        camera->stopCapturing();
        isCapturing = false;

        // Safety check: verify cameraRestartTimer
        if (cameraRestartTimer) {
            cameraRestartTimer->stop();
            qDebug() << "Camera restart timer stopped";
        }

        // 카메라가 꺼졌으므로 슬라이더 위젯 숨김
        if (sliderWidget) {
            sliderWidget->hide();
            qDebug() << "Circle slider widget hidden";
        }

        // 카메라 뷰에 메시지 표시
        if (cameraView) {
            cameraView->setText("Camera is off");
            cameraView->setStyleSheet("QLabel { color: gray; font-weight: bold; }");
        }

        qDebug() << "Camera capture stopped";
    } else {
        qDebug() << "Camera is already stopped.";
    }
}

// 채도가 낮은 랜덤 색상 생성
void MultiGameWidget::generateRandomColors() {
    // 9개의 색상을 담을 배열
    QList<QColor> colors;

    // 색상환(0-359)을 9개 영역으로 나눔 (각 영역에서 하나씩 선택)
    const int segments = 9;
    const int hueStep = 360 / segments;

    // 각 영역에서 하나씩 색상 선택
    for (int i = 0; i < segments; ++i) {
        int baseHue = i * hueStep;
        // 각 영역 내에서 랜덤한 hue 선택
        int hue = baseHue + QRandomGenerator::global()->bounded(hueStep);

        // 랜덤 채도 (40-255 범위로 설정하여 너무 회색에 가까운 색상 방지)
        int saturation = QRandomGenerator::global()->bounded(40, 255);

        // 랜덤 명도 (140-255 범위로 설정하여 어두운 색상 방지)
        int value = QRandomGenerator::global()->bounded(140, 255);

        // HSV 색상 생성 후 목록에 추가
        colors.append(QColor::fromHsv(hue, saturation, value));
    }

    // 색상 목록을 섞기 (셔플링)
    for (int i = 0; i < colors.size(); ++i) {
        int j = QRandomGenerator::global()->bounded(colors.size());
        colors.swapItemsAt(i, j);
    }

    // 섞인 색상을 빙고판에 적용
    int colorIndex = 0;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            cellColors[row][col] = colors[colorIndex++];

            // 경계선 스타일 생성
            QString borderStyle = "border-top: 1px solid black; border-left: 1px solid black;";

            if (row == 2) {
                borderStyle += " border-bottom: 1px solid black;";
            }
            if (col == 2) {
                borderStyle += " border-right: 1px solid black;";
            }

            // 배경색 적용, 텍스트 제거함
            bingoCells[row][col]->setText("");
            bingoCells[row][col]->setStyleSheet(
                QString("background-color: %1; %2")
                .arg(cellColors[row][col].name())
                .arg(borderStyle)
            );
        }
    }
}

void MultiGameWidget::updateCellStyle(int row, int col) {
    // 경계선 스타일 생성
    QString borderStyle = "border-top: 1px solid black; border-left: 1px solid black;";

    if (row == 2) {
        borderStyle += " border-bottom: 1px solid black;";
    }
    if (col == 2) {
        borderStyle += " border-right: 1px solid black;";
    }

    // 기본 스타일 적용
    QString style = QString("background-color: %1; %2")
                   .arg(cellColors[row][col].name())
                   .arg(borderStyle);

    bingoCells[row][col]->setStyleSheet(style);

    if (bingoStatus[row][col]) {
        // 곰돌이 이미지 적용 - 더 크게 스케일링
        QPixmap scaledBear = bearImage.scaled(
            bingoCells[row][col]->width() - 20,  // 여백 축소 (30→20)
            bingoCells[row][col]->height() - 20, // 여백 축소 (30→20)
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        // 텍스트 지우고 이미지 설정
        bingoCells[row][col]->clear();
        bingoCells[row][col]->setAlignment(Qt::AlignCenter);

        // 내부 여백을 균등하게 설정하여 정확히 중앙 정렬
        int margin = 10; // 여백 축소 (15→10)
        bingoCells[row][col]->setContentsMargins(margin, margin, margin, margin);

        bingoCells[row][col]->setPixmap(scaledBear);
    } else {
        // 이미지와 텍스트 모두 지우기
        bingoCells[row][col]->clear();
        bingoCells[row][col]->setContentsMargins(0, 0, 0, 0);
    }
}

QColor MultiGameWidget::getCellColor(int row, int col) {
    return cellColors[row][col];
}

QString MultiGameWidget::getCellColorName(int /* row */, int /* col */) {
    return "";
}

int MultiGameWidget::colorDistance(const QColor &c1, const QColor &c2) {
    // HSV 색상 모델로 변환
    int h1, s1, v1, h2, s2, v2;
    c1.getHsv(&h1, &s1, &v1);
    c2.getHsv(&h2, &s2, &v2);

    // 색조(Hue)가 원형이므로 특별 처리
    int hueDiff = qAbs(h1 - h2);
    hueDiff = qMin(hueDiff, 360 - hueDiff); // 원형 거리 계산

    // 색조, 채도, 명도에 가중치 적용 (가중치 증가하여 더 엄격한 판정)
    double hueWeight = 3.0;    // 색조 차이에 매우 높은 가중치 (2.0 -> 3.0)
    double satWeight = 1.5;    // 채도 차이에 증가된 가중치 (1.0 -> 1.5)
    double valWeight = 0.7;    // 명도 차이에 약간 증가된 가중치 (0.5 -> 0.7)

    // 정규화된 거리 계산
    double normHueDiff = (hueDiff / 180.0) * hueWeight;
    double normSatDiff = (qAbs(s1 - s2) / 255.0) * satWeight;
    double normValDiff = (qAbs(v1 - v2) / 255.0) * valWeight;

    // 거리 합산 (각 거리의 제곱합)
    double distance = (normHueDiff * normHueDiff) +
                     (normSatDiff * normSatDiff) +
                     (normValDiff * normValDiff);

    // 거리를 0-100 범위로 스케일링
    return static_cast<int>(distance * 100);
}

bool MultiGameWidget::isColorBright(const QColor &color) {
    // YIQ 공식으로 색상의 밝기 확인 (텍스트 색상 결정용)
    return ((color.red() * 299) + (color.green() * 587) + (color.blue() * 114)) / 1000 > 128;
}

// 색상 보정 함수 추가
QImage MultiGameWidget::adjustColorBalance(const QImage &image) {
    // qDebug() << "Color balance adjustment started";

    // Check if image is valid
    if (image.isNull()) {
        qDebug() << "ERROR: Input image is null, returning original";
        return image;
    }

    // qDebug() << "Creating adjusted image copy, dimensions: " << image.width() << "x" << image.height();

    try {
        // Create new image with same size
        QImage adjustedImage = image.copy();

        // Color adjustment parameters
        const double redReduction = 0.85;
        const int blueGreenBoost = 5;

        // qDebug() << "Processing image pixels with redReduction=" << redReduction << ", blueGreenBoost=" << blueGreenBoost;

        // Process image pixels - sample every 2nd pixel for performance
        for (int y = 0; y < adjustedImage.height(); y += 2) {
            for (int x = 0; x < adjustedImage.width(); x += 2) {
                QColor pixel(adjustedImage.pixel(x, y));

                // Reduce red channel, increase blue and green channels
                int newRed = qBound(0, static_cast<int>(pixel.red() * redReduction), 255);
                int newGreen = qBound(0, pixel.green() + blueGreenBoost, 255);
                int newBlue = qBound(0, pixel.blue() + blueGreenBoost, 255);

                // Apply new color
                adjustedImage.setPixelColor(x, y, QColor(newRed, newGreen, newBlue));
            }
        }

        // qDebug() << "Color balance adjustment completed successfully";
        return adjustedImage;
    }
    catch (const std::exception& e) {
        qDebug() << "ERROR: Exception in adjustColorBalance: " << e.what();
        return image;  // Return original image on error
    }
    catch (...) {
        qDebug() << "ERROR: Unknown exception in adjustColorBalance";
        return image;  // Return original image on error
    }
}

// updateCameraFrame 함수에 색상 보정 적용
void MultiGameWidget::updateCameraFrame() {
    if (!camera) {
        qDebug() << "ERROR: Camera object is null";
        return;
    }

    // Get new frame from camera
    QImage frame = camera->getCurrentFrame();

    if (frame.isNull()) {
        qDebug() << "Camera frame is null";
        return;
    }

    // Safety check: verify cameraView is valid
    if (!cameraView) {
        qDebug() << "ERROR: cameraView is null";
        return;
    }

    try {
        // qDebug() << "Applying color balance adjustment";
        // Apply color balance correction - with safety check
        QImage adjustedFrame;
        try {
            adjustedFrame = adjustColorBalance(frame);
            if (adjustedFrame.isNull()) {
                qDebug() << "WARNING: Color adjustment returned null image, using original";
                adjustedFrame = frame;
            }
        }
        catch (...) {
            qDebug() << "ERROR: Exception during color adjustment, using original frame";
            adjustedFrame = frame;
        }

        // Scale image to fit the QLabel while maintaining aspect ratio
        QPixmap scaledPixmap = QPixmap::fromImage(adjustedFrame).scaled(
            cameraView->size(),
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation);

        // Create a copy of the pixmap for painting
        QPixmap paintPixmap = scaledPixmap;
        QPainter painter(&paintPixmap);
        if (!painter.isActive()) {
            qDebug() << "ERROR: Failed to create active painter";
            cameraView->setPixmap(scaledPixmap);
            return;
        }

            painter.setRenderHint(QPainter::Antialiasing);

        // Calculate circle center coordinates
        int centerX = paintPixmap.width() / 2;
        int centerY = paintPixmap.height() / 2;

        // Calculate circle radius (percentage of image width)
        int radius = (paintPixmap.width() * circleRadius) / 100;

        // Set green pen with increased width
            QPen pen(QColor(0, 255, 0, 180));
        pen.setWidth(5);
            painter.setPen(pen);

        // Draw circle
            painter.drawEllipse(QPoint(centerX, centerY), radius, radius);
        painter.end();

        // Calculate RGB average inside circle area
        if (adjustedFrame.width() > 0 && adjustedFrame.height() > 0) {
            // Calculate safe radius
            int safeRadius = qMin((adjustedFrame.width() * circleRadius) / 100,
                               qMin(adjustedFrame.width()/2, adjustedFrame.height()/2));

            calculateAverageRGB(adjustedFrame, adjustedFrame.width()/2, adjustedFrame.height()/2, safeRadius);
        }

        // Update RGB values (always, no need for conditional check)
        if (rgbValueLabel) {
                rgbValueLabel->setText(QString("R: %1  G: %2  B: %3").arg(avgRed).arg(avgGreen).arg(avgBlue));

            // Set background color to average RGB
                QPalette palette = rgbValueLabel->palette();
                palette.setColor(QPalette::Window, QColor(avgRed, avgGreen, avgBlue));
                palette.setColor(QPalette::WindowText, (avgRed + avgGreen + avgBlue > 380) ? Qt::black : Qt::white);
                rgbValueLabel->setPalette(palette);
                rgbValueLabel->setAutoFillBackground(true);
        }

        // Display the final image with circle
        cameraView->setPixmap(paintPixmap);
    }
    catch (const std::exception& e) {
        qDebug() << "ERROR: Exception in updateCameraFrame: " << e.what();
    }
    catch (...) {
        qDebug() << "ERROR: Unknown exception in updateCameraFrame";
    }
}

void MultiGameWidget::calculateAverageRGB(const QImage &image, int centerX, int centerY, int radius) {
    // Return immediately if image is invalid
    if (image.isNull() || radius <= 0) {
        qDebug() << "Image is invalid or radius is less than or equal to 0.";
        avgRed = avgGreen = avgBlue = 0;
        return;
    }

    // Adjust center point to avoid exceeding image boundaries
    int safeX = qBound(radius, centerX, image.width() - radius);
    int safeY = qBound(radius, centerY, image.height() - radius);

    // Initialize pixel sums and count
    long totalR = 0, totalG = 0, totalB = 0;
    int pixelCount = 0;

    // Check pixels inside the circle
    for (int y = safeY - radius; y < safeY + radius; y++) {
        for (int x = safeX - radius; x < safeX + radius; x++) {
            // Check if point is inside circle (using circle equation)
            int dx = x - safeX;
            int dy = y - safeY;
            int distSquared = dx*dx + dy*dy;

            if (distSquared <= radius*radius) {
                // Safety check: verify point is within image boundaries
                if (x >= 0 && x < image.width() && y >= 0 && y < image.height()) {
                    QColor pixel(image.pixel(x, y));
                    totalR += pixel.red();
                    totalG += pixel.green();
                    totalB += pixel.blue();
                pixelCount++;
                }
            }
        }
    }

    // Calculate averages
    if (pixelCount > 0) {
        avgRed = totalR / pixelCount;
        avgGreen = totalG / pixelCount;
        avgBlue = totalB / pixelCount;
    } else {
        avgRed = avgGreen = avgBlue = 0;
    }

    // qDebug() << "calculateAverageRGB completed: Average RGB=(" << avgRed << "," << avgGreen << "," << avgBlue
    //          << "), pixel count=" << pixelCount;
}

void MultiGameWidget::handleCameraDisconnect() {
    cameraView->setText("Camera disconnected");
    qDebug() << "Camera disconnected";

    // 플래그 설정만 업데이트 (버튼 참조 제거)
    isCapturing = false;

    // 선택된 셀이 있으면 선택 해제
    if (selectedCell.first >= 0 && selectedCell.second >= 0) {
        deselectCell();
    }
}

void MultiGameWidget::onCircleSliderValueChanged(int value) {
    circleRadius = value;
    circleValueLabel->setText(QString::number(value) + "%");
}

void MultiGameWidget::restartCamera()
{
    if (isCapturing) {
        qDebug() << "Performing periodic camera reinitialization...";
        // Stop camera
        camera->stopCapturing();

        // Wait briefly
        QThread::msleep(500);

        // Restart camera
        if (!camera->startCapturing()) {
            QMessageBox::warning(this, "Warning", "Camera restart failed. Will try again later.");
            return;
        }

        qDebug() << "Camera reinitialization completed!";
    }
}

void MultiGameWidget::onCaptureButtonClicked() {
    if (!isCapturing || selectedCell.first < 0)
        return;

    // 현재 선택된 셀 좌표
    int row = selectedCell.first;
    int col = selectedCell.second;

    // 선택된 셀이 이미 빙고 상태이면 무시
    if (bingoStatus[row][col])
        return;

    // RGB 값 확인
    QColor selectedColor = cellColors[row][col];
    QColor capturedColor(avgRed, avgGreen, avgBlue);

    // 색상 거리 계산
    int distance = colorDistance(selectedColor, capturedColor);

    // 디버그 로그 추가
    qDebug() << "Capture button clicked - Color distance: " << distance;
    qDebug() << "Selected cell color: " << selectedColor.red() << "," << selectedColor.green() << "," << selectedColor.blue();
    qDebug() << "Captured color: " << capturedColor.red() << "," << capturedColor.green() << "," << capturedColor.blue();

    // 색상 유사도 임계값 (updateCameraFrame과 동일하게 20으로 설정)
    const int THRESHOLD = 30;

    // 색상 유사도에 따라 처리
    if (distance <= THRESHOLD) {  // 색상이 유사함 - 빙고 처리
        qDebug() << "Color match successful! Processing bingo";
        bingoStatus[row][col] = true;
        updateCellStyle(row, col);

        // 선택 초기화
        selectedCell = qMakePair(-1, -1);

        // 빙고 점수 업데이트
        updateBingoScore();

        // 상태 메시지 업데이트
        statusMessageLabel->setText("Great job! Perfect color match!");

        // 2초 후에 메시지 변경하는 타이머 설정
        QTimer::singleShot(2000, this, [this]() {
            statusMessageLabel->setText("Please select a cell to match colors");
        });

        // 카메라 중지 - 물리 버튼 사용시 상태만 업데이트
        if (isCapturing) {
            // 카메라 캡처 중지
            camera->stopCapturing();
            isCapturing = false;

            // 재시작 타이머 중지
            if (cameraRestartTimer) {
                cameraRestartTimer->stop();
            }

            // 카메라 뷰에 메시지 표시
            if (cameraView) {
                cameraView->setText("Camera is off");
                cameraView->setStyleSheet("QLabel { color: gray; font-weight: bold; }");
            }

            // 슬라이더 위젯 숨김
            if (sliderWidget) {
                sliderWidget->hide();
            }
        }
    } else {  // 색상이 다름 - X 표시 (개선된 코드)
        qDebug() << "Color match failed - drawing X mark";

        // 안전하게 새로운 접근법: 셀 자체에 X 표시 그리기
        QPixmap cellBg(bingoCells[row][col]->size());
        cellBg.fill(cellColors[row][col]);

        // X 이미지 준비 - 셀 크기에 맞게 스케일링
        QPixmap scaledX = xImage.scaled(
            bingoCells[row][col]->width() - 20,
            bingoCells[row][col]->height() - 20,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        // 배경에 X 이미지 합성
        QPainter painter(&cellBg);
        painter.drawPixmap(
            (cellBg.width() - scaledX.width()) / 2,
            (cellBg.height() - scaledX.height()) / 2,
            scaledX
        );

        // 테두리 추가
        QString borderStyle = "border-top: 1px solid black; border-left: 1px solid black;";
        if (row == 2) borderStyle += " border-bottom: 1px solid black;";
        if (col == 2) borderStyle += " border-right: 1px solid black;";

        // 셀에 합성된 이미지 적용
        bingoCells[row][col]->setPixmap(cellBg);
        bingoCells[row][col]->setStyleSheet(borderStyle);

        // 상태 메시지 업데이트
        statusMessageLabel->setText("The colors don't match. Keep trying!");

        // 2초 후에 X 표시 제거하고 원래 스타일로 돌려놓기
        QTimer::singleShot(2000, this, [this, row, col]() {
            if (row >= 0 && row < 3 && col >= 0 && col < 3) {
                if (!bingoStatus[row][col]) {
                    updateCellStyle(row, col);
                }
            }
            statusMessageLabel->setText("Try again or select another cell");
        });
    }
}

void MultiGameWidget::clearXMark() {
    // X 표시가 있는 셀이 있으면 원래대로 되돌리기
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            // deprecated 경고 수정: pixmap(Qt::ReturnByValue) 사용
            QPixmap cellPixmap = bingoCells[row][col]->pixmap(Qt::ReturnByValue);

            // pixmap이 설정되어 있고, 체크되지 않은 셀인 경우
            if (!cellPixmap.isNull() && !bingoStatus[row][col]) {
                updateCellStyle(row, col);
            }
        }
    }
}

void MultiGameWidget::updateBingoScore() {
    bingoCount = 0;

    // 가로줄 확인
    for (int row = 0; row < 3; ++row) {
        if (bingoStatus[row][0] && bingoStatus[row][1] && bingoStatus[row][2]) {
            bingoCount++;
        }
    }

    // 세로줄 확인
    for (int col = 0; col < 3; ++col) {
        if (bingoStatus[0][col] && bingoStatus[1][col] && bingoStatus[2][col]) {
            bingoCount++;
        }
    }

    // 대각선 확인 (좌상단 -> 우하단)
    if (bingoStatus[0][0] && bingoStatus[1][1] && bingoStatus[2][2]) {
        bingoCount++;
    }

    // 대각선 확인 (우상단 -> 좌하단)
    if (bingoStatus[0][2] && bingoStatus[1][1] && bingoStatus[2][0]) {
        bingoCount++;
    }

    // 빙고 점수 표시 업데이트 (영어로 변경)
    bingoScoreLabel->setText(QString("Bingo: %1").arg(bingoCount));

    // 상대 플레이어에 빙고 점수 전송
    if (network) {
        network->sendBingoScore(bingoCount);
    }

    // 빙고 완성시 축하 메시지
    if (bingoCount > 0) {
        // 빙고 완성 효과 (배경색 변경 등)
        bingoScoreLabel->setStyleSheet("color: red; font-weight: bold;");
    } else {
        bingoScoreLabel->setStyleSheet("");
    }

    // 3빙고 이상 달성 확인
    if (bingoCount >= 3) {
        // 상대 플레이어에 결과 전송
        network->sendGameOverMessage();
        // SUCCESS 메시지 표시
        showSuccessMessage();
    }
}

// 상대 플레이어의 빙고 점수 업데이트
void MultiGameWidget::updateOpponentScore(int score) {
    qDebug() << "DEBUG: 🎯 Opponent's Bingo Score Updated:" << score;
}

// 새로운 함수 추가: 성공 메시지 표시 및 게임 초기화
void MultiGameWidget::showSuccessMessage() {
    // 성공 메시지 레이블 크기 설정 (전체 위젯 크기로)
    successLabel->setGeometry(0, 0, width(), height());
    successLabel->raise(); // 다른 위젯 위에 표시
    successLabel->show();

    // 1초 후 메시지 숨기고 게임 초기화
    successTimer->start(1000);
}

// 새로운 함수 추가: 성공 메시지 숨기고 게임 초기화
void MultiGameWidget::hideSuccessAndReset() {
    successLabel->hide();
    resetGame();
}

// 게임 초기화 함수 수정
void MultiGameWidget::resetGame() {
    // 빙고 상태 초기화
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            bingoStatus[row][col] = false;
            bingoCells[row][col]->clear();  // 모든 내용 지우기
            bingoCells[row][col]->setContentsMargins(0, 0, 0, 0);  // 여백 초기화

            // 색상은 유지하고 스타일만 업데이트
            updateCellStyle(row, col);
        }
    }

    // 빙고 점수 초기화
    bingoCount = 0;
    bingoScoreLabel->setText("Bingo: 0");
    bingoScoreLabel->setStyleSheet("");

    // 선택된 셀 초기화
    selectedCell = qMakePair(-1, -1);

    // 셀 색상 새로 생성
    generateRandomColors();

    // 네트워크 초기화
    network->disconnectFromPeer();

    // 색상 생성 코드 제거 - 기존 색상 유지
    // generateRandomColors(); <- 이 줄 제거 또는 주석 처리

    // 타이머 재시작
    startGameTimer();
}

// 리사이즈 이벤트 처리 (successLabel 크기 조정)
void MultiGameWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    // 성공 메시지 레이블 크기 조정
    if (successLabel) {
        successLabel->setGeometry(0, 0, width(), height());
    }

    // 실패 메시지 레이블 크기 조정
    if (failLabel) {
        failLabel->setGeometry(0, 0, width(), height());
    }

    // 타이머 위치 업데이트
    updateTimerPosition();

    // Back 버튼 위치 업데이트
    updateBackButtonPosition();
}

void MultiGameWidget::onBackButtonClicked() {
    qDebug() << "DEBUG: MultiGameWidget - Back button clicked";

    // 카메라가 켜져 있다면 중지
    if (isCapturing) {
        qDebug() << "DEBUG: MultiGameWidget - Stopping camera before emitting back signal";
        stopCamera();
    }

    qDebug() << "DEBUG: MultiGameWidget - Emitting backToMainRequested signal";
    emit backToMainRequested();
}

QPixmap MultiGameWidget::createXImage() {
    qDebug() << "Creating smooth line-based X mark";

    // Create X image with transparent background
    QPixmap xImg(100, 100);
    xImg.fill(Qt::transparent);

    QPainter painter(&xImg);
    // 선을 부드럽게 그리기 위해 안티앨리어싱 활성화
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 선 두께와 색상 설정
    QPen pen(Qt::white);
    pen.setWidth(8);  // 선 두께 설정
    pen.setCapStyle(Qt::RoundCap);  // 선 끝을 둥글게
    pen.setJoinStyle(Qt::RoundJoin);  // 선 연결 부분을 둥글게
    painter.setPen(pen);

    // 중심점 계산
    const int center = 50;  // 100x100 이미지의 중심
    const int padding = 20;  // 외곽 여백

    // 대각선 두 개 그리기
    painter.drawLine(padding, padding, 100-padding, 100-padding);  // 왼쪽 위에서 오른쪽 아래로
    painter.drawLine(100-padding, padding, padding, 100-padding);  // 오른쪽 위에서 왼쪽 아래로

    painter.end();

    qDebug() << "Smooth line-based X mark created successfully";
    return xImg;
}

// Add a separate color correction function
QColor MultiGameWidget::correctBluecast(const QColor &color) {
    qDebug() << "Correcting color cast: input RGB=" << color.red() << "," << color.green() << "," << color.blue();

    // Reduce red factor
    double redFactor = 0.85;

    // Boost green and blue
    int gbBoost = 5;

    // Adjust color channels
    int r = qBound(0, static_cast<int>(color.red() * redFactor), 255);
    int g = qBound(0, color.green() + gbBoost, 255);
    int b = qBound(0, color.blue() + gbBoost, 255);

    QColor corrected(r, g, b);
    qDebug() << "Corrected color: RGB=" << corrected.red() << "," << corrected.green() << "," << corrected.blue();
    return corrected;
}

void MultiGameWidget::updateBackButtonPosition() {
    if (backButton && restartButton) {
        // 화면 우측 하단에서 약간 떨어진 위치에 배치
        int margin = 20;
        int buttonSpacing = 10; // 버튼 사이 간격

        // Back 버튼 위치
        backButton->move(width() - backButton->width() - margin,
                         height() - backButton->height() - margin);

        // Restart 버튼 위치 (Back 버튼 왼쪽)
        restartButton->move(width() - backButton->width() - margin - buttonSpacing - restartButton->width(),
                            height() - restartButton->height() - margin);

        // 버튼들을 맨 위로 올림
        backButton->raise();
        restartButton->raise();
    }
}

void MultiGameWidget::onRestartButtonClicked() {
    // 게임 진행 중 확인 (카메라가 작동 중이면 중지)
    if (isCapturing) {
        stopCamera();
    }

    // 게임 초기화
    resetGame();

    // 상태 메시지 업데이트
    statusMessageLabel->setText("Game restarted! Please select a cell to match colors");
}

// 타이머 틱마다 호출되는 함수
void MultiGameWidget::onTimerTick() {
    remainingSeconds--;

    // 타이머 디스플레이 업데이트
    updateTimerDisplay();

    // 시간이 다 되었는지 확인
    if (remainingSeconds <= 0) {
        // 타이머 정지
        gameTimer->stop();

        // 빙고 3개 이상인지 확인
        if (bingoCount >= 3) {
            // 이미 성공한 경우 아무것도 하지 않음
            return;
        }

        // 실패 처리
        showFailMessage();
    }
}

// 타이머 디스플레이 업데이트
void MultiGameWidget::updateTimerDisplay() {
    // 남은 시간을 분:초 형식으로 표시 (time 00:00)
    int minutes = remainingSeconds / 60;
    int seconds = remainingSeconds % 60;

    QString timeText = QString("Time %1:%2")
                        .arg(minutes, 2, 10, QChar('0'))
                        .arg(seconds, 2, 10, QChar('0'));

    timerLabel->setText(timeText);

    // 남은 시간에 따라 색상 변경
    if (remainingSeconds <= 10) {
        // 10초 이하면 빨간색
        timerLabel->setStyleSheet("QLabel { background-color: rgba(50, 50, 50, 200); color: red; "
                                 "border-radius: 12px; padding: 8px 15px; font-weight: bold; font-size: 24px; }");
    } else {
        // 그 외에는 하얀색
        timerLabel->setStyleSheet("QLabel { background-color: rgba(50, 50, 50, 200); color: white; "
                                 "border-radius: 12px; padding: 8px 15px; font-weight: bold; font-size: 24px; }");
    }

    // 타이머 레이블 크기 및 위치 조정
    timerLabel->adjustSize();
    updateTimerPosition();
}

// 타이머 위치 업데이트
void MultiGameWidget::updateTimerPosition() {
    if (timerLabel) {
        // 화면 상단 중앙에 배치
        timerLabel->move((width() - timerLabel->width()) / 2, 10);
        timerLabel->raise(); // 다른 위젯 위에 표시
    }
}

// 타이머 시작
void MultiGameWidget::startGameTimer() {
    // 타이머 초기화
    remainingSeconds = 120; // 2분
    updateTimerDisplay();

    // 타이머 시작
    gameTimer->start();
}

// 타이머 정지
void MultiGameWidget::stopGameTimer() {
    if (gameTimer) {
        gameTimer->stop();
    }
}

// 실패 메시지 표시
void MultiGameWidget::showFailMessage() {
    // 카메라가 실행 중이면 중지
    if (isCapturing) {
        stopCamera();
    }

    // 실패 메시지 레이블 크기와 위치 설정
    failLabel->setGeometry(0, 0, width(), height());
    failLabel->raise(); // 다른 위젯 위에 표시
    failLabel->show();

    // 2초 후 메시지 숨기고 메인 화면으로 돌아가기
    QTimer::singleShot(2000, this, &MultiGameWidget::hideFailAndReset);
}

// 실패 메시지 숨기고 게임 초기화
void MultiGameWidget::hideFailAndReset() {
    failLabel->hide();

    // 카메라가 실행 중이면 중지
    if (isCapturing) {
        stopCamera();
    }

    // 게임 상태 초기화
    resetGame();

    // 메인 화면으로 돌아가는 신호 발생
    emit backToMainRequested();
}

// 전달받은 색상을 셀에 설정하는 메서드
void MultiGameWidget::setCustomColors(const QList<QColor> &colors) {
    qDebug() << "Setting custom colors to bingo cells";

    if (colors.size() < 9) {
        qDebug() << "Not enough colors provided. Expected 9, got" << colors.size();
        return;
    }

    int colorIndex = 0;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            cellColors[row][col] = colors[colorIndex++];

            // 경계선 스타일 생성
            QString borderStyle = "border-top: 1px solid black; border-left: 1px solid black;";

            if (row == 2) {
                borderStyle += " border-bottom: 1px solid black;";
            }
            if (col == 2) {
                borderStyle += " border-right: 1px solid black;";
            }

            // 배경색 적용, 텍스트 제거함
            bingoCells[row][col]->setText("");
            bingoCells[row][col]->setStyleSheet(
                QString("background-color: %1; %2")
                .arg(cellColors[row][col].name())
                .arg(borderStyle)
            );
        }
    }
}

// 원 오버레이 생성 함수 구현
void MultiGameWidget::createCircleOverlay(int width, int height) {
    // 빈 투명 오버레이 생성
    overlayCircle = QPixmap(width, height);
    overlayCircle.fill(Qt::transparent);

    // 원 그리기 설정이 켜져 있으면 원 그리기
    if (showCircle) {
        QPainter painter(&overlayCircle);
        painter.setRenderHint(QPainter::Antialiasing);

        // 원 중심 좌표 계산 (이미지 중앙)
        int centerX = width / 2;
        int centerY = height / 2;

        // 원의 반지름 계산 (이미지 너비의 percentage)
        int radius = (width * circleRadius) / 100;

        // 녹색 반투명 펜 설정
        QPen pen(QColor(0, 255, 0, 180));
        pen.setWidth(2);
        painter.setPen(pen);

        // 원 그리기
        painter.drawEllipse(QPoint(centerX, centerY), radius, radius);
    }
}

// RGB 값 업데이트 함수 구현
void MultiGameWidget::updateRgbValues() {
    if (!isCapturing || !showCircle || !showRgbValues)
        return;

    // 현재 프레임 가져오기
    QImage frame = camera->getCurrentFrame();
    if (frame.isNull())
        return;

    // 원 영역 내부 RGB 평균 계산
    calculateAverageRGB(frame, frame.width()/2, frame.height()/2,
                       (frame.width() * circleRadius) / 100);

    // RGB 값 업데이트
    rgbValueLabel->setText(QString("R: %1  G: %2  B: %3").arg(avgRed).arg(avgGreen).arg(avgBlue));

    // 배경색 설정 (평균 RGB 값)
    QPalette palette = rgbValueLabel->palette();
    palette.setColor(QPalette::Window, QColor(avgRed, avgGreen, avgBlue));
    palette.setColor(QPalette::WindowText, (avgRed + avgGreen + avgBlue > 380) ? Qt::black : Qt::white);
    rgbValueLabel->setPalette(palette);
    rgbValueLabel->setAutoFillBackground(true);
}

void MultiGameWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    qDebug() << "DEBUG: MultiGameWidget showEvent triggered";
}

void MultiGameWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    qDebug() << "DEBUG: MultiGameWidget hideEvent triggered";

    if(camera) {
        stopCamera();
        camera->closeCamera();
    }
}

// createBearImage 함수 추가 (BingoWidget 클래스의 멤버 함수)
QPixmap MultiGameWidget::createBearImage() {
    QPixmap bearImage(80, 80);
    bearImage.fill(Qt::transparent);
    QPainter painter(&bearImage);

    // 안티앨리어싱 비활성화 (픽셀 느낌을 위해)
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 갈색 곰돌이 색상
    QColor bearColor(165, 113, 78);
    QColor darkBearColor(120, 80, 60);

    // 중앙 정렬을 위한 오프셋
    int offsetX = 5;
    int offsetY = 5;

    // 기본 얼굴 사각형
    painter.setPen(Qt::NoPen);
    painter.setBrush(bearColor);
    painter.drawRect(15 + offsetX, 20 + offsetY, 40, 40);

    // 얼굴 둥글게 만들기 - 픽셀 추가
    painter.drawRect(11 + offsetX, 25 + offsetY, 4, 30);  // 왼쪽
    painter.drawRect(55 + offsetX, 25 + offsetY, 4, 30);  // 오른쪽
    painter.drawRect(20 + offsetX, 16 + offsetY, 30, 4);  // 위
    painter.drawRect(20 + offsetX, 60 + offsetY, 30, 4);  // 아래

    // 추가 픽셀로 더 둥글게 표현
    painter.drawRect(15 + offsetX, 20 + offsetY, 5, 5);   // 좌상단 보강
    painter.drawRect(50 + offsetX, 20 + offsetY, 5, 5);   // 우상단 보강
    painter.drawRect(15 + offsetX, 55 + offsetY, 5, 5);   // 좌하단 보강
    painter.drawRect(50 + offsetX, 55 + offsetY, 5, 5);   // 우하단 보강

    // 모서리 픽셀 추가
    painter.drawRect(12 + offsetX, 21 + offsetY, 3, 4);   // 좌상단 모서리
    painter.drawRect(55 + offsetX, 21 + offsetY, 3, 4);   // 우상단 모서리
    painter.drawRect(12 + offsetX, 55 + offsetY, 3, 4);   // 좌하단 모서리
    painter.drawRect(55 + offsetX, 55 + offsetY, 3, 4);   // 우하단 모서리

    // 귀 위치 및 크기 조정 (가로 길이 축소)
    // 왼쪽 귀 - 가로 길이 축소 (13→10)
    painter.drawRect(16 + offsetX, 6 + offsetY, 10, 16);  // 기본 왼쪽 귀 (가로 축소)
    painter.drawRect(11 + offsetX, 10 + offsetY, 5, 12);  // 왼쪽 귀 왼쪽 보강
    painter.drawRect(26 + offsetX, 10 + offsetY, 5, 12);  // 왼쪽 귀 오른쪽 보강 (좌표 조정)

    // 오른쪽 귀 - 가로 길이 축소 (13→10)
    painter.drawRect(44 + offsetX, 6 + offsetY, 10, 16);  // 기본 오른쪽 귀 (가로 축소)
    painter.drawRect(39 + offsetX, 10 + offsetY, 5, 12);  // 오른쪽 귀 왼쪽 보강 (좌표 조정)
    painter.drawRect(54 + offsetX, 10 + offsetY, 5, 12);  // 오른쪽 귀 오른쪽 보강

    // 귀 안쪽 (더 어두운 색) - 가로 길이 축소 (7→6)
    painter.setBrush(darkBearColor);
    painter.drawRect(19 + offsetX, 9 + offsetY, 6, 10);   // 왼쪽 귀 안쪽 (가로 축소)
    painter.drawRect(45 + offsetX, 9 + offsetY, 6, 10);   // 오른쪽 귀 안쪽 (가로 축소)

    // 눈 (간격 넓히기)
    painter.setBrush(Qt::black);
    painter.drawRect(22 + offsetX, 35 + offsetY, 6, 6);   // 왼쪽 눈 (좌표 조정 - 더 왼쪽으로)
    painter.drawRect(42 + offsetX, 35 + offsetY, 6, 6);   // 오른쪽 눈 (좌표 조정 - 더 오른쪽으로)

    // 코 (위치 위로 올리고 크기 축소)
    painter.drawRect(32 + offsetX, 42 + offsetY, 6, 4);   // 코 (위치 위로, 크기 축소 8x5→6x4)

    qDebug() << "Created adjusted pixel-style bear drawing, size:" << bearImage.width() << "x" << bearImage.height();
    return bearImage;
}
