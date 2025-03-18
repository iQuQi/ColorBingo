#include "pixelartgenerator.h"
#include <QDebug>
#include <cmath>  // 수학 함수 및 상수(M_PI, cos 등)를 위한 헤더 추가

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
        // 내부 픽셀 느낌의 그라데이션 효과 - 픽셀 크기 증가 (4px→8px)
        "   background-image: repeating-linear-gradient("
        "       to bottom,"
        "       %5 0px,"
        "       %5 8px,"
        "       %1 8px,"
        "       %1 16px"
        "   );"
        "}"
        
        "QPushButton:hover {"
        "   background-color: %5;"
        "   background-image: repeating-linear-gradient("
        "       to bottom,"
        "       %5 0px,"
        "       %5 8px,"
        "       %1 8px,"
        "       %1 16px"
        "   );"
        "   border-color: white;"
        "}"
        
        "QPushButton:pressed {"
        "   background-color: %3;"
        "   background-image: repeating-linear-gradient("
        "       to bottom,"
        "       %3 0px,"
        "       %3 8px,"
        "       %1 8px,"
        "       %1 16px"
        "   );"
        "   padding-top: 17px;"
        "   padding-bottom: 13px;"
        "   padding-left: 32px;"
        "   padding-right: 28px;"
        "}"
        
        "QPushButton:focus {"
        "   outline: none;"
        "   border: %2px solid %3;"
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
    
    // 눈 그리기 - 크기를 줄이고 위치를 낮춤
    for (int y = 40 * scale; y < 46 * scale; y += pixelSize) {  // 눈 위치를 아래로 이동 (35->40)
        // 왼쪽 눈 - 크기 줄임
        for (int x = 37 * scale; x < 43 * scale; x += pixelSize) {  // 35-45 -> 37-43
            painter.fillRect(x, y, pixelSize, pixelSize, outlineColor);
        }
        // 오른쪽 눈 - 크기 줄임
        for (int x = 57 * scale; x < 63 * scale; x += pixelSize) {  // 55-65 -> 57-63
            painter.fillRect(x, y, pixelSize, pixelSize, outlineColor);
        }
    }
    
    // 슬픈 입 그리기 (아래로 향한 곡선) - 크기를 줄이고 위치를 위로 올림
    for (int x = 40 * scale; x < 60 * scale; x += pixelSize) {  // 35-65 -> 40-60 (입 너비 줄임)
        int y = 60 * scale + (x - 50 * scale) * (x - 50 * scale) / (45 * scale);  // 65->60 (입 위치 위로 올림)
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

// 데이지 꽃 픽셀 아트 생성
QPixmap PixelArtGenerator::createDaisyFlowerImage(int size) {
    qDebug() << "Creating daisy flower image with size:" << size;
    
    QPixmap daisyImage(size, size);
    daisyImage.fill(Qt::transparent);
    QPainter painter(&daisyImage);
    
    // 픽셀 느낌을 조금만 남기기 위해 안티앨리어싱 활성화
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // 크기 비율 계산 (원본 40x40 기준)
    float scale = size / 40.0f;
    
    // 데이지 꽃 색상
    QColor petalColor(255, 255, 255); // 흰색 꽃잎
    QColor petalShadowColor(240, 240, 240); // 꽃잎 그림자
    QColor centerColor(255, 220, 0);  // 노란색 꽃 중심
    QColor centerDarkColor(230, 180, 0); // 꽃 중심 그림자
    
    // 배경 원 - 꽃 전체 영역 정의
    int flowerSize = 36 * scale;
    QRectF flowerRect((size - flowerSize) / 2, (size - flowerSize) / 2, flowerSize, flowerSize);
    
    // 꽃잎 그리기 (6방향으로 변경)
    painter.setPen(Qt::NoPen);
    painter.setBrush(petalColor);
    
    // 각 꽃잎을 개별적으로 그리기 위한 준비
    int petalCount = 6; // 8개에서 6개로 변경
    double angleStep = 360.0 / petalCount;
    int petalLength = 18 * scale; // 14에서 18로 증가시켜 더 길게
    int petalWidth = 10 * scale;
    QPointF center(size / 2, size / 2);
    
    // 각 꽃잎을 타원형으로 그리기
    for (int i = 0; i < petalCount; i++) {
        double angle = i * angleStep;
        
        // 꽃잎의 중심 위치 계산
        double radians = angle * M_PI / 180.0;
        double distance = 8 * scale; // 중심에서의 거리
        QPointF petalCenter(
            center.x() + cos(radians) * distance,
            center.y() + sin(radians) * distance
        );
        
        // 꽃잎 그리기
        painter.save();
        painter.translate(petalCenter);
        painter.rotate(angle);
        
        // 꽃잎 그림자 (살짝 어두운 영역)
        painter.setBrush(petalShadowColor);
        QRectF petalShadowRect(-petalLength/2 * 0.9, -petalWidth/2 * 0.9, petalLength * 0.9, petalWidth * 0.9);
        painter.drawEllipse(petalShadowRect);
        
        // 꽃잎 메인
        painter.setBrush(petalColor);
        QRectF petalRect(-petalLength/2, -petalWidth/2, petalLength, petalWidth);
        painter.drawEllipse(petalRect);
        
        painter.restore();
    }
    
    // 꽃 중심 (노란색 원) - 크기 줄임
    int centerSize = 12 * scale; // 16에서 12로 줄임
    QRectF centerRect((size - centerSize) / 2, (size - centerSize) / 2, centerSize, centerSize);
    
    // 약간의 그라데이션 효과를 위한 그림자
    painter.setBrush(centerDarkColor);
    painter.drawEllipse(centerRect.adjusted(1, 1, -1, -1));
    
    // 메인 중앙 원
    painter.setBrush(centerColor);
    painter.drawEllipse(centerRect);
    
    // 중앙에 텍스처 느낌 추가 (작은 점들)
    painter.setPen(QPen(QColor(230, 160, 0), 1));
    int dotsCount = 10;
    for (int i = 0; i < dotsCount; i++) {
        double dotAngle = i * (360.0 / dotsCount) * M_PI / 180.0;
        double dotDistance = centerSize / 5;
        QPointF dotPos(
            center.x() + cos(dotAngle) * dotDistance,
            center.y() + sin(dotAngle) * dotDistance
        );
        painter.drawPoint(dotPos);
    }
    
    // 페인터 종료 - 중요!
    painter.end();
    
    // 이미지가 제대로 생성되었는지 확인
    qDebug() << "Daisy flower image created - size:" << daisyImage.size() 
             << "isNull:" << daisyImage.isNull();
    
    return daisyImage;
}

// 보라색 귀여운 악마 픽셀 아트 생성
QPixmap PixelArtGenerator::createCuteDevilImage(int size) {
    qDebug() << "Creating cute devil image with size:" << size;
    
    QPixmap devilImage(size, size);
    devilImage.fill(Qt::transparent);
    QPainter painter(&devilImage);
    
    // 안티앨리어싱 활성화 (부드러운 이미지를 위해)
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // 크기 비율 계산 (원본 40x40 기준)
    float scale = size / 40.0f;
    
    // 악마 색상 - 참조 이미지 색상에 맞춤
    QColor bodyColor(138, 73, 214);  // 보라색 몸통
    QColor hornColor(116, 55, 188);  // 조금 더 어두운 보라색 뿔
    QColor eyeColor(255, 255, 255);  // 하얀색 눈
    QColor pupilColor(0, 0, 0);      // 검은색 동공
    QColor mouthColor(70, 30, 120);  // 어두운 보라색 (입)
    
    // 원형 몸통 그리기
    painter.setPen(Qt::NoPen);
    painter.setBrush(bodyColor);
    painter.drawEllipse(QRectF(2 * scale, 2 * scale, 36 * scale, 36 * scale));
    
    // 뿔 그리기 (삼각형 형태)
    QPolygonF leftHorn;
    leftHorn << QPointF(10 * scale, 12 * scale)
             << QPointF(4 * scale, 2 * scale)
             << QPointF(16 * scale, 5 * scale);
             
    QPolygonF rightHorn;
    rightHorn << QPointF(30 * scale, 12 * scale)
              << QPointF(36 * scale, 2 * scale)
              << QPointF(24 * scale, 5 * scale);
              
    painter.setBrush(bodyColor);
    painter.drawPolygon(leftHorn);
    painter.drawPolygon(rightHorn);
    
    // 내부에 약간 어두운 뿔 표현
    QPolygonF leftHornShade;
    leftHornShade << QPointF(10 * scale, 10 * scale)
                  << QPointF(6 * scale, 3 * scale)
                  << QPointF(14 * scale, 5 * scale);
                  
    QPolygonF rightHornShade;
    rightHornShade << QPointF(30 * scale, 10 * scale)
                   << QPointF(34 * scale, 3 * scale)
                   << QPointF(26 * scale, 5 * scale);
                   
    painter.setBrush(hornColor);
    painter.drawPolygon(leftHornShade);
    painter.drawPolygon(rightHornShade);
    
    // 눈 그리기 - 타원형
    painter.setBrush(eyeColor);
    painter.drawEllipse(QRectF(12 * scale, 14 * scale, 6 * scale, 8 * scale));  // 왼쪽 눈
    painter.drawEllipse(QRectF(22 * scale, 14 * scale, 6 * scale, 8 * scale));  // 오른쪽 눈
    
    // 동공 그리기 - 타원형
    painter.setBrush(pupilColor);
    painter.drawEllipse(QRectF(13 * scale, 16 * scale, 4 * scale, 5 * scale));  // 왼쪽 동공
    painter.drawEllipse(QRectF(23 * scale, 16 * scale, 4 * scale, 5 * scale));  // 오른쪽 동공
    
    // 눈썹 그리기 - 사선형
    QPen eyebrowPen(pupilColor, 1.5 * scale, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(eyebrowPen);
    painter.drawLine(QPointF(11 * scale, 13 * scale), QPointF(16 * scale, 12 * scale));  // 왼쪽 눈썹
    painter.drawLine(QPointF(24 * scale, 12 * scale), QPointF(29 * scale, 13 * scale));  // 오른쪽 눈썹
    
    // 미소 그리기 - 반원형
    painter.setPen(Qt::NoPen);
    painter.setBrush(mouthColor);
    painter.drawChord(QRectF(12 * scale, 24 * scale, 16 * scale, 10 * scale), 0, 180 * 16);  // 미소
    
    // 페인터 종료 - 중요!
    painter.end();
    
    // 이미지가 제대로 생성되었는지 확인
    qDebug() << "Cute devil image created - size:" << devilImage.size() 
             << "isNull:" << devilImage.isNull();
    
    return devilImage;
} 