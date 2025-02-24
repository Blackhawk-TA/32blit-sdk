#include <cstring>
#include <string>
#include <sstream>
#include <cstdlib>
#include <array>
#include "raycaster.hpp"
#include "assets.hpp"
#include "32blit.hpp"

using namespace blit;

float z_buffer[SCREEN_WIDTH];
float lut_camera_displacement[SCREEN_WIDTH];
Vec2 ray_cache[SCREEN_WIDTH];

std::vector<sprite> map_sprites(NUM_SPRITES);
std::vector<star> stars(NUM_STARS);
std::vector<sprite> spray(MAX_SPRAY);

/* Ambient Occlusion Mask */
uint8_t __m[SCREEN_WIDTH * SCREEN_HEIGHT];
Surface mask((uint8_t *)__m, PixelFormat::M, Size(SCREEN_WIDTH, SCREEN_HEIGHT));

player player1{
	Vec2(0,0),
	Vec2(0,0),
	Vec2(0,0),
	0.0f,
	false,
	0.0f,
	false
};
Map map(Rect(0, 0, 16, 16));
MapLayer *map_layer_walls;
MapLayer *map_layer_floor;

enum TileFlags { WALL = 1, NO_GRASS = 2 };
enum TileFacing { NONE = 0, NORTH = 1, SOUTH = 2, EAST = 4, WEST = 8 };

bool visibility_map[MAP_WIDTH * MAP_HEIGHT] = { 0 };

std::vector<uint8_t> map_data_walls = {
	0x01, 0x02, 0x03, 0x04, 0x03, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x01, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01,
	0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01,
	0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
};

std::vector<uint8_t> map_data_floor = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x01, 0x05, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
	0x00, 0x05, 0x03, 0x04, 0x05, 0x01, 0x01, 0x05, 0x00, 0x01, 0x01, 0x01, 0x01, 0x05, 0x01, 0x00,
	0x00, 0x04, 0x01, 0x05, 0x01, 0x01, 0x05, 0x01, 0x00, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
	0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
	0x00, 0x03, 0x01, 0x00, 0x04, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x05, 0x01, 0x00, 0x01, 0x02, 0x01, 0x01, 0x00, 0x01, 0x00, 0x01, 0x05, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x01, 0x00, 0x05, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x01, 0x04, 0x05, 0x01, 0x00,
	0x00, 0x05, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x00, 0x05, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x05, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x05, 0x01, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x03, 0x00, 0x03, 0x01, 0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00,
	0x00, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x05, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x05, 0x01, 0x01, 0x01, 0x00,
	0x00, 0x01, 0x01, 0x00, 0x05, 0x01, 0x01, 0x01, 0x00, 0x05, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


void get_random_empty_tile_location(Point &pos) {
	while (1) {
		pos.x = blit::random() % MAP_WIDTH;
		pos.y = blit::random() % MAP_HEIGHT;
		if (map.has_flag(pos, TileFlags::NO_GRASS)) continue;
		return;
	}
}


