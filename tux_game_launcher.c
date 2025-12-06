/**
 * Tux Gaming System - 게임 런처 with MPU-6050
 * 
 * 3개의 게임을 실행하고 MPU-6050으로 진동 감지
 * lgpio 라이브러리 사용 (최신)
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/wait.h>
 #include <lgpio.h>
 #include <math.h>
 #include <pthread.h>
 #include <string.h>
 
 // MPU-6050 I2C 주소 및 레지스터
 #define MPU6050_ADDR 0x68
 #define PWR_MGMT_1   0x6B
 #define ACCEL_XOUT_H 0x3B
 #define GYRO_XOUT_H  0x43
 
 // I2C 버스
 #define I2C_BUS 1
 
 // 진동 감지 임계값
 #define VIBRATION_THRESHOLD 2000
 
 // 게임 정보 구조체
 typedef struct {
     int id;
     char name[50];
     char command[100];
     char description[100];
 } Game;
 
 // MPU-6050 데이터 구조체
 typedef struct {
     int16_t accel_x, accel_y, accel_z;
     int16_t gyro_x, gyro_y, gyro_z;
     float magnitude;
 } MPU6050Data;
 
 // 전역 변수
 int gpio_chip = -1;
 int i2c_handle = -1;
 int vibration_detected = 0;
 pthread_t vibration_thread;
 int monitoring_active = 0;
 
 /**
  * 16비트 값 읽기 (빅엔디안)
  */
 int16_t read_word_2c(int reg) {
     char buf[2];
     
     // 2바이트 읽기
     // lgI2cReadI2CBlockData(handle, register, buffer, count)
     if (lgI2cReadI2CBlockData(i2c_handle, reg, buf, 2) != 2) {
         return 0;
     }
     
     int val = (buf[0] << 8) | buf[1];
     
     // 2의 보수 변환
     if (val >= 0x8000) {
         return -((65535 - val) + 1);
     } else {
         return val;
     }
 }
 
 /**
  * MPU-6050 초기화
  */
 int init_mpu6050() {
     // GPIO 칩 열기
     gpio_chip = lgGpiochipOpen(0);
     if (gpio_chip < 0) {
         fprintf(stderr, "GPIO 칩 열기 실패\n");
         return -1;
     }
     
     // I2C 장치 열기
     // lgI2cOpen(i2c_bus, i2c_address, flags)
     i2c_handle = lgI2cOpen(I2C_BUS, MPU6050_ADDR, 0);
     if (i2c_handle < 0) {
         fprintf(stderr, "MPU-6050 연결 실패 (I2C)\n");
         fprintf(stderr, "I2C가 활성화되었는지 확인: sudo raspi-config\n");
         fprintf(stderr, "센서 연결 확인: VCC(3.3V), GND, SDA(Pin3), SCL(Pin5)\n");
         lgGpiochipClose(gpio_chip);
         return -1;
     }
     
     // MPU-6050 Wake up (PWR_MGMT_1 = 0)
     if (lgI2cWriteByteData(i2c_handle, PWR_MGMT_1, 0) < 0) {
         fprintf(stderr, "MPU-6050 초기화 명령 실패\n");
         lgI2cClose(i2c_handle);
         lgGpiochipClose(gpio_chip);
         return -1;
     }
     
     usleep(100000); // 100ms 대기
     
     printf("✓ MPU-6050 초기화 완료\n");
     return 0;
 }
 
 /**
  * MPU-6050 데이터 읽기
  */
 void read_mpu6050(MPU6050Data* data) {
     data->accel_x = read_word_2c(ACCEL_XOUT_H);
     data->accel_y = read_word_2c(ACCEL_XOUT_H + 2);
     data->accel_z = read_word_2c(ACCEL_XOUT_H + 4);
     data->gyro_x = read_word_2c(GYRO_XOUT_H);
     data->gyro_y = read_word_2c(GYRO_XOUT_H + 2);
     data->gyro_z = read_word_2c(GYRO_XOUT_H + 4);
     
     // 가속도 크기 계산
     data->magnitude = sqrt(
         data->accel_x * data->accel_x +
         data->accel_y * data->accel_y +
         data->accel_z * data->accel_z
     );
 }
 
 /**
  * 진동 모니터링 스레드
  */
 void* vibration_monitor(void* arg) {
     MPU6050Data data;
     MPU6050Data prev_data = {0};
     
     printf("진동 모니터링 시작...\n");
     
     while (monitoring_active) {
         read_mpu6050(&data);
         
         // 이전 값과의 차이 계산
         float delta = fabs(data.magnitude - prev_data.magnitude);
         
         // 진동 감지
         if (delta > VIBRATION_THRESHOLD) {
             vibration_detected = 1;
             printf("🔴 진동 감지! (강도: %.2f)\n", delta);
         }
         
         prev_data = data;
         usleep(50000); // 50ms마다 체크
     }
     
     return NULL;
 }
 
 /**
  * 진동 모니터링 시작
  */
 void start_vibration_monitoring() {
     vibration_detected = 0;
     monitoring_active = 1;
     
     if (pthread_create(&vibration_thread, NULL, vibration_monitor, NULL) != 0) {
         fprintf(stderr, "진동 모니터링 스레드 생성 실패\n");
     }
 }
 
 /**
  * 진동 모니터링 중지
  */
 void stop_vibration_monitoring() {
     monitoring_active = 0;
     pthread_join(vibration_thread, NULL);
     
     if (vibration_detected) {
         printf("✓ 게임 중 진동이 감지되었습니다.\n");
     } else {
         printf("✓ 게임 중 진동이 감지되지 않았습니다.\n");
     }
 }
 
 /**
  * MPU-6050 상태 확인 및 출력
  */
 void check_mpu6050_status() {
     MPU6050Data data;
     read_mpu6050(&data);
     
     printf("\n=== MPU-6050 센서 상태 ===\n");
     printf("가속도계:\n");
     printf("  X: %6d  Y: %6d  Z: %6d\n", data.accel_x, data.accel_y, data.accel_z);
     printf("자이로스코프:\n");
     printf("  X: %6d  Y: %6d  Z: %6d\n", data.gyro_x, data.gyro_y, data.gyro_z);
     printf("가속도 크기: %.2f\n", data.magnitude);
     printf("========================\n\n");
 }
 
 /**
  * 게임 실행 함수
  */
 int run_game(const char* command) {
     printf("\n게임을 실행합니다: %s\n", command);
     printf("게임을 플레이하세요!\n");
     
     if (i2c_handle >= 0) {
         printf("진동 감지가 활성화됩니다...\n\n");
         start_vibration_monitoring();
     } else {
         printf("\n");
     }
     
     pid_t pid = fork();
     
     if (pid < 0) {
         fprintf(stderr, "프로세스 생성 실패\n");
         if (i2c_handle >= 0) {
             stop_vibration_monitoring();
         }
         return -1;
     }
     else if (pid == 0) {
         // 자식: 게임 실행
         execlp(command, command, NULL);
         fprintf(stderr, "게임 실행 실패: %s\n", command);
         exit(1);
     }
     else {
         // 부모: 게임 종료 대기
         int status;
         waitpid(pid, &status, 0);
         
         // 진동 모니터링 중지
         if (i2c_handle >= 0) {
             stop_vibration_monitoring();
         }
         
         if (WIFEXITED(status)) {
             printf("\n게임이 종료되었습니다.\n");
             printf("로그는 Spring Boot에서 자동으로 처리됩니다.\n");
             return 0;
         }
         else {
             fprintf(stderr, "게임이 비정상 종료되었습니다.\n");
             return -1;
         }
     }
 }
 
 /**
  * 게임 메뉴 출력
  */
 void show_game_menu(Game* games, int game_count) {
     printf("\n╔════════════════════════════════════════════════╗\n");
     printf("║         Tux 게임 로깅 시스템 (C)              ║\n");
     if (i2c_handle >= 0) {
         printf("║            with MPU-6050 진동 감지            ║\n");
     }
     printf("╚════════════════════════════════════════════════╝\n\n");
     
     printf("플레이할 게임을 선택하세요:\n\n");
     
     for (int i = 0; i < game_count; i++) {
         printf("  [%d] %s\n", games[i].id, games[i].name);
         printf("      %s\n\n", games[i].description);
     }
     
     if (i2c_handle >= 0) {
         printf("  [9] MPU-6050 상태 확인\n");
     }
     printf("  [0] 종료\n\n");
     printf("선택: ");
 }
 
 /**
  * 메인 함수
  */
 int main() {
     // MPU-6050 초기화
     printf("MPU-6050 센서 초기화 중...\n");
     if (init_mpu6050() == -1) {
         fprintf(stderr, "\n⚠️  MPU-6050 초기화 실패\n");
         fprintf(stderr, "진동 감지 기능 없이 계속 진행합니다.\n\n");
         i2c_handle = -1;
         gpio_chip = -1;
     }
     
     // 3개 게임 정의
     Game games[] = {
         {1, "Neverball", "neverball", "🎱 공 굴리기 퍼즐 게임"},
         {2, "SuperTux", "supertux2", "🐧 슈퍼마리오 스타일 플랫포머"},
         {3, "Extreme Tux Racer", "etr", "⛷️  펭귄 스키 레이싱"}
     };
     int game_count = sizeof(games) / sizeof(Game);
     
     printf("╔════════════════════════════════════════════════╗\n");
     printf("║              Tux Gaming System                 ║\n");
     printf("║          게임패드 로깅 프로젝트                ║\n");
     printf("╚════════════════════════════════════════════════╝\n");
     
     while (1) {
         show_game_menu(games, game_count);
         
         int choice;
         if (scanf("%d", &choice) != 1) {
             printf("잘못된 입력입니다.\n");
             while (getchar() != '\n');
             continue;
         }
         
         if (choice == 0) {
             printf("\n프로그램을 종료합니다.\n");
             printf("즐거운 게임이었습니다! 🐧\n\n");
             break;
         }
         
         // MPU-6050 상태 확인
         if (choice == 9 && i2c_handle >= 0) {
             check_mpu6050_status();
             printf("\n계속하려면 Enter를 누르세요...");
             getchar();
             getchar();
             continue;
         }
         
         int game_index = -1;
         for (int i = 0; i < game_count; i++) {
             if (games[i].id == choice) {
                 game_index = i;
                 break;
             }
         }
         
         if (game_index == -1) {
             printf("잘못된 선택입니다. 1~%d 중에서 선택하세요.\n", game_count);
             continue;
         }
         
         run_game(games[game_index].command);
         
         printf("\n계속하려면 Enter를 누르세요...");
         getchar();
         getchar();
     }
     
     // 종료 시 정리
     if (i2c_handle >= 0) {
         lgI2cClose(i2c_handle);
     }
     if (gpio_chip >= 0) {
         lgGpiochipClose(gpio_chip);
     }
     
     return 0;
 }