#include "bingowidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QSizePolicy>

BingoWidget::BingoWidget(QWidget *parent) : QWidget(parent),
    isCapturing(false),
    showCircle(true),
    circleRadius(15),
    avgRed(0),
    avgGreen(0),
    avgBlue(0),
    showRgbValues(true),
    selectedCell(-1, -1),
    bingoCount(0)
{
    // 메인 레이아웃 생성 (가로 분할)
    mainLayout = new QHBoxLayout(this);
    
    // 왼쪽 부분: 빙고 영역
    bingoArea = new QWidget(this);
    QVBoxLayout* bingoVLayout = new QVBoxLayout(bingoArea);
    
    // 빙고 점수 표시 레이블
    bingoScoreLabel = new QLabel("빙고: 0줄", bingoArea);
    bingoScoreLabel->setAlignment(Qt::AlignCenter);
    QFont scoreFont = bingoScoreLabel->font();
    scoreFont.setPointSize(14);
    scoreFont.setBold(true);
    bingoScoreLabel->setFont(scoreFont);
    bingoScoreLabel->setMinimumHeight(40);
    
    bingoVLayout->addWidget(bingoScoreLabel);
    
    // 빙고 그리드 생성
    gridLayout = new QGridLayout();
    gridLayout->setSpacing(0); // 셀 간격 제거 - 표 형태로 보이게

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            bingoCells[row][col] = new QLabel(bingoArea);
            bingoCells[row][col]->setFixedSize(100, 100);
            bingoCells[row][col]->setAutoFillBackground(true);
            bingoCells[row][col]->setFrameShape(QFrame::Box); // 테두리 추가
            bingoCells[row][col]->setAlignment(Qt::AlignCenter);
            bingoCells[row][col]->setStyleSheet("border: 1px solid black;"); // 경계선 추가
            
            // 마우스 클릭 이벤트 활성화
            bingoCells[row][col]->installEventFilter(this);
            
            // 빙고 상태 초기화 (체크 안됨)
            bingoStatus[row][col] = false;
            
            gridLayout->addWidget(bingoCells[row][col], row, col);
        }
    }
    
    // 오른쪽 부분: 카메라 영역
    cameraArea = new QWidget(this);
    cameraLayout = new QVBoxLayout(cameraArea);
    
    // 카메라 뷰
    cameraView = new QLabel(cameraArea);
    cameraView->setMinimumSize(480, 360);
    cameraView->setFixedSize(480, 360);
    cameraView->setAlignment(Qt::AlignCenter);
    cameraView->setText("Camera connecting...");
    cameraView->setFrameShape(QFrame::Box);
    cameraView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    bingoVLayout->addLayout(gridLayout);
    
    // 카메라 컨트롤
    QWidget *controlsWidget = new QWidget(cameraArea);
    QHBoxLayout *controlsLayout = new QHBoxLayout(controlsWidget);
    
    startButton = new QPushButton("Start", controlsWidget);
    stopButton = new QPushButton("Stop", controlsWidget);
    captureButton = new QPushButton("Capture", controlsWidget);
    stopButton->setEnabled(false);
    captureButton->setEnabled(false); // 초기에는 비활성화
    
    QFont buttonFont = startButton->font();
    buttonFont.setPointSize(14);
    startButton->setFont(buttonFont);
    stopButton->setFont(buttonFont);
    startButton->setMinimumHeight(40);
    stopButton->setMinimumHeight(40);
    
    controlsLayout->addWidget(startButton);
    controlsLayout->addWidget(stopButton);
    controlsLayout->addWidget(captureButton);
    controlsWidget->setLayout(controlsLayout);
    
    // 원 설정 위젯
    QWidget *circleWidget = new QWidget(cameraArea);
    QVBoxLayout *circleLayout = new QVBoxLayout(circleWidget);
    
    // 체크박스
    circleCheckBox = new QCheckBox("Show Green Circle", circleWidget);
    circleCheckBox->setChecked(showCircle);
    
    // 슬라이더 및 값 표시
    QHBoxLayout *sliderLayout = new QHBoxLayout();
    QLabel *sliderLabel = new QLabel("Circle Size:", circleWidget);
    circleSlider = new QSlider(Qt::Horizontal, circleWidget);
    circleSlider->setMinimum(5);
    circleSlider->setMaximum(100);
    circleSlider->setValue(circleRadius);
    circleValueLabel = new QLabel(QString::number(circleRadius) + "%", circleWidget);
    circleValueLabel->setMinimumWidth(30);
    
    sliderLayout->addWidget(sliderLabel);
    sliderLayout->addWidget(circleSlider);
    sliderLayout->addWidget(circleValueLabel);
    
    // RGB 값 표시
    rgbCheckBox = new QCheckBox("Show RGB Average", circleWidget);
    rgbCheckBox->setChecked(showRgbValues);
    
    rgbValueLabel = new QLabel("R: 0  G: 0  B: 0", circleWidget);
    rgbValueLabel->setAlignment(Qt::AlignCenter);
    rgbValueLabel->setMinimumHeight(50);
    rgbValueLabel->setFrameShape(QFrame::Panel);
    rgbValueLabel->setFrameShadow(QFrame::Sunken);
    QFont labelFont = rgbValueLabel->font();
    labelFont.setBold(true);
    labelFont.setPointSize(14);
    rgbValueLabel->setFont(labelFont);
    
    // 원 설정 레이아웃 완성
    circleLayout->addWidget(circleCheckBox);
    circleLayout->addLayout(sliderLayout);
    circleLayout->addWidget(rgbCheckBox);
    circleLayout->addWidget(rgbValueLabel);
    circleWidget->setLayout(circleLayout);
    
    // 카메라 영역에 위젯 추가
    cameraLayout->addWidget(cameraView);
    cameraLayout->addWidget(controlsWidget);
    cameraLayout->addWidget(circleWidget);
    cameraLayout->addStretch();
    
    cameraArea->setLayout(cameraLayout);
    
    // 메인 레이아웃에 두 영역 추가
    mainLayout->addWidget(bingoArea);
    mainLayout->addWidget(cameraArea);
    
    // 분할 비율 고정 (왼쪽:오른쪽 = 1:1)
    mainLayout->setStretchFactor(bingoArea, 1);
    mainLayout->setStretchFactor(cameraArea, 1);
    
    // 각 영역이 고정된 너비를 유지하도록 설정
    bingoArea->setMinimumWidth(500);
    bingoArea->setMaximumWidth(500);
    cameraArea->setMinimumWidth(500);
    cameraArea->setMaximumWidth(500);
    
    setLayout(mainLayout);
    
    // X 표시 사라지는 타이머 초기화
    fadeXTimer = new QTimer(this);
    fadeXTimer->setSingleShot(true);
    connect(fadeXTimer, &QTimer::timeout, this, &BingoWidget::clearXMark);
    
    // 빙고 셀에 낮은 채도의 랜덤 색상 생성
    generateRandomColors();
    
    // 카메라 초기화
    camera = new V4L2Camera(this);
    
    // 카메라 신호 연결
    connect(camera, &V4L2Camera::newFrameAvailable, this, &BingoWidget::updateCameraFrame);
    connect(camera, &V4L2Camera::deviceDisconnected, this, &BingoWidget::handleCameraDisconnect);
    
    // 위젯 컨트롤 신호 연결
    connect(circleSlider, &QSlider::valueChanged, this, &BingoWidget::onCircleSliderValueChanged);
    connect(circleCheckBox, &QCheckBox::toggled, this, &BingoWidget::onCircleCheckBoxToggled);
    connect(rgbCheckBox, &QCheckBox::toggled, this, &BingoWidget::onRgbCheckBoxToggled);
    connect(startButton, &QPushButton::clicked, this, &BingoWidget::onStartButtonClicked);
    connect(stopButton, &QPushButton::clicked, this, &BingoWidget::onStopButtonClicked);
    connect(captureButton, &QPushButton::clicked, this, &BingoWidget::onCaptureButtonClicked);
    
    // 카메라 재시작 타이머 추가
    cameraRestartTimer = new QTimer(this);
    cameraRestartTimer->setInterval(30 * 60 * 1000); // 30분마다 재시작
    connect(cameraRestartTimer, &QTimer::timeout, this, &BingoWidget::restartCamera);
    
    // 카메라 열기 및 시작 시도
    if (camera->openCamera("/dev/video4")) {
        cameraView->setText("Click 'Start' button to start the camera");
    } else {
        cameraView->setText("Camera connection failed");
        startButton->setEnabled(false);
    }

    // 체크박스 폰트 크기 증가
    QFont checkboxFont = circleCheckBox->font();
    checkboxFont.setPointSize(12);
    circleCheckBox->setFont(checkboxFont);
    rgbCheckBox->setFont(checkboxFont);

    // 슬라이더 설정
    circleSlider->setMinimumHeight(30);
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
                // 이전 선택된 셀의 스타일 원래대로
                if (selectedCell.first >= 0 && selectedCell.second >= 0) {
                    updateCellStyle(selectedCell.first, selectedCell.second);
                }
                
                // 새 셀 선택
                selectedCell = qMakePair(row, col);
                
                // 선택된 셀 강조 표시
                QString style = "border: 3px solid red; background-color: " + 
                                getCellColor(row, col).name() + ";";
                bingoCells[row][col]->setStyleSheet(style);
                
                // 카메라가 켜져있고 셀이 선택되었을 때만 캡처 버튼 활성화
                captureButton->setEnabled(isCapturing);
                
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
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
            
            // 배경색 적용
            updateCellStyle(row, col);
            
            // 셀 위치 표시
            bingoCells[row][col]->setText(QString("Cell %1,%2").arg(row+1).arg(col+1));
            bingoCells[row][col]->setStyleSheet(
                QString("background-color: %1; color: %2; border: 1px solid black;")
                .arg(cellColor.name())
                .arg(isColorBright(cellColor) ? "black" : "white")
            );
        }
    }
}

