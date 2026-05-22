-- ============================================================
-- GameServer DB 테이블 (household_rpg DB 내부)
-- LoginWebServer의 Users 테이블이 이미 존재해야 합니다.
-- ============================================================

USE household_rpg;

-- 플레이어 기본 정보
CREATE TABLE IF NOT EXISTS Players (
    player_id   BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    account_id  INT             NOT NULL,               -- Users.Id FK
    name        VARCHAR(50)     NOT NULL,
    level       INT             NOT NULL DEFAULT 1,
    template_id INT             NOT NULL,               -- 1=전사, 2=도적
    PRIMARY KEY (player_id),
    UNIQUE  KEY uk_name (name),
    FOREIGN KEY (account_id) REFERENCES Users(Id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 플레이어 상세 정보 (위치, 스탯)
CREATE TABLE IF NOT EXISTS PlayerDetails (
    player_id BIGINT UNSIGNED NOT NULL,
    pos_x     FLOAT           NOT NULL DEFAULT 0.0,
    pos_y     FLOAT           NOT NULL DEFAULT 0.0,
    pos_z     FLOAT           NOT NULL DEFAULT 0.0,
    rot_y     FLOAT           NOT NULL DEFAULT 0.0,
    hp        INT             NOT NULL DEFAULT 200,
    mp        INT             NOT NULL DEFAULT 100,
    exp       BIGINT          NOT NULL DEFAULT 0,
    PRIMARY KEY (player_id),
    FOREIGN KEY (player_id) REFERENCES Players(player_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
