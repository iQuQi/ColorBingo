#include "pixelartgenerator.h"

// 싱글톤 인스턴스 초기화
PixelArtGenerator* PixelArtGenerator::instance = nullptr;

// 싱글톤 인스턴스 접근 메서드
PixelArtGenerator* PixelArtGenerator::getInstance() {
    if (instance == nullptr) {
        instance = new PixelArtGenerator();
    }
    return instance;
}

// 생성자
PixelArtGenerator::PixelArtGenerator() {
    // 필요한 초기화 작업이 있다면 여기에 추가
}

// 픽셀 스타일 버튼 CSS 생성
QString PixelArtGenerator::createPixelButtonStyle(const QColor &baseColor, int borderWidth, int borderRadius) {
    // 테두리 색상 (어두운 버전)
    QColor borderColor = baseColor.darker(150);
    
    // 내부 그라데이션 색상 (밝은 버전과 원래 색상)
    QColor lightColor = baseColor.lighter(120);
    
    // 픽셀 느낌의 버튼 스타일시트 생성
    QString style = QString(
        "QPushButton {"
        "   font-size: 24px; font-weight: bold;"
        "   color: white;"
        "   background-color: %1;"
        "   border: %2px solid %3;"
        "   border-radius: %4px;"
        "   padding: 15px 30px;"
        // 내부 픽셀 느낌의 그라데이션 효과
        "   background-image: repeating-linear-gradient("
        "       to bottom,"
        "       %5 0px,"
        "       %5 4px,"
        "       %1 4px,"
        "       %1 8px"
        "   );"
        // 텍스트에 픽셀 느낌의 그림자 효과
        "   text-shadow: 2px 2px 0px rgba(0, 0, 0, 0.5);"
        "}"
        
        "QPushButton:hover {"
        "   background-color: %5;"
        "   background-image: repeating-linear-gradient("
        "       to bottom,"
        "       %5 0px,"
        "       %5 4px,"
        "       %1 4px,"
        "       %1 8px"
        "   );"
        "   border-color: white;"
        "}"
        
        "QPushButton:pressed {"
        "   background-color: %3;"
        "   background-image: repeating-linear-gradient("
        "       to bottom,"
        "       %3 0px,"
        "       %3 4px,"
        "       %1 4px,"
        "       %1 8px"
        "   );"
        "   padding-top: 17px;"
        "   padding-bottom: 13px;"
        "   padding-left: 32px;"
        "   padding-right: 28px;"
        "}"
        
        "QPushButton:focus {"
        "   outline: none;"
        "   border: %2px dashed white;"
        "}"
    ).arg(baseColor.name())
     .arg(borderWidth)
     .arg(borderColor.name())
     .arg(borderRadius)
     .arg(lightColor.name());
    
    return style;
}

