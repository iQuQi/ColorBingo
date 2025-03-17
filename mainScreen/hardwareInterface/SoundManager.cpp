#include "SoundManager.h"
#include <QFile>
#include <QDebug>
#include <unistd.h>
#include <fcntl.h>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QStringList>
#include <QTemporaryFile>
#include <algorithm>

SoundManager* SoundManager::instance = nullptr;

SoundManager* SoundManager::getInstance()
{
    if (instance == nullptr) {
        instance = new SoundManager();
    }
    return instance;
}

SoundManager::SoundManager() : 
    backgroundVolume(0.8f),
    effectVolume(1.0f),
    backgroundPcm(nullptr),
    effectPcm(nullptr),
    isBackgroundPlaying(false)
{
    qDebug() << "DEBUG: SoundManager initialization started...";
    
    // ALSA 장치 환경 변수 설정
    setenv("ALSA_CARD", "0", 1);  // 0번 카드 사용
    setenv("ALSA_PCM_CARD", "0", 1);
    setenv("ALSA_PCM_DEVICE", "0", 1);  // 0번 장치 사용
    
    // 사운드 파일 경로 초기화
    QString musicPath = "mainScreen/music/";
    
    // 디버깅: 현재 작업 디렉터리 확인
    QDir currentDir = QDir::current();
    qDebug() << "DEBUG: Current working directory: " << currentDir.absolutePath();
    
    // 디렉터리 존재 확인
    QDir musicDir(musicPath);
    if (!musicDir.exists()) {
        qDebug() << "ERROR: Music directory does not exist: " << musicDir.absolutePath();
        
        // 상위 디렉터리 확인
        QDir parentDir = currentDir;
        parentDir.cdUp();
        qDebug() << "DEBUG: Parent directory: " << parentDir.absolutePath();
        qDebug() << "DEBUG: Parent directory contents:";
        QStringList parentEntries = parentDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
        for (const QString &entry : parentEntries) {
            qDebug() << "  - " << entry;
        }
    } else {
        qDebug() << "DEBUG: Music directory exists: " << musicDir.absolutePath();
        
        // 디렉터리 내용 확인
        qDebug() << "DEBUG: Music directory contents:";
        QStringList musicFiles = musicDir.entryList(QDir::Files);
        for (const QString &file : musicFiles) {
            qDebug() << "  - " << file;
        }
    }
    
    // 사운드 파일 경로 설정 및 존재 확인
    backgroundMusicPath = ":/music/background_sound.wav";
    QFileInfo bgFile(backgroundMusicPath);
    
    if (bgFile.exists()) {
        qDebug() << "DEBUG: Background music file exists: " << backgroundMusicPath;
        qDebug() << "DEBUG: File size: " << bgFile.size() << " bytes";
        qDebug() << "DEBUG: File permissions: " << bgFile.permissions();
    } else {
        qDebug() << "ERROR: Background music file does not exist: " << backgroundMusicPath;
        
        // 다른 경로 시도
        QStringList possiblePaths = {
            "music/background_sound.wav",
            "../music/background_sound.wav",
            "./music/background_sound.wav",
            "/music/background_sound.wav"
        };
        
        qDebug() << "DEBUG: Trying alternative paths:";
        for (const QString &path : possiblePaths) {
            QFileInfo altFile(path);
            if (altFile.exists()) {
                qDebug() << "  - FOUND at: " << path;
                backgroundMusicPath = path;
                break;
            } else {
                qDebug() << "  - Not found at: " << path;
            }
        }
    }
    
    // BINGO_SOUND 항목 제거
    soundFilePath[CORRECT_SOUND] = ":/music/correct_sound.wav";
    soundFilePath[INCORRECT_SOUND] = ":/music/incorrect_sound.wav";
    soundFilePath[SUCCESS_SOUND] = ":/music/success_sound.wav";
    soundFilePath[FAIL_SOUND] = ":/music/fail_sound.wav";
    
    // PCM 디바이스 열기 - 라즈베리파이 헤드폰 직접 지정
    if (!openPcm(&backgroundPcm, "hw:0,0")) {
        qDebug() << "ERROR: Failed to open hw:0,0, trying alternative device";
        if (!openPcm(&backgroundPcm, "default")) {
            qDebug() << "ERROR: Failed to open default device as well";
        }
    }
    
    if (!openPcm(&effectPcm, "hw:0,0")) {
        qDebug() << "ERROR: Failed to open effect device hw:0,0";
        if (!openPcm(&effectPcm, "default")) {
            qDebug() << "ERROR: Failed to open default device for effects";
        }
    }
    
    // 볼륨 설정 테스트
    qDebug() << "DEBUG: Testing mixer settings...";
    
    // ALSA 믹서 테스트
    snd_mixer_t *mixer = nullptr;
    int err = snd_mixer_open(&mixer, 0);
    if (err < 0) {
        qDebug() << "ERROR: Cannot open mixer:" << snd_strerror(err);
    } else {
        err = snd_mixer_attach(mixer, "default");
        if (err < 0) {
            qDebug() << "ERROR: Cannot attach mixer to default device:" << snd_strerror(err);
        } else {
            err = snd_mixer_selem_register(mixer, NULL, NULL);
            if (err < 0) {
                qDebug() << "ERROR: Cannot register mixer elements:" << snd_strerror(err);
            } else {
                err = snd_mixer_load(mixer);
                if (err < 0) {
                    qDebug() << "ERROR: Cannot load mixer elements:" << snd_strerror(err);
                } else {
                    qDebug() << "DEBUG: Mixer successfully loaded";
                    
                    // 사용 가능한 모든 믹서 요소 출력
                    qDebug() << "DEBUG: Available mixer controls:";
                    snd_mixer_elem_t *elem = snd_mixer_first_elem(mixer);
                    while (elem) {
                        if (snd_mixer_elem_get_type(elem) == SND_MIXER_ELEM_SIMPLE) {
                            const char *name = snd_mixer_selem_get_name(elem);
                            qDebug() << "  -" << name;
                            
                            // 볼륨 설정 시도
                            if (snd_mixer_selem_has_playback_volume(elem)) {
                                long min, max;
                                snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
                                qDebug() << "    * Volume range:" << min << "to" << max;
                                
                                // 볼륨을 최대로 설정
                                err = snd_mixer_selem_set_playback_volume_all(elem, max);
                                if (err < 0) {
                                    qDebug() << "    * ERROR: Cannot set volume:" << snd_strerror(err);
                                } else {
                                    qDebug() << "    * Volume set to maximum";
                                }
                            }
                        }
                        elem = snd_mixer_elem_next(elem);
                    }
                }
            }
        }
        snd_mixer_close(mixer);
    }
    
    // PCM 장치 정보 출력
    qDebug() << "DEBUG: Checking available PCM devices:";
    void **hints;
    char **n, **h;
    
    if (snd_device_name_hint(-1, "pcm", &hints) >= 0) {
        n = (char**)hints;
        while (*n != NULL) {
            char *name = snd_device_name_get_hint(*n, "NAME");
            if (name != NULL && name[0] != '\0') {
                char *desc = snd_device_name_get_hint(*n, "DESC");
                char *ioid = snd_device_name_get_hint(*n, "IOID");
                
                if (ioid == NULL || strcmp(ioid, "Output") == 0) {
                    qDebug() << "  - Device:" << name;
                    if (desc != NULL) {
                        qDebug() << "    * Description:" << desc;
                    }
                }
                
                if (desc) free(desc);
                if (ioid) free(ioid);
                free(name);
            }
            n++;
        }
        snd_device_name_free_hint(hints);
    } else {
        qDebug() << "ERROR: Cannot get device list";
    }
    
    qDebug() << "DEBUG: SoundManager initialization completed";
    
    setBackgroundVolume(backgroundVolume);
}

