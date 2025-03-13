#include "bingowidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QSizePolicy>
#include <QThread>

BingoWidget::BingoWidget(QWidget *parent) : QWidget(parent),
    isCapturing(false),
    showCircle(true),
    circleRadius(10),
    avgRed(0),
    avgGreen(0),
    avgBlue(0),
    showRgbValues(true),
    selectedCell(-1, -1),
    bingoCount(0)
{
    qDebug() << "BingoWidget constructor started";
    
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
    connect(successTimer, &QTimer::timeout, this, &BingoWidget::hideSuccessAndReset);
    
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
    QWidget *sliderWidget = new QWidget(cameraArea);
    sliderWidget->setFixedWidth(300); // Same width as camera
    QHBoxLayout *circleSliderLayout = new QHBoxLayout(sliderWidget);
    circleSliderLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *circleLabel = new QLabel("Circle Size:");
    circleSlider = new QSlider(Qt::Horizontal);
    circleSlider->setMinimum(5);
    circleSlider->setMaximum(30);
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
    connect(fadeXTimer, &QTimer::timeout, this, &BingoWidget::clearXMark);
    
    // 빙고 셀에 낮은 채도의 랜덤 색상 생성
    generateRandomColors();
    
    // 카메라 초기화
    camera = new V4L2Camera(this);
    
    // FPS 설정 줄 제거 (setFrameRate 메서드가 없음)
    
    // 카메라 신호 연결
    connect(camera, &V4L2Camera::newFrameAvailable, this, &BingoWidget::updateCameraFrame);
    connect(camera, &V4L2Camera::deviceDisconnected, this, &BingoWidget::handleCameraDisconnect);
    
    // 위젯 컨트롤 신호 연결 - remove RGB checkbox connection
    connect(circleSlider, &QSlider::valueChanged, this, &BingoWidget::onCircleSliderValueChanged);
    
    // 카메라 재시작 타이머 설정 (유지)
    cameraRestartTimer = new QTimer(this);
    cameraRestartTimer->setInterval(30 * 60 * 1000); // 30분마다 재시작
    connect(cameraRestartTimer, &QTimer::timeout, this, &BingoWidget::restartCamera);
    
    // 카메라 열기만 하고 자동 시작은 하지 않음
    if (!camera->openCamera("/dev/video4")) {
        cameraView->setText("Camera connection failed");
    }

    // 슬라이더 설정
    circleSlider->setMinimumHeight(30);

    // 픽셀 스타일 곰돌이 이미지 생성 (디자인 수정)
    bearImage = QPixmap(80, 80);
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

    // X 이미지 생성
    xImage = createXImage();
    
    qDebug() << "Basic variable initialization completed";
    
    qDebug() << "BingoWidget constructor completed";
}

BingoWidget::~BingoWidget() {
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
}

bool BingoWidget::eventFilter(QObject *obj, QEvent *event) {
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
void BingoWidget::selectCell(int row, int col) {
    qDebug() << "selectCell called: (" << row << "," << col << ")";
    
    // Range check
    if (row < 0 || row >= 3 || col < 0 || col >= 3) {
        qDebug() << "Invalid cell coordinates.";
        return;
    }
    
    selectedCell = qMakePair(row, col);
    
    // Safety check: verify bingoCells array
    if (!bingoCells[row][col]) {
        qDebug() << "bingoCells[" << row << "][" << col << "] is null.";
        return;
    }
    
    // Create border style
    QString borderStyle = "border-top: 1px solid black; border-left: 1px solid black;";
    
    if (row == 2) {
        borderStyle += " border-bottom: 1px solid black;";
    }
    if (col == 2) {
        borderStyle += " border-right: 1px solid black;";
    }
    
    // Highlight selected cell (3px red border)
    QString style = QString("background-color: %1; %2 border: 3px solid red;")
                   .arg(cellColors[row][col].name())
                   .arg(borderStyle);
    bingoCells[row][col]->setStyleSheet(style);
    
    // Don't set X mark (will be set after first frame in updateCameraFrame)
    bingoCells[row][col]->clear();
    
    qDebug() << "Cell style update completed";
    
    // Start camera automatically
    startCamera();
    
    // Update status message for selected cell
    statusMessageLabel->setText("Try to match the color in the camera!");
    
    qDebug() << "selectCell completed";
}

// 셀 선택 해제 및 카메라 중지 함수
void BingoWidget::deselectCell() {
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
void BingoWidget::startCamera() {
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
            
            qDebug() << "Camera capture started successfully";
        } else {
            qDebug() << "Failed to start camera capture";
            QMessageBox::critical(this, "Error", "Failed to start camera");
        }
    } else {
        qDebug() << "Camera is already capturing.";
    }
}

