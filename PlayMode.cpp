#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint room_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > room_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("room.pnct"));
	room_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > room_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("room.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = room_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = room_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*room_scene) {
	//get pointers to leg for convenience:
	/*for (auto &transform : scene.transforms) {
		if (transform.name == "Hip.FL") hip = &transform;
		else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
		else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	}
	if (hip == nullptr) throw std::runtime_error("Hip not found.");
	if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	hip_base_rotation = hip->rotation;
	upper_leg_base_rotation = upper_leg->rotation;
	lower_leg_base_rotation = lower_leg->rotation;*/

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//init tiles vector
	tiles = std::vector<Tile>(6);
	tiles[0].color = COLOR::RED;
	tiles[1].color = COLOR::ORANGE;
	tiles[2].color = COLOR::YELLOW;
	tiles[3].color = COLOR::GREEN;
	tiles[4].color = COLOR::PINK;
	tiles[5].color = COLOR::PURPLE;

	tiles[2].hanging = true; // If these are changed/added, need to update hangingStr position setting at end of update()
	tiles[3].hanging = true;

	//init pegs vector
	pegs = std::vector<Peg>(6);
	pegs[0].color = COLOR::NO_COLOR;
	pegs[1].color = COLOR::PINK;
	pegs[2].color = COLOR::RED;
	pegs[3].color = COLOR::ORANGE;
	pegs[4].color = COLOR::YELLOW;
	pegs[5].color = COLOR::GREEN;

	//init gates vector
	gates = std::vector<Gate>(3);

	//get pointer to player, tiles, pegs, and gates
	for (auto& transform : scene.transforms) {
		if (transform.name == "Player") player = &transform;
		if (transform.name == "PickupPt") pickupPt = &transform;
		if (transform.name == "HangingStr0") hangingStr0 = &transform;
		if (transform.name == "HangingStr1") hangingStr1 = &transform;
		if (transform.name == "TileRed") tiles[0].transform = &transform;
		if (transform.name == "TileOrange") tiles[1].transform = &transform;
		if (transform.name == "TileYellow") tiles[2].transform = &transform;
		if (transform.name == "TileGreen") tiles[3].transform = &transform;
		if (transform.name == "TilePink") tiles[4].transform = &transform;
		if (transform.name == "TilePurple") tiles[5].transform = &transform;
		if (transform.name == "Peg0") pegs[0].transform = &transform;
		if (transform.name == "Peg1") pegs[1].transform = &transform;
		if (transform.name == "Peg2.0") pegs[2].transform = &transform;
		if (transform.name == "Peg2.1") pegs[3].transform = &transform;
		if (transform.name == "Peg2.2") pegs[4].transform = &transform;
		if (transform.name == "Peg2.3") pegs[5].transform = &transform;
		if (transform.name == "Gate0") gates[0].transform = &transform;
		if (transform.name == "Gate1") gates[1].transform = &transform;
		if (transform.name == "Gate2") gates[2].transform = &transform;
	}
	// Check for missing transforms
	if (player == nullptr) throw std::runtime_error("Player not found.");
	if (pickupPt == nullptr) throw std::runtime_error("pickupPt not found.");
	if (hangingStr0 == nullptr) throw std::runtime_error("hangingStr0 not found.");
	if (hangingStr1 == nullptr) throw std::runtime_error("hangingStr1 not found.");
	// Check for missing transforms in the vectors
	for (auto tileIter = tiles.begin(); tileIter != tiles.end(); tileIter++) {
		if (tileIter->transform == nullptr) {
			std::cerr << "missing tile " << (tileIter - tiles.begin()) << std::endl;
			throw std::runtime_error("tile not found.");
		}
	}
	for (auto pegIter = pegs.begin(); pegIter != pegs.end(); pegIter++) {
		if (pegIter->transform == nullptr) {
			std::cerr << "missing peg " << (pegIter - pegs.begin()) << std::endl;
			throw std::runtime_error("peg not found.");
		}
	}
	for (auto gateIter = gates.begin(); gateIter != gates.end(); gateIter++) {
		if (gateIter->transform == nullptr) {
			std::cerr << "missing gate " << (gateIter - gates.begin()) << std::endl;
			throw std::runtime_error("gate not found.");
		}
	}
	
	// Get hanging tile offsets
	for (auto tileIter = tiles.begin(); tileIter != tiles.end(); tileIter++) {
		if (tileIter->hanging) {
			tileIter->hanging_offset = tileIter->transform->position - camera->transform->position;
		}
	}

	// init some standards
	TILE_STD_ROTATION = tiles[0].transform->rotation;
	GATE_MIN_Z = gates[0].transform->position.z;
	HANGING_STR_OFFSET = hangingStr0->position - tiles[2].transform->position;
}