void BingoWidget::updateCellStyle(int row, int col) {
    QString style;
    
    if (bingoStatus[row][col]) {
        // O 표시가 있는 경우
        style = QString("background-color: %1; color: %2; border: 1px solid black; "
                      "font-size: 60px; font-weight: bold;")
                .arg(cellColors[row][col].name())
                .arg(isColorBright(cellColors[row][col]) ? "black" : "white");
        bingoCells[row][col]->setText("O");
    } else {
        // 기본 색상 (셀 위치 표시)
        style = QString("background-color: %1; color: %2; border: 1px solid black;")
                .arg(cellColors[row][col].name())
                .arg(isColorBright(cellColors[row][col]) ? "black" : "white");
        bingoCells[row][col]->setText(getCellColorName(row, col));
    }
    
    bingoCells[row][col]->setStyleSheet(style);
}

QColor BingoWidget::getCellColor(int row, int col) {
    return cellColors[row][col];
}

QString BingoWidget::getCellColorName(int row, int col) {
    return QString("Cell %1,%2").arg(row+1).arg(col+1);
}

int BingoWidget::colorDistance(const QColor &c1, const QColor &c2) {
    // 색상 간의 유클리드 거리 계산
    int rdiff = c1.red() - c2.red();
    int gdiff = c1.green() - c2.green();
    int bdiff = c1.blue() - c2.blue();
    
    return rdiff*rdiff + gdiff*gdiff + bdiff*bdiff;
}