// 곰돌이 이미지 생성
QPixmap PixelArtGenerator::createBearImage(int size) {
    QPixmap bearImage(size, size);
    bearImage.fill(Qt::transparent);
    QPainter painter(&bearImage);
    
    // 안티앨리어싱 비활성화 (픽셀 느낌을 위해)
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    // 크기 비율 계산 (원본 80x80 기준)
    float scale = size / 80.0f;
    
    // 갈색 곰돌이 색상
    QColor bearColor(165, 113, 78);
    QColor darkBearColor(120, 80, 60);
    
    // 중앙 정렬을 위한 오프셋
    int offsetX = 5 * scale;
    int offsetY = 5 * scale;
    
    // 기본 얼굴 사각형
    painter.setPen(Qt::NoPen);
    painter.setBrush(bearColor);
    painter.drawRect(15 * scale + offsetX, 20 * scale + offsetY, 40 * scale, 40 * scale);
    
    // 얼굴 둥글게 만들기 - 픽셀 추가
    painter.drawRect(11 * scale + offsetX, 25 * scale + offsetY, 4 * scale, 30 * scale);  // 왼쪽
    painter.drawRect(55 * scale + offsetX, 25 * scale + offsetY, 4 * scale, 30 * scale);  // 오른쪽
    painter.drawRect(20 * scale + offsetX, 16 * scale + offsetY, 30 * scale, 4 * scale);  // 위
    painter.drawRect(20 * scale + offsetX, 60 * scale + offsetY, 30 * scale, 4 * scale);  // 아래
    
    // 추가 픽셀로 더 둥글게 표현
    painter.drawRect(15 * scale + offsetX, 20 * scale + offsetY, 5 * scale, 5 * scale);   // 좌상단 보강
    painter.drawRect(50 * scale + offsetX, 20 * scale + offsetY, 5 * scale, 5 * scale);   // 우상단 보강
    painter.drawRect(15 * scale + offsetX, 55 * scale + offsetY, 5 * scale, 5 * scale);   // 좌하단 보강
    painter.drawRect(50 * scale + offsetX, 55 * scale + offsetY, 5 * scale, 5 * scale);   // 우하단 보강
    
    // 모서리 픽셀 추가
    painter.drawRect(12 * scale + offsetX, 21 * scale + offsetY, 3 * scale, 4 * scale);   // 좌상단 모서리
    painter.drawRect(55 * scale + offsetX, 21 * scale + offsetY, 3 * scale, 4 * scale);   // 우상단 모서리
    painter.drawRect(12 * scale + offsetX, 55 * scale + offsetY, 3 * scale, 4 * scale);   // 좌하단 모서리
    painter.drawRect(55 * scale + offsetX, 55 * scale + offsetY, 3 * scale, 4 * scale);   // 우하단 모서리
    
    // 귀 위치 및 크기 조정 (가로 길이 축소)
    // 왼쪽 귀 - 가로 길이 축소 (13→10)
    painter.drawRect(16 * scale + offsetX, 6 * scale + offsetY, 10 * scale, 16 * scale);  // 기본 왼쪽 귀 (가로 축소)
    painter.drawRect(11 * scale + offsetX, 10 * scale + offsetY, 5 * scale, 12 * scale);  // 왼쪽 귀 왼쪽 보강
    painter.drawRect(26 * scale + offsetX, 10 * scale + offsetY, 5 * scale, 12 * scale);  // 왼쪽 귀 오른쪽 보강 (좌표 조정)
    
    // 오른쪽 귀 - 가로 길이 축소 (13→10)
    painter.drawRect(44 * scale + offsetX, 6 * scale + offsetY, 10 * scale, 16 * scale);  // 기본 오른쪽 귀 (가로 축소)
    painter.drawRect(39 * scale + offsetX, 10 * scale + offsetY, 5 * scale, 12 * scale);  // 오른쪽 귀 왼쪽 보강 (좌표 조정)
    painter.drawRect(54 * scale + offsetX, 10 * scale + offsetY, 5 * scale, 12 * scale);  // 오른쪽 귀 오른쪽 보강
    
    // 귀 안쪽 (더 어두운 색) - 가로 길이 축소 (7→6)
    painter.setBrush(darkBearColor);
    painter.drawRect(19 * scale + offsetX, 9 * scale + offsetY, 6 * scale, 10 * scale);   // 왼쪽 귀 안쪽 (가로 축소)
    painter.drawRect(45 * scale + offsetX, 9 * scale + offsetY, 6 * scale, 10 * scale);   // 오른쪽 귀 안쪽 (가로 축소)
    
    // 눈 (간격 넓히기)
    painter.setBrush(Qt::black);
    painter.drawRect(22 * scale + offsetX, 35 * scale + offsetY, 6 * scale, 6 * scale);   // 왼쪽 눈 (좌표 조정 - 더 왼쪽으로)
    painter.drawRect(42 * scale + offsetX, 35 * scale + offsetY, 6 * scale, 6 * scale);   // 오른쪽 눈 (좌표 조정 - 더 오른쪽으로)
    
    // 코 (위치 위로 올리고 크기 축소)
    painter.drawRect(32 * scale + offsetX, 42 * scale + offsetY, 6 * scale, 4 * scale);   // 코 (위치 위로, 크기 축소 8x5→6x4)
    
    return bearImage;
}

