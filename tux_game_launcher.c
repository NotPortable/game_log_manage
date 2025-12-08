#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lgpio.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

// 초음파 센서 핀 설정 (GPIO 번호)
#define TRIG_PIN 23
#define ECHO_PIN 24

// 이상 감지 임계값 (cm 단위)
#define ANOMALY_THRESHOLD 10.0
#define MEASUREMENT_INTERVAL 2  // 2초 간격

// 전역 변수
int gpio_handle = -1;
pthread_t sensor_thread;
int sensor_running = 0;
float last_distance = -1.0;
int anomaly_detected = 0;

// 게임 로그 파일 경로
const char* NEVERBALL_LOG = "/home/jungwoo/.neverball/game_log.txt";
const char* SUPERTUX_LOG = "/home/jungwoo/.local/share/supertux2/profile/game_log.txt";
const char* ETR_LOG = "/home/jungwoo/.config/etr/game_log.txt";

// 초음파 센서 초기화
int init_ultrasonic() {
    gpio_handle = lgGpiochipOpen(0);
    if (gpio_handle < 0) {
        printf("❌ GPIO 초기화 실패\n");
        return -1;
    }
    
    // TRIG 핀을 출력으로 설정
    if (lgGpioClaimOutput(gpio_handle, 0, TRIG_PIN, 0) < 0) {
        printf("❌ TRIG 핀 설정 실패\n");
        return -1;
    }
    
    // ECHO 핀을 입력으로 설정
    if (lgGpioClaimInput(gpio_handle, 0, ECHO_PIN) < 0) {
        printf("❌ ECHO 핀 설정 실패\n");
        return -1;
    }
    
    printf("✅ 초음파 센서 초기화 완료\n");
    return 0;
}

// 거리 측정
float measure_distance() {
    if (gpio_handle < 0) return -1.0;
    
    // TRIG 핀에 10us 펄스 전송
    lgGpioWrite(gpio_handle, TRIG_PIN, 1);
    usleep(10);
    lgGpioWrite(gpio_handle, TRIG_PIN, 0);
    
    // ECHO 핀이 HIGH가 될 때까지 대기
    long start_time = 0, end_time = 0;
    long timeout = 1000000; // 1초 타임아웃
    long wait_start = time(NULL);
    
    while (lgGpioRead(gpio_handle, ECHO_PIN) == 0) {
        start_time = time(NULL);
        if (start_time - wait_start > timeout) return -1.0;
    }
    
    // ECHO 핀이 LOW가 될 때까지 대기
    while (lgGpioRead(gpio_handle, ECHO_PIN) == 1) {
        end_time = time(NULL);
        if (end_time - start_time > timeout) return -1.0;
    }
    
    // 거리 계산 (cm)
    long duration = end_time - start_time;
    float distance = (duration * 34300.0) / 2.0 / 1000000.0;
    
    return distance;
}

// 이상 감지 스레드
void* sensor_monitoring_thread(void* arg) {
    printf("🔍 초음파 센서 모니터링 시작 (간격: %d초, 임계값: %.1fcm)\n", 
           MEASUREMENT_INTERVAL, ANOMALY_THRESHOLD);
    
    while (sensor_running) {
        float distance = measure_distance();
        
        if (distance > 0) {
            printf("📏 현재 거리: %.2f cm", distance);
            
            // 이전 측정값과 비교
            if (last_distance > 0) {
                float diff = fabs(distance - last_distance);
                
                if (diff > ANOMALY_THRESHOLD) {
                    anomaly_detected = 1;
                    printf(" ⚠️  이상 감지! (변화량: %.2f cm)\n", diff);
                } else {
                    anomaly_detected = 0;
                    printf(" ✅ 정상\n");
                }
            } else {
                printf(" (초기 측정)\n");
            }
            
            last_distance = distance;
        } else {
            printf("⚠️  거리 측정 실패\n");
        }
        
        sleep(MEASUREMENT_INTERVAL);
    }
    
    return NULL;
}

// 센서 모니터링 시작
int start_sensor_monitoring() {
    if (gpio_handle < 0) {
        if (init_ultrasonic() < 0) {
            printf("⚠️  센서 초기화 실패 - 모니터링 시작 불가\n");
            return -1;
        }
    }
    
    sensor_running = 1;
    if (pthread_create(&sensor_thread, NULL, sensor_monitoring_thread, NULL) != 0) {
        printf("❌ 센서 스레드 생성 실패\n");
        return -1;
    }
    
    printf("✅ 센서 모니터링 시작됨\n");
    return 0;
}