bool BingoWidget::isColorBright(const QColor &color) {
    // YIQ 공식으로 색상의 밝기 확인 (텍스트 색상 결정용)
    return ((color.red() * 299) + (color.green() * 587) + (color.blue() * 114)) / 1000 > 128;
}

void BingoWidget::updateCameraFrame() {
    // 카메라에서 새 프레임을 가져와 QLabel에 표시
    QImage frame = camera->getCurrentFrame();
    
    if (!frame.isNull()) {
        // 영상 크기에 맞게 스케일링하되 QLabel 크기는 유지
        QPixmap scaledPixmap = QPixmap::fromImage(frame).scaled(
            cameraView->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
            
        // 중앙에 녹색 원 그리기
        if (showCircle) {
            // 픽셀맵에 그리기 위한 QPainter 생성
            QPainter painter(&scaledPixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            
            // 원 중심 좌표 계산 (이미지 중앙)
            int centerX = scaledPixmap.width() / 2;
            int centerY = scaledPixmap.height() / 2;
            
            // 원의 반지름 계산 (이미지 너비의 percentage)
            int radius = (scaledPixmap.width() * circleRadius) / 100;
            
            // 녹색 반투명 펜 설정
            QPen pen(QColor(0, 255, 0, 180));
            pen.setWidth(2);
            painter.setPen(pen);
            
            // 원 그리기
            painter.drawEllipse(QPoint(centerX, centerY), radius, radius);
            
            // 원 영역 내부 RGB 평균 계산
            calculateAverageRGB(frame, frame.width()/2, frame.height()/2, 
                               (frame.width() * circleRadius) / 100);
                               
            // RGB 값 업데이트
            if (showRgbValues) {
                rgbValueLabel->setText(QString("R: %1  G: %2  B: %3").arg(avgRed).arg(avgGreen).arg(avgBlue));
                
                // 배경색 설정 (평균 RGB 값)
                QPalette palette = rgbValueLabel->palette();
                palette.setColor(QPalette::Window, QColor(avgRed, avgGreen, avgBlue));
                palette.setColor(QPalette::WindowText, (avgRed + avgGreen + avgBlue > 380) ? Qt::black : Qt::white);
                rgbValueLabel->setPalette(palette);
                rgbValueLabel->setAutoFillBackground(true);
            }
        }
        
        // QLabel에 이미지 표시 (크기 변경 없이)
        cameraView->setPixmap(scaledPixmap);
    }
}

void BingoWidget::calculateAverageRGB(const QImage &image, int centerX, int centerY, int radius) {
    long long sumR = 0, sumG = 0, sumB = 0;
    int pixelCount = 0;
    
    // 원 영역을 효율적으로 스캔하기 위해 원을 포함하는 사각형 영역만 처리
    int startX = qMax(centerX - radius, 0);
    int startY = qMax(centerY - radius, 0);
    int endX = qMin(centerX + radius, image.width() - 1);
    int endY = qMin(centerY + radius, image.height() - 1);
    
    // 사각형 내부의 각 픽셀에 대해
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            // 픽셀이 원 안에 있는지 확인 (피타고라스 정리 사용)
            int distX = x - centerX;
            int distY = y - centerY;
            int distSquared = distX * distX + distY * distY;
            
            if (distSquared <= radius * radius) {
                // 원 안에 있는 픽셀의 RGB 값 추출 및 합산
                QRgb pixel = image.pixel(x, y);
                sumR += qRed(pixel);
                sumG += qGreen(pixel);
                sumB += qBlue(pixel);
                pixelCount++;
            }
        }
    }
    
    // 픽셀이 없는 경우 예외 처리
    if (pixelCount > 0) {
        // 평균값 계산
        avgRed = sumR / pixelCount;
        avgGreen = sumG / pixelCount;
        avgBlue = sumB / pixelCount;
    } else {
        avgRed = avgGreen = avgBlue = 0;
    }
}

