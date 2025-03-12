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
//    bingoLayout = new QVBoxLayout(bingoArea);

    // 빙고 점수 표시 레이블
    bingoScoreLabel = new QLabel("Bingo: 0", bingoArea);
    bingoScoreLabel->setAlignment(Qt::AlignCenter);
    QFont scoreFont = bingoScoreLabel->font();
    scoreFont.setPointSize(12);
    scoreFont.setBold(true);
    bingoScoreLabel->setFont(scoreFont);
    bingoScoreLabel->setMinimumHeight(30);
    
    // 빙고판이 세로 중앙 정렬되도록 위쪽에 stretch 추가
    bingoVLayout->addStretch(1);
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
    
    // 빙고판이 세로 중앙 정렬되도록 아래쪽에 stretch 추가
    bingoVLayout->addStretch(1);

    backButton = new QPushButton("Back", this);
    bingoVLayout->addWidget(backButton, 0, Qt::AlignCenter);
    connect(backButton, &QPushButton::clicked, this, &BingoWidget::onBackButtonClicked);
    
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
    cameraLayout = new QVBoxLayout(cameraArea);
    
    // 카메라 뷰
    cameraView = new QLabel(cameraArea);
    cameraView->setMinimumSize(480, 360);
    cameraView->setFixedSize(480, 360);
    cameraView->setAlignment(Qt::AlignCenter);
    cameraView->setText("Camera connecting...");
    cameraView->setFrameShape(QFrame::Box);
    cameraView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    // 카메라 컨트롤
    QWidget *controlsWidget = new QWidget(cameraArea);
    QHBoxLayout *controlsLayout = new QHBoxLayout(controlsWidget);
    
    startButton = new QPushButton("Start", controlsWidget);
    stopButton = new QPushButton("Stop", controlsWidget);
    captureButton = new QPushButton("Capture", controlsWidget);
    stopButton->setEnabled(false);
    captureButton->setEnabled(false);
    
    QFont buttonFont = startButton->font();
    buttonFont.setPointSize(14);
    startButton->setFont(buttonFont);
    stopButton->setFont(buttonFont);
    captureButton->setFont(buttonFont);
    
    // 모든 버튼 높이를 통일
    startButton->setMinimumHeight(40);
    stopButton->setMinimumHeight(40);
    captureButton->setMinimumHeight(40);
    
    controlsLayout->addWidget(startButton);
    controlsLayout->addWidget(stopButton);
    controlsLayout->addWidget(captureButton);
    
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
                
                // 이전 선택된 셀의 스타일 원래대로
                if (selectedCell.first >= 0 && selectedCell.second >= 0) {
                    updateCellStyle(selectedCell.first, selectedCell.second);
                }
                
                // 새 셀 선택
                selectedCell = qMakePair(row, col);
                
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

QString BingoWidget::getCellColorName(int row, int col) {
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
    
    // 색조, 채도, 명도에 가중치 적용 (색조와 채도 가중치 증가)
    double hueWeight = 2.0;    // 색조 차이에 더 높은 가중치 (1.5 -> 2.0)
    double satWeight = 1.0;    // 채도 차이에 증가된 가중치 (0.8 -> 1.0)
    double valWeight = 0.5;    // 명도 차이에 감소된 가중치 (0.7 -> 0.5)
    
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
        
        // 추출된 RGB 값에 붉은 기 추가 (R 증가, G/B 감소)
        avgRed = qBound(0, (int)(avgRed * 1.15), 255);   // 빨간색 15% 증가
        avgGreen = qBound(0, (int)(avgGreen * 0.92), 255); // 녹색 8% 감소
        avgBlue = qBound(0, (int)(avgBlue * 0.92), 255);  // 파란색 8% 감소
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
    
    // 이미 O로 표시된 칸이면 무시 (추가 안전장치)
    if (bingoStatus[row][col]) {
        return;
    }
    
    // 카메라 색상과 선택된 셀 색상의 유사도 검사
    QColor cameraColor(avgRed, avgGreen, avgBlue);
    QColor cellColor = getCellColor(row, col);
    
    // 색상 거리 계산 (값이 작을수록 유사)
    int distance = colorDistance(cameraColor, cellColor);
    
    // 임계값 축소 (40 -> 30) - 더 엄격한 유사도 판정
    const int THRESHOLD = 30; // HSV 기반 거리가 30 이하면 유사하다고 판단
    
    qDebug() << "Color distance:" << distance << "(Threshold:" << THRESHOLD << ")";
    
    if (distance <= THRESHOLD) {
        // 색상이 유사하면 O 표시
        bingoStatus[row][col] = true;
        updateCellStyle(row, col);
        
        // 선택 상태 초기화
        selectedCell = qMakePair(-1, -1);
        
        // 빙고 점수 업데이트
        updateBingoScore();
    } else {
        // 경계선 스타일 생성
        QString borderStyle = "border-top: 1px solid black; border-left: 1px solid black;";
        
        if (row == 2) {
            borderStyle += " border-bottom: 1px solid black;";
        }
        if (col == 2) {
            borderStyle += " border-right: 1px solid black;";
        }
        
        // 배경색 스타일 설정
        QString style = QString("background-color: %1; %2")
                       .arg(cellColors[row][col].name())
                       .arg(borderStyle);
        
        bingoCells[row][col]->setStyleSheet(style);
        
        // 이미지 표시
        bingoCells[row][col]->clear();
        bingoCells[row][col]->setAlignment(Qt::AlignCenter);
        
        // X 이미지 여백도 동일하게 조정
        int margin = 10; // 여백 축소 (15→10)
        bingoCells[row][col]->setContentsMargins(margin, margin, margin, margin);
        
        // 이미지 크기도 더 크게 조정
        QPixmap xImage = createXImage();
        QPixmap scaledX = xImage.scaled(
            bingoCells[row][col]->width() - 20, // 여백 축소 (30→20)
            bingoCells[row][col]->height() - 20, // 여백 축소 (30→20)
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        bingoCells[row][col]->setPixmap(scaledX);
        
        // 1초 후 X를 지우는 타이머 시작
        fadeXTimer->start(1000);
    }
}

void BingoWidget::clearXMark() {
    // X 표시가 있는 셀이 있으면 원래대로 되돌리기
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            // pixmap이 설정되어 있고, 체크되지 않은 셀인 경우
            if (bingoCells[row][col]->pixmap() != nullptr && !bingoStatus[row][col]) {
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


void BingoWidget::onBackButtonClicked() {
    emit backToMainRequested();
}

QPixmap BingoWidget::createXImage() {
    QPixmap xImage(80, 80);
    xImage.fill(Qt::transparent);
    QPainter painter(&xImage);
    
    // 안티앨리어싱 비활성화 (픽셀 느낌을 위해)
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    // 흰색 픽셀로 X 그리기
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white); // 흰색으로 설정
    
    // 왼쪽 위에서 오른쪽 아래로 가는 대각선
    painter.drawRect(16, 16, 8, 8);
    painter.drawRect(24, 24, 8, 8);
    painter.drawRect(32, 32, 8, 8);
    painter.drawRect(40, 40, 8, 8);
    painter.drawRect(48, 48, 8, 8);
    painter.drawRect(56, 56, 8, 8);
    
    // 오른쪽 위에서 왼쪽 아래로 가는 대각선
    painter.drawRect(56, 16, 8, 8);
    painter.drawRect(48, 24, 8, 8);
    painter.drawRect(40, 32, 8, 8);
    painter.drawRect(32, 40, 8, 8);
    painter.drawRect(24, 48, 8, 8);
    painter.drawRect(16, 56, 8, 8);
    
    return xImage;
}