// 센서 모니터링 중지
void stop_sensor_monitoring() {
    if (sensor_running) {
        sensor_running = 0;
        pthread_join(sensor_thread, NULL);
        printf("🛑 센서 모니터링 중지됨\n");
    }
}

// 게임 로그 기록
void log_game_result(const char* game_name, const char* username, const char* log_file, const char* data) {
    FILE* fp = fopen(log_file, "a");
    if (fp == NULL) {
        printf("❌ 로그 파일 열기 실패: %s\n", log_file);
        return;
    }
    
    // 이상 감지 플래그 추가
    const char* anomaly_flag = anomaly_detected ? "ANOMALY" : "NORMAL";
    
    // 시간 정보
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    // 로그 작성
    fprintf(fp, "%s %s %s %s\n", username, data, anomaly_flag, timestamp);
    fclose(fp);
    
    if (anomaly_detected) {
        printf("⚠️  [%s] 이상 데이터로 기록됨\n", game_name);
    } else {
        printf("✅ [%s] 정상 데이터로 기록됨\n", game_name);
    }
}

// 게임 실행
void launch_game(int choice) {
    char username[100];
    printf("\n사용자 이름을 입력하세요: ");
    scanf("%s", username);
    
    // 센서 모니터링 시작
    start_sensor_monitoring();
    
    printf("\n🎮 게임 실행 중...\n");
    printf("📊 거리 측정 중 (이상 감지 활성화)\n\n");
    
    switch(choice) {
        case 1:
            system("neverball");
            // 게임 종료 후 로그 기록 (예시)
            log_game_result("Neverball", username, NEVERBALL_LOG, "107 10000 187 05:23");
            break;
            
        case 2:
            system("supertux2");
            log_game_result("SuperTux", username, SUPERTUX_LOG, "world1-3 156 2 142.8");
            break;
            
        case 3:
            system("etracer");
            log_game_result("ETR", username, ETR_LOG, "Easy_Run 8562 23 02:15.32");
            break;
    }
    
    // 센서 모니터링 중지
    stop_sensor_monitoring();
}

// 센서 상태 확인
void check_sensor_status() {
    printf("\n📊 센서 상태 확인\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    if (gpio_handle < 0) {
        if (init_ultrasonic() < 0) {
            printf("❌ 센서 사용 불가\n");
            return;
        }
    }
    
    printf("✅ 센서 상태: 정상\n");
    printf("📏 측정 간격: %d초\n", MEASUREMENT_INTERVAL);
    printf("⚠️  임계값: %.1fcm\n", ANOMALY_THRESHOLD);
    
    // 테스트 측정
    printf("\n🔍 테스트 측정 중...\n");
    for (int i = 0; i < 5; i++) {
        float distance = measure_distance();
        if (distance > 0) {
            printf("   측정 %d: %.2f cm\n", i+1, distance);
        } else {
            printf("   측정 %d: 실패\n", i+1);
        }
        sleep(1);
    }
}

// 메인 메뉴
void show_menu() {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║       🎮 NotPortable 게임 런처 🎮       ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║  [1] 🏀 Neverball                      ║\n");
    printf("║  [2] 🐧 SuperTux                       ║\n");
    printf("║  [3] 🎿 Extreme Tux Racer              ║\n");
    printf("║  ────────────────────────────────────  ║\n");
    printf("║  [9] 📊 센서 상태 확인                  ║\n");
    printf("║  [0] 🚪 종료                           ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n선택: ");
}

int main() {
    int choice;
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   NotPortable - 초음파 센서 이상 감지   ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    // 센서 초기화 시도
    if (init_ultrasonic() == 0) {
        printf("✅ 초음파 센서 준비 완료\n");
    } else {
        printf("⚠️  센서 없이 계속 진행 (이상 감지 비활성화)\n");
    }
    
    while (1) {
        show_menu();
        scanf("%d", &choice);
        
        switch(choice) {
            case 1:
            case 2:
            case 3:
                launch_game(choice);
                break;
                
            case 9:
                check_sensor_status();
                break;
                
            case 0:
                printf("\n👋 프로그램을 종료합니다.\n");
                if (gpio_handle >= 0) {
                    lgGpiochipClose(gpio_handle);
                }
                return 0;
                
            default:
                printf("\n❌ 잘못된 선택입니다.\n");
        }
    }
    
    return 0;
}