void BingoWidget::handleCameraDisconnect() {
    cameraView->setText("Camera disconnected");
    qDebug() << "Camera disconnected";
    
    // 버튼 상태 업데이트
    startButton->setEnabled(false);
    stopButton->setEnabled(false);
    captureButton->setEnabled(false);
    isCapturing = false;
}

void BingoWidget::onCircleSliderValueChanged(int value) {
    // 원 크기 값 업데이트
    circleRadius = value;
    circleValueLabel->setText(QString::number(value) + "%");
}

void BingoWidget::onCircleCheckBoxToggled(bool checked) {
    // 원 표시 여부 설정
    showCircle = checked;
    circleSlider->setEnabled(checked);
    
    // RGB 값 표시 관련 UI 요소 활성화/비활성화
    if (checked) {
        rgbCheckBox->setEnabled(true);
        rgbValueLabel->setEnabled(showRgbValues);
    } else {
        rgbCheckBox->setEnabled(false);
        rgbValueLabel->setEnabled(false);
    }
}

void BingoWidget::onRgbCheckBoxToggled(bool checked) {
    showRgbValues = checked;
    rgbValueLabel->setEnabled(checked);
}

void BingoWidget::restartCamera()
{
    if (isCapturing) {
        qDebug() << "정기적인 카메라 재초기화 수행...";
        // 카메라 중지
        camera->stopCapturing();
        
        // 잠시 대기
        QThread::msleep(500);
        
        // 카메라 재시작
        if (!camera->startCapturing()) {
            QMessageBox::warning(this, "Warning", "Camera restart failed. Will try again later.");
            return;
        }
        
        qDebug() << "카메라 재초기화 완료!";
    }
}

