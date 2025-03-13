#include "colorsampling.h"
#include "bingowidget.h"
#include <QDebug>

// onStartBingoClicked 함수 수정
void MainScreen::onStartBingoClicked() {
    qDebug() << "Start Bingo button clicked!";
    startColorSampling();
}

// 새 함수 추가
void MainScreen::startColorSampling() {
    qDebug() << "Creating color sampling dialog...";
    
    // 모달 다이얼로그로 생성
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("색상 샘플링");
    dialog->setModal(true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    
    // 다이얼로그 레이아웃
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    // ColorSamplingWidget을 다이얼로그에 추가
    ColorSamplingWidget* colorSampling = new ColorSamplingWidget(dialog);
    layout->addWidget(colorSampling);
    
    // 색상 샘플링 완료 시 다이얼로그 닫고 빙고 게임 표시
    connect(colorSampling, &ColorSamplingWidget::colorSamplingCompleted, 
            this, [this, dialog](const std::vector<QColor>& colors) {
        qDebug() << "Color sampling completed, closing dialog";
        dialog->accept(); // 다이얼로그 닫기
        
        // 빙고 위젯 표시
        BingoWidget* bingoWidget = new BingoWidget(colors);
        bingoWidget->setWindowFlags(Qt::Window);
        bingoWidget->setAttribute(Qt::WA_DeleteOnClose);
        bingoWidget->show();
        
        // 메인 화면 숨기기
        this->hide();
    });
    
    // 뒤로가기 버튼 클릭 시 다이얼로그만 닫기
    connect(colorSampling, &ColorSamplingWidget::backButtonClicked,
            this, [dialog]() {
        qDebug() << "Back button clicked, closing dialog";
        dialog->reject();
    });
    
    // 다이얼로그 표시
    dialog->setMinimumSize(800, 600);
    dialog->show();
    
    qDebug() << "Color sampling dialog shown";
} 