void init() {
	set_screen_mode(ScreenMode::lores);

	screen.sprites = Surface::load(asset_raycaster);

	map.add_layer("walls", map_data_walls);
	map_layer_walls = &map.layers["walls"];
	map_layer_walls->add_flags({ 1, 2, 3, 4, 5 }, TileFlags::WALL);
	map_layer_walls->add_flags({ 1, 2, 3, 4, 5 }, TileFlags::NO_GRASS);

	map.add_layer("floor", map_data_floor);
	map_layer_floor = &map.layers["floor"];
	map_layer_floor->add_flags({ 3, 4, 5 }, TileFlags::NO_GRASS);

	player1.direction.y = -1;
	player1.position.x = 3.5;
	player1.position.y = 3.5;
	player1.rotation = 0.0f;

	for (int x = 0; x < SCREEN_WIDTH; x++) {
		lut_camera_displacement[x] = (float)(2 * x) / (float)(SCREEN_WIDTH) - 1.0f;
	}

	srand(0x32bl);

	for (int s = 0; s < NUM_STARS; s++) {
		stars[s].position.x = blit::random() % 360;
		stars[s].position.y = blit::random() % (HORIZON / 2);
		stars[s].brightness = 32 + (blit::random() % 128);
	}

	Point tile;
	Vec2 offset;

	for (int s = 0; s < NUM_SPRITES; s++) {
		int texture = blit::random() % 100;
		if (texture == 0) {
			map_sprites[s].texture = 0;
		}
		else if (texture == 1) {
			map_sprites[s].texture = 1;
		}
		else if (texture == 2) {
			map_sprites[s].texture = 2;
		}
		else if (texture == 3) {
			map_sprites[s].texture = 3;
		}
		else {
			map_sprites[s].texture = 4 + (texture % 3);
		}
		get_random_empty_tile_location(tile);
		offset.x = (float)blit::random() / 4294967295.0f;
		offset.y = (float)blit::random() / 4294967295.0f;
		map_sprites[s].position.x = tile.x + offset.x;
		map_sprites[s].position.y = tile.y + offset.y;
		map_sprites[s].color = blit::random() % 3;
		map_sprites[s].velocity.x = 0;
		map_sprites[s].velocity.y = 0;
	}
}


void update(uint32_t time) {
	static Vec2 size(0.2f, 0.2f);
	static Vec2 rmove(0, 0);
	Vec2 move(0, 0);
	int check_tile = 0;

	if (pressed(Button::DPAD_UP)) {
		move.x = 0.02f;
	}
	else if (pressed(Button::DPAD_DOWN)) {
		move.x = -0.02f;
	}
	else if (joystick.y < -0.1f || joystick.y > 0.1f) {
		move.x = -joystick.y * 0.02f;
	}

	rmove.x = move.x * player1.direction.x - move.y * player1.direction.y;
	rmove.y = move.x * player1.direction.y + move.y * player1.direction.x;

	if (rmove.x < 0) {
		int bound = (int)std::floor(player1.position.x + rmove.x - size.x);

		check_tile = check_tile | map.get_flags(Point(bound, (int32_t)std::floor(player1.position.y)));
		if (rmove.y > 0) {
			check_tile = check_tile | map.get_flags(Point(bound, (int32_t)std::floor(player1.position.y - size.y)));
		}
		else if (rmove.y < 0) {
			check_tile = check_tile | map.get_flags(Point(bound, (int32_t)std::floor(player1.position.y + size.y)));
		}
	}
	else if (rmove.x > 0) {
		int bound = (int)std::floor(player1.position.x + rmove.x + size.x);

		check_tile = check_tile | map.get_flags(Point(bound, (int32_t)std::floor(player1.position.y)));
		if (rmove.y > 0) {
			check_tile = check_tile | map.get_flags(Point(bound, (int32_t)std::floor(player1.position.y - size.y)));
		}
		else if (rmove.y < 0) {
			check_tile = check_tile | map.get_flags(Point(bound, (int32_t)std::floor(player1.position.y + size.y)));
		}
	}

	if ((check_tile & TileFlags::WALL) == 0) {
		player1.position.x += rmove.x;
	}

	check_tile = 0;

	if (rmove.y < 0) {
		int bound = (int)std::floor(player1.position.y + rmove.y - size.y);

		check_tile = check_tile | map.get_flags(Point((int32_t)std::floor(player1.position.x), bound));
		if (rmove.x > 0) {
			check_tile = check_tile | map.get_flags(Point((int32_t)std::floor(player1.position.x - size.x), bound));
		}
		else if (rmove.x < 0) {
			check_tile = check_tile | map.get_flags(Point((int32_t)std::floor(player1.position.x + size.x), bound));
		}
	}
	else if (rmove.y > 0) {
		int bound = (int)std::floor(player1.position.y + rmove.y + size.y);

		check_tile = check_tile | map.get_flags(Point((int32_t)std::floor(player1.position.x), bound));
		if (rmove.x > 0) {
			check_tile = check_tile | map.get_flags(Point((int32_t)std::floor(player1.position.x - size.x), bound));
		}
		else if (rmove.x < 0) {
			check_tile = check_tile | map.get_flags(Point((int32_t)std::floor(player1.position.x + size.x), bound));
		}
	}

	if ((check_tile & TileFlags::WALL) == 0) {
		player1.position.y += rmove.y;
	}

	static unsigned int spray_index = 0;

	if (pressed(Button::DPAD_LEFT)) {
		player1.facing = false;
		player1.rotation += -0.02f;
	}
	else if (pressed(Button::DPAD_RIGHT)) {
		player1.facing = true;
		player1.rotation += 0.02f;
	}
	else if (joystick.x < -0.1f || joystick.x > 0.1f) {
		player1.rotation += joystick.x * 0.02f;
		player1.facing = joystick.x > 0;
	}

	player1.direction = Vec2(0, -1);
	player1.direction.rotate(player1.rotation);
	player1.direction.normalize();

	if (pressed(Button::A)) {
		Vec2 spray_offset(0.5f, 0.0f);
		spray_offset.rotate(player1.rotation);
		player1.spraying = 1;
		spray[spray_index].position = player1.position + player1.direction + spray_offset;
		spray[spray_index].color = 64;
		spray[spray_index].texture = 64;
		spray[spray_index].velocity = (player1.direction * 0.01f);
		spray_index++;
		if(spray_index >= spray.size()) {
			spray_index = 0;
		}
	}
	else
	{
		player1.spraying = 0;
	}
}