// 픽셀 아트 트로피 생성
QPixmap PixelArtGenerator::createTrophyPixelArt(int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, false); // 픽셀 아트를 위해 안티앨리어싱 끄기
    
    // 크기 비율 계산 (원본 100x100 기준)
    float scale = size / 100.0f;
    
    // 트로피 색상
    QColor goldColor(255, 215, 0);
    QColor darkGoldColor(218, 165, 32);
    
    // 픽셀 크기
    int pixelSize = 4 * scale;
    
    // 트로피 그리기 (픽셀 단위로)
    // 트로피 받침대
    for (int y = 80 * scale; y < 90 * scale; y += pixelSize) {
        for (int x = 35 * scale; x < 65 * scale; x += pixelSize) {
            painter.fillRect(x, y, pixelSize, pixelSize, darkGoldColor);
        }
    }
    
    // 트로피 중앙 기둥
    for (int y = 60 * scale; y < 80 * scale; y += pixelSize) {
        for (int x = 45 * scale; x < 55 * scale; x += pixelSize) {
            painter.fillRect(x, y, pixelSize, pixelSize, darkGoldColor);
        }
    }
    
    // 트로피 컵 부분
    for (int y = 30 * scale; y < 60 * scale; y += pixelSize) {
        for (int x = 25 * scale; x < 75 * scale; x += pixelSize) {
            // 컵 모양 만들기 위한 조건
            if ((y >= 50 * scale || (x >= 35 * scale && x < 65 * scale)) && 
                (x >= 25 * scale + ((y-30 * scale)/3) && x < 75 * scale - ((y-30 * scale)/3))) {
                painter.fillRect(x, y, pixelSize, pixelSize, goldColor);
            }
        }
    }
    
    // 트로피 손잡이
    for (int y = 40 * scale; y < 50 * scale; y += pixelSize) {
        // 왼쪽 손잡이
        for (int x = 15 * scale; x < 25 * scale; x += pixelSize) {
            painter.fillRect(x, y, pixelSize, pixelSize, goldColor);
        }
        // 오른쪽 손잡이
        for (int x = 75 * scale; x < 85 * scale; x += pixelSize) {
            painter.fillRect(x, y, pixelSize, pixelSize, goldColor);
        }
    }
    
    return pixmap;
}

// 픽셀 아트 슬픈 얼굴 생성
QPixmap PixelArtGenerator::createSadFacePixelArt(int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, false); // 픽셀 아트를 위해 안티앨리어싱 끄기
    
    // 크기 비율 계산 (원본 100x100 기준)
    float scale = size / 100.0f;
    
    // 얼굴 색상
    QColor faceColor(255, 200, 0);
    QColor outlineColor(0, 0, 0);
    
    // 픽셀 크기
    int pixelSize = 4 * scale;
    
    // 원형 얼굴 그리기
    for (int y = 20 * scale; y < 80 * scale; y += pixelSize) {
        for (int x = 20 * scale; x < 80 * scale; x += pixelSize) {
            int centerX = 50 * scale;
            int centerY = 50 * scale;
            int radius = 30 * scale;
            
            // 원 안에 있는지 확인
            int dx = x - centerX;
            int dy = y - centerY;
            if (dx*dx + dy*dy <= radius*radius) {
                painter.fillRect(x, y, pixelSize, pixelSize, faceColor);
            }
        }
    }
    
    // 눈 그리기
    for (int y = 35 * scale; y < 45 * scale; y += pixelSize) {
        // 왼쪽 눈
        for (int x = 35 * scale; x < 45 * scale; x += pixelSize) {
            painter.fillRect(x, y, pixelSize, pixelSize, outlineColor);
        }
        // 오른쪽 눈
        for (int x = 55 * scale; x < 65 * scale; x += pixelSize) {
            painter.fillRect(x, y, pixelSize, pixelSize, outlineColor);
        }
    }
    
    // 슬픈 입 그리기 (아래로 향한 곡선)
    for (int x = 35 * scale; x < 65 * scale; x += pixelSize) {
        int y = 65 * scale + (x - 50 * scale) * (x - 50 * scale) / (40 * scale);
        painter.fillRect(x, y, pixelSize, pixelSize, outlineColor);
    }
    
    return pixmap;
}

// X 표시 이미지 생성
QPixmap PixelArtGenerator::createXImage(int size) {
    QPixmap xImg(size, size);
    xImg.fill(Qt::transparent);
    
    QPainter painter(&xImg);
    // 선을 부드럽게 그리기 위해 안티앨리어싱 활성화
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // 크기 비율 계산 (원본 100x100 기준)
    float scale = size / 100.0f;
    
    // 선 두께와 색상 설정
    QPen pen(Qt::white);
    pen.setWidth(8 * scale);  // 선 두께 설정
    pen.setCapStyle(Qt::RoundCap);  // 선 끝을 둥글게
    pen.setJoinStyle(Qt::RoundJoin);  // 선 연결 부분을 둥글게
    painter.setPen(pen);
    
    // 중심점 계산
    const int center = 50 * scale;  // 100x100 이미지의 중심
    const int padding = 20 * scale;  // 외곽 여백
    
    // 대각선 두 개 그리기
    painter.drawLine(padding, padding, size-padding, size-padding);  // 왼쪽 위에서 오른쪽 아래로
    painter.drawLine(size-padding, padding, padding, size-padding);  // 오른쪽 위에서 왼쪽 아래로
    
    painter.end();
    
    return xImg;
}

