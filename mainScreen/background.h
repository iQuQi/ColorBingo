#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <QPixmap>

/**
 * 내장된 배경 이미지를 제공하는 클래스
 * Qt 리소스 시스템을 통해 내장된 이미지를 로드합니다.
 */
class BackgroundResource {
public:
    // Qt 리소스 시스템에서 배경 이미지 로드
    static QPixmap getBackgroundImage() {
        // 내장된 이미지 로드 (":/images/background.jpg" 경로로 리소스 파일에 등록된 이미지)
        QPixmap background(":/images/background.jpg");
        
        if (background.isNull()) {
            // 이미지 로드 실패 시 로그 출력 (qDebug 포함하지 않고 사용 가능)
            qWarning("Failed to load embedded background image");
        }
        
        return background;
    }
};

#endif // BACKGROUND_H 