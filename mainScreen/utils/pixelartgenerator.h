#ifndef PIXELARTGENERATOR_H
#define PIXELARTGENERATOR_H

#include <QPixmap>
#include <QPainter>
#include <QColor>
#include <QString>
#include <QVector>
#include <QPointF>

class PixelArtGenerator
{
public:
    // 싱글톤 인스턴스 접근 메서드
    static PixelArtGenerator* getInstance();
    
    // 픽셀 아트 생성 함수들
    QPixmap createBearImage(int size = 80);
    QPixmap createTrophyPixelArt(int size = 100);
    QPixmap createSadFacePixelArt(int size = 100);
    QPixmap createXImage(int size = 100);
    QPixmap createVolumeImage(int volumeLevel, int size = 40);
    QPixmap createStarImage(int size = 40);  // 별 모양 이미지 생성
    QPixmap createCuteDevilImage(int size = 40);
    
    // 픽셀 스타일 버튼 생성 함수
    QString createPixelButtonStyle(const QColor &baseColor, int borderWidth = 4, int borderRadius = 12);
    
private:
    // 싱글톤 패턴을 위한 private 생성자
    PixelArtGenerator();
    
    // 싱글톤 인스턴스
    static PixelArtGenerator* instance;
};

#endif // PIXELARTGENERATOR_H 