SoundManager::~SoundManager()
{
    qDebug() << "DEBUG: SoundManager destructor called";
    cleanup();
}

void SoundManager::cleanup()
{
    qDebug() << "DEBUG: Cleaning up SoundManager resources...";
    
    // 배경음악 중지
    stopBackgroundMusic();
    
    // PCM 디바이스 닫기
    if (backgroundPcm) {
        snd_pcm_close(backgroundPcm);
        backgroundPcm = nullptr;
    }
    
    if (effectPcm) {
        snd_pcm_close(effectPcm);
        effectPcm = nullptr;
    }
    
    qDebug() << "DEBUG: SoundManager resource cleanup completed";
}

bool SoundManager::openPcm(snd_pcm_t **pcm, const char *device)
{
    int err;
    
    // 먼저 명시적인 헤드폰 장치 열기 시도
    if ((err = snd_pcm_open(pcm, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        qDebug() << "Cannot open hw:0,0 device: " << snd_strerror(err);
        
        // 실패하면 기본 장치 시도
        if ((err = snd_pcm_open(pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            qDebug() << "Cannot open default PCM device: " << snd_strerror(err);
            
            // 마지막으로 제공된 장치 시도
            if ((err = snd_pcm_open(pcm, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
                qDebug() << "Cannot open provided PCM device: " << snd_strerror(err);
                return false;
            }
        }
    }
    
    // 성공적으로 열렸으면 PCM 설정 최적화
    snd_pcm_nonblock(*pcm, 0);  // 블로킹 모드로 설정
    
    // 버퍼 언더런 복구 옵션 설정
    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
    snd_pcm_sw_params_current(*pcm, swparams);
    snd_pcm_sw_params_set_start_threshold(*pcm, swparams, 0);
    snd_pcm_sw_params_set_avail_min(*pcm, swparams, 4);
    snd_pcm_sw_params(*pcm, swparams);
    
    return true;
}

void SoundManager::playBackgroundMusic()
{
    if (isBackgroundPlaying) {
        qDebug() << "Background music is already playing";
        return;
    }
    
    qDebug() << "Starting background music playback";
    isBackgroundPlaying = true;
    
    // 배경음악 스레드 생성
    pthread_create(&backgroundThread, NULL, backgroundThreadFunc, this);
}

void SoundManager::stopBackgroundMusic()
{
    if (!isBackgroundPlaying) {
        qDebug() << "Background music is not currently playing";
        return;
    }
    
    qDebug() << "Stopping background music playback";
    isBackgroundPlaying = false;
    
    // 재생 스레드가 종료될 때까지 대기
    pthread_join(backgroundThread, NULL);
    qDebug() << "Background music thread terminated";
}

void* SoundManager::backgroundThreadFunc(void *arg)
{
    SoundManager *soundManager = static_cast<SoundManager*>(arg);
    
    // PCM 디바이스가 열려있지 않으면 그대로 종료
    if (!soundManager->backgroundPcm) {
        qDebug() << "ERROR: Background PCM device is not open";
        return nullptr;
    }
    
    qDebug() << "Background music thread started";
    soundManager->playWavFile(soundManager->backgroundPcm, soundManager->backgroundMusicPath, true, 1.0f);
    qDebug() << "Background music thread finished";
    
    return nullptr;
}

void SoundManager::playEffect(SoundEffect effect)
{
    // 효과음 파일 경로 가져오기
    QString filePath = soundFilePath[effect];
    
    qDebug() << "Playing sound effect: " << filePath;
    
    // 모든 효과음은 스레드에서 재생
    QThread *thread = QThread::create([this, filePath, effect]() {
        // 효과음용 임시 PCM 핸들 생성
        snd_pcm_t *tempPcm = nullptr;
        if (!openPcm(&tempPcm, "hw:0,0")) {
            qDebug() << "ERROR: Failed to open temporary PCM device for effect";
            return;
        }
        
        // 효과음 재생 (볼륨 증가)
        float volumeMult = 2.0f;
        // SUCCESS_SOUND와 FAIL_SOUND는 볼륨을 더 높임
        if (effect == SUCCESS_SOUND || effect == FAIL_SOUND) {
            volumeMult = 3.0f;
        }
        
        qDebug() << "Starting sound effect playback - Volume:" << volumeMult;
        playWavFile(tempPcm, filePath, false, volumeMult);
        qDebug() << "Sound effect playback completed";
        
        // 사용 후 PCM 핸들 닫기
        snd_pcm_drain(tempPcm);
        snd_pcm_close(tempPcm);
    });
    
    thread->start();
    
    // 스레드 자동 삭제 설정
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void SoundManager::setBackgroundVolume(float volume)
{
    // Save current volume
    float oldVolume = backgroundVolume;
    
    // Set new volume
    backgroundVolume = qBound(0.0f, volume, 1.0f);
    
    qDebug() << "Background music volume changed: " << oldVolume << " -> " << backgroundVolume;
    
    // If currently playing, restart background music to apply volume change immediately
    if (isBackgroundPlaying) {
        qDebug() << "Background music is playing - restarting to apply volume change immediately";
        
        // Stop background music
        bool wasPlaying = isBackgroundPlaying;
        isBackgroundPlaying = false;
        
        // Wait for existing thread to terminate
        pthread_join(backgroundThread, NULL);
        
        // Reset PCM device state
        if (backgroundPcm) {
            snd_pcm_drop(backgroundPcm);
            snd_pcm_close(backgroundPcm);
            backgroundPcm = nullptr;
            
            // Reopen PCM device
            if (!openPcm(&backgroundPcm, "hw:0,0")) {
                if (!openPcm(&backgroundPcm, "default")) {
                    qDebug() << "ERROR: Failed to reopen background PCM device";
                    return;
                }
            }
            qDebug() << "Background music PCM device reinitialized";
        }
        
        // Restart background music (with new volume)
        if (wasPlaying) {
            isBackgroundPlaying = true;
            pthread_create(&backgroundThread, NULL, backgroundThreadFunc, this);
            qDebug() << "Background music restarted with new volume: " << backgroundVolume;
        }
    } else {
        qDebug() << "Background music not playing - new volume will be applied on next playback: " << backgroundVolume;
    }
    
    // ALSA volume control
    snd_mixer_t *mixer = nullptr;
    snd_mixer_elem_t *elem = nullptr;
    snd_mixer_selem_id_t *sid = nullptr;
    
    int err = snd_mixer_open(&mixer, 0);
    if (err < 0) {
        qDebug() << "ERROR: Cannot open mixer: " << snd_strerror(err);
        return;
    }
    
    err = snd_mixer_attach(mixer, "hw:0"); // Specify card 0
    if (err < 0) {
        qDebug() << "ERROR: Cannot attach mixer to device: " << snd_strerror(err);
        snd_mixer_close(mixer);
        return;
    }
    
    err = snd_mixer_selem_register(mixer, NULL, NULL);
    if (err < 0) {
        qDebug() << "ERROR: Cannot register mixer elements: " << snd_strerror(err);
        snd_mixer_close(mixer);
        return;
    }
    
    err = snd_mixer_load(mixer);
    if (err < 0) {
        qDebug() << "ERROR: Cannot load mixer elements: " << snd_strerror(err);
        snd_mixer_close(mixer);
        return;
    }
    
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, "PCM"); // Mixer name
    
    elem = snd_mixer_find_selem(mixer, sid);
    
    if (elem) {
        long min, max;
        snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
        
        // Apply actual volume value (calculated as ratio of total range)
        long value = min + (max - min) * backgroundVolume;
        
        qDebug() << "ALSA hardware mixer volume setting:" 
                 << " Range [" << min << ", " << max << "]"
                 << " Volume level: " << backgroundVolume 
                 << " -> Actual value: " << value;
        
        err = snd_mixer_selem_set_playback_volume_all(elem, value);
        if (err < 0) {
            qDebug() << "ERROR: Cannot set volume: " << snd_strerror(err);
        } else {
            qDebug() << "ALSA mixer volume set successfully";
        }
    } else {
        qDebug() << "ERROR: Cannot find 'PCM' mixer element";
    }
    
    snd_mixer_close(mixer);
}

void SoundManager::setEffectVolume(float volume)
{
    // 기존 볼륨 저장
    float oldVolume = effectVolume;
    
    // 새 볼륨 설정
    effectVolume = qBound(0.0f, volume, 1.0f);
    
    qDebug() << "Sound effect volume changed:" << oldVolume << " -> " << effectVolume
             << " (will be applied to subsequent sound effects)";
    
    // 이 메서드는 즉시 효과음 재생에 영향을 주지 않음
    // 다음 효과음 재생부터 적용됨
}

void SoundManager::playWavFile(snd_pcm_t *pcm, const QString &filename, bool loop, float volumeMultiplier)
{
    if (!pcm) {
        qDebug() << "ERROR: PCM device is NULL";
        return;
    }
    
    // 적용할 실제 볼륨 계산 (배경음악인지 효과음인지에 따라 다른 볼륨 값 사용)
    float actualVolumeMultiplier = volumeMultiplier;
    if (loop) {
        // 배경음악인 경우
        actualVolumeMultiplier *= backgroundVolume;
        qDebug() << "Background music playback - Volume multiplier:" << volumeMultiplier << " x Volume setting:" << backgroundVolume 
                 << " = Final volume:" << actualVolumeMultiplier;
    } else {
        // 효과음인 경우
        actualVolumeMultiplier *= effectVolume;
        qDebug() << "Sound effect playback - Volume multiplier:" << volumeMultiplier << " x Volume setting:" << effectVolume 
                 << " = Final volume:" << actualVolumeMultiplier;
    }
    
    QFile file;
    QString actualFilename = filename;
    QTemporaryFile *tempFile = nullptr;
    
    // 리소스 파일인 경우 임시 파일에 복사
    if (filename.startsWith(":/")) {
        QFile resourceFile(filename);
        if (!resourceFile.open(QIODevice::ReadOnly)) {
            qDebug() << "ERROR: Cannot open resource file:" << filename;
            return;
        }
        
        qDebug() << "Successfully opened resource file: " << filename;
        
        // 임시 파일 생성
        tempFile = new QTemporaryFile();
        tempFile->setAutoRemove(true);
        if (!tempFile->open()) {
            qDebug() << "ERROR: Cannot create temporary file";
            resourceFile.close();
            delete tempFile;
            return;
        }
        
        qDebug() << "Successfully created temporary file: " << tempFile->fileName();
        
        // 리소스 데이터를 임시 파일에 복사
        QByteArray data = resourceFile.readAll();
        tempFile->write(data);
        tempFile->flush();
        
        actualFilename = tempFile->fileName();
        resourceFile.close();
    }
    
    // 임시 파일이나 일반 파일 열기
    file.setFileName(actualFilename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "ERROR: Cannot open WAV file:" << actualFilename;
        delete tempFile;
        return;
    }
    
    // WAV 헤더 크기
    const int WAV_HEADER_SIZE = 44;
    
    // WAV 헤더 읽기
    QByteArray header = file.read(WAV_HEADER_SIZE);
    
    if (header.size() < WAV_HEADER_SIZE) {
        qDebug() << "ERROR: Invalid WAV header size:" << header.size();
        file.close();
        delete tempFile;
        return;
    }
    
    // PCM 파라미터 확인
    unsigned int channels = header[22];
    unsigned int sample_rate = (unsigned char)header[24] + 
                               ((unsigned char)header[25] << 8) + 
                               ((unsigned char)header[26] << 16) + 
                               ((unsigned char)header[27] << 24);
    unsigned int bits_per_sample = header[34];
    
    // 간단한 파일 정보만 출력
    qDebug() << "WAV file information: " << filename;
    qDebug() << "Channels: " << channels << ", Sample rate: " << sample_rate 
             << "Hz, Bit depth: " << bits_per_sample << "bit";
    
    // ALSA 하드웨어 파라미터 설정
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    
    int err = snd_pcm_hw_params_any(pcm, params);
    if (err < 0) {
        qDebug() << "ERROR: Cannot initialize hardware parameters:" << snd_strerror(err);
        file.close();
        delete tempFile;
        return;
    }
    
    // 접근 모드 설정
    err = snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        qDebug() << "ERROR: Cannot set access mode:" << snd_strerror(err);
        file.close();
        delete tempFile;
        return;
    }
    
    // 포맷 설정 - 24비트 특수 처리 제거하고 단순화
    snd_pcm_format_t format;
    
    if (bits_per_sample == 8)
        format = SND_PCM_FORMAT_U8;
    else if (bits_per_sample == 16)
        format = SND_PCM_FORMAT_S16_LE;
    else {
        qDebug() << "ERROR: Unsupported bits per sample:" << bits_per_sample;
        file.close();
        delete tempFile;
        return;
    }
    
    // 포맷 설정
    err = snd_pcm_hw_params_set_format(pcm, params, format);
    if (err < 0) {
        qDebug() << "ERROR: Cannot set format:" << snd_strerror(err);
        file.close();
        delete tempFile;
        return;
    }
    
    // 채널 수 설정
    err = snd_pcm_hw_params_set_channels(pcm, params, channels);
    if (err < 0) {
        qDebug() << "ERROR: Cannot set channels:" << snd_strerror(err);
        file.close();
        delete tempFile;
        return;
    }
    
    // 샘플 레이트 설정
    unsigned int exactRate = sample_rate;
    err = snd_pcm_hw_params_set_rate_near(pcm, params, &exactRate, 0);
    if (err < 0) {
        qDebug() << "ERROR: Cannot set sample rate:" << snd_strerror(err);
        file.close();
        delete tempFile;
        return;
    }
    
    // 버퍼 크기 및 주기 설정
    snd_pcm_uframes_t buffer_size = 16384;
    err = snd_pcm_hw_params_set_buffer_size_near(pcm, params, &buffer_size);
    if (err < 0) {
        qDebug() << "ERROR: Cannot set buffer size:" << snd_strerror(err);
        file.close();
        delete tempFile;
        return;
    }
    
    snd_pcm_uframes_t period_size = 4096;
    err = snd_pcm_hw_params_set_period_size_near(pcm, params, &period_size, 0);
    if (err < 0) {
        qDebug() << "ERROR: Cannot set period size:" << snd_strerror(err);
        file.close();
        delete tempFile;
        return;
    }
    
    // 파라미터 적용
    err = snd_pcm_hw_params(pcm, params);
    if (err < 0) {
        qDebug() << "ERROR: Cannot set hardware parameters:" << snd_strerror(err);
        file.close();
        delete tempFile;
        return;
    }
    
    // 재생 버퍼 할당
    int frameSize = channels * (bits_per_sample / 8);
    char *buffer = new char[period_size * frameSize];
    
    do {
        file.seek(WAV_HEADER_SIZE);
        
        // 파일에서 데이터 읽고 재생
        while (!file.atEnd() && (loop || !loop)) {
            qint64 bytesRead = file.read(buffer, period_size * frameSize);
            
            if (bytesRead <= 0) {
                break;
            }
            
            // 변수 미리 선언
            snd_pcm_uframes_t frames;
            snd_pcm_sframes_t written;
                    
            // 샘플의 볼륨 조절 (재생 전)
            if (bits_per_sample == 16) {
                // 계산된 볼륨 적용
                for (unsigned int i = 0; i < bytesRead / 2; i++) {
                    short *sample = reinterpret_cast<short*>(buffer) + i;
                    float adjusted = static_cast<float>(*sample) * actualVolumeMultiplier;
                    *sample = static_cast<short>(std::min(32767.0f, std::max(-32768.0f, adjusted)));
                }
            }
            
            // 재생
            frames = bytesRead / frameSize;
            written = snd_pcm_writei(pcm, buffer, frames);
            
            if (written < 0) {
                written = snd_pcm_recover(pcm, written, 0);
                if (written < 0) {
                    qDebug() << "ERROR: Recovery failed:" << snd_strerror(written);
                    break;
                }
            }
            
            // 효과음이거나 재생 중지 명령이 있으면 종료
            if (!loop || (loop && !isBackgroundPlaying)) {
                if (bytesRead < period_size * frameSize || !isBackgroundPlaying) {
                    break;
                }
            }
        }
        
        // 루프 아니면 종료
        if (!loop) {
            break;
        }
        
    } while (loop && isBackgroundPlaying);
    
    delete[] buffer;
    file.close();
    
    // 임시 파일 정리
    if (tempFile) {
        delete tempFile;
    }
    
    // 효과음이면 drain
    if (!loop) {
        snd_pcm_drain(pcm);
    }
} 