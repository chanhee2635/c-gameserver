#pragma once

struct MoveResult {
	Vector<ZoneRef> leaveZones;
	Vector<ZoneRef> enterZones;
	Vector<ZoneRef> keepZones;
};

class World : public JobQueue
{
public:
	void Init();
	void Start();
	void PlayerEnterToGame(GameSessionRef session, SummaryDataRef summary, StatData stat);
	void EnterCreature(CreatureRef creature);
	void LeaveCreature(CreatureRef creature);
	ZoneRef GetZoneByPos(Vector3 pos);
	ZoneRef GetZoneById(int32 id);
	Vector<ZoneRef> GetAdjacentZones(Vector3 pos);
	/*
	* @brief 두 Zone 간 이동에 따른 시야 변화 결과(MoveResult)를 획득
	* @details
	* 1. 미리 계산된 MoveTable을 우선 참조하여 런타임 연산 최소화
	* 2. 테이블에 없는 예외적인 이동 거리일 경우, 실시간 계산으로 보완
	*/
	MoveResultRef GetMoveResult(ZoneRef oldZone, ZoneRef newZone);

private:
	bool IsTooFar(int32 oldId, int32 newId);
	/* 
	* @brief 월드를 구성하는 개별 게임 씬(GameScene)들을 생성하고 초기화 
	* @details
	* 1. 심리스 월드 분할: 거대한 월드를 분할하여 관리
	* 2. 부하 분산(Load Balancing): 각 Scene은 독립적인 JobQueue와 스레드를 가질 수 있도록 설계
	* 3. 병렬 처리: CPU 코어 자원을 효율적으로 활용하기 위해 Scene 단위로 병렬 업데이트 가능
	*/
	void CreateScene();
	/*
	* @brief 월드를 격자(Grid) 형태의 Zone으로 분할하고, 각 Zone을 적절한 Scene에 배정
	* @details
	* 1. 공간 분할: 맵을 Zone 단위로 나눠 시야 처리(Broadcast)
	* 2. 부하 분산: Grid를 Scene 개수에 맞춰 등분하여, Zone 연산을 Scene 스레드가 전담
	* 3. 확장성: MapSize와 ZoneSize에 따라 동적으로 Grid 생성
	*/
	void CreateZone();
	/*
	* @brief Zone 간 이동 시 발생하는 시야 추가/삭제 대상을 미리 계산하여 테이블화
	* @details
	* 1. 런타임 최적화: 유저 이동 시마다 9개 존을 탐색하는 연산을 제거하고, 메모리 참조(O(1))로 대체
	*/
	void CreateMoveTable();
	/*
	* @brief 이동 전/후의 주변 Zone 리스트를 비교하여 시야 변화 결과를 계산
	* @details New, Keep, Old Zone을 분리
	*/
	MoveResultRef GetCalculateMoveResult(ZoneRef oldZone, ZoneRef newZone);
	void SpawnMonsters();

private:
	WorldConfig _config;

	Vector<GameSceneRef> _scenes;
	Vector<ZoneRef> _zones;
	Vector<Vector<MoveResultRef>> _moveTable;
};

extern shared_ptr<World> GWorld;