void BingoWidget::onStartButtonClicked() {
    // 카메라 캡처 시작
    if (!isCapturing) {
        if (!camera->startCapturing()) {
            QMessageBox::critical(this, "Error", "Failed to start capture");
            return;
        }
        
        // 버튼 상태 업데이트
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
        
        // 셀이 선택되어 있으면 캡처 버튼 활성화
        if (selectedCell.first >= 0)
            captureButton->setEnabled(true);
        
        // 플래그 설정
        isCapturing = true;
        
        // 재시작 타이머 시작
        cameraRestartTimer->start();
    }
}

void BingoWidget::onStopButtonClicked() {
    // 카메라 캡처 중지
    if (isCapturing) {
        // 카메라 캡처 중지
        camera->stopCapturing();
        
        // 버튼 상태 업데이트
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
        captureButton->setEnabled(false);
        
        // 플래그 설정
        isCapturing = false;
        
        // 재시작 타이머 중지
        cameraRestartTimer->stop();
        
        // 마지막 프레임 유지
        cameraView->setText("Click 'Start' button to start the camera");
    }
}

void BingoWidget::onCaptureButtonClicked() {
    // 선택된 셀이 없으면 무시
    if (selectedCell.first < 0 || selectedCell.second < 0 || !isCapturing) {
        return;
    }
    
    int row = selectedCell.first;
    int col = selectedCell.second;
    
    // 카메라 색상과 선택된 셀 색상의 유사도 검사
    QColor cameraColor(avgRed, avgGreen, avgBlue);
    QColor cellColor = getCellColor(row, col);
    
    // 색상 거리 계산 (값이 작을수록 유사)
    int distance = colorDistance(cameraColor, cellColor);
    
    // 유사도 임계값 (여유있게 설정)
    const int THRESHOLD = 20000; // RGB 거리 제곱의 합이 이 값보다 작으면 유사하다고 판단
    
    if (distance < THRESHOLD) {
        // 색상이 유사하면 O 표시
        bingoStatus[row][col] = true;
        updateCellStyle(row, col);
        
        // 빙고 점수 업데이트
        updateBingoScore();
    } else {
        // 색상이 다르면 X 표시 (1초 후 사라짐)
        QString style = QString("background-color: %1; color: red; border: 1px solid black; "
                              "font-size: 60px; font-weight: bold;")
                          .arg(cellColors[row][col].name());
        bingoCells[row][col]->setStyleSheet(style);
        bingoCells[row][col]->setText("X");
        
        // 1초 후 X를 지우는 타이머 시작
        fadeXTimer->start(1000);
    }
}

void BingoWidget::clearXMark() {
    // X 표시가 있는 셀이 있으면 원래대로 되돌리기
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (bingoCells[row][col]->text() == "X") {
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
    
    // 빙고 점수 표시 업데이트
    bingoScoreLabel->setText(QString("빙고: %1줄").arg(bingoCount));
    
    // 빙고 완성시 축하 메시지
    if (bingoCount > 0) {
        // 빙고 완성 효과 (배경색 변경 등)
        bingoScoreLabel->setStyleSheet("color: red; font-weight: bold;");
    } else {
        bingoScoreLabel->setStyleSheet("");
    }
}
