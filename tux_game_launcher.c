#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// 게임 실행
void launch_game(int choice) {
    pid_t pid = fork();
    
    if (pid < 0) {
        printf("❌ 프로세스 생성 실패\n");
        return;
    }
    
    if (pid == 0) {
        // 자식 프로세스
        switch(choice) {
            case 1:
                printf("🏀 Neverball 실행 중...\n");
                execl("/usr/games/neverball", "neverball", NULL);
                // execl 실패시
                printf("❌ Neverball 실행 실패\n");
                exit(1);
                
            case 2:
                printf("🐧 SuperTux 실행 중...\n");
                execl("/usr/games/supertux2", "supertux2", NULL);
                // execl 실패시
                printf("❌ SuperTux 실행 실패\n");
                exit(1);
                
            case 3:
                printf("🎿 ETR 실행 중...\n");
                execl("/usr/games/etr", "etracer", NULL);
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