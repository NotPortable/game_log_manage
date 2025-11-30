/**
 * Tux Gaming System - 게임패드 로깅 프로젝트
 * 
 * 4개의 Tux 테마 게임을 실행하고 자동으로 로그를 파싱하는 프로그램
 * - Neverball: 공 굴리기 퍼즐
 * - SuperTux: 플랫포머
 * - Extreme Tux Racer: 스키 레이싱
 * - Frozen Bubble: 버블 슈터
 */

 #include <stdio.h>      // printf, fopen, fgets 등 입출력 함수
 #include <stdlib.h>     // atof, exit 등 유틸리티 함수
 #include <string.h>     // strcpy, strstr 등 문자열 처리 함수
 #include <unistd.h>     // fork, execlp 등 프로세스 함수
 #include <sys/wait.h>   // waitpid 등 프로세스 대기 함수
 
 // 상수 정의
 #define MAX_SCORES 100      // 최대 저장 가능한 스코어 개수
 #define MAX_LINE 512        // 파일에서 읽을 수 있는 한 줄 최대 길이
 #define MAX_NAME 50         // 플레이어 이름 최대 길이
 #define MAX_PATH 256        // 파일 경로 최대 길이
 
 // ============= 구조체 정의 =============
 
 /**
  * 게임 정보를 저장하는 구조체
  */
 typedef struct {
     int id;                  // 게임 번호 (1~4)
     char name[50];           // 게임 이름 (예: "Neverball")
     char command[100];       // 실행 명령어 (예: "neverball")
     char description[100];   // 게임 설명
 } Game;
 
 /**
  * Neverball 스코어 구조체
  * ~/.neverball/easy.txt 파일에서 파싱
  */
 typedef struct {
     char player_id[MAX_NAME];  // 플레이어 이름
     int time_ms;               // 완료 시간 (밀리초)
     float time_sec;            // 완료 시간 (초)
     int coins;                 // 수집한 코인 개수
     char level[100];           // 레벨 경로 (예: "map-easy/easy.sol")
 } NeverballScore;
 
 /**
  * SuperTux 스코어 구조체
  * ~/.local/share/supertux2/profile/world1.stsg 파일에서 파싱
  */
 typedef struct {
     char level_name[100];      // 레벨 이름 (예: "welcome_antarctica.stl")
     int coins_collected;       // 수집한 코인 개수
     int secrets_found;         // 발견한 비밀 개수
     float time_needed;         // 클리어 시간 (초)
     int badguys_killed;        // 처치한 적 개수
     int solved;                // 클리어 여부 (1: 클리어, 0: 미클리어)
 } SuperTuxScore;
 
 /**
  * Extreme Tux Racer 스코어 구조체
  * ~/.config/etr/highscore 파일에서 파싱
  */
 typedef struct {
     char player[MAX_NAME];     // 플레이어 이름
     char course[MAX_NAME];     // 코스 이름 (예: "bunny_hill")
     int points;                // 획득 점수
     int herrings;              // 수집한 물고기 개수
     float time;                // 완주 시간 (초)
 } ETRScore;
 
 /**
  * Frozen Bubble 스코어 구조체
  * ~/.frozen-bubble/highscores 파일에서 파싱
  */
 typedef struct {
     char name[MAX_NAME];       // 플레이어 이름
     int level;                 // 도달한 레벨
     int piclevel;              // 그래픽 레벨 (게임 내부 설정)
     float time;                // 플레이 시간 (초)
 } FrozenBubbleScore;
 
 // ============= 게임 실행 함수 =============
 
 /**
  * 게임을 실행하고 종료까지 대기하는 함수
  * 
  * @param command 실행할 게임 명령어 (예: "neverball")
  * @return 성공시 0, 실패시 -1
  * 
  * 동작 원리:
  * 1. fork()로 자식 프로세스 생성
  * 2. 자식 프로세스는 execlp()로 게임 실행
  * 3. 부모 프로세스는 waitpid()로 게임 종료 대기
  */
 int run_game(const char* command) {
     printf("\n게임을 실행합니다: %s\n", command);
     printf("게임 종료 후 스코어가 파싱됩니다.\n\n");
     
     // fork(): 현재 프로세스를 복제
     // 반환값: 부모는 자식의 PID, 자식은 0, 실패는 -1
     pid_t pid = fork();
     
     if (pid < 0) {
         // fork 실패 (메모리 부족 등)
         fprintf(stderr, "프로세스 생성 실패\n");
         return -1;
     }
     else if (pid == 0) {
         // 자식 프로세스 영역 (pid == 0일 때만 실행)
         
         // execlp(): 현재 프로세스를 새 프로그램으로 교체
         // 성공하면 이 함수는 절대 리턴하지 않음 (완전히 변신)
         // 실패하면 리턴함 (원래 코드로 계속)
         execlp(command, command, NULL);
         
         // 여기 도달 = execlp 실패 (게임을 찾을 수 없음)
         fprintf(stderr, "게임 실행 실패: %s\n", command);
         exit(1);  // 자식 프로세스 종료
     }
     else {
         // 부모 프로세스 영역 (pid > 0일 때 실행)
         
         int status;  // 자식의 종료 상태를 받을 변수
         
         // waitpid(): 자식 프로세스가 종료될 때까지 대기
         // 즉, 게임이 끝날 때까지 여기서 멈춤
         waitpid(pid, &status, 0);
         
         // WIFEXITED(): 자식이 정상 종료했는지 확인하는 매크로
         if (WIFEXITED(status)) {
             printf("\n게임이 종료되었습니다.\n");
             return 0;
         }
         else {
             fprintf(stderr, "게임이 비정상 종료되었습니다.\n");
             return -1;
         }
     }
 }
 
 // ============= Neverball 파서 =============
 
 /**
  * Neverball 로그 파일을 파싱하는 함수
  * 
  * 파일 형식:
  * level 2 1 map-easy/easy.sol
  * 2695 11 jungwooD
  * 3378 17 jungwoo
  * 
  * @param scores 파싱한 스코어를 저장할 배열
  * @param max_scores 배열의 최대 크기
  * @return 파싱한 스코어 개수
  */
 int parse_neverball(NeverballScore* scores, int max_scores) {
     // 로그 파일 경로 생성 (~/.neverball/easy.txt)
     char log_path[MAX_PATH];
     const char* home = getenv("HOME");  // 홈 디렉토리 경로 가져오기
     snprintf(log_path, sizeof(log_path), "%s/.neverball/easy.txt", home);
     
     // 파일 열기 (읽기 모드)
     FILE* fp = fopen(log_path, "r");
     if (!fp) {
         printf("Neverball 로그 파일을 찾을 수 없습니다: %s\n", log_path);
         return 0;  // 파일 없으면 0 반환
     }
     
     char line[MAX_LINE];           // 한 줄씩 읽을 버퍼
     char current_level[100] = "";  // 현재 처리 중인 레벨 이름
     int count = 0;                 // 파싱한 스코어 개수
     
     // 파일을 한 줄씩 읽기
     while (fgets(line, sizeof(line), fp) && count < max_scores) {
         
         // "level"로 시작하는 줄: 레벨 정보
         if (strncmp(line, "level", 5) == 0) {
             // "level 2 1 map-easy/easy.sol" 형식
             // 공백으로 분리해서 4번째 필드(레벨 경로)를 추출
             char* token = strtok(line, " ");  // 첫 번째 토큰 "level"
             int field = 0;
             while (token != NULL && field < 4) {
                 if (field == 3) {  // 4번째 필드 (0부터 시작)
                     token[strcspn(token, "\n")] = 0;  // 개행 문자 제거
                     strncpy(current_level, token, sizeof(current_level) - 1);
                 }
                 token = strtok(NULL, " ");  // 다음 토큰
                 field++;
             }
         }
         // 스코어 줄: "2695 11 jungwooD" 형식
         else {
             int time_ms, coins;
             char player[MAX_NAME];
             
             // sscanf: 형식에 맞춰 파싱, 성공하면 3 반환
             if (sscanf(line, "%d %d %s", &time_ms, &coins, player) == 3) {
                 // Hard/Medium/Easy는 목표 기록이므로 제외
                 if (strcmp(player, "Hard") != 0 && 
                     strcmp(player, "Medium") != 0 && 
                     strcmp(player, "Easy") != 0) {
                     
                     // 스코어 정보 저장
                     scores[count].time_ms = time_ms;
                     scores[count].time_sec = time_ms / 1000.0f;  // ms를 초로 변환
                     scores[count].coins = coins;
                     strncpy(scores[count].player_id, player, MAX_NAME - 1);
                     strncpy(scores[count].level, current_level, 99);
                     count++;
                 }
             }
         }
     }
     
     fclose(fp);  // 파일 닫기
     return count;
 }
 
 /**
  * Neverball 스코어를 화면에 출력
  * 최근 5개만 출력
  */
 void print_neverball_scores(NeverballScore* scores, int count) {
     printf("\n=== Neverball 최근 스코어 ===\n");
     
     // 최근 5개만 출력 (배열 끝에서 5개)
     int start = count > 5 ? count - 5 : 0;
     
     for (int i = start; i < count; i++) {
         printf("  플레이어: %s\n", scores[i].player_id);
         printf("  시간: %.3f초 | 코인: %d개\n", scores[i].time_sec, scores[i].coins);
         printf("  레벨: %s\n", scores[i].level);
         printf("  --------------------------------\n");
     }
 }
 
 // ============= SuperTux 파서 =============
 
 /**
  * SuperTux 로그 파일을 파싱하는 함수
  * 
  * 파일 형식: Lisp 스타일
  * ("welcome_antarctica.stl"
  *   (perfect #f)
  *   ("statistics"
  *     (coins-collected 87)
  *     (time-needed 171.9988)
  *     ...
  *   )
  *   (solved #t)
  * )
  * 
  * @param scores 파싱한 스코어를 저장할 배열
  * @param max_scores 배열의 최대 크기
  * @return 파싱한 스코어 개수
  */
 int parse_supertux(SuperTuxScore* scores, int max_scores) {
     // 로그 파일 경로 생성
     char log_path[MAX_PATH];
     const char* home = getenv("HOME");
     snprintf(log_path, sizeof(log_path), "%s/.local/share/supertux2/profile/world1.stsg", home);
     
     FILE* fp = fopen(log_path, "r");
     if (!fp) {
         printf("SuperTux 로그 파일을 찾을 수 없습니다: %s\n", log_path);
         return 0;
     }
     
     char line[MAX_LINE];
     int count = 0;
     char current_level[100] = "";
     int in_statistics = 0;  // statistics 섹션 안에 있는지 플래그
     
     while (fgets(line, sizeof(line), fp) && count < max_scores) {
         
         // 레벨 이름 찾기: ("level_name.stl" 형식
         if (strstr(line, ".stl\"")) {
             // ("welcome_antarctica.stl" 에서 레벨 이름 추출
             sscanf(line, " (\"%[^\"]", current_level);
         }
         
         // (solved #t): 레벨 클리어 표시
         if (strstr(line, "(solved #t)")) {
             scores[count].solved = 1;
             strncpy(scores[count].level_name, current_level, 99);
         }
         
         // ("statistics" 섹션 시작
         if (strstr(line, "(\"statistics\"")) {
             in_statistics = 1;
         }
         
         // statistics 섹션 안에서 데이터 파싱
         if (in_statistics && count < max_scores) {
             // (coins-collected 87)
             if (strstr(line, "coins-collected ")) {
                 sscanf(line, " (coins-collected %d)", &scores[count].coins_collected);
             }
             // (secrets-found 1)
             else if (strstr(line, "secrets-found ")) {
                 sscanf(line, " (secrets-found %d)", &scores[count].secrets_found);
             }
             // (time-needed 171.9988)
             else if (strstr(line, "time-needed ")) {
                 sscanf(line, " (time-needed %f)", &scores[count].time_needed);
             }
             // (badguys-killed 13) - "total"이 붙지 않은 것만
             else if (strstr(line, "badguys-killed ") && !strstr(line, "total")) {
                 sscanf(line, " (badguys-killed %d)", &scores[count].badguys_killed);
                 
                 // statistics 섹션 끝 - 하나의 레벨 데이터 완성
                 if (scores[count].solved) {
                     count++;
                 }
                 in_statistics = 0;
             }
         }
     }
     
     fclose(fp);
     return count;
 }
 
 /**
  * SuperTux 스코어를 화면에 출력
  */
 void print_supertux_scores(SuperTuxScore* scores, int count) {
     printf("\n=== SuperTux 클리어 레벨 ===\n");
     
     for (int i = 0; i < count && i < 5; i++) {
         printf("  레벨: %s\n", scores[i].level_name);
         printf("  시간: %.2f초 | 코인: %d개 | 적 처치: %d\n", 
                scores[i].time_needed, scores[i].coins_collected, scores[i].badguys_killed);
         printf("  비밀: %d개\n", scores[i].secrets_found);
         printf("  --------------------------------\n");
     }
 }
 
 // ============= ETR 파서 =============
 
 /**
  * Extreme Tux Racer 로그 파일을 파싱하는 함수
  * 
  * 파일 형식:
  * *[group] default [course] bunny_hill [plyr] gyumin [pts] 443 [herr] 23 [time] 30.7
  * 
  * @param scores 파싱한 스코어를 저장할 배열
  * @param max_scores 배열의 최대 크기
  * @return 파싱한 스코어 개수
  */
 int parse_etr(ETRScore* scores, int max_scores) {
     char log_path[MAX_PATH];
     const char* home = getenv("HOME");
     snprintf(log_path, sizeof(log_path), "%s/.config/etr/highscore", home);
     
     FILE* fp = fopen(log_path, "r");
     if (!fp) {
         printf("ETR 로그 파일을 찾을 수 없습니다: %s\n", log_path);
         return 0;
     }
     
     char line[MAX_LINE];
     int count = 0;
     
     while (fgets(line, sizeof(line), fp) && count < max_scores) {
         char course[MAX_NAME], player[MAX_NAME];
         int points, herrings;
         float time;
         
         // sscanf로 한 번에 모든 필드 파싱
         // %*s: 읽지만 저장하지 않음 (group 필드 무시)
         if (sscanf(line, "*[group] %*s [course] %s [plyr] %s [pts] %d [herr] %d [time] %f",
                    course, player, &points, &herrings, &time) == 5) {
             
             strncpy(scores[count].course, course, MAX_NAME - 1);
             strncpy(scores[count].player, player, MAX_NAME - 1);
             scores[count].points = points;
             scores[count].herrings = herrings;
             scores[count].time = time;
             count++;
         }
     }
     
     fclose(fp);
     return count;
 }
 
 /**
  * ETR 스코어를 화면에 출력
  */
 void print_etr_scores(ETRScore* scores, int count) {
     printf("\n=== Extreme Tux Racer 기록 ===\n");
     
     for (int i = 0; i < count && i < 5; i++) {
         printf("  플레이어: %s\n", scores[i].player);
         printf("  코스: %s\n", scores[i].course);
         printf("  시간: %.2f초 | 점수: %d점 | 물고기: %d개\n", 
                scores[i].time, scores[i].points, scores[i].herrings);
         printf("  --------------------------------\n");
     }
 }
 
 // ============= Frozen Bubble 파서 =============
 
 /**
  * Frozen Bubble 로그 파일을 파싱하는 함수
  * 
  * 파일 형식: Perl 해시
  * $HISCORES = [
  *   {
  *     'name' => 'wjddn',
  *     'level' => 1,
  *     'piclevel' => 2,
  *     'time' => '69.039'
  *   }
  * ];
  * 
  * @param scores 파싱한 스코어를 저장할 배열
  * @param max_scores 배열의 최대 크기
  * @return 파싱한 스코어 개수
  */
 int parse_frozen_bubble(FrozenBubbleScore* scores, int max_scores) {
     char log_path[MAX_PATH];
     const char* home = getenv("HOME");
     snprintf(log_path, sizeof(log_path), "%s/.frozen-bubble/highscores", home);
     
     FILE* fp = fopen(log_path, "r");
     if (!fp) {
         printf("Frozen Bubble 로그 파일을 찾을 수 없습니다: %s\n", log_path);
         return 0;
     }
     
     char line[MAX_LINE];
     int count = 0;
     
     // 한 스코어 항목의 데이터를 임시 저장
     char name[MAX_NAME] = "";
     int level = 0, piclevel = 0;
     float time = 0.0f;
     
     while (fgets(line, sizeof(line), fp) && count < max_scores) {
         
         // 'name' => 'wjddn',
         if (strstr(line, "'name'")) {
             // 작은따옴표 사이의 문자열 추출
             char* start = strchr(line, '\'');  // 첫 번째 '
             if (start) {
                 start = strchr(start + 1, '\'');  // 두 번째 '
                 if (start) {
                     start++;  // ' 다음 문자부터
                     char* end = strchr(start, '\'');  // 세 번째 '
                     if (end) {
                         int len = end - start;
                         if (len < MAX_NAME) {
                             strncpy(name, start, len);
                             name[len] = '\0';  // null 종료
                         }
                     }
                 }
             }
         }
         // 'level' => 1,
         else if (strstr(line, "'level'")) {
             sscanf(line, " 'level' => %d", &level);
         }
         // 'piclevel' => 2,
         else if (strstr(line, "'piclevel'")) {
             sscanf(line, " 'piclevel' => %d", &piclevel);
         }
         // 'time' => '69.039'
         else if (strstr(line, "'time'")) {
             char time_str[50];
             sscanf(line, " 'time' => '%[^']", time_str);  // '...' 사이 문자열
             time = atof(time_str);  // 문자열을 float로 변환
             
             // 모든 필드 수집 완료 - 배열에 저장
             if (strlen(name) > 0) {
                 strncpy(scores[count].name, name, MAX_NAME - 1);
                 scores[count].level = level;
                 scores[count].piclevel = piclevel;
                 scores[count].time = time;
                 count++;
             }
         }
     }
     
     fclose(fp);
     return count;
 }
 
 /**
  * Frozen Bubble 스코어를 화면에 출력
  */
 void print_frozen_bubble_scores(FrozenBubbleScore* scores, int count) {
     printf("\n=== Frozen Bubble 하이스코어 ===\n");
     
     for (int i = 0; i < count && i < 5; i++) {
         printf("  플레이어: %s\n", scores[i].name);
         printf("  레벨: %d | 시간: %.2f초\n", scores[i].level, scores[i].time);
         printf("  --------------------------------\n");
     }
 }
 
 // ============= 통합 로그 파싱 =============
 
 /**
  * 게임 ID에 따라 적절한 파서를 호출하는 함수
  * 
  * @param game_id 게임 번호 (1: Neverball, 2: SuperTux, 3: ETR, 4: Frozen Bubble)
  */
 void parse_game_logs(int game_id) {
     printf("\n=== 로그 파싱 중... ===\n");
     
     // switch-case: game_id 값에 따라 분기
     switch(game_id) {
         case 1: {
             // 중괄호 블록: case 안에서 변수 선언하려면 필요
             NeverballScore scores[MAX_SCORES];
             int count = parse_neverball(scores, MAX_SCORES);
             if (count > 0) {
                 print_neverball_scores(scores, count);
             }
             break;  // switch 탈출
         }
         case 2: {
             SuperTuxScore scores[MAX_SCORES];
             int count = parse_supertux(scores, MAX_SCORES);
             if (count > 0) {
                 print_supertux_scores(scores, count);
             } else {
                 printf("아직 클리어한 레벨이 없습니다.\n");
             }
             break;
         }
         case 3: {
             ETRScore scores[MAX_SCORES];
             int count = parse_etr(scores, MAX_SCORES);
             if (count > 0) {
                 print_etr_scores(scores, count);
             }
             break;
         }
         case 4: {
             FrozenBubbleScore scores[MAX_SCORES];
             int count = parse_frozen_bubble(scores, MAX_SCORES);
             if (count > 0) {
                 print_frozen_bubble_scores(scores, count);
             }
             break;
         }
         default:
             printf("알 수 없는 게임\n");
     }
     
     printf("\n");
 }
 
 // ============= 메뉴 =============
 
 /**
  * 게임 선택 메뉴를 화면에 출력
  * 
  * @param games 게임 배열
  * @param game_count 게임 개수
  */
 void show_game_menu(Game* games, int game_count) {
     printf("\n╔════════════════════════════════════════════════╗\n");
     printf("║         Tux 게임 로깅 시스템 (C)              ║\n");
     printf("╚════════════════════════════════════════════════╝\n\n");
     
     printf("플레이할 게임을 선택하세요:\n\n");
     
     // 모든 게임을 순회하며 출력
     for (int i = 0; i < game_count; i++) {
         printf("  [%d] %s\n", games[i].id, games[i].name);
         printf("      %s\n\n", games[i].description);
     }
     
     printf("  [0] 종료\n\n");
     printf("선택: ");
 }
 
 // ============= 메인 함수 =============
 
 /**
  * 프로그램의 시작점
  * 
  * 동작 흐름:
  * 1. 게임 목록 초기화
  * 2. 무한 루프로 메뉴 표시
  * 3. 사용자 입력 받기
  * 4. 게임 실행
  * 5. 로그 파싱 및 출력
  * 6. 다시 메뉴로
  */
 int main() {
     // 게임 배열 초기화
     // {ID, 이름, 명령어, 설명}
     Game games[] = {
         {1, "Neverball", "neverball", "🎱 공 굴리기 퍼즐 게임"},
         {2, "SuperTux", "supertux2", "🐧 슈퍼마리오 스타일 플랫포머"},
         {3, "Extreme Tux Racer", "etr", "⛷️  펭귄 스키 레이싱"},
         {4, "Frozen Bubble", "frozen-bubble", "🫧 버블 슈터 퍼즐"}
     };
     
     // sizeof(games): 전체 배열의 바이트 크기
     // sizeof(Game): 구조체 하나의 바이트 크기
     // 나누면 배열 원소 개수 (4개)
     int game_count = sizeof(games) / sizeof(Game);
     
     // 시작 배너 출력
     printf("╔════════════════════════════════════════════════╗\n");
     printf("║              Tux Gaming System                 ║\n");
     printf("║          게임패드 로깅 프로젝트                ║\n");
     printf("╚════════════════════════════════════════════════╝\n");
     
     // 메인 루프: 사용자가 0을 입력할 때까지 반복
     while (1) {
         // 1. 메뉴 표시
         show_game_menu(games, game_count);
         
         // 2. 사용자 입력 받기
         int choice;
         // scanf: 정수 입력, 성공하면 1 반환
         if (scanf("%d", &choice) != 1) {
             printf("잘못된 입력입니다.\n");
             // 입력 버퍼 비우기 (잘못된 입력 제거)
             while (getchar() != '\n');
             continue;  // 다시 메뉴로
         }
         
         // 3. 종료 처리
         if (choice == 0) {
             printf("\n프로그램을 종료합니다.\n");
             printf("즐거운 게임이었습니다! 🐧\n\n");
             break;  // while 루프 탈출 -> 프로그램 종료
         }
         
         // 4. 선택한 게임 찾기
         int game_index = -1;  // -1은 "못 찾음"
         for (int i = 0; i < game_count; i++) {
             if (games[i].id == choice) {
                 game_index = i;  // 찾으면 인덱스 저장
                 break;  // for 루프 탈출
             }
         }
         
         // 5. 잘못된 선택 처리
         if (game_index == -1) {
             printf("잘못된 선택입니다. 1~%d 중에서 선택하세요.\n", game_count);
             continue;  // 다시 메뉴로
         }
         
         // 6. 게임 실행
         if (run_game(games[game_index].command) == 0) {
             // 게임이 정상 종료됨 -> 로그 파싱
             parse_game_logs(games[game_index].id);
         }
         
         // 7. 계속하려면 Enter 대기
         printf("\n계속하려면 Enter를 누르세요...");
         getchar();  // scanf 후 남은 개행 문자 제거
         getchar();  // 실제 Enter 키 입력 대기
     }
     
     return 0;  // 프로그램 정상 종료
 }