void render(uint32_t time) {
#ifdef SHOW_FPS
	uint32_t ms_start = now();
#endif

	// update the orientation of the player camera plane
	player1.camera = Vec2(-player1.direction.y, player1.direction.x);
	player1.inverse_det = 1.0f / (player1.camera.x * player1.direction.y - player1.direction.x * player1.camera.y);

	// the current ray direction is the player direction, plus the camera direction multiplied by the displacement
	for(auto column = 0u; column < SCREEN_WIDTH; column++) {
		ray_cache[column].x = player1.direction.x + player1.camera.x * lut_camera_displacement[column];
		ray_cache[column].y = player1.direction.y + player1.camera.y * lut_camera_displacement[column];
	}

	// clear the mask
#ifdef AMBIENT_OCCLUSION
	mask.alpha = 255;
	mask.pen = Pen(0);
	mask.clear();
#endif

	// clear the canvas
	screen.alpha = 255;
	screen.pen = Pen(22, 21, 31);
	screen.clear();

	render_sky();

	render_stars();

	screen.clip = Rect(0, 0, SCREEN_WIDTH, OFFSET_TOP + VIEW_HEIGHT);
	render_world(time);

#ifdef AMBIENT_OCCLUSION
	blur(1);

	screen.pen = Pen(10, 36, 24);
	screen.mask = &mask;
	screen.clear();
	screen.mask = nullptr;
#endif

	render_sprites(time);
	render_spray(time);
	screen.clip = Rect(Point(0, 0), screen.bounds);

	// draw bug spray    
	int offset = OFFSET_TOP + int(sinf((player1.position.x + player1.position.y) * 4) * 3); // bob
	screen.sprite(player1.spraying ? Rect(8, 16, 3, 4) : Rect(5, 16, 3, 4), Point(SCREEN_WIDTH - 48, VIEW_HEIGHT - 30 + offset));

	// draw the HUD
	screen.pen = Pen(37, 36, 46);
	screen.rectangle(Rect(0, SCREEN_HEIGHT - 24, SCREEN_WIDTH, 24));
	for (int x = 0; x < SCREEN_WIDTH / 8; x++) {
		screen.sprite(340, Point(x * 8, SCREEN_HEIGHT - 24));
	}

	// draw the health bar
	for (int x = 0; x < 4; x++) {
		screen.sprite(x > 1 ? 322 : 321, Point(32 + x * 10, SCREEN_HEIGHT - 16));
	}

	// draw DOOM guy (phil)
	screen.sprite(Rect(11, 16, 3, 4), Point(0, SCREEN_HEIGHT - 32), player1.facing ? SpriteTransform::HORIZONTAL : 0);

#ifdef SHOW_FPS
	uint32_t ms_end = now();

	// draw FPS meter
	screen.mask = nullptr;
	screen.pen = Pen(255, 0, 0);
	for (unsigned int i = 0; i < (ms_end - ms_start); i++) {
		screen.pen = Pen(i * 5, 255 - (i * 5), 0);
		screen.rectangle(Rect(i * 3 + 1, SCREEN_HEIGHT - 3, 2, 2));
	}
#endif
}

