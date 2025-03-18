#include "bingowidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QSizePolicy>
#include <QThread>
#include <QFile>
#include <QSocketNotifier>
#include <linux/input.h>
#include <QShowEvent>
#include <QHideEvent>
#include "hardwareInterface/webcambutton.h"
#include "hardwareInterface/SoundManager.h"
#include "hardwareInterface/accelerometer.h"
#include <QSettings>
#include "../../utils/pixelartgenerator.h"

BingoWidget::BingoWidget(QWidget *parent, const QList<QColor> &initialColors) : QWidget(parent),
    isCapturing(false),
    showCircle(true),
    circleRadius(10),
    avgRed(0),
    avgGreen(0),
    avgBlue(0),
    showRgbValues(true),
    selectedCell(qMakePair(-1, -1)),
    bingoCount(0),
    remainingSeconds(120), // 2분 = 120초 타이머 초기화
    sliderWidget(nullptr),  // 추가된 부분
    cellRgbValueLabel(nullptr),  // 새 RGB 라벨 초기화
    accelerometer(nullptr),  // 가속도계 초기화
    submitButton(nullptr)
{
    qDebug() << "BingoWidget constructor started";
    
    // Initialize main variables (prevent null pointers)
    cameraView = nullptr;
    cameraRgbValueLabel = nullptr;  // 변수 이름 변경
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
    bingoVLayout->setSpacing(10); // 빙고 영역 내부 위젯 간격 일관성 설정

    // 빙고 점수 표시 레이블
    bingoScoreLabel = new QLabel("Bingo: 0", bingoArea);
    bingoScoreLabel->setAlignment(Qt::AlignCenter);
    QFont scoreFont = bingoScoreLabel->font();
    scoreFont.setPointSize(12);
    scoreFont.setBold(true);
    bingoScoreLabel->setFont(scoreFont);
    bingoScoreLabel->setMinimumHeight(30);

    // 빙고 셀 RGB 값 표시 레이블 추가
    cellRgbValueLabel = new QLabel("R: 0  G: 0  B: 0", bingoArea);
    cellRgbValueLabel->setFixedHeight(30);
    cellRgbValueLabel->setFixedWidth(300); // 카메라 라벨과 같은 너비
    cellRgbValueLabel->setAlignment(Qt::AlignCenter);
    QFont cellRgbFont = cellRgbValueLabel->font();
    cellRgbFont.setPointSize(12);
    cellRgbValueLabel->setFont(cellRgbFont);
    cellRgbValueLabel->setAutoFillBackground(true); // 배경색 설정 가능하도록
    
    // 모서리가 둥근 스타일 적용
    cellRgbValueLabel->setStyleSheet("background-color: black; color: white; border-radius: 15px; padding: 3px;");

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

    // 빙고 셀 RGB 값 라벨 추가
    bingoVLayout->addWidget(cellRgbValueLabel, 0, Qt::AlignCenter);
    
    // 초기 RGB 값 표시 확실하게 설정
    QColor initialColor(0, 0, 0);
    updateCellRgbLabel(initialColor);

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
    cameraVLayout->setAlignment(Qt::AlignCenter); // 중앙 정렬로 변경
    cameraVLayout->setContentsMargins(0, 0, 0, 0);
    cameraVLayout->setSpacing(10); // 카메라 영역 내부 위젯 간격 일관성 설정

    // Add stretch for vertical centering (top)
    cameraVLayout->addStretch(1);

    // 1. RGB 값 표시 레이블 (맨 위에 배치)
    cameraRgbValueLabel = new QLabel("R: 0  G: 0  B: 0");
    cameraRgbValueLabel->setFixedHeight(30);
    cameraRgbValueLabel->setFixedWidth(300); // Same width as camera
    cameraRgbValueLabel->setAlignment(Qt::AlignCenter);
    QFont rgbFont = cameraRgbValueLabel->font();
    rgbFont.setPointSize(12);
    cameraRgbValueLabel->setFont(rgbFont);
    cameraRgbValueLabel->setAutoFillBackground(true); // 배경색 설정 활성화
    
    // 모서리가 둥근 스타일 적용
    cameraRgbValueLabel->setStyleSheet("background-color: black; color: white; border-radius: 15px; padding: 3px;");
    
    cameraVLayout->addWidget(cameraRgbValueLabel, 0, Qt::AlignCenter);

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
    QFont sliderFont = circleLabel->font();
    sliderFont.setPointSize(11);
    circleLabel->setFont(sliderFont);
    circleSliderLayout->addWidget(circleLabel);
    
    circleSlider = new QSlider(Qt::Horizontal);
    circleSlider->setRange(5, 50); // 최소 5px, 최대 50px
    circleSlider->setValue(circleRadius);
    circleSlider->setFixedWidth(150);
    circleSliderLayout->addWidget(circleSlider);

    circleValueLabel = new QLabel(QString::number(circleRadius));
    circleValueLabel->setFont(sliderFont);
    circleValueLabel->setFixedWidth(30);
    circleValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    circleSliderLayout->addWidget(circleValueLabel);

    // 슬라이더 레이아웃을 카메라 레이아웃에 추가
    cameraVLayout->addWidget(sliderWidget, 0, Qt::AlignCenter);
    
    // 제출 버튼 생성
    submitButton = new QPushButton("Submit", cameraArea);
    submitButton->setFixedSize(100, 30);
    submitButton->setStyleSheet("QPushButton { background-color: #4a86e8; color: white; "
                               "border-radius: 6px; font-weight: bold; } "
                               "QPushButton:hover { background-color: #3a76d8; }");
    
    // 버튼들을 수평으로 담을 레이아웃 생성
    QHBoxLayout *tiltControlsHLayout = new QHBoxLayout();
    tiltControlsHLayout->setContentsMargins(10, 0, 10, 0);
    
    // 가운데 정렬을 위한 스트레치 추가
    tiltControlsHLayout->addStretch(1);
    
    // 버튼을 추가할 컨테이너
    QWidget *controlsContainer = new QWidget(cameraArea);
    QHBoxLayout *containerLayout = new QHBoxLayout(controlsContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    containerLayout->addWidget(submitButton);
    
    // 컨테이너 추가
    tiltControlsHLayout->addWidget(controlsContainer);
    
    // 가운데 정렬을 위한 스트레치 추가
    tiltControlsHLayout->addStretch(1);
    
    // 버튼들을 카메라 레이아웃에 직접 추가
    cameraVLayout->addLayout(tiltControlsHLayout);
    
    // 초기에는 두 버튼 모두 숨김
    submitButton->hide();
    
    // 하단에 stretch 추가하여 콘텐츠가 세로로 가운데 정렬되도록 함
    cameraVLayout->addStretch(1);
    
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
    
    // 빙고 셀에 낮은 채도의 랜덤 색상 생성 대신 initialColors 사용
    if (initialColors.size() >= 9) {
        setCustomColors(initialColors);
    } else {
        generateRandomColors();
    }
    
    // 카메라 초기화
    camera = new V4L2Camera(this);
    
    // 카메라 신호 연결
    connect(camera, &V4L2Camera::newFrameAvailable, this, &BingoWidget::updateCameraFrame);
    connect(camera, &V4L2Camera::deviceDisconnected, this, &BingoWidget::handleCameraDisconnect);
    
    // 위젯 컨트롤 신호 연결 - remove RGB checkbox connection
    connect(circleSlider, &QSlider::valueChanged, this, &BingoWidget::onCircleSliderValueChanged);
    
    // 버튼 신호 연결
    connect(submitButton, &QPushButton::clicked, this, &BingoWidget::onSubmitButtonClicked);
    
    // 카메라 재시작 타이머 설정 (유지)
    cameraRestartTimer = new QTimer(this);
    cameraRestartTimer->setInterval(30 * 60 * 1000); // 30분마다 재시작
    connect(cameraRestartTimer, &QTimer::timeout, this, &BingoWidget::restartCamera);
    
    // 카메라 열기만 하고 자동 시작은 하지 않음
    if (!camera->openCamera()) {
        cameraView->setText("Camera connection failed");
    }

    // 슬라이더 설정
    circleSlider->setMinimumHeight(30);

    // X 이미지 생성
    xImage = PixelArtGenerator::getInstance()->createXImage();

    // 곰돌이 이미지 생성
    bearImage = PixelArtGenerator::getInstance()->createBearImage();
    
    qDebug() << "Basic variable initialization completed";

    // Back 버튼 설정
    backButton = new QPushButton("Back", this);
    backButton->setFixedSize(120, 50); // 버튼 크기 증가 (80,30 -> 120,50)
    connect(backButton, &QPushButton::clicked, this, &BingoWidget::onBackButtonClicked);

    // Restart 버튼 추가
    restartButton = new QPushButton("Restart", this);
    restartButton->setFixedSize(120, 50); // 버튼 크기 증가 (80,30 -> 120,50)
    connect(restartButton, &QPushButton::clicked, this, &BingoWidget::onRestartButtonClicked);

    // 픽셀 스타일 적용
    QString backButtonStyle = PixelArtGenerator::getInstance()->createPixelButtonStyle(
        QColor(50, 50, 50, 200), // 어두운 회색
        2,   // 얇은 테두리
        12    // 타이머와 동일한 라운드 코너
    );
    
    QString restartButtonStyle = PixelArtGenerator::getInstance()->createPixelButtonStyle(
        QColor(70, 70, 70, 200), // 조금 더 밝은 회색
        2,    // 얇은 테두리
        12     // 타이머와 동일한 라운드 코너
    );
    
    // 버튼 폰트 크기 조정 (버튼 크기 증가에 맞게 폰트 크기도 증가)
    backButtonStyle.replace("font-size: 24px", "font-size: 18px");
    restartButtonStyle.replace("font-size: 24px", "font-size: 18px");
    
    // 패딩 조정 (더 큰 버튼에 맞게 패딩 증가)
    backButtonStyle.replace("padding: 15px 30px", "padding: 10px 20px");
    restartButtonStyle.replace("padding: 15px 30px", "padding: 10px 20px");
    
    backButton->setStyleSheet(backButtonStyle);
    restartButton->setStyleSheet(restartButtonStyle);

    // 초기 위치 설정
    updateBackButtonPosition();

    // 타이머 초기화 - 이 부분이 빠져 있었습니다
    gameTimer = new QTimer(this);
    gameTimer->setInterval(1000); // 1초마다 업데이트
    connect(gameTimer, &QTimer::timeout, this, &BingoWidget::onTimerTick);
    
    // 타이머 레이블 초기화
    timerLabel = new QLabel(this);
    timerLabel->setAlignment(Qt::AlignCenter);
    timerLabel->setFixedSize(QSize(140, 60)); // 크기 설정: 너비 140px, 음량 버튼과 동일한 높이 60px
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
    
    // 슬라이더 최적화 변수 초기화
    isSliderDragging = false;
    pendingCircleRadius = circleRadius;
    
    // 체크박스 디바운싱 변수 초기화
    isCheckboxProcessing = false;
    checkboxDebounceTimer = new QTimer(this);
    checkboxDebounceTimer->setSingleShot(true);
    
    // 슬라이더 업데이트 타이머 설정
    sliderUpdateTimer = new QTimer(this);
    sliderUpdateTimer->setSingleShot(true);
    connect(sliderUpdateTimer, &QTimer::timeout, this, [this]() {
        // 타이머가 끝나면 원 반지름 값을 업데이트하고 프레임 다시 그리기
        circleRadius = pendingCircleRadius;
        updateCameraFrame();
        isSliderDragging = false;
    });
    
    // 슬라이더의 슬롯 연결 - valueChanged 대신 두 개의 이벤트로 분리
    disconnect(circleSlider, &QSlider::valueChanged, this, &BingoWidget::onCircleSliderValueChanged);
    
    // 슬라이더가 움직이는 동안 빠른 업데이트를 위한 연결
    connect(circleSlider, &QSlider::sliderPressed, this, [this]() {
        isSliderDragging = true;
    });
    
    // 슬라이더가 움직이는 동안의 미리보기 업데이트
    connect(circleSlider, &QSlider::valueChanged, this, [this](int value) {
        pendingCircleRadius = value;
        circleValueLabel->setText(QString::number(value));
        
        // 미리보기 업데이트 - 기존 카메라 프레임에 원만 다시 그림
        updateCirclePreview(value);
        
        // 타이머 재시작 (디바운싱)
        sliderUpdateTimer->start(150); // 150ms 후 실제 업데이트
    });
    
    // 슬라이더를 놓았을 때 최종 업데이트
    connect(circleSlider, &QSlider::sliderReleased, this, [this]() {
        // 즉시 업데이트
        circleRadius = pendingCircleRadius;
        sliderUpdateTimer->stop();
        updateCameraFrame();
        isSliderDragging = false;
    });
    
    // 웹캠 물리 버튼 초기화 코드 변경
    webcamButton = new WebcamButton(this);
    if (webcamButton->initialize()) {
        // 버튼 신호 연결 - 명시적 연결로 변경하고 디버그 메시지 추가
        qDebug() << "Connecting WebcamButton signal to BingoWidget slot...";
        bool connected = connect(webcamButton, &WebcamButton::captureButtonPressed, 
                       this, &BingoWidget::onCaptureButtonClicked,
                       Qt::QueuedConnection); // 큐드 연결 방식 사용
        if (connected) {
            qDebug() << "WebcamButton signal connected successfully!";
        } else {
            qDebug() << "Failed to connect WebcamButton signal!";
        }
    } else {
        qDebug() << "WebcamButton initialization failed";
    }
    
    // 가속도계 초기화
    initializeAccelerometer();

    // 보너스 메시지 레이블 초기화
    bonusMessageLabel = new QLabel(this);
    bonusMessageLabel->setAlignment(Qt::AlignCenter);
    bonusMessageLabel->hide(); // 초기에는 숨김

    // 보너스 메시지 타이머 초기화
    bonusMessageTimer = new QTimer(this);
    bonusMessageTimer->setSingleShot(true);
    connect(bonusMessageTimer, &QTimer::timeout, this, &BingoWidget::hideBonusMessage);

    // 보너스 추적 변수 초기화
    hadBonusInLastLine = false;
}

BingoWidget::~BingoWidget() {
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
    
    // 슬라이더 업데이트 타이머 정리
    if (sliderUpdateTimer) {
        sliderUpdateTimer->stop();
        delete sliderUpdateTimer;
    }
    
    // 체크박스 디바운싱 타이머 정리
    if (checkboxDebounceTimer) {
        checkboxDebounceTimer->stop();
        delete checkboxDebounceTimer;
    }
    
    if (camera) {
        camera->stopCapturing();
        camera->closeCamera();
    }
    
    if (gameTimer) {
        gameTimer->stop();
        delete gameTimer;
    }
    
    // 가속도계 정리
    stopAccelerometer();
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
    // 이미 선택된 셀이면 무시
    if (row == selectedCell.first && col == selectedCell.second)
        return;
        
    // 이미 완료된 셀이면 무시
    if (bingoStatus[row][col])
        return;
    
    // 틸트 모드 상태 완전 초기화
    qDebug() << "Selecting new cell, resetting tilt mode state";
    
    // 틸트 관련 상태 초기화
    capturedColor = QColor(); // 캡처된 색상 초기화
    tiltAdjustedColor = QColor(); // 틸트 조정된 색상 초기화
    
    // 틸트 관련 UI 초기화
    updateTiltColorDisplay(QColor(0, 0, 0)); // 틸트 디스플레이 초기화
    
    // 틸트 관련 버튼 숨김
    submitButton->hide();

    // 이전에 선택된 셀이 있으면 선택 해제
    if (selectedCell.first >= 0 && selectedCell.second >= 0) {
        if (!bingoStatus[selectedCell.first][selectedCell.second]) {
            updateCellStyle(selectedCell.first, selectedCell.second);
        }
    }
    
    // 선택된 셀 갱신
    selectedCell = qMakePair(row, col);
    
    // 선택된 셀 스타일 업데이트 - 선택 표시
    QColor cellColor = getCellColor(row, col);
    QString styleSheet = QString("background-color: %1; border: 3px solid red;")
                             .arg(cellColor.name());
    bingoCells[row][col]->setStyleSheet(styleSheet);
    
    // 선택된 셀의 색상을 RGB 라벨에 표시
    updateCellRgbLabel(cellColor);
    
    // 카메라 시작 (이미 시작 중인 경우에도 재시작)
    if (isCapturing) {
        stopCamera();
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
    
    // 보너스 칸인 경우 데이지 꽃 이미지를 유지
    if (isBonusCell[row][col] && !bingoStatus[row][col]) {
        QPixmap daisyImage = PixelArtGenerator::getInstance()->createDaisyFlowerImage(70);
        bingoCells[row][col]->setPixmap(daisyImage);
        bingoCells[row][col]->setAlignment(Qt::AlignCenter);
    }
    
    // 카메라 시작
    startCamera();
    
    // 셀이 선택되었으니 상태 메시지 업데이트
    if (isBonusCell[row][col]) {
        // 보너스 셀인 경우 추가 메시지 표시
        statusMessageLabel->setText(QString("This is the bonus cell. Bingo here gives +1 point!\n Press camera button to match colors").arg(row+1).arg(col+1));
    } else {
        // 일반 셀인 경우 기본 메시지 표시
        statusMessageLabel->setText(QString("Press camera button to match colors").arg(row+1).arg(col+1));
    }
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
        
        // RGB 값 라벨 초기화 (0,0,0으로 설정)
        if (cellRgbValueLabel) {
            QColor defaultColor(0, 0, 0);
            updateCellRgbLabel(defaultColor);
        }
        
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
void BingoWidget::generateRandomColors() {
    // 초기화: 모든 셀을 보너스 아님으로 설정
    for(int row = 0; row < 3; ++row) {
        for(int col = 0; col < 3; ++col) {
            isBonusCell[row][col] = false;
        }
    }
    
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

    // 색상 배치를 위한 셀 위치 랜덤화
    QList<QPair<int, int>> cellPositions;
    for(int row = 0; row < 3; ++row) {
        for(int col = 0; col < 3; ++col) {
            cellPositions.append(qMakePair(row, col));
        }
    }
    
    // 위치 섞기
    for(int i = 0; i < cellPositions.size(); ++i) {
        int j = QRandomGenerator::global()->bounded(cellPositions.size());
        cellPositions.swapItemsAt(i, j);
    }
    
    // 무작위로 2개의 위치를 보너스 칸으로 지정
    for (int i = 0; i < 2; ++i) {
        int row = cellPositions[i].first;
        int col = cellPositions[i].second;
        isBonusCell[row][col] = true;
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
            
            // 보너스 칸인 경우 시각적 표시 추가
            if (isBonusCell[row][col]) {
                bingoCells[row][col]->setText("B");
                bingoCells[row][col]->setAlignment(Qt::AlignTop | Qt::AlignRight);
                QFont font = bingoCells[row][col]->font();
                font.setBold(true);
                font.setPointSize(14);
                bingoCells[row][col]->setFont(font);
            } else {
                bingoCells[row][col]->setText("");
            }
            
            // 배경색 적용
            bingoCells[row][col]->setStyleSheet(
                QString("background-color: %1; %2")
                .arg(cellColors[row][col].name())
                .arg(borderStyle)
            );
        }
    }
}

QColor BingoWidget::getCellColor(int row, int col) {
    return cellColors[row][col];
}

QString BingoWidget::getCellColorName(int /* row */, int /* col */) {
    return "";
}

int BingoWidget::colorDistance(const QColor &c1, const QColor &c2) {
    // RGB 값 추출
    int r1 = c1.red();
    int g1 = c1.green();
    int b1 = c1.blue();
    
    int r2 = c2.red();
    int g2 = c2.green();
    int b2 = c2.blue();
    
    // 단순 RGB 유클리드 거리 계산 (가중치 없음)
    int rDiff = r1 - r2;
    int gDiff = g1 - g2;
    int bDiff = b1 - b2;
    
    // 유클리드 거리 계산: sqrt(rDiff² + gDiff² + bDiff²)
    double distance = sqrt(rDiff*rDiff + gDiff*gDiff + bDiff*bDiff);
    
    // 0-100 스케일로 변환 (색상 범위 255를 고려하여 스케일링)
    // sqrt(3 * 255²) = 약 441.67이 최대 거리
    return static_cast<int>(distance * 100 / 441.67);
}

bool BingoWidget::isColorBright(const QColor &color) {
    // YIQ 공식으로 색상의 밝기 확인 (텍스트 색상 결정용)
    return ((color.red() * 299) + (color.green() * 587) + (color.blue() * 114)) / 1000 > 128;
}

// 색상 보정 함수 추가
// QImage BingoWidget::adjustColorBalance(const QImage &image) {
//     // qDebug() << "Color balance adjustment started";
    
//     // Check if image is valid
//     if (image.isNull()) {
//         qDebug() << "ERROR: Input image is null, returning original";
//         return image;
//     }
    
//     // qDebug() << "Creating adjusted image copy, dimensions: " << image.width() << "x" << image.height();
    
//     try {
//         // Create new image with same size
//         QImage adjustedImage = image.copy();
        
//         // Color adjustment parameters
//         const double redReduction = 0.85;
//         const int blueGreenBoost = 5;
        
//         // qDebug() << "Processing image pixels with redReduction=" << redReduction << ", blueGreenBoost=" << blueGreenBoost;
        
//         // Process image pixels - sample every 2nd pixel for performance
//         for (int y = 0; y < adjustedImage.height(); y += 2) {
//             for (int x = 0; x < adjustedImage.width(); x += 2) {
//                 QColor pixel(adjustedImage.pixel(x, y));
                
//                 // Reduce red channel, increase blue and green channels
//                 int newRed = qBound(0, static_cast<int>(pixel.red() * redReduction), 255);
//                 int newGreen = qBound(0, pixel.green() + blueGreenBoost, 255);
//                 int newBlue = qBound(0, pixel.blue() + blueGreenBoost, 255);
                
//                 // Apply new color
//                 adjustedImage.setPixelColor(x, y, QColor(newRed, newGreen, newBlue));
//             }
//         }
        
//         // qDebug() << "Color balance adjustment completed successfully";
//         return adjustedImage;
//     }
//     catch (const std::exception& e) {
//         qDebug() << "ERROR: Exception in adjustColorBalance: " << e.what();
//         return image;  // Return original image on error
//     }
//     catch (...) {
//         qDebug() << "ERROR: Unknown exception in adjustColorBalance";
//         return image;  // Return original image on error
//     }
// }

// updateCameraFrame 함수에 색상 보정 적용
void BingoWidget::updateCameraFrame() {
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
    
    // 현재 프레임을 originalFrame에 저장
    originalFrame = frame;
    
    // 슬라이더 드래그 중이면 미리보기 업데이트만 수행하고 리턴
    if (isSliderDragging) {
        updateCirclePreview(pendingCircleRadius);
        return;
    }
    
    // Safety check: verify cameraView is valid
    if (!cameraView) {
        qDebug() << "ERROR: cameraView is null";
        return;
    }
    
    try {
        // Apply color balance correction - with safety check
        // QImage adjustedFrame;
        // try {
        //     adjustedFrame = adjustColorBalance(frame);
        //     if (adjustedFrame.isNull()) {
        //         qDebug() << "WARNING: Color adjustment returned null image, using original";
        //         adjustedFrame = frame;
        //     }
        // }
        // catch (...) {
        //     qDebug() << "ERROR: Exception during color adjustment, using original frame";
        //     adjustedFrame = frame;
        // }
        
        // Scale image to fit the QLabel while maintaining aspect ratio
        QPixmap scaledPixmap = QPixmap::fromImage(frame).scaled(
            cameraView->size(),
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation);
        
        // 미리보기 픽스맵 업데이트 (다음 슬라이더 변경시 사용)
        previewPixmap = scaledPixmap.copy();
        
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
        
        // Set pen color to match RGB values
        QColor circleColor(avgRed, avgGreen, avgBlue);
        QPen pen(circleColor);
        pen.setWidth(5);
        painter.setPen(pen);
            
        // Draw circle
        painter.drawEllipse(QPoint(centerX, centerY), radius, radius);
        painter.end();
        
        // Calculate RGB average inside circle area
        // if (adjustedFrame.width() > 0 && adjustedFrame.height() > 0) {
        //     // Calculate safe radius
        //     int safeRadius = qMin((adjustedFrame.width() * circleRadius) / 100, 
        //                        qMin(adjustedFrame.width()/2, adjustedFrame.height()/2));
                               
        //     calculateAverageRGB(adjustedFrame, adjustedFrame.width()/2, adjustedFrame.height()/2, safeRadius);
        // }

        if (frame.width() > 0 && frame.height() > 0) {
            // Calculate safe radius
            int safeRadius = qMin((frame.width() * circleRadius) / 100, 
                               qMin(frame.width()/2, frame.height()/2));
                               
            calculateAverageRGB(frame, frame.width()/2, frame.height()/2, safeRadius);
        }
        
        // Update RGB values (always, no need for conditional check)
        if (cameraRgbValueLabel) {
                cameraRgbValueLabel->setText(QString("R: %1  G: %2  B: %3").arg(avgRed).arg(avgGreen).arg(avgBlue));
                
            // 밝기에 따라 텍스트 색상 결정
            QString textColor = (avgRed + avgGreen + avgBlue > 380) ? "black" : "white";
            
            // 스타일시트 설정 (둥근 모서리 유지)
            cameraRgbValueLabel->setStyleSheet(QString("background-color: rgb(%1, %2, %3); color: %4; border-radius: 15px; padding: 3px;")
                                             .arg(avgRed).arg(avgGreen).arg(avgBlue).arg(textColor));
                                             
            // Circle 슬라이더 값 라벨 색상도 함께 업데이트
            if (circleValueLabel) {
                circleValueLabel->setStyleSheet(QString(
                    "background-color: rgb(%1, %2, %3);"
                    "color: %4;"
                    "border-radius: 10px;"
                    "padding: 3px 8px;"
                    "min-width: 30px;"
                    "font-weight: bold;"
                ).arg(avgRed).arg(avgGreen).arg(avgBlue).arg(textColor));
            }
            
            // 슬라이더 손잡이 색상 업데이트
            if (circleSlider) {
                // 슬라이더 핸들 색상을 현재 RGB 값으로 설정
                circleSlider->setStyleSheet(QString(
                    "QSlider::groove:horizontal {"
                    "    border: 1px solid #999999;"
                    "    height: 8px;"
                    "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #B1B1B1, stop:1 #c4c4c4);"
                    "    margin: 2px 0;"
                    "    border-radius: 4px;"
                    "}"
                    "QSlider::handle:horizontal {"
                    "    background: rgb(%1, %2, %3);"
                    "    border: 1px solid #5c5c5c;"
                    "    width: 18px;"
                    "    margin: -6px 0;"
                    "    border-radius: 9px;"
                    "}"
                    "QSlider::handle:horizontal:hover {"
                    "    background: rgb(%4, %5, %6);"
                    "    border: 1px solid #333333;"
                    "}"
                ).arg(avgRed).arg(avgGreen).arg(avgBlue)
                 .arg(qMin(avgRed + 20, 255)).arg(qMin(avgGreen + 20, 255)).arg(qMin(avgBlue + 20, 255)));
            }
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

void BingoWidget::calculateAverageRGB(const QImage &image, int centerX, int centerY, int radius) {
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
    // 더 이상 사용되지 않음 - 새 최적화된 구현으로 대체됨
    // 단, 호환성을 위해 함수는 유지
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

void BingoWidget::onCaptureButtonClicked() {
    qDebug() << "BingoWidget::onCaptureButtonClicked called!";
    
    if (!isCapturing || selectedCell.first < 0) {
        qDebug() << "Capture ignored: Camera not capturing or no cell selected";
        return;
    }
    
    int row = selectedCell.first;
    int col = selectedCell.second;
    
    if (bingoStatus[row][col])
        return;
    
    // 이전에 캡처된 색상이 있는지 확인
    bool isFreshCapture = !capturedColor.isValid();
    
    // 새로운 색상 캡처
    capturedColor = QColor(avgRed, avgGreen, avgBlue);
    
    qDebug() << "Capture button clicked - Captured color: " << capturedColor.red() << "," << capturedColor.green() << "," << capturedColor.blue();
    qDebug() << "Fresh capture: " << (isFreshCapture ? "Yes" : "No");
    
    // 카메라 중지
    if (isCapturing) {
        camera->stopCapturing();
        isCapturing = false;
        
        if (cameraRestartTimer) {
            cameraRestartTimer->stop();
        }
    }
    
    // 빙고 셀 색상과 비교
    QColor selectedColor = cellColors[row][col];
    int distance = colorDistance(selectedColor, capturedColor);
    
    qDebug() << "Initial color comparison - Distance: " << distance;
    qDebug() << "Selected cell color: " << selectedColor.red() << "," << selectedColor.green() << "," << selectedColor.blue();
    
    // 색상이 이미 유사하면 바로 성공 처리
    if (distance <= THRESHOLD) {
        qDebug() << "Immediate color match successful!";
        processColorMatch(capturedColor);
        
        // 상태 초기화
        capturedColor = QColor();
        tiltAdjustedColor = QColor();
        submitButton->hide();
    } else {
        // 색상이 다르면 틸트 조절 모드 활성화
        qDebug() << "Colors don't match initially. Activating tilt adjustment mode.";
        
        tiltAdjustedColor = capturedColor;
        updateTiltColorDisplay(capturedColor);
        
        // Submit 버튼을 표시하고 메시지 업데이트
        submitButton->show();
        
        statusMessageLabel->setText("Colors don't match! Tilt device to adjust color and submit");
    }
}

// 제출 버튼 클릭 처리 함수
void BingoWidget::onSubmitButtonClicked() {
    if (selectedCell.first < 0 || !capturedColor.isValid())
        return;
    
    int row = selectedCell.first;
    int col = selectedCell.second;
    
    // 빙고 셀 색상과 비교
    QColor selectedColor = cellColors[row][col];
    int distance = colorDistance(selectedColor, tiltAdjustedColor);
    
    qDebug() << "Tilt-adjusted color comparison - Distance: " << distance;
    qDebug() << "Selected cell color: " << selectedColor.red() << "," << selectedColor.green() << "," << selectedColor.blue();
    qDebug() << "Tilt-adjusted color: " << tiltAdjustedColor.red() << "," << tiltAdjustedColor.green() << "," << tiltAdjustedColor.blue();
    
    // 색상이 유사하면 성공 처리
    if (distance <= THRESHOLD) {
        qDebug() << "Tilt-adjusted color match successful!";
        
        // 틸트 조절된 색상으로 매칭 처리
        processColorMatch(tiltAdjustedColor);
        
        // Submit 버튼 숨김
        submitButton->hide();
        
        // 상태 초기화
        capturedColor = QColor();
        tiltAdjustedColor = QColor();
        
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
    } else {
        // 색상이 여전히 다르면 실패 메시지 및 재시도 버튼 표시
        qDebug() << "Tilt-adjusted color match failed!";
        
        // 실패 메시지 업데이트
        statusMessageLabel->setText("Colors still don't match! Try again or press Retry button");
    }
}

// 색상 매치 처리 함수 (기존 onCaptureButtonClicked 코드 일부 분리)
void BingoWidget::processColorMatch(const QColor &colorToMatch) {
    qDebug() << "processColorMatch: Function started, selectedCell:" << selectedCell.first << "," << selectedCell.second;
    
    if (selectedCell.first < 0) {
        qDebug() << "processColorMatch: No cell selected, function exit";
        return;
    }
    
    int row = selectedCell.first;
    int col = selectedCell.second;
    
    qDebug() << "processColorMatch: Selected cell coordinates: row=" << row << ", col=" << col;
    
    // 선택된 셀이 이미 빙고 상태이면 무시
    if (bingoStatus[row][col]) {
        qDebug() << "processColorMatch: Cell already in bingo state, function exit";
        return;
    }
    
    // RGB 값 확인
    QColor selectedColor = cellColors[row][col];
    
    // 색상 거리 계산
    int distance = colorDistance(selectedColor, colorToMatch);
    
    // 디버그 로그 추가
    qDebug() << "Color match processing - Color distance: " << distance;
    qDebug() << "Selected cell color: " << selectedColor.red() << "," << selectedColor.green() << "," << selectedColor.blue();
    qDebug() << "Matched color: " << colorToMatch.red() << "," << colorToMatch.green() << "," << colorToMatch.blue();

    // 색상 유사도에 따라 처리
    if (distance <= THRESHOLD) {  // 색상이 유사함 - 빙고 처리
        qDebug() << "Color match successful! Processing bingo";
        bingoStatus[row][col] = true;
        updateCellStyle(row, col);
        
        // 정답 소리 재생 추가
        SoundManager::getInstance()->playEffect(SoundManager::CORRECT_SOUND);
        
        // 선택 초기화
        selectedCell = qMakePair(-1, -1);
        
        // 빙고 점수 업데이트
        updateBingoScore();
        
        // 상태 메시지 업데이트
        statusMessageLabel->setText("Great job! Perfect color match!");
        
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
        
        // 셀 자체에 X 표시 그리기
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
        
        // 셀에 합성된 이미지 적용 - 보너스 셀이더라도 X만 표시하도록 수정
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
        
        // 실패 효과음 재생
        SoundManager::getInstance()->playEffect(SoundManager::INCORRECT_SOUND);
        
        // 기존 타이머가 돌고 있으면 중지하고 새로 시작
        if (fadeXTimer->isActive()) {
            fadeXTimer->stop();
        }
        fadeXTimer->start(2000);
    }

    capturedColor = QColor();
}

void BingoWidget::clearXMark() {
    // X 표시가 있는 셀이 있으면 원래대로 되돌리기
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            // deprecated 경고 수정: pixmap(Qt::ReturnByValue) 사용
            QPixmap cellPixmap = bingoCells[row][col]->pixmap(Qt::ReturnByValue);

            // pixmap이 설정되어 있고, 체크되지 않은 셀인 경우
            if (!cellPixmap.isNull() && !bingoStatus[row][col]) {
                // 원래 스타일로 복원 (보너스 칸인 경우 데이지 꽃 이미지 다시 표시)
                updateCellStyle(row, col);
            }
        }
    }
    
    // 상태 메시지 업데이트
    statusMessageLabel->setText("Try again or select another cell");
}

void BingoWidget::updateBingoScore() {
    int oldBingoCount = bingoCount;
    
    // 점수 계산 전에 보너스 칸 카운트 초기화
    countedBonusCells.clear();
    hadBonusInLastLine = false;
    
    bingoCount = 0;
    
    // 가로줄 확인
    for (int row = 0; row < 3; ++row) {
        if (bingoStatus[row][0] && bingoStatus[row][1] && bingoStatus[row][2]) {
            // 이 줄에 보너스 칸이 있는지 확인
            bool hasBonus = false;
            for (int col = 0; col < 3; ++col) {
                if (isBonusCell[row][col] && !countedBonusCells.contains(qMakePair(row, col))) {
                    hasBonus = true;
                    countedBonusCells.insert(qMakePair(row, col));
                    break; // 한 줄에 여러 보너스 칸이 있어도 한 번만 보너스 적용
                }
            }
            
            // 보너스 칸이 포함된 경우 2점, 아니면 1점
            bingoCount += (hasBonus ? 2 : 1);
            
            // 새로운 빙고가 생겼고, 보너스가 포함되어 있으면 표시
            if (bingoCount > oldBingoCount && hasBonus) {
                hadBonusInLastLine = true;
            }
        }
    }
    
    // 세로줄 확인
    for (int col = 0; col < 3; ++col) {
        if (bingoStatus[0][col] && bingoStatus[1][col] && bingoStatus[2][col]) {
            // 이 줄에 보너스 칸이 있는지 확인
            bool hasBonus = false;
            for (int row = 0; row < 3; ++row) {
                if (isBonusCell[row][col] && !countedBonusCells.contains(qMakePair(row, col))) {
                    hasBonus = true;
                    countedBonusCells.insert(qMakePair(row, col));
                    break; // 한 줄에 여러 보너스 칸이 있어도 한 번만 보너스 적용
                }
            }
            
            // 보너스 칸이 포함된 경우 2점, 아니면 1점
            bingoCount += (hasBonus ? 2 : 1);
            
            // 새로운 빙고가 생겼고, 보너스가 포함되어 있으면 표시
            if (bingoCount > oldBingoCount && hasBonus) {
                hadBonusInLastLine = true;
            }
        }
    }
    
    // 대각선 확인 (좌상단 -> 우하단)
    if (bingoStatus[0][0] && bingoStatus[1][1] && bingoStatus[2][2]) {
        // 이 줄에 보너스 칸이 있는지 확인
        bool hasBonus = false;
        for (int i = 0; i < 3; ++i) {
            if (isBonusCell[i][i] && !countedBonusCells.contains(qMakePair(i, i))) {
                hasBonus = true;
                countedBonusCells.insert(qMakePair(i, i));
                break; // 한 줄에 여러 보너스 칸이 있어도 한 번만 보너스 적용
            }
        }
        
        // 보너스 칸이 포함된 경우 2점, 아니면 1점
        bingoCount += (hasBonus ? 2 : 1);
        
        // 새로운 빙고가 생겼고, 보너스가 포함되어 있으면 표시
        if (bingoCount > oldBingoCount && hasBonus) {
            hadBonusInLastLine = true;
        }
    }
    
    // 대각선 확인 (우상단 -> 좌하단)
    if (bingoStatus[0][2] && bingoStatus[1][1] && bingoStatus[2][0]) {
        // 이 줄에 보너스 칸이 있는지 확인
        bool hasBonus = false;
        for (int i = 0; i < 3; ++i) {
            int row = i;
            int col = 2 - i;
            if (isBonusCell[row][col] && !countedBonusCells.contains(qMakePair(row, col))) {
                hasBonus = true;
                countedBonusCells.insert(qMakePair(row, col));
                break; // 한 줄에 여러 보너스 칸이 있어도 한 번만 보너스 적용
            }
        }
        
        // 보너스 칸이 포함된 경우 2점, 아니면 1점
        bingoCount += (hasBonus ? 2 : 1);
        
        // 새로운 빙고가 생겼고, 보너스가 포함되어 있으면 표시
        if (bingoCount > oldBingoCount && hasBonus) {
            hadBonusInLastLine = true;
        }
    }
    
    // 빙고 점수 표시 업데이트 (영어로 변경)
    bingoScoreLabel->setText(QString("Bingo: %1").arg(bingoCount));

    if (bingoCount > oldBingoCount) {
        // 빙고 완성시에는 효과음 없음
        qDebug() << "SUCCESS! (bingoCount: " << bingoCount << ")";
        
        // 보너스 칸이 포함된 빙고가 있으면 BONUS 메시지 표시
        if (hadBonusInLastLine) {
            qDebug() << "Bingo completed with bonus cell! Displaying BONUS message";
            showBonusMessage();
        }
    }
    
    // 빙고 완성시 축하 메시지
    if (bingoCount > 0) {
        // 빙고 완성 효과 (배경색 변경 등)
        bingoScoreLabel->setStyleSheet("color: red; font-weight: bold;");
    } else {
        bingoScoreLabel->setStyleSheet("");
    }

    // 3빙고 이상 달성 확인 - 디버그 로그 추가 및 가시성 향상
    qDebug() << "DEBUG: Current bingo count:" << bingoCount;
    if (bingoCount >= 3) {
        qDebug() << "DEBUG: Achieved 3 or more bingos! Displaying success message";
        // SUCCESS 메시지 표시
        showSuccessMessage();
    }
}

// 새로운 함수 추가: 성공 메시지 표시 및 게임 초기화
void BingoWidget::showSuccessMessage() {
    qDebug() << "DEBUG: showSuccessMessage function started";
    
    // 성공 메시지 레이블 초기화 및 표시
    successLabel->setVisible(true);
    successLabel->raise(); // 다른 위젯 위에 표시
    
    // 성공 메시지 레이블 크기 설정 (전체 위젯 크기로)
    successLabel->setGeometry(0, 0, width(), height());
    
    // 배경과 텍스트가 포함된 픽스맵 생성
    QPixmap combinedPixmap(width(), height());
    combinedPixmap.fill(QColor(0, 0, 0, 150)); // 반투명 검은색 배경
    
    // 트로피 이미지 가져오기
    QPixmap trophyPixelArt = PixelArtGenerator::getInstance()->createTrophyPixelArt();
    
    // 이미지와 텍스트를 합성할 페인터 생성
    QPainter painter(&combinedPixmap);
    
    // 텍스트 설정
    QFont font = painter.font();
    font.setPointSize(64);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    
    // 텍스트 크기 계산
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect("SUCCESS");
    
    // 이미지와 텍스트의 전체 너비 계산
    int totalWidth = trophyPixelArt.width() + textRect.width() + 30; // 30px의 간격 추가
    
    // 시작 x 위치 계산 (화면 중앙에 배치)
    int startX = (width() - totalWidth) / 2;
    int centerY = height() / 2;
    
    // 텍스트 그리기 (이미지 왼쪽에)
    painter.drawText(QRect(startX, centerY - 50, textRect.width(), 100), Qt::AlignVCenter, "SUCCESS");
    
    // 트로피 이미지 그리기 (텍스트 오른쪽에)
    int trophyX = startX + textRect.width() + 30; // 텍스트 오른쪽에 30px 간격 추가
    int trophyY = centerY - trophyPixelArt.height() / 2;
    painter.drawPixmap(trophyX, trophyY, trophyPixelArt);
    
    // 합성 완료
    painter.end();
    
    // 결과물을 레이블에 설정
    successLabel->setPixmap(combinedPixmap);
    qDebug() << "DEBUG: Trophy image and SUCCESS text setup completed";
    
    // 5초 후 메시지 숨기고 게임 초기화 (효과음이 완전히 재생될 때까지 대기)
    successTimer->start(5000);
    qDebug() << "DEBUG: Success timer started, message will disappear after 5 seconds";
    
    // 성공 효과음 재생
    SoundManager::getInstance()->playEffect(SoundManager::SUCCESS_SOUND);
    qDebug() << "DEBUG: Playing success sound effect";
}

// 새로운 함수 추가: 성공 메시지 숨기고 게임 초기화
void BingoWidget::hideSuccessAndReset() {
    successLabel->hide();
    
    // 타이머 중지
    if (gameTimer) {
        qDebug() << "DEBUG: BingoWidget - Stopping game timer in hideSuccessAndReset";
        gameTimer->stop();
    }
    
    // 게임 상태만 초기화 (타이머 재시작하지 않음)
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
    
    // 메인 화면으로 돌아가는 신호 발생
    emit backToMainRequested();
}

// 게임 초기화 함수 수정
void BingoWidget::resetGame() {
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
    
    // 색상 생성 코드 제거 - 기존 색상 유지
    // generateRandomColors(); <- 이 줄 제거 또는 주석 처리
    
    // 타이머 재시작
    startGameTimer();
}

// 리사이즈 이벤트 처리 (successLabel 크기 조정)
void BingoWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    
    // 성공 메시지 레이블 크기 조정
    if (successLabel) {
        successLabel->setGeometry(0, 0, width(), height());
    }
    
    // 실패 메시지 레이블 크기 조정
    if (failLabel) {
        failLabel->setGeometry(0, 0, width(), height());
    }
    
    // 보너스 메시지 레이블 크기 조정
    if (bonusMessageLabel) {
        bonusMessageLabel->setGeometry(0, 0, width(), height());
    }
    
    // 타이머 위치 업데이트
    updateTimerPosition();
    
    // Back 버튼 위치 업데이트
    updateBackButtonPosition();
}

void BingoWidget::onBackButtonClicked() {
    qDebug() << "DEBUG: BingoWidget - Back button clicked";
    
    // 카메라가 켜져 있다면 중지
    if (isCapturing) {
        qDebug() << "DEBUG: BingoWidget - Stopping camera before emitting back signal";
        stopCamera();
    }
    
    // 타이머 중지
    if (gameTimer) {
        qDebug() << "DEBUG: BingoWidget - Stopping game timer before emitting back signal";
        gameTimer->stop();
    }
    
    qDebug() << "DEBUG: BingoWidget - Emitting backToMainRequested signal";
    emit backToMainRequested();
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

void BingoWidget::updateBackButtonPosition() {
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

void BingoWidget::onRestartButtonClicked() {
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
void BingoWidget::onTimerTick() {
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
void BingoWidget::updateTimerDisplay() {
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
    
    // 타이머 위치 업데이트
    updateTimerPosition();
}

// 타이머 위치 업데이트
void BingoWidget::updateTimerPosition() {
    if (timerLabel) {
        // 타이머를 화면 중앙 상단에 배치, 상단 여백(20px)은 유지하면서 수평으로 중앙 정렬
        int margin = 20;
        timerLabel->move((width() - timerLabel->width()) / 2, margin);
        timerLabel->raise(); // 다른 위젯 위에 표시
    }
}

// 타이머 시작
void BingoWidget::startGameTimer() {
    // 타이머 초기화
    remainingSeconds = 120; // 2분
    updateTimerDisplay();
    
    // 타이머 시작
    gameTimer->start();
}

// 타이머 정지
void BingoWidget::stopGameTimer() {
    if (gameTimer) {
        gameTimer->stop();
    }
}

// 실패 메시지 표시
void BingoWidget::showFailMessage() {
    qDebug() << "DEBUG: showFailMessage function started";
    
    // 카메라가 실행 중이면 중지
    if (isCapturing) {
        stopCamera();
    }
    
    // 실패 메시지 레이블 초기화 및 표시
    failLabel->setVisible(true);
    failLabel->raise(); // 다른 위젯 위에 표시
    
    // 실패 메시지 레이블 크기와 위치 설정
    failLabel->setGeometry(0, 0, width(), height());
    failLabel->show();
    qDebug() << "DEBUG: Fail message label setup completed";
    
    // 배경과 텍스트가 포함된 픽스맵 생성
    QPixmap combinedPixmap(width(), height());
    combinedPixmap.fill(QColor(0, 0, 0, 150)); // 반투명 검은색 배경
    
    // 슬픈 얼굴 이미지 가져오기
    QPixmap sadFacePixelArt = PixelArtGenerator::getInstance()->createSadFacePixelArt();
    
    // 이미지와 텍스트를 합성할 페인터 생성
    QPainter painter(&combinedPixmap);
    
    // 텍스트 설정
    QFont font = painter.font();
    font.setPointSize(64);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::red);
    
    // 텍스트 크기 계산
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect("FAIL");
    
    // 이미지와 텍스트의 전체 너비 계산
    int totalWidth = sadFacePixelArt.width() + textRect.width() + 30; // 30px의 간격 추가
    
    // 시작 x 위치 계산 (화면 중앙에 배치)
    int startX = (width() - totalWidth) / 2;
    int centerY = height() / 2;
    
    // 텍스트 그리기 (이미지 왼쪽에)
    painter.drawText(QRect(startX, centerY - 50, textRect.width(), 100), Qt::AlignVCenter, "FAIL");
    
    // 슬픈 얼굴 이미지 그리기 (텍스트 오른쪽에)
    int imageX = startX + textRect.width() + 30; // 텍스트 오른쪽에 30px 간격 추가
    int imageY = centerY - sadFacePixelArt.height() / 2;
    painter.drawPixmap(imageX, imageY, sadFacePixelArt);
    
    // 합성 완료
    painter.end();
    
    // 결과물을 레이블에 설정
    failLabel->setPixmap(combinedPixmap);
    qDebug() << "DEBUG: Sad face image and FAIL text setup completed";
    
    // 5초 후 메시지 숨기고 메인 화면으로 돌아가기 (효과음이 완전히 재생될 때까지 대기)
    QTimer::singleShot(5000, this, &BingoWidget::hideFailAndReset);
    qDebug() << "DEBUG: Fail timer started, message will disappear after 5 seconds";
    
    // 실패 효과음 재생
    SoundManager::getInstance()->playEffect(SoundManager::FAIL_SOUND);
}

// 실패 메시지 숨기고 게임 초기화
void BingoWidget::hideFailAndReset() {
    failLabel->hide();
    
    // 카메라가 실행 중이면 중지
    if (isCapturing) {
        stopCamera();
    }
    
    // 타이머 중지
    if (gameTimer) {
        qDebug() << "DEBUG: BingoWidget - Stopping game timer in hideFailAndReset";
        gameTimer->stop();
    }
    
    // 게임 상태만 초기화 (타이머 재시작하지 않음)
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
    
    // 메인 화면으로 돌아가는 신호 발생
    emit backToMainRequested();
}

// 전달받은 색상을 셀에 설정하는 메서드
void BingoWidget::setCustomColors(const QList<QColor> &colors) {
    qDebug() << "Setting custom colors to bingo cells";
    
    // 초기화: 모든 셀을 보너스 아님으로 설정
    for(int row = 0; row < 3; ++row) {
        for(int col = 0; col < 3; ++col) {
            isBonusCell[row][col] = false;
        }
    }
    
    if (colors.size() < 7) {
        qDebug() << "Not enough colors provided. Expected at least 7, got" << colors.size();
        generateRandomColors(); // 충분한 색상이 없으면 전부 랜덤으로
        return;
    }
    
    // 카메라에서 가져온 색상 7개를 무작위로 배치
    QList<QColor> mixedColors = colors.mid(0, 7); // 처음 7개 색상을 사용
    
    // 2개의 랜덤 색상 생성
    QList<QColor> randomColors;
    for(int i = 0; i < 2; ++i) {
        // 완전 랜덤 색상 생성 (HSV 모델 사용)
        int hue = QRandomGenerator::global()->bounded(360); // 0-359 색조
        int saturation = QRandomGenerator::global()->bounded(180, 255); // 선명한 색상을 위해 180-255 채도
        int value = QRandomGenerator::global()->bounded(180, 255); // 밝은 색상을 위해 180-255 명도
        
        QColor randomColor = QColor::fromHsv(hue, saturation, value);
        randomColors.append(randomColor);
    }
    
    // 전체 색상 목록 준비 (7개 카메라 색상 + 2개 랜덤 색상 = 총 9개)
    QList<QColor> allColors = mixedColors + randomColors;
    
    // 보너스 칸이 아닌 위치를 랜덤화
    QList<QPair<int, int>> nonBonusCellPositions;
    for(int row = 0; row < 3; ++row) {
        for(int col = 0; col < 3; ++col) {
            nonBonusCellPositions.append(qMakePair(row, col));
            
            // 모든 칸을 일단 보너스 아님으로 초기화
            isBonusCell[row][col] = false;
        }
    }
    
    // 위치 섞기
    for(int i = 0; i < nonBonusCellPositions.size(); ++i) {
        int j = QRandomGenerator::global()->bounded(nonBonusCellPositions.size());
        nonBonusCellPositions.swapItemsAt(i, j);
    }
    
    // 섞인 색상 할당 및 보너스 셀 설정
    // 먼저 일반 색상 7개 배치 (7개 카메라 색상)
    for(int i = 0; i < 7; ++i) {
        int row = nonBonusCellPositions[i].first;
        int col = nonBonusCellPositions[i].second;
        cellColors[row][col] = allColors[i];
    }
    
    // 랜덤 색상 2개를 보너스 칸에 배치 (마지막 2개 위치에)
    for(int i = 7; i < 9; ++i) {
        int row = nonBonusCellPositions[i].first;
        int col = nonBonusCellPositions[i].second;
        cellColors[row][col] = allColors[i];
        isBonusCell[row][col] = true; // 랜덤 색상이 있는 칸을 보너스 칸으로 지정
    }
    
    // 모든 셀에 스타일 적용
    for(int row = 0; row < 3; ++row) {
        for(int col = 0; col < 3; ++col) {
            // 경계선 스타일 생성
            QString borderStyle = "border-top: 1px solid black; border-left: 1px solid black;";
            
            if (row == 2) {
                borderStyle += " border-bottom: 1px solid black;";
            }
            if (col == 2) {
                borderStyle += " border-right: 1px solid black;";
            }
            
            // 보너스 칸인 경우 데이지 꽃 이미지 표시
            if (isBonusCell[row][col]) {
                // 데이지 꽃 이미지 생성 - 크기 키움
                QPixmap daisyImage = PixelArtGenerator::getInstance()->createDaisyFlowerImage(70);
                
                // 디버깅: 이미지 생성 확인
                qDebug() << "Daisy image created - size:" << daisyImage.size() 
                         << "isNull:" << daisyImage.isNull() 
                         << "for cell:" << row << col;
                
                // 셀에 이미지 설정 (스타일시트 적용 전에 이미지 설정이 필요)
                bingoCells[row][col]->clear();
                bingoCells[row][col]->setPixmap(daisyImage);
                bingoCells[row][col]->setAlignment(Qt::AlignCenter); // 중앙 정렬로 변경
                bingoCells[row][col]->setScaledContents(false); // 이미지 크기 자동 조정 비활성화
                        
                // 배경색과 함께 이미지가 표시되도록 스타일시트 수정
                QString pixmapStyle = QString("background-color: %1; %2")
                                   .arg(cellColors[row][col].name())
                                   .arg(borderStyle);
                bingoCells[row][col]->setStyleSheet(pixmapStyle);
                
                // 이미지 설정 이후에도 보이지 않는 경우를 대비한 추가 설정
                bingoCells[row][col]->update();
            } else {
                bingoCells[row][col]->setText("");
                bingoCells[row][col]->setPixmap(QPixmap()); // 이미지 제거
                
                // 배경색 적용
                bingoCells[row][col]->setStyleSheet(
                    QString("background-color: %1; %2")
                    .arg(cellColors[row][col].name())
                    .arg(borderStyle)
                );
            }
        }
    }
}

// updateCellStyle 함수 수정: 셀이 선택되거나 X 표시가 나타날 때 데이지 꽃 이미지가 사라지도록 함
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
        
        // 보너스 칸인 경우 데이지 꽃 이미지 다시 표시
        if (isBonusCell[row][col]) {
            QPixmap daisyImage = PixelArtGenerator::getInstance()->createDaisyFlowerImage(70);
            
            // 디버깅: updateCellStyle에서 이미지 생성 확인
            qDebug() << "updateCellStyle: Daisy image created - size:" << daisyImage.size() 
                     << "isNull:" << daisyImage.isNull() 
                     << "for cell:" << row << col;
            
            bingoCells[row][col]->clear();
            bingoCells[row][col]->setPixmap(daisyImage);
            bingoCells[row][col]->setAlignment(Qt::AlignCenter);
            bingoCells[row][col]->setScaledContents(false); // 이미지 크기 자동 조정 비활성화
            
            // 배경색과 함께 이미지가 표시되도록 스타일시트 수정
            QString pixmapStyle = QString("background-color: %1; %2")
                               .arg(cellColors[row][col].name())
                               .arg(borderStyle);
            bingoCells[row][col]->setStyleSheet(pixmapStyle);
            
            // 이미지 설정 이후에도 보이지 않는 경우를 대비한 추가 설정
            bingoCells[row][col]->update();
        } else {
            // 보너스 셀이 아닌 경우 기본 스타일만 적용
            bingoCells[row][col]->setStyleSheet(style);
        }
    }
}

// 보너스 메시지 표시 함수
void BingoWidget::showBonusMessage() {
    // QPixmap을 사용하여 테두리가 있는 텍스트 생성
    QPixmap bonusPixmap(width(), height());
    bonusPixmap.fill(Qt::transparent);  // 배경을 투명하게 설정
    
    QPainter painter(&bonusPixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);  // 부드러운 텍스트를 위해
    
    // 텍스트 설정
    QFont bonusFont = painter.font();
    bonusFont.setPointSize(72);
    bonusFont.setBold(true);
    painter.setFont(bonusFont);
    
    // 텍스트 크기 계산
    QFontMetrics fm(bonusFont);
    QRect textRect = fm.boundingRect("BONUS!");
    
    // 텍스트 중앙 위치 계산
    int x = (width() - textRect.width()) / 2;
    int y = (height() - textRect.height()) / 2 + fm.ascent();
    
    // 테두리 그리기 (여러 방향으로 오프셋하여 검은색 테두리 효과 생성)
    painter.setPen(Qt::black);
    for (int offsetX = -3; offsetX <= 3; offsetX++) {
        for (int offsetY = -3; offsetY <= 3; offsetY++) {
            // 가장자리 부분만 그리기 (중앙은 건너뛰기)
            if (abs(offsetX) > 1 || abs(offsetY) > 1) {
                painter.drawText(x + offsetX, y + offsetY, "BONUS!");
            }
        }
    }
    
    // 실제 텍스트 그리기 (파란색으로 변경 - Single Game 버튼 배경색)
    painter.setPen(QColor(50, 120, 220));  // 파란색
    painter.drawText(x, y, "BONUS!");
    
    painter.end();
    
    // 메시지 레이블 크기 설정 (전체 위젯 크기로)
    bonusMessageLabel->setGeometry(0, 0, width(), height());
    bonusMessageLabel->setPixmap(bonusPixmap);  // 생성한 픽스맵 설정
    bonusMessageLabel->raise(); // 다른 위젯 위에 표시
    bonusMessageLabel->show();
    
    // 2초 후 메시지 숨기기 (1초에서 2초로 변경)
    bonusMessageTimer->start(2000);
}

// 보너스 메시지 숨기기 함수
void BingoWidget::hideBonusMessage() {
    bonusMessageLabel->hide();
}

// 원 오버레이 생성 함수 구현
void BingoWidget::createCircleOverlay(int width, int height) {
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
void BingoWidget::updateRgbValues() {
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
    cameraRgbValueLabel->setText(QString("R: %1  G: %2  B: %3").arg(avgRed).arg(avgGreen).arg(avgBlue));
    
    // 배경색 설정 (평균 RGB 값)
    QPalette palette = cameraRgbValueLabel->palette();
    palette.setColor(QPalette::Window, QColor(avgRed, avgGreen, avgBlue));
    palette.setColor(QPalette::WindowText, (avgRed + avgGreen + avgBlue > 380) ? Qt::black : Qt::white);
    cameraRgbValueLabel->setPalette(palette);
    cameraRgbValueLabel->setAutoFillBackground(true);
}

void BingoWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    qDebug() << "DEBUG: BingoWidget showEvent triggered";
}

void BingoWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    qDebug() << "DEBUG: BingoWidget hideEvent triggered";

    // 카메라 중지 및 닫기
    if(camera) {
        stopCamera();
        camera->closeCamera();
    }
    
    // 타이머 중지
    if (gameTimer) {
        qDebug() << "DEBUG: BingoWidget - Stopping game timer in hideEvent";
        gameTimer->stop();
    }
}

// 선택된 셀의 RGB 값 업데이트 함수 추가
void BingoWidget::updateCellRgbLabel(const QColor &color) {
    if (cellRgbValueLabel) {
        int r = color.red();
        int g = color.green();
        int b = color.blue();
        
        cellRgbValueLabel->setText(QString("R: %1  G: %2  B: %3").arg(r).arg(g).arg(b));
        
        // 밝기에 따라 텍스트 색상 결정
        QString textColor = (r + g + b > 380) ? "black" : "white";
        
        // 스타일시트 설정 (둥근 모서리 유지)
        cellRgbValueLabel->setStyleSheet(QString("background-color: rgb(%1, %2, %3); color: %4; border-radius: 15px; padding: 3px;")
                                       .arg(r).arg(g).arg(b).arg(textColor));
    }
}

// 가속도계 초기화 함수 구현
void BingoWidget::initializeAccelerometer() {
    // 이미 초기화된 경우 정리
    if (accelerometer) {
        stopAccelerometer();
    }
    
    // 새 가속도계 객체 생성
    accelerometer = new Accelerometer(this);
    
    // 가속도계 초기화
    if (!accelerometer->initialize()) {
        qDebug() << "Accelerometer initialization failed";
        delete accelerometer;
        accelerometer = nullptr;
        return;
    }
    
    // 가속도계 데이터 변경 신호 연결
    connect(accelerometer, &Accelerometer::accelerometerDataChanged,
            this, &BingoWidget::handleAccelerometerDataChanged);
    
    qDebug() << "Accelerometer initialized successfully";
}

// 가속도계 정리 함수 구현
void BingoWidget::stopAccelerometer() {
    if (accelerometer) {
        // 가속도계 신호 연결 해제
        disconnect(accelerometer, &Accelerometer::accelerometerDataChanged,
                  this, &BingoWidget::handleAccelerometerDataChanged);
        
        // 메모리 해제
        delete accelerometer;
        accelerometer = nullptr;
    }
}

// 가속도계 데이터 변경 처리 함수
void BingoWidget::handleAccelerometerDataChanged(const AccelerometerData &data) {
    // 틸트 모드가 활성화되어 있고 캡처된 색상이 있을 때만 처리
    if (!capturedColor.isValid()) {
        return;
    }
    
    // 캡처된 색상이 있으면 틸트 값에 따라 색상 조정
    // 색상 조정
    tiltAdjustedColor = adjustColorByTilt(capturedColor, data);
    
    // 조정된 색상을 카메라 뷰에 표시
    updateTiltColorDisplay(tiltAdjustedColor);
}

// 틸트 색상 표시 업데이트 함수
void BingoWidget::updateTiltColorDisplay(const QColor &color) {
    if (cameraView) {
        // 카메라 중지 상태를 저장
        bool wasCameraStopped = !isCapturing;
        
        // 카메라 뷰의 크기에 맞는 픽스맵 생성
        QPixmap colorPixmap(cameraView->width(), cameraView->height());
        colorPixmap.fill(color);
        
        // 가운데에 RGB 값 표시
        QPainter painter(&colorPixmap);
        
        // 밝기에 따라 텍스트 색상 결정
        bool isBright = isColorBright(color);
        QColor textColor = isBright ? Qt::black : Qt::white;
        
        // 큰 폰트로 RGB 값 표시
        QFont font = painter.font();
        font.setPointSize(16);
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(textColor);
        
        QString rgbText = QString("R: %1  G: %2  B: %3\n\nTilt to adjust color").arg(color.red()).arg(color.green()).arg(color.blue());
        
        // 텍스트를 가운데에 그림
        painter.drawText(colorPixmap.rect(), Qt::AlignCenter, rgbText);
        
        // 카메라 RGB 값 레이블도 업데이트
        if (cameraRgbValueLabel) {
            cameraRgbValueLabel->setText(QString("R: %1  G: %2  B: %3").arg(color.red()).arg(color.green()).arg(color.blue()));
            
            // 밝기에 따라 텍스트 색상 결정
            QString textColor = (color.red() + color.green() + color.blue() > 380) ? "black" : "white";
        
        // 스타일시트 설정 (둥근 모서리 유지)
            cameraRgbValueLabel->setStyleSheet(QString("background-color: rgb(%1, %2, %3); color: %4; border-radius: 15px; padding: 3px;")
                                             .arg(color.red()).arg(color.green()).arg(color.blue()).arg(textColor));
        }
        
        // 카메라 뷰에 픽스맵 설정
        cameraView->setPixmap(colorPixmap);
        
        // 슬라이더 핸들 색상도 업데이트
        if (circleSlider) {
            // 슬라이더 핸들 색상을 현재 RGB 값으로 설정
            circleSlider->setStyleSheet(QString(
                "QSlider::groove:horizontal {"
                "    border: 1px solid #999999;"
                "    height: 8px;"
                "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #B1B1B1, stop:1 #c4c4c4);"
                "    margin: 2px 0;"
                "    border-radius: 4px;"
                "}"
                "QSlider::handle:horizontal {"
                "    background: rgb(%1, %2, %3);"
                "    border: 1px solid #5c5c5c;"
                "    width: 18px;"
                "    margin: -6px 0;"
                "    border-radius: 9px;"
                "}"
                "QSlider::handle:horizontal:hover {"
                "    background: rgb(%4, %5, %6);"
                "    border: 1px solid #333333;"
                "}"
            ).arg(color.red()).arg(color.green()).arg(color.blue())
             .arg(qMin(color.red() + 20, 255)).arg(qMin(color.green() + 20, 255)).arg(qMin(color.blue() + 20, 255)));
        }
        
        // Circle 슬라이더 값 라벨 색상도 함께 업데이트
        if (circleValueLabel) {
            circleValueLabel->setStyleSheet(QString(
                "background-color: rgb(%1, %2, %3);"
                "color: %4;"
                "border-radius: 10px;"
                "padding: 3px 8px;"
                "min-width: 30px;"
                "font-weight: bold;"
            ).arg(color.red()).arg(color.green()).arg(color.blue()).arg(isBright ? "black" : "white"));
        }
    }
}

// 색상을 기울기에 따라 조정하는 함수
QColor BingoWidget::adjustColorByTilt(const QColor &color, const AccelerometerData &tiltData) {
    // HSV 색상 모델로 변환
    int h, s, v;
    color.getHsv(&h, &s, &v);
    
    // X축 기울기에 따라 채도(Saturation) 조정 (좌우 기울기)
    // 일반적으로 가속도계는 기기가 평평할 때 0에 가까운 값, 기울일 때 양수 또는 음수
    int tiltX = tiltData.x;
    
    // x축 틸트에 따른 채도 조정 계수 (범위를 조정하기 위한 매핑)
    // 기울기 범위(-2000~2000)를 채도 조정 범위로 매핑 - 계수 증가
    double saturationAdjustment = tiltX / 4.0;  // 20.0에서 8.0으로 변경하여 약 2.5배 더 민감하게
    
    // 새 채도 계산 (범위를 0-255 내로 유지)
    int newSaturation = qBound(0, s + static_cast<int>(saturationAdjustment), 255);
    
    // Y축 기울기에 따라 명도(Value) 조정 (앞뒤 기울기)
    int tiltY = tiltData.y;
    
    // y축 틸트에 따른 명도 조정 계수 - 계수 증가
    double valueAdjustment = tiltY / 6.0;  // 30.0에서 12.0으로 변경하여 약 2.5배 더 민감하게
    
    // 새 명도 계산 (범위를 0-255 내로 유지)
    int newValue = qBound(0, v + static_cast<int>(valueAdjustment), 255);
    
    // 색상(Hue)은 유지하고 채도와 명도만 조정된 새 색상 반환
    return QColor::fromHsv(h, newSaturation, newValue);
}

// 원 미리보기만 빠르게 업데이트하는 함수
void BingoWidget::updateCirclePreview(int radius) {
    // 원본 프레임이 없으면 리턴
    if (originalFrame.isNull() || !cameraView) {
        return;
    }
    
    // 기존의 미리보기 픽스맵이 없거나 크기가 다르면 새로 생성
    if (previewPixmap.isNull() || previewPixmap.width() != cameraView->width() || 
        previewPixmap.height() != cameraView->height()) {
        
        // 원래 프레임으로부터 미리보기 픽스맵 생성 (최초 1회만 실행)
        // QImage adjustedFrame;
        // try {
        //     adjustedFrame = adjustColorBalance(originalFrame);
        //     if (adjustedFrame.isNull()) {
        //         adjustedFrame = originalFrame;
        //     }
        // } catch (...) {
            // adjustedFrame = originalFrame;
        // }
        
        previewPixmap = QPixmap::fromImage(originalFrame).scaled(
            cameraView->size(),
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation);
    }
    
    // 미리보기 픽스맵 복사본 생성 (원본 유지)
    QPixmap tempPixmap = previewPixmap.copy();
    
    // 원 그리기
    QPainter painter(&tempPixmap);
    if (!painter.isActive()) {
        return;
    }
    
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 원 중심과 반지름 계산
    int centerX = tempPixmap.width() / 2;
    int centerY = tempPixmap.height() / 2;
    int drawRadius = (tempPixmap.width() * radius) / 100;
    
    // 현재 RGB 값으로 원 색상 설정
    QPen pen(QColor(avgRed, avgGreen, avgBlue));
    pen.setWidth(5);
    painter.setPen(pen);
    
    // 원 그리기
    painter.drawEllipse(QPoint(centerX, centerY), drawRadius, drawRadius);
    painter.end();
    
    // 카메라 뷰에 미리보기 표시
    cameraView->setPixmap(tempPixmap);
}