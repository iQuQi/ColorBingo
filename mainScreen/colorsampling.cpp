#include "colorsampling.h"
#include <QRandomGenerator>
#include <QSet>
#include <cmath>
#include <algorithm>

ColorSamplingWidget::ColorSamplingWidget(QWidget *parent) : QWidget(parent) {
    qDebug() << "ColorSamplingWidget constructor called";
    
    // UI 초기화
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 카메라 피드를 보여줄 레이블
    cameraFeedLabel = new QLabel(this);
    cameraFeedLabel->setMinimumSize(640, 480);
    cameraFeedLabel->setAlignment(Qt::AlignCenter);
    cameraFeedLabel->setStyleSheet("background-color: black; color: white;");
    cameraFeedLabel->setText("카메라 초기화 중...");
    
    // 버튼 레이아웃
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    // 뒤로가기 버튼
    backButton = new QPushButton("뒤로가기", this);
    
    // 빙고판 생성 버튼
    generateBoardButton = new QPushButton("빙고판 생성", this);
    generateBoardButton->setStyleSheet("font-weight: bold; padding: 10px; font-size: 16px;");
    
    buttonLayout->addWidget(backButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(generateBoardButton);
    
    mainLayout->addWidget(cameraFeedLabel, 1);
    mainLayout->addLayout(buttonLayout);
    
    // 카메라 객체 생성
    camera = new V4L2Camera(this);
    
    // 시그널/슬롯 연결
    connect(camera, &V4L2Camera::newFrame, this, &ColorSamplingWidget::updateCameraFeed);
    connect(generateBoardButton, &QPushButton::clicked, this, &ColorSamplingWidget::onGenerateBoardClicked);
    connect(backButton, &QPushButton::clicked, this, &ColorSamplingWidget::backButtonClicked);
    
    // 카메라 시작
    QTimer::singleShot(500, this, [this]() {
        qDebug() << "Starting camera";
        startCamera();
    });
    
    qDebug() << "ColorSamplingWidget constructor finished";
}

ColorSamplingWidget::~ColorSamplingWidget() {
    qDebug() << "ColorSamplingWidget destructor called";
    stopCamera();
}

void ColorSamplingWidget::startCamera() {
    if (camera) {
        camera->startCapture();
    }
}

void ColorSamplingWidget::stopCamera() {
    if (camera) {
        camera->stopCapture();
    }
}

void ColorSamplingWidget::updateCameraFeed(const QImage& image) {
    cameraFeedLabel->setPixmap(QPixmap::fromImage(image).scaled(
        cameraFeedLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

// 색상 간 유사도 계산 함수
bool isSimilarColor(const QColor& color1, const QColor& color2, int threshold = 50) {
    int rDiff = color1.red() - color2.red();
    int gDiff = color1.green() - color2.green();
    int bDiff = color1.blue() - color2.blue();
    
    int distance = std::sqrt(rDiff*rDiff + gDiff*gDiff + bDiff*bDiff);
    return distance < threshold;
}

void ColorSamplingWidget::extractColorsFromImage(const QImage& image) {
    // 이미지에서 색상 추출 (9개 색상 필요)
    sampledColors.clear();
    QSet<QRgb> uniqueColors;
    
    // 화면을 그리드로 나누어 샘플링
    int gridSize = 5; // 5x5 그리드
    int cellWidth = image.width() / gridSize;
    int cellHeight = image.height() / gridSize;
    
    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            int x = cellWidth * i + cellWidth / 2;
            int y = cellHeight * j + cellHeight / 2;
            
            if (x < image.width() && y < image.height()) {
                QRgb pixelColor = image.pixel(x, y);
                uniqueColors.insert(pixelColor);
            }
        }
    }
    
    // 다양성 확보를 위한 추가 샘플링
    int maxSamples = 200;
    for (int i = 0; i < maxSamples && uniqueColors.size() < 50; i++) {
        int x = QRandomGenerator::global()->bounded(image.width());
        int y = QRandomGenerator::global()->bounded(image.height());
        QRgb pixelColor = image.pixel(x, y);
        uniqueColors.insert(pixelColor);
    }
    
    // 수집된 색상을 QColor 벡터로 변환
    std::vector<QColor> allColors;
    for (QRgb rgb : uniqueColors) {
        allColors.push_back(QColor(rgb));
    }
    
    // 색상이 충분하지 않은 경우 일부 색상 변형
    if (allColors.size() < 9) {
        int origSize = allColors.size();
        for (int i = 0; i < origSize && allColors.size() < 15; i++) {
            QColor baseColor = allColors[i];
            
            // 밝기 변형
            int hue = baseColor.hue();
            int saturation = baseColor.saturation();
            int value = baseColor.value();
            
            // 더 밝은 버전
            if (value < 220) {
                QColor lighterColor = QColor::fromHsv(hue, saturation, std::min(value + 40, 255));
                allColors.push_back(lighterColor);
            }
            
            // 더 어두운 버전
            if (value > 40) {
                QColor darkerColor = QColor::fromHsv(hue, saturation, std::max(value - 40, 0));
                allColors.push_back(darkerColor);
            }
        }
    }
    
    // 색상이 매우 부족한 경우 기본 색상 추가
    if (allColors.size() < 9) {
        allColors.push_back(QColor(255, 0, 0));    // 빨강
        allColors.push_back(QColor(0, 255, 0));    // 초록
        allColors.push_back(QColor(0, 0, 255));    // 파랑
        allColors.push_back(QColor(255, 255, 0));  // 노랑
        allColors.push_back(QColor(255, 0, 255));  // 마젠타
        allColors.push_back(QColor(0, 255, 255));  // 시안
        allColors.push_back(QColor(255, 128, 0));  // 주황
        allColors.push_back(QColor(128, 0, 128));  // 보라
        allColors.push_back(QColor(0, 128, 0));    // 다크 그린
    }
    
    // 너무 유사한 색상 필터링
    std::vector<QColor> filteredColors;
    for (const QColor& color : allColors) {
        bool isTooSimilar = false;
        
        for (const QColor& existingColor : filteredColors) {
            if (isSimilarColor(color, existingColor, 30)) {
                isTooSimilar = true;
                break;
            }
        }
        
        if (!isTooSimilar) {
            filteredColors.push_back(color);
        }
    }
    
    // 난수 발생기를 사용하여 색상 섞기
    auto rng = QRandomGenerator::global();
    std::shuffle(filteredColors.begin(), filteredColors.end(), 
                 std::default_random_engine(rng->generate()));
    
    // 9개 색상 선택
    int colorsToSelect = std::min(9, static_cast<int>(filteredColors.size()));
    for (int i = 0; i < colorsToSelect; i++) {
        sampledColors.push_back(filteredColors[i]);
    }
    
    // 9개가 안 되면 남은 색상을 랜덤으로 생성
    while (sampledColors.size() < 9) {
        QColor randomColor(
            rng->bounded(256),
            rng->bounded(256),
            rng->bounded(256)
        );
        
        bool isTooSimilar = false;
        for (const QColor& existingColor : sampledColors) {
            if (isSimilarColor(randomColor, existingColor, 30)) {
                isTooSimilar = true;
                break;
            }
        }
        
        if (!isTooSimilar) {
            sampledColors.push_back(randomColor);
        }
    }
}

void ColorSamplingWidget::onGenerateBoardClicked() {
    qDebug() << "Generate board button clicked";
    QImage currentFrame = cameraFeedLabel->pixmap().toImage();
    if (!currentFrame.isNull()) {
        extractColorsFromImage(currentFrame);
        
        if (sampledColors.size() >= 9) {
            qDebug() << "Emitting colorSamplingCompleted signal with" << sampledColors.size() << "colors";
            emit colorSamplingCompleted(sampledColors);
        }
    }
}

std::vector<QColor> ColorSamplingWidget::getSampledColors() const {
    return sampledColors;
} 