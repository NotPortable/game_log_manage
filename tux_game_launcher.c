#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct {
    int id;
    char name[50];
    char command[100];
    char description[100];
} Game;

/**
 * 게임 실행
 */
int run_game(const char* command) {
    printf("\n게임을 실행합니다: %s\n", command);
    printf("게임 종료 후 스코어가 파싱됩니다.\n\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        fprintf(stderr, "프로세스 생성 실패\n");
        return -1;
    }
    else if (pid == 0) {
        // 자식 프로세스: 게임 실행
        execlp(command, command, NULL);
        // execlp 실패 시
        fprintf(stderr, "게임 실행 실패: %s\n", command);
        exit(1);
    }
    else {
        // 부모 프로세스: 게임 종료 대기
        int status;
        waitpid(pid, &status, 0);
        
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

/**
 * 게임 선택 메뉴 표시
 */
void show_game_menu(Game* games, int game_count) {
    printf("\n╔════════════════════════════════════════════════╗\n");
    printf("║         Tux 게임 로깅 시스템 (C)              ║\n");
    printf("╚════════════════════════════════════════════════╝\n\n");
    
    printf("플레이할 게임을 선택하세요:\n\n");
    
    for (int i = 0; i < game_count; i++) {
        printf("  [%d] %s\n", games[i].id, games[i].name);
        printf("      %s\n\n", games[i].description);
    }
    
    printf("  [0] 종료\n\n");
    printf("선택: ");
}

/**
 * 게임별 로그 파싱 (추후 구현)
 */
void parse_game_logs(int game_id) {
    printf("\n=== 로그 파싱 중... ===\n");
    
    switch(game_id) {
        case 1:
            printf("Neverball 로그 파싱 (추후 구현)\n");
            // TODO: Neverball 로그 파싱
            break;
        case 2:
            printf("SuperTux 로그 파싱 (추후 구현)\n");
            // TODO: SuperTux 로그 파싱
            break;
        case 3:
            printf("Extreme Tux Racer 로그 파싱 (추후 구현)\n");
            // TODO: ETR 로그 파싱
            break;
        case 4:
            printf("Frozen Bubble 로그 파싱 (추후 구현)\n");
            // TODO: Frozen Bubble 로그 파싱
            break;
        default:
            printf("알 수 없는 게임\n");
    }
    
    printf("\n");
}

int main() {
    // 4개 게임 정의
    Game games[] = {
        {1, "Neverball", "neverball", "🎱 공 굴리기 퍼즐 게임"},
        {2, "SuperTux", "supertux2", "🐧 슈퍼마리오 스타일 플랫포머"},
        {3, "Extreme Tux Racer", "etracer", "⛷️  펭귄 스키 레이싱"},
        {4, "Frozen Bubble", "frozen-bubble", "🫧 버블 슈터 퍼즐"}
    };
    int game_count = sizeof(games) / sizeof(Game);
    
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║              Tux Gaming System                 ║\n");
    printf("║          게임패드 로깅 프로젝트                ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    
    while (1) {
        // 게임 메뉴 표시
        show_game_menu(games, game_count);
        
        int choice;
        if (scanf("%d", &choice) != 1) {
            printf("잘못된 입력입니다.\n");
            while (getchar() != '\n'); // 입력 버퍼 비우기
            continue;
        }
        
        // 종료
        if (choice == 0) {
            printf("\n프로그램을 종료합니다.\n");
            printf("즐거운 게임이었습니다! 🐧\n\n");
            break;
        }
        
        // 게임 선택 확인
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
        
        // 게임 실행
        if (run_game(games[game_index].command) == 0) {
            // 게임 종료 후 로그 파싱
            parse_game_logs(games[game_index].id);
        }
        
        printf("\n계속하려면 Enter를 누르세요...");
        getchar(); // 이전 입력의 개행 제거
        getchar(); // Enter 대기
    }
    
    return 0;
}