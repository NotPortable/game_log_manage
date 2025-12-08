#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// SuperTux 사용자 이름 저장 파일
const char* SUPERTUX_USERNAME_FILE = "/tmp/supertux_username.txt";

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

// 게임 실행
void launch_game(int choice) {
    char username[100];
    printf("\n사용자 이름을 입력하세요: ");
    scanf("%s", username);
    
    // SuperTux의 경우 사용자 이름 파일에 저장
    if (choice == 2) {
        save_username_to_file(username);
    }
    
    pid_t pid = fork();
    
    if (pid < 0) {
        printf("❌ 프로세스 생성 실패\n");
        return;
    }
    
    if (pid == 0) {
        // 자식 프로세스
        switch(choice) {
            case 1:
                printf("🏀 Neverball 실행 (플레이어: %s)\n", username);
                execl("/usr/games/neverball", "neverball", NULL);
                // execl 실패시
                printf("❌ Neverball 실행 실패\n");
                exit(1);
                
            case 2:
                printf("🐧 SuperTux 실행 (플레이어: %s)\n", username);
                execl("/usr/games/supertux2", "supertux2", NULL);
                // execl 실패시
                printf("❌ SuperTux 실행 실패\n");
                exit(1);
                
            case 3:
                printf("🎿 ETR 실행 (플레이어: %s)\n", username);
                execl("/usr/games/etracer", "etracer", NULL);
                // execl 실패시
                printf("❌ ETR 실행 실패\n");
                exit(1);
        }
    } else {
        // 부모 프로세스 - 게임 종료 대기
        int status;
        waitpid(pid, &status, 0);
        printf("\n✅ 게임 종료\n");
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
    printf("║  [0] 🚪 종료                           ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n선택: ");
}

int main() {
    int choice;
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║          NotPortable 게임 런처          ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    while (1) {
        show_menu();
        scanf("%d", &choice);
        
        switch(choice) {
            case 1:
            case 2:
            case 3:
                launch_game(choice);
                break;
                
            case 0:
                printf("\n👋 프로그램을 종료합니다.\n");
                return 0;
                
            default:
                printf("\n❌ 잘못된 선택입니다.\n");
        }
    }
    
    return 0;
}