void render_sky() {
	for (uint16_t column = 0; column < SCREEN_WIDTH; column++) {
		Vec2 ray = ray_cache[column];
		// ray.normalize();  // WHY? Has no visual impact

		float r = std::atan2(ray.x, ray.y);
		r = (r > 0.0f ? r : (2.0f * pi + r)) * 360.0f / (2.0f * pi);

		Point uv(24 + (int(r * 3.0f) % 16), 160 - 32);

		screen.stretch_blit_vspan(screen.sprites, uv, 32, Point(column, 0), HORIZON + OFFSET_TOP); // TODO: blit from spritesheet?

		// Apply radial darkness to simulate directional sunset
		uint8_t fade = std::max(-120, std::min(120, std::abs(int(r) - 120))) + 60;  // calculate a `fog` based on angle
		screen.pen = Pen(12, 33, 52, fade);
		screen.line(Point(column, 0), Point(column, OFFSET_TOP + HORIZON));
	}
}

void render_stars() {
	// Get the player's facing angle in degrees from 0 to 359
	float r = std::atan2(player1.direction.x, player1.direction.y);
	r = (r > 0.0f ? r : (2.0f * pi + r)) * 360.0f / (2.0f * pi);
	screen.pen = Pen(255, 255, 255, 255);

	for (int s = 0; s < NUM_STARS; s++) {
		star *sp = &stars[s];

		// If the stars radial X position is within our field of view
		if ((180 - std::abs(std::abs(r - sp->position.x) - 180)) < 45) {
			// Get the difference between the star and player angle as degrees, signed
			int x = (int)r - sp->position.x + 180;
			x = x - std::floor(float(x) / 360.0f) * 360;
			x -= 180;

			// Convert the degrees to screen columns
			x = 80 + (x / 45.0f) * 80;
			screen.alpha = sp->brightness;
			screen.pixel(Point(
				x,
				sp->position.y * 2
			));
		}
	}
	screen.alpha = 255;
}