// 볼륨 아이콘 생성
QPixmap PixelArtGenerator::createVolumeImage(int volumeLevel, int size) {
    QPixmap volumeImage(size, size);
    volumeImage.fill(Qt::transparent);
    QPainter painter(&volumeImage);
    
    // 크기 비율 계산 (원본 40x40 기준)
    float scale = size / 40.0f;
    
    // Disable antialiasing (for pixel feel)
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    // Common colors
    QColor darkGray(50, 50, 50);
    
    // Draw speaker body (always displayed)
    painter.setPen(Qt::NoPen);
    painter.setBrush(darkGray);
    
    // Speaker body (pixel style)
    painter.drawRect(8 * scale, 14 * scale, 8 * scale, 12 * scale); // Speaker rectangular body
    
    // Speaker front (triangle pixel style)
    painter.drawRect(16 * scale, 14 * scale, 2 * scale, 12 * scale); // Connector
    painter.drawRect(18 * scale, 12 * scale, 2 * scale, 16 * scale); // Front part 1
    painter.drawRect(20 * scale, 11 * scale, 2 * scale, 18 * scale); // Front part 2
    painter.drawRect(22 * scale, 10 * scale, 2 * scale, 20 * scale); // Front part 3
    
    // Show sound waves (represented by individual rectangles in pixel style)
    painter.setPen(Qt::NoPen);
    
    // First sound wave curve (always displayed, smallest curve)
    if (volumeLevel >= 1) {
        painter.drawRect(26 * scale, 15 * scale, 2 * scale, 2 * scale); // Top
        painter.drawRect(26 * scale, 23 * scale, 2 * scale, 2 * scale); // Bottom
        painter.drawRect(28 * scale, 13 * scale, 2 * scale, 2 * scale); // Top 2
        painter.drawRect(28 * scale, 25 * scale, 2 * scale, 2 * scale); // Bottom 2
        painter.drawRect(30 * scale, 14 * scale, 2 * scale, 12 * scale); // Vertical line
    }
    
    // Second sound wave curve (displayed in levels 2, 3)
    if (volumeLevel >= 2) {
        painter.drawRect(32 * scale, 10 * scale, 2 * scale, 2 * scale); // Top
        painter.drawRect(32 * scale, 28 * scale, 2 * scale, 2 * scale); // Bottom
        painter.drawRect(34 * scale, 8 * scale, 2 * scale, 2 * scale); // Top 2
        painter.drawRect(34 * scale, 30 * scale, 2 * scale, 2 * scale); // Bottom 2
        painter.drawRect(36 * scale, 10 * scale, 2 * scale, 20 * scale); // Vertical line
    }
    
    // Third sound wave curve (displayed only in level 3, largest curve)
    if (volumeLevel == 3) {
        painter.drawRect(38 * scale, 6 * scale, 2 * scale, 2 * scale); // Top
        painter.drawRect(38 * scale, 32 * scale, 2 * scale, 2 * scale); // Bottom
        painter.drawRect(40 * scale, 4 * scale, 2 * scale, 2 * scale); // Top 2
        painter.drawRect(40 * scale, 34 * scale, 2 * scale, 2 * scale); // Bottom 2
        painter.drawRect(42 * scale, 6 * scale, 2 * scale, 28 * scale); // Vertical line
    }
    
    // Add X mark for low volume (level 1)
    if (volumeLevel == 1) {
        painter.setBrush(Qt::red);
        // Pixel style X mark
        painter.drawRect(30 * scale, 16 * scale, 2 * scale, 2 * scale);
        painter.drawRect(32 * scale, 14 * scale, 2 * scale, 2 * scale);
        painter.drawRect(34 * scale, 16 * scale, 2 * scale, 2 * scale);
        painter.drawRect(32 * scale, 18 * scale, 2 * scale, 2 * scale);
        painter.drawRect(30 * scale, 20 * scale, 2 * scale, 2 * scale);
        painter.drawRect(34 * scale, 20 * scale, 2 * scale, 2 * scale);
        painter.drawRect(32 * scale, 22 * scale, 2 * scale, 2 * scale);
    }
    
    return volumeImage;
} 