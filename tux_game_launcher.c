#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lgpio.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

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

// SuperTux 사용자 이름 저장 파일
const char* SUPERTUX_USERNAME_FILE = "/tmp/supertux_username.txt";

// 마이크로초 단위 시간 가져오기
long long get_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

// 사용자 이름을 파일에 저장
void save_username_to_file(const char* username) {
    FILE* fp = fopen(SUPERTUX_USERNAME_FILE, "w");
    if (fp) {
        fprintf(fp, "%s", username);
        fclose(fp);
        printf("✅ 사용자 이름 저장: %s\n", username);
    } else {
        printf("⚠️  사용자 이름 저장 실패\n");
    }
}

// 초음파 센서 초기화
int init_ultrasonic() {
    gpio_handle = lgGpiochipOpen(0);
    if (gpio_handle < 0) {
        printf("❌ GPIO 초기화 실패: %d\n", gpio_handle);
        return -1;
    }
    
    // TRIG 핀을 출력으로 설정
    int trig_result = lgGpioClaimOutput(gpio_handle, 0, TRIG_PIN, 0);
    if (trig_result < 0) {
        printf("❌ TRIG 핀 설정 실패 (GPIO %d): %d\n", TRIG_PIN, trig_result);
        return -1;
    }
    
    // ECHO 핀을 입력으로 설정
    int echo_result = lgGpioClaimInput(gpio_handle, 0, ECHO_PIN);
    if (echo_result < 0) {
        printf("❌ ECHO 핀 설정 실패 (GPIO %d): %d\n", ECHO_PIN, echo_result);
        return -1;
    }
    
    printf("✅ 초음파 센서 초기화 완료\n");
    printf("   TRIG: GPIO %d (Physical Pin 16)\n", TRIG_PIN);
    printf("   ECHO: GPIO %d (Physical Pin 18)\n", ECHO_PIN);
    return 0;
}

// 거리 측정 (마이크로초 단위)
float measure_distance() {
    if (gpio_handle < 0) return -1.0;
    
    // TRIG 핀에 10us 펄스 전송
    lgGpioWrite(gpio_handle, TRIG_PIN, 0);
    usleep(2);
    lgGpioWrite(gpio_handle, TRIG_PIN, 1);
    usleep(10);
    lgGpioWrite(gpio_handle, TRIG_PIN, 0);
    
    // ECHO 핀이 HIGH가 될 때까지 대기 (타임아웃 100ms)
    long long timeout_start = get_microseconds();
    while (lgGpioRead(gpio_handle, ECHO_PIN) == 0) {
        if (get_microseconds() - timeout_start > 100000) {
            printf("⚠️  ECHO HIGH 대기 타임아웃\n");
            return -1.0;
        }
    }
    long long pulse_start = get_microseconds();
    
    // ECHO 핀이 LOW가 될 때까지 대기 (타임아웃 100ms)
    timeout_start = get_microseconds();
    while (lgGpioRead(gpio_handle, ECHO_PIN) == 1) {
        if (get_microseconds() - timeout_start > 100000) {
            printf("⚠️  ECHO LOW 대기 타임아웃\n");
            return -1.0;
        }
    }
    long long pulse_end = get_microseconds();
    
    // 거리 계산 (cm)
    long long duration_us = pulse_end - pulse_start;
    float distance = (duration_us * 0.0343) / 2.0;  // 음속 343m/s = 0.0343cm/us
    
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
                    printf(" 🚨 이상 감지! (변화량: %.2f cm)\n", diff);
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

// 게임 실행
void launch_game(int choice) {
    char username[100];
    printf("\n사용자 이름을 입력하세요: ");
    scanf("%s", username);
    
    // SuperTux의 경우 사용자 이름 파일에 저장
    if (choice == 2) {
        save_username_to_file(username);
    }
    
    // 센서 모니터링 시작
    start_sensor_monitoring();
    
    printf("\n🎮 게임 실행 중...\n");
    printf("📊 거리 측정 중 (이상 감지 활성화)\n\n");
    
    switch(choice) {
        case 1:
            printf("🏀 Neverball 실행 (플레이어: %s)\n", username);
            system("neverball");
            break;
            
        case 2:
            printf("🐧 SuperTux 실행 (플레이어: %s)\n", username);
            system("supertux2");
            break;
            
        case 3:
            printf("🎿 ETR 실행 (플레이어: %s)\n", username);
            system("etracer");
            break;
    }
    
    // 센서 모니터링 중지
    stop_sensor_monitoring();
    
    printf("\n✅ 게임 종료\n");
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