void render_world(uint32_t time) {
	float perpendicular_wall_distance, wall_x;
	Point player_map_location((int32_t)std::floor(player1.position.x), (int32_t)std::floor(player1.position.y));
	Point map_location;
	Point last_map_location(-1, -1);
#ifdef AMBIENT_OCCLUSION
	int last_side = -1;
	float last_wall_distance = 0;
#endif

	// Reset the visibility map
	for (int x = 0; x < MAP_HEIGHT * MAP_WIDTH; x++) {
		visibility_map[x] = false;
	}

	// Add the current player map location to the player visibility  map
	visibility_map[player_map_location.x + player_map_location.y * MAP_WIDTH] = true;

	for (uint16_t column = 0; column < SCREEN_WIDTH; column++) { // trace SCREEN_WIDTH rays from left to right
		// calculate the amount we need to scale the plane_x/y camera displacement
		// this gives us a step along the camera displacement that corresponds to the current ray
		map_location = player_map_location;
		Vec2 ray = ray_cache[column];

		Vec2 delta_dist(
			std::abs(1.0f / ray.x),
			std::abs(1.0f / ray.y)
		);

		Vec2 side_dist(0, 0);
		int8_t step_x, step_y;

		if (ray.x < 0) {
			step_x = -1;
			side_dist.x = (player1.position.x - map_location.x) * delta_dist.x;
		}
		else
		{
			step_x = 1;
			side_dist.x = (map_location.x + 1.0f - player1.position.x) * delta_dist.x;
		}

		if (ray.y < 0) {
			step_y = -1;
			side_dist.y = (player1.position.y - map_location.y) * delta_dist.y;
		}
		else
		{
			step_y = 1;
			side_dist.y = (map_location.y + 1.0f - player1.position.y) * delta_dist.y;
		}

		bool hit = false;

		int side = 0;
		for (int s = 0; s < MAX_RAY_STEPS; s++) {

			if (side_dist.x < side_dist.y) {
				side_dist.x += delta_dist.x;
				map_location.x += step_x;
				side = 0;
			}
			else
			{
				side_dist.y += delta_dist.y;
				map_location.y += step_y;
				side = 1;
			}

			// Add any map tile a ray steps through to the player visible map
			if (map_location.x + map_location.y * MAP_WIDTH < MAP_WIDTH * MAP_HEIGHT) {
				visibility_map[map_location.x + map_location.y * MAP_WIDTH] = true;
			}

			if (map.has_flag(map_location, TileFlags::WALL)) {
				hit = true;
				break;
			}
		}

		if (hit) {
			uint8_t texture_wall = map_layer_walls->tile_at(map_location) - 1;// tile & 0x0f;

			if (side == 0) {
				perpendicular_wall_distance = ((float)map_location.x - player1.position.x + (1 - step_x) / 2.0f) / ray.x;
				wall_x = player1.position.y + perpendicular_wall_distance * ray.y;
			}
			else {
				perpendicular_wall_distance = ((float)map_location.y - player1.position.y + (1 - step_y) / 2.0f) / ray.y;
				wall_x = player1.position.x + perpendicular_wall_distance * ray.x;
			}

			wall_x -= std::floor(wall_x);

			// While the perpendicular wall distance prevents fish-eye effect, generally we want
			// lighting and distance based overlay effects to use the "real" wall distance
			// otherwise the player can see further out of the corner of their eye
			// real_wall_distance = perpendicular_wall_distance / cos(atan2(player1.direction.x, player1.direction.y)) - atan2(ray.x, ray.y);

			int wall_half_height = (int)((float)HORIZON / perpendicular_wall_distance);
			int start_y = HORIZON - wall_half_height;
			int end_y = HORIZON + wall_half_height;

			z_buffer[column] = perpendicular_wall_distance;

#ifdef AMBIENT_OCCLUSION
			mask.pen = 200;

			float line_distance = std::abs(perpendicular_wall_distance - last_wall_distance);

			int width = wall_half_height / 8.0f;

			if (column > 0 && (side != last_side) && line_distance < 0.5f && !(map_location.x == last_map_location.x && map_location.y == last_map_location.y)) {
				for (int c = column - width; c < column + width; c++) {
					int alpha = (std::abs(column - c) * 160) / width;
					mask.pen = 160 - alpha;
					mask.line(Point(c, start_y + OFFSET_TOP), Point(c, end_y + OFFSET_TOP));
				};
			}
			else {
				for (int r = end_y - width; r < end_y + width; r++) {
					int alpha = (std::abs(end_y - r) * 160) / width;
					mask.pen = 160 - alpha;
					mask.pixel(Point(column, r + OFFSET_TOP));
				}
			}

			last_side = side;
			last_wall_distance = perpendicular_wall_distance;
#endif

			last_map_location = map_location;

			/* draw the walls */

			// TODO: add mipmap support? automatic based on scale?          

			// texture_wall
			//
			//  0 = mossy stone
			//  1 = crumbled brick
			//  2 = good brick
			//  3 = spoopy door
			//  4 = good brick support
			uint16_t texture_offset_x = texture_wall * 32;
			Point uv = Point(uint16_t(wall_x * 32.0f) + texture_offset_x, 0);

			screen.stretch_blit_vspan(screen.sprites, uv, 32, Point(column, start_y + OFFSET_TOP), end_y - start_y); // TODO Blit from Spritesheet

			float wall_distance = perpendicular_wall_distance / MAX_RAY_STEPS;
			float alpha = wall_distance * 255.0f;
			screen.pen = Pen(0, 0, 0, int(alpha));
			screen.line(Point(column, start_y + OFFSET_TOP), Point(column, end_y + OFFSET_TOP));

			Vec2 floor_wall(map_location.x, map_location.y);

			if (side == 0 && ray.x > 0) {
				floor_wall.y += wall_x;
			}
			else if (side == 0 && ray.x < 0) {
				floor_wall.x += 1.0f;
				floor_wall.y += wall_x;
			}
			else if (side == 1 && ray.y > 0) {
				floor_wall.x += wall_x;
			}
			else {
				floor_wall.x += wall_x;
				floor_wall.y += 1.0f;
			}

			// Draw the floor
			for (int y = end_y + 1; y < VIEW_HEIGHT + 1; y++) {
				float distance = (float)VIEW_HEIGHT / (2.0f * y - VIEW_HEIGHT);
				float weight = distance / perpendicular_wall_distance;

				Vec2 current_floor(
					weight * floor_wall.x + (1.0f - weight) * player1.position.x,
					weight * floor_wall.y + (1.0f - weight) * player1.position.y
				);

				// Get the tile-relative x/y texture coordinates
				Point tile_uv(
					(current_floor.x - std::floor(current_floor.x)) * 32,
					(current_floor.y - std::floor(current_floor.y)) * 32
				);

				uint8_t floor_texture = map_layer_floor->tile_at(Point(int(current_floor.x), int(current_floor.y))) - 1;
				//uint8_t floor_texture = get_map_tile(point(int(current_floor.x), int(current_floor.y))) & 0x0f;

				Point floor_texture_sprite(
					32 * floor_texture,
					32
				);
				// Get the distance from the player to the point on the floor
				// and use this to create a distance shadowing effect
				// dist = current_floor - player1.position;
				// p_distance = dist.length();

				int fragment_x = floor_texture_sprite.x + tile_uv.x;
				int fragment_y = floor_texture_sprite.y + tile_uv.y;


				uint8_t fragment_c_idx = *screen.sprites->ptr(fragment_x, fragment_y);
				screen.pen = screen.sprites->palette[fragment_c_idx];
				screen.pixel(Point(column, y - 1 + OFFSET_TOP));

				float floor_distance = distance / MAX_RAY_STEPS;

				screen.pen = Pen(0, 0, 0, int(floor_distance * 255.0f));
				screen.pixel(Point(column, y - 1 + OFFSET_TOP));
			}
		}
	}
}

