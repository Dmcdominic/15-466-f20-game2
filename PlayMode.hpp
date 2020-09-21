#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed, bool *quit_asap) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- settings -----
	const float PLAYER_X_MIN = -8.0f;
	const float PLAYER_X_MAX = 100.0f;
	const float PLAYER_Y_MIN = -22.0f;
	const float PLAYER_Y_MAX = 25.0f;

	const float GATE_RAD = 3.0f;

	const float PEG_RAD = 3.0f;

	const float PICKUP_RAD = 3.0f;
	const glm::vec3 TILE_PICKUP_POS = glm::vec3(2.7f, 0.0f, 1.3f);
	const glm::quat TILE_PICKUP_ROTATION = glm::vec3(1.0f, 0.0f, M_PI * 0.5f);
	glm::quat TILE_STD_ROTATION = glm::vec3(0.0f, 0.0f, 90.0f);

	const float GATE_SPEED = 4.0f;
	float GATE_MIN_Z;
	float GATE_RAISE_HEIGHT = 20.0f;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, LMB, RMB;

	bool quit_pressed = false;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	/*Scene::Transform *hip = nullptr;
	Scene::Transform *upper_leg = nullptr;
	Scene::Transform *lower_leg = nullptr;
	glm::quat hip_base_rotation;
	glm::quat upper_leg_base_rotation;
	glm::quat lower_leg_base_rotation;
	float wobble = 0.0f;*/
	
	//camera:
	Scene::Camera *camera = nullptr;

	//player:
	Scene::Transform *player = nullptr;
	Scene::Transform *pickupPt = nullptr;

	enum class COLOR{
		NO_COLOR, RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE
	};

	struct Peg;

	//tiles:
	struct Tile {
		Scene::Transform *transform = nullptr;
		Peg *peg = nullptr;
		COLOR color = COLOR::NO_COLOR;
	};
	std::vector<Tile> tiles;
	Tile *carried_tile = nullptr;

	//pegs:
	struct Peg {
		Scene::Transform *transform = nullptr;
		Tile *tile = nullptr;
		COLOR color = COLOR::NO_COLOR;
	};
	std::vector<Peg> pegs;

	//gates:
	struct Gate {
		Scene::Transform *transform = nullptr;
		bool open = false;
	};
	std::vector<Gate> gates;

};