PlayMode::~PlayMode() {}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) { // LEFT button
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) { // RIGHT button
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) { // UP button
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) { // DOWN button
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) { // PICK UP button
			// Drop tile, if you're already carrying one
			if (carried_tile != nullptr) {
				// Check if you're close enough to place it on a peg
				for (auto pegIter = pegs.begin(); pegIter != pegs.end(); pegIter++) {
					if (pegIter->tile != nullptr) {
						continue;
					}
					glm::vec3 pegPos = glm::vec3(pegIter->transform->make_local_to_world()[3]);
					glm::vec3 carriedTilePos = glm::vec3(carried_tile->transform->make_local_to_world()[3]);
					float dist = glm::length(pegPos - carriedTilePos);
					//std::cout << "dist: " << dist << std::endl;
					if (dist < PEG_RAD) {
						pegIter->tile = carried_tile;
						carried_tile->peg = &(*pegIter);
						//carried_tile->transform->position = pegPos;
						carried_tile->transform->position = glm::vec3(0.0f);
						carried_tile->transform->rotation = TILE_STD_ROTATION;
						carried_tile->transform->parent = carried_tile->peg->transform;
						carried_tile = nullptr;
						return true;
					}
				}
				// Otherwise, just drop it
				carried_tile->transform->position = glm::vec3(carried_tile->transform->make_local_to_world()[3]);
				carried_tile->transform->rotation = TILE_STD_ROTATION;
				carried_tile->transform->parent = nullptr;
				carried_tile = nullptr;
				return true;
			}
			// Pickup nearby tile, if there is one
			for (auto tilesIter = tiles.begin(); tilesIter != tiles.end(); tilesIter++) {
				glm::vec3 tilePos = glm::vec3(tilesIter->transform->make_local_to_world()[3]);
				glm::vec3 pickupPtPos = glm::vec3(pickupPt->make_local_to_world()[3]);
				float dist = glm::length(tilePos - pickupPtPos);
				//std::cout << "dist: " << dist << std::endl;
				if (dist < PICKUP_RAD) {
					carried_tile = &(*tilesIter);
					if (carried_tile->peg != nullptr) {
						carried_tile->peg->tile = nullptr;
						carried_tile->peg = nullptr;
					}
					carried_tile->transform->position = TILE_PICKUP_POS;
					carried_tile->transform->rotation = TILE_PICKUP_ROTATION;
					carried_tile->transform->parent = pickupPt;
					carried_tile->hanging = false;
					return true;
				}
			}
		} else if (evt.key.keysym.sym == SDLK_q) { // QUIT button
			quit_pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
		if (evt.button.button == SDL_BUTTON_LEFT) {
			LMB.downs += 1;
			LMB.pressed = true;
			return true;
		} else if (evt.button.button == SDL_BUTTON_RIGHT) {
			RMB.downs += 1;
			RMB.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONUP) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
		if (evt.button.button == SDL_BUTTON_LEFT) {
			LMB.pressed = false;
			return true;
		} else if (evt.button.button == SDL_BUTTON_RIGHT) {
			RMB.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed, bool *quit_asap) {

	if (quit_pressed) {
		*quit_asap = true;
		return;
	}

	//slowly rotates through [0,1):
	/*wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	hip->rotation = hip_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);*/

	//set gates open/closed according to pegs, and update their position
	{
		gates[0].open = (pegs[0].tile != nullptr);
		gates[1].open = (pegs[1].tile != nullptr && pegs[1].tile->color == pegs[1].color);
		gates[2].open = (pegs[2].tile != nullptr && pegs[2].tile->color == pegs[2].color && 
			               pegs[3].tile != nullptr && pegs[3].tile->color == pegs[3].color &&
			               pegs[4].tile != nullptr && pegs[4].tile->color == pegs[4].color &&
			               pegs[5].tile != nullptr && pegs[5].tile->color == pegs[5].color);
		// Update gates position
		for (auto gateIter = gates.begin(); gateIter != gates.end(); gateIter++) {
			if (gateIter->open) {
				gateIter->transform->position.z = std::min(GATE_MIN_Z + GATE_RAISE_HEIGHT, gateIter->transform->position.z + GATE_SPEED * elapsed);
			} else {
				gateIter->transform->position.z = std::max(GATE_MIN_Z, gateIter->transform->position.z - GATE_SPEED * 1.5f * elapsed);
			}
		}
	}

	// spin any tiles that are on pegs
	{
		spinning_tile_rot.x += elapsed;
		for (auto pegIter = pegs.begin(); pegIter != pegs.end(); pegIter++) {
			if (pegIter->tile != nullptr) {
				if (pegIter->color == COLOR::NO_COLOR || pegIter->color == pegIter->tile->color) {
					// color is correct
					pegIter->tile->transform->rotation = spinning_tile_rot;
				} else {
					// color is incorrect

				}
			}
		}
	}

	//move camera:
	{
		//combine inputs into a move:
		constexpr float CameraSpeed = 12.0f;
		glm::vec2 move = glm::vec2(0.0f);
		/*if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;*/

		if (LMB.pressed && !RMB.pressed) move.x = -1.0f;
		if (!LMB.pressed && RMB.pressed) move.x = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * CameraSpeed * elapsed;

		// Absolute camera movement
		camera->transform->position += move.x * glm::vec3(1.0f, 0, 0) + move.y * glm::vec3(0, 1.0f, 0);

		// Relative camera movement
		//glm::mat4x3 frame = camera->transform->make_local_to_parent();
		//glm::vec3 right = frame[0];
		// //glm::vec3 up = frame[1];
		//glm::vec3 forward = -frame[2];

		//camera->transform->position += move.x * right + move.y * forward;

		// Bound the camera's position by the same as the player's X min & max
		camera->transform->position.x = std::min(PLAYER_X_MAX, std::max(PLAYER_X_MIN, camera->transform->position.x));
	}

	//move player:
	{
		constexpr float PlayerSpeed = 12.0f;
		constexpr float PlayerAngularSpeed = 0.05f;
		//combine inputs into a move:
		glm::vec2 move = glm::vec2(0.0f);
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//rotation:
		glm::vec3 rot = glm::vec3(0.0f);
		if (left.pressed && !right.pressed) rot.z = 1.0f;
		if (!left.pressed && right.pressed) rot.z =-1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;
		//scale rot by PlayerAngularSpeed
		rot *= PlayerAngularSpeed;

		// Absolute player movement
		/*player->position += move.x * glm::vec3(1.0f, 0, 0) + move.y * glm::vec3(0, 1.0f, 0);
		player->rotation *= glm::angleAxis(rot.z, glm::vec3(0.0f, 0.0f, 1.0f));*/

		// Relative player movement
		glm::mat4x3 frame = player->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		//glm::vec3 forward = -frame[2];

		player->position += move.y * right;
		player->rotation *= glm::angleAxis(rot.z, glm::vec3(0.0f, 0.0f, 1.0f));

		//If they've passed the third gate, float them to heaven
		if (gates[2].open) {
			player->position.z += elapsed * PlayerSpeed * 0.2f;
		}

		//Bound the player's position
		player->position.x = std::min(PLAYER_X_MAX, std::max(PLAYER_X_MIN, player->position.x));
		player->position.y = std::min(PLAYER_Y_MAX, std::max(PLAYER_Y_MIN, player->position.y));
	}

	//bound the camera AND player to the gates
	{
		for (auto gateIter = gates.begin(); gateIter != gates.end(); gateIter++) {
			if (!gateIter->open) {
				player->position.x = std::min(gateIter->transform->position.x - GATE_RAD, player->position.x);
				camera->transform->position.x = std::min(gateIter->transform->position.x - GATE_RAD, camera->transform->position.x);
			}
		}
	}

	//update the "hanging" tiles and strs based on the camera position
	{
		for (auto tileIter = tiles.begin(); tileIter != tiles.end(); tileIter++) {
			if (tileIter->hanging) {
				tileIter->transform->position = tileIter->hanging_offset + camera->transform->position;
			}
		}
		hangingStr0->position = tiles[2].hanging_offset + HANGING_STR_OFFSET + camera->transform->position;
		hangingStr1->position = tiles[3].hanging_offset + HANGING_STR_OFFSET + camera->transform->position;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	LMB.downs = 0;
	RMB.downs = 0;
}


void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	GL_ERRORS();
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse controls camera; WASD moves body; E interacts; escape ungrabs mouse; Q to quit",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse controls camera; WASD moves body; E interacts; escape ungrabs mouse; Q to quit",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