void render_spray(uint32_t time) {

	// Calculate distance from player to each sprite
	for (auto i = 0u; i < MAX_SPRAY; i++) {
		Vec2 sprite_distance(
			spray[i].position.x - player1.position.x,
			spray[i].position.y - player1.position.y
		);
		spray[i].distance = (sprite_distance.x * sprite_distance.x) + (sprite_distance.y * sprite_distance.y);
	}

	// sort the sprites by distance
	std::sort(spray.begin(), spray.end());

	for (int i = 0; i < MAX_SPRAY; i++) {
		sprite *psprite = &spray[i];
		if(spray[i].color) {
			spray[i].color--;
		}
		if(spray[i].texture < 255) {
			spray[i].texture++;
		}
		spray[i].position += spray[i].velocity;
		if(spray[i].position.x < 0) {spray[i].position.x = 0;}
		if(spray[i].position.y < 0) {spray[i].position.y = 0;}
		if(spray[i].position.x > MAP_WIDTH) {spray[i].position.x = MAP_WIDTH - 1;}
		if(spray[i].position.y > MAP_HEIGHT) {spray[i].position.y = MAP_HEIGHT - 1;}

		// Skip any sprites that aren't in a map tile that's "visible" to the player.
		// This might cull sprites that might have visible foleage, but it's pretty tricky to notice
		if (!visibility_map[int(psprite->position.x) + int(psprite->position.y) * MAP_WIDTH]) {
			continue;
		}

		// Give the larger sprites a better view distance
		float max_distance = 64.0;

		float distance = std::min(max_distance, psprite->distance) / max_distance;
		if (distance == 1.0f) {
			continue;
		}

		// Get the player-relative position of the sprite
		Vec2 relative_position = psprite->position - player1.position;

		Vec2 screen_transform(
			player1.inverse_det * (player1.direction.y * relative_position.x - player1.direction.x * relative_position.y),
			player1.inverse_det * (-player1.camera.y * relative_position.x + player1.camera.x * relative_position.y)
		);

		// Skip any sprites which are behind the player
		if (screen_transform.y < 0) {
			continue;
		}

		Rect bounds(0, 0, psprite->texture / 4, psprite->texture / 4);

		int sprite_height = std::abs(int(bounds.h * SPRITE_SCALE / screen_transform.y));
		//int sprite_width = ((float)bounds.w / (float)bounds.h) * sprite_height;

		// Get the screen-space position of the sprites base on the floor
		Vec2 screen_pos(
			int((SCREEN_WIDTH / 2) * (1 + screen_transform.x / screen_transform.y)),
			HORIZON + (HORIZON / screen_transform.y)
		);

		/* DEBUG: Plot the sprite's base with a red dot
		screen.alpha = 255 - int(255 * distance);
		screen.pen = Pen(255, 0, 0);
		screen.pixel(point(screen_pos.x, screen_pos.y));
		*/

		// offset screen coordinate with sprite bounds
		//screen_pos -= Vec2(sprite_width / 2, sprite_height);
		screen_pos.y -= sprite_height;
		screen_pos.y += OFFSET_TOP;

		screen.pen = Pen(255, 0, 255, psprite->color / 4);
		screen.circle(screen_pos, sprite_height / 2);
	}
}