void BingoWidget::stopCamera() {
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
        
        qDebug() << "Camera capture stopped";
    } else {
        qDebug() << "Camera is already stopped.";
    }
}

// 채도가 낮은 랜덤 색상 생성
void BingoWidget::generateRandomColors() {
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            // HSV 색상 모델을 사용하여 채도가 적당한 색상 생성
            int hue = QRandomGenerator::global()->bounded(360);  // 0-359 범위의 색조
            int saturation = QRandomGenerator::global()->bounded(70, 160);  // 70-160 범위의 채도 (너무 높지 않음)
            int value = QRandomGenerator::global()->bounded(180, 240);  // 180-240 범위의 명도 (너무 어둡거나 밝지 않음)
            
            QColor cellColor = QColor::fromHsv(hue, saturation, value);
            cellColors[row][col] = cellColor;
            
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
                .arg(cellColor.name())
                .arg(borderStyle)
            );
        }
    }
}

void BingoWidget::updateCellStyle(int row, int col) {
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

QColor BingoWidget::getCellColor(int row, int col) {
    return cellColors[row][col];
}

QString BingoWidget::getCellColorName(int /* row */, int /* col */) {
    return "";
}

int BingoWidget::colorDistance(const QColor &c1, const QColor &c2) {
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

bool BingoWidget::isColorBright(const QColor &color) {
    // YIQ 공식으로 색상의 밝기 확인 (텍스트 색상 결정용)
    return ((color.red() * 299) + (color.green() * 587) + (color.blue() * 114)) / 1000 > 128;
}

// 색상 보정 함수 추가
QImage BingoWidget::adjustColorBalance(const QImage &image) {
    qDebug() << "Color balance adjustment started";
    
    // Check if image is valid
    if (image.isNull()) {
        qDebug() << "ERROR: Input image is null, returning original";
        return image;
    }
    
    qDebug() << "Creating adjusted image copy, dimensions: " << image.width() << "x" << image.height();
    
    try {
        // Create new image with same size
        QImage adjustedImage = image.copy();
        
        // Color adjustment parameters
        const double redReduction = 0.85;
        const int blueGreenBoost = 5;
        
        qDebug() << "Processing image pixels with redReduction=" << redReduction << ", blueGreenBoost=" << blueGreenBoost;
        
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
        
        qDebug() << "Color balance adjustment completed successfully";
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
void BingoWidget::updateCameraFrame() {
    qDebug() << "updateCameraFrame started";
    
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
        qDebug() << "Applying color balance adjustment";
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
            Qt::SmoothTransformation);
            
        qDebug() << "Starting to draw circle";
        
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
        
        qDebug() << "Circle drawing completed";
        
        // Calculate RGB average inside circle area
        if (adjustedFrame.width() > 0 && adjustedFrame.height() > 0) {
            qDebug() << "Starting RGB average calculation";
            // Calculate safe radius
            int safeRadius = qMin((adjustedFrame.width() * circleRadius) / 100, 
                               qMin(adjustedFrame.width()/2, adjustedFrame.height()/2));
                               
            calculateAverageRGB(adjustedFrame, adjustedFrame.width()/2, adjustedFrame.height()/2, safeRadius);
            qDebug() << "RGB average calculation completed: R=" << avgRed << ", G=" << avgGreen << ", B=" << avgBlue;
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
        
        // Process selected cell if there is one
        qDebug() << "Checking selected cell: " << selectedCell.first << ", " << selectedCell.second;
        if (selectedCell.first >= 0 && selectedCell.first < 3 && 
            selectedCell.second >= 0 && selectedCell.second < 3 && 
            !bingoStatus[selectedCell.first][selectedCell.second]) {
            
            // Check color similarity between camera and selected cell
            QColor cameraColor(avgRed, avgGreen, avgBlue);
            QColor cellColor = getCellColor(selectedCell.first, selectedCell.second);
            
            // Calculate color distance
            int distance = colorDistance(cameraColor, cellColor);
            qDebug() << "Color distance: " << distance << " (threshold: 20)";
            
            // Check if bingoCells array element is valid
            if (bingoCells[selectedCell.first][selectedCell.second]) {
                qDebug() << "Setting X mark on cell";
                
                // Make sure xImage is valid
                if (xImage.isNull()) {
                    qDebug() << "xImage is null, creating new one";
                    xImage = createXImage();
                }
                
                // Check cell size before scaling
                QSize cellSize = bingoCells[selectedCell.first][selectedCell.second]->size();
                if (cellSize.width() > 20 && cellSize.height() > 20) {
                    bingoCells[selectedCell.first][selectedCell.second]->setAlignment(Qt::AlignCenter);
                    bingoCells[selectedCell.first][selectedCell.second]->setContentsMargins(10, 10, 10, 10);
                    
                    // Create a scaled copy of xImage (2/3 of cell size)
                    QPixmap scaledX = xImage.scaled(
                        cellSize.width() * 2/3,  // 2/3 of cell width
                        cellSize.height() * 2/3, // 2/3 of cell height  
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation
                    );
                    
                    // Set the pixmap if scaling was successful
                    if (!scaledX.isNull()) {
                        bingoCells[selectedCell.first][selectedCell.second]->setPixmap(scaledX);
                        qDebug() << "X mark successfully set";
                    }
                    else {
                        qDebug() << "ERROR: Failed to scale X image";
                    }
                }
                else {
                    qDebug() << "WARNING: Cell size too small for X mark: " << cellSize;
                }
            }
            
            // If color distance is below threshold, mark as successful match
            const int THRESHOLD = 20; // 30 -> 20으로 임계값 낮춤
            if (distance <= THRESHOLD) {
                qDebug() << "Color match successful! Processing bingo";
                bingoStatus[selectedCell.first][selectedCell.second] = true;
                updateCellStyle(selectedCell.first, selectedCell.second);
                
                // Reset selection
                selectedCell = qMakePair(-1, -1);
                
                // Stop camera
                stopCamera();
                
                // Update bingo score
                updateBingoScore();
                
                // Update status message for successful match
                statusMessageLabel->setText("Great job! Perfect color match!");
                
                // 1초 후에 메시지 변경하는 타이머 설정 -> 2초로 변경
                QTimer::singleShot(2000, this, [this]() {
                    statusMessageLabel->setText("Please select a cell to match colors");
                });
            } else {
                // Update status message based on color distance
                if (!statusMessageLabel->text().contains("don't match")) {
                    statusMessageLabel->setText("The colors don't match. Keep trying!");
                }
            }
        }
        
        // Display the final image with circle
        cameraView->setPixmap(paintPixmap);
        qDebug() << "Camera view updated successfully";
    }
    catch (const std::exception& e) {
        qDebug() << "ERROR: Exception in updateCameraFrame: " << e.what();
    }
    catch (...) {
        qDebug() << "ERROR: Unknown exception in updateCameraFrame";
    }
    
    qDebug() << "updateCameraFrame completed";
}

void BingoWidget::calculateAverageRGB(const QImage &image, int centerX, int centerY, int radius) {
    qDebug() << "calculateAverageRGB started: center=(" << centerX << "," << centerY << "), radius=" << radius;
    
    // Return immediately if image is invalid
    if (image.isNull() || radius <= 0) {
        qDebug() << "Image is invalid or radius is less than or equal to 0.";
        avgRed = avgGreen = avgBlue = 0;
        return;
    }
    
    // Adjust center point to avoid exceeding image boundaries
    int safeX = qBound(radius, centerX, image.width() - radius);
    int safeY = qBound(radius, centerY, image.height() - radius);
    
    qDebug() << "Safe center point: (" << safeX << "," << safeY << ")";
    
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
    
    qDebug() << "calculateAverageRGB completed: Average RGB=(" << avgRed << "," << avgGreen << "," << avgBlue 
             << "), pixel count=" << pixelCount;
}

void BingoWidget::handleCameraDisconnect() {
    cameraView->setText("Camera disconnected");
    qDebug() << "Camera disconnected";
    
    // 플래그 설정만 업데이트 (버튼 참조 제거)
    isCapturing = false;
    
    // 선택된 셀이 있으면 선택 해제
    if (selectedCell.first >= 0 && selectedCell.second >= 0) {
        deselectCell();
    }
}

void BingoWidget::onCircleSliderValueChanged(int value) {
    circleRadius = value;
    circleValueLabel->setText(QString::number(value) + "%");
}

void BingoWidget::onCircleCheckBoxToggled(bool checked) {
    // 사용하지 않음 (항상 표시)
    // showCircle 변수는 그대로 true로 유지
}

void BingoWidget::onRgbCheckBoxToggled(bool checked) {
    // Function not used anymore since RGB values are always shown
}

void BingoWidget::restartCamera()
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

void BingoWidget::clearXMark() {
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

void BingoWidget::updateBingoScore() {
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
    
    // 빙고 완성시 축하 메시지
    if (bingoCount > 0) {
        // 빙고 완성 효과 (배경색 변경 등)
        bingoScoreLabel->setStyleSheet("color: red; font-weight: bold;");
    } else {
        bingoScoreLabel->setStyleSheet("");
    }
    
    // 3빙고 이상 달성 확인
    if (bingoCount >= 3) {
        // SUCCESS 메시지 표시
        showSuccessMessage();
    }
}

// 새로운 함수 추가: 성공 메시지 표시 및 게임 초기화
void BingoWidget::showSuccessMessage() {
    // 성공 메시지 레이블 크기 설정 (전체 위젯 크기로)
    successLabel->setGeometry(0, 0, width(), height());
    successLabel->raise(); // 다른 위젯 위에 표시
    successLabel->show();
    
    // 1초 후 메시지 숨기고 게임 초기화
    successTimer->start(1000);
}

// 새로운 함수 추가: 성공 메시지 숨기고 게임 초기화
void BingoWidget::hideSuccessAndReset() {
    successLabel->hide();
    resetGame();
}

// 새로운 함수 추가: 게임 초기화
void BingoWidget::resetGame() {
    // 빙고 상태 초기화
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            bingoStatus[row][col] = false;
            bingoCells[row][col]->clear();  // 모든 내용 지우기
            bingoCells[row][col]->setContentsMargins(0, 0, 0, 0);  // 여백 초기화
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
}

// 리사이즈 이벤트 처리 (successLabel 크기 조정)
void BingoWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    
    // 성공 메시지 레이블 크기 조정
    if (successLabel) {
        successLabel->setGeometry(0, 0, width(), height());
    }
}

QPixmap BingoWidget::createXImage() {
    qDebug() << "Creating optimized pixelated X mark";
    
    // Create X image with transparent background
    QPixmap xImg(100, 100);
    xImg.fill(Qt::transparent);
    
    QPainter painter(&xImg);
    // Disable antialiasing for pixelated look
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    // Use white color
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(Qt::white));
    
    // Define pixel size for a cleaner look
    const int pixelSize = 5;
    
    // Define center point and size for a 2/3 cell size X
    const int center = 50; // Center of the 100x100 pixmap
    const int xSize = 33;  // Will make X extend ~33px from center (2/3 of size)
    
    // Draw first diagonal (top-left to bottom-right)
    for (int i = -xSize; i <= xSize; i++) {
        int x = center + i;
        int y = center + i;
        painter.drawRect(x, y, pixelSize, pixelSize);
    }
    
    // Draw second diagonal (top-right to bottom-left)
    for (int i = -xSize; i <= xSize; i++) {
        int x = center + i;
        int y = center - i;
        painter.drawRect(x, y, pixelSize, pixelSize);
    }
    
    // Thicken the diagonals by adding adjacent pixels
    for (int i = -xSize; i <= xSize; i++) {
        // Thicken first diagonal
        int x1 = center + i;
        int y1 = center + i;
        painter.drawRect(x1 - pixelSize, y1, pixelSize, pixelSize);
        
        // Thicken second diagonal
        int x2 = center + i;
        int y2 = center - i;
        painter.drawRect(x2 - pixelSize, y2, pixelSize, pixelSize);
    }
    
    qDebug() << "X mark created: properly centered, 2/3 cell size";
    return xImg;
}

// Add a separate color correction function
QColor BingoWidget::correctBluecast(const QColor &color) {
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


