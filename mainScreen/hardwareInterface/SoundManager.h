#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <QObject>
#include <QString>
#include <QMap>
#include <QThread>
#include <atomic>
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>

class SoundManager : public QObject
{
    Q_OBJECT

public:
    // 오디오 파일 타입
    enum SoundType {
        BACKGROUND_MUSIC,
        EFFECT_SOUND
    };

    // 사운드 효과 종류
    enum SoundEffect {
        CORRECT_SOUND,
        INCORRECT_SOUND,
        SUCCESS_SOUND,
        FAIL_SOUND
    };

    // 싱글톤 인스턴스 가져오기
    static SoundManager* getInstance();

    // 배경음악 재생/정지
    void playBackgroundMusic();
    void stopBackgroundMusic();
    
    // 볼륨 조절 (0.0 ~ 1.0)
    void setBackgroundVolume(float volume);
    void setEffectVolume(float volume);
    
    // 효과음 재생
    void playEffect(SoundEffect effect);

    // 리소스 정리
    void cleanup();

private:
    SoundManager();
    ~SoundManager();
    
    // 파일 경로 매핑
    QMap<SoundEffect, QString> soundFilePath;
    
    // 볼륨 레벨
    float backgroundVolume;
    float effectVolume;
    
    // ALSA 핸들
    snd_pcm_t *backgroundPcm;
    snd_pcm_t *effectPcm;
    
    // 배경음악 스레드
    pthread_t backgroundThread;
    std::atomic<bool> isBackgroundPlaying;
    
    // WAV 파일 재생 함수
    bool openPcm(snd_pcm_t **pcm, const char *device);
    void playWavFile(snd_pcm_t *pcm, const QString &filename, bool loop, float volumeMultiplier);
    static void* backgroundThreadFunc(void *arg);

    // 싱글톤 인스턴스
    static SoundManager *instance;
    
    // 배경음악 경로
    QString backgroundMusicPath;
};

#endif // SOUNDMANAGER_H 