void render_sprites(uint32_t time) {

	// Calculate distance from player to each sprite
	for (auto i = 0u; i < NUM_SPRITES; i++) {
		Vec2 sprite_distance(
			map_sprites[i].position.x - player1.position.x,
			map_sprites[i].position.y - player1.position.y
		);
		map_sprites[i].distance = (sprite_distance.x * sprite_distance.x) + (sprite_distance.y * sprite_distance.y);
	}

	// sort the sprites by distance
	std::sort(map_sprites.begin(), map_sprites.end());

	for (int i = 0; i < NUM_SPRITES; i++) {
		sprite *psprite = &map_sprites[i];

		Pen cols_a[]{
			Pen(0x15, 0x98, 0x5d, 200),
			Pen(0x35, 0xA8, 0x3d, 200),
			Pen(0x45, 0x88, 0x2d, 200)
		};

		Pen cols_b[]{
			Pen(0x00, 0x7f, 0x43, 200),
			Pen(0x20, 0x6f, 0x33, 200),
			Pen(0x30, 0x8f, 0x23, 200)
		};

		// Skip any sprites that aren't in a map tile that's "visible" to the player.
		// This might cull sprites that might have visible foleage, but it's pretty tricky to notice
		if (!visibility_map[int(psprite->position.x) + int(psprite->position.y) * MAP_WIDTH]) {
			continue;
		}

		// Give the larger sprites a better view distance
		float max_distance = (psprite->texture == 0 || psprite->texture == 1) ? 64.0 : 16.0;

		float distance = std::min(max_distance, psprite->distance) / max_distance;
		if (distance == 1.0f) {
			continue;
		}

		// Get the player-relative position of the sprite
		Vec2 relative_position = psprite->position - player1.position;

		Vec2 screen_transform(
			player1.inverse_det * (player1.direction.y * relative_position.x - player1.direction.x * relative_position.y),
			player1.inverse_det * (-player1.camera.y * relative_position.x + player1.camera.x * relative_position.y)
		);

		// Skip any sprites which are behind the player
		if (screen_transform.y < 0) {
			continue;
		}

		screen.sprites->palette[11] = cols_a[psprite->color];
		screen.sprites->palette[12] = cols_b[psprite->color];

		// TODO:: palette change
		//int color_offset = spr.color * 4;

		Rect sprite_bounds[7] = {
			Rect(0, 64, 28, 64),   // Full-grown tree
			Rect(28, 74, 28, 54),  // Mature tree
			Rect(56, 94, 28, 34),  // Tall shrub
			Rect(56, 70, 28, 24),  // Short shrub
			Rect(48, 65, 8, 8),    // Tall grass
			Rect(38, 66, 9, 7),    // Mid grass
			Rect(30, 68, 7, 5)     // Smol grass
		};

		Rect bounds = sprite_bounds[psprite->texture];

		int sprite_height = std::abs(int(bounds.h * SPRITE_SCALE / screen_transform.y));
		int sprite_width = ((float)bounds.w / (float)bounds.h) * sprite_height;

		// Unused?
		//int sprite_top_y = ((VIEW_HEIGHT - bounds.h) * SPRITE_SCALE) / screen_transform.y;

		// Get the screen-space position of the sprites base on the floor
		Vec2 screen_pos(
			int((SCREEN_WIDTH / 2) * (1 + screen_transform.x / screen_transform.y)),
			HORIZON + (HORIZON / screen_transform.y)
		);

		/* DEBUG: Plot the sprite's base with a red dot
		screen.alpha = 255 - int(255 * distance);
		screen.pen = Pen(255, 0, 0);
		screen.pixel(point(screen_pos.x, screen_pos.y));
		*/

		// offset screen coordinate with sprite bounds
		screen_pos -= Vec2(sprite_width / 2, sprite_height);

		for (int x = std::max((uint16_t)0, uint16_t(screen_pos.x)); x < std::min(SCREEN_WIDTH, uint16_t(screen_pos.x + sprite_width)); x++) {
			if (screen_transform.y > z_buffer[x]) continue;

			Vec2 uv(
				bounds.x + ((float(x - screen_pos.x) / float(sprite_width)) * bounds.w),
				bounds.y
			);

			screen.stretch_blit_vspan(screen.sprites, uv, bounds.h, Point(x, screen_pos.y + OFFSET_TOP), sprite_height); // TODO: blit from spritesheet?
		}
		screen.sprites->palette[11] = Pen(0x15, 0x98, 0x5d, 200);
		screen.sprites->palette[12] = Pen(0x00, 0x7f, 0x43, 200);
	}
}

/* Exclusively for blurring the ambient occlusion mask */
void blur(uint8_t passes) {
	uint8_t last;

	for (uint8_t pass = 0; pass < passes; pass++) {
		uint8_t *p = (uint8_t *)mask.data;
		for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
			last = *p;
			p++;

			for (uint16_t x = 1; x < SCREEN_WIDTH - 1; x++) {
				*p = (*(p + 1) + last + *p + *p) >> 2;
				last = *p;
				p++;
			}

			p++;
		}
	}

	// vertical      
	for (uint8_t pass = 0; pass < passes; pass++) {
		for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
			uint8_t *p = (uint8_t *)mask.data + x;

			last = *p;
			p += SCREEN_WIDTH;

			for (uint16_t y = 1; y < SCREEN_HEIGHT - 1; y++) {
				*p = (*(p + SCREEN_WIDTH) + last + *p + *p) >> 2;
				last = *p;
				p += SCREEN_WIDTH;
			}
		}
	}
}
