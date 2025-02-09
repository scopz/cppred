#pragma once

#include "common_types.h"
#include "GraphicsAsset.h"
#include "Objects.h"
#include "../CodeGeneration/output/maps.h"
#include "../CodeGeneration/output/items.h"
#include "../CodeGeneration/output/audio.h"
#include "../common/TilesetType.h"
#include "ScriptStore.h"
#include "TrainerData.h"
#include "utility.h"
#ifndef HAVE_PCH
#include <vector>
#include <memory>
#include <map>
#endif

class MapStore;
enum class TextResourceId;
class Coroutine;

namespace CppRed{
class Actor;
class Game;
enum class VisibilityFlagId;
}

struct Blockset{
	std::string name;
	std::vector<byte_t> data;

	Blockset(const byte_t *, size_t &, size_t);
};

struct Collision{
	std::string name;
	std::vector<byte_t> data;

	Collision(const byte_t *, size_t &, size_t);
};

enum class MapObjectFacingDirection{
	Undefined,
	None,
	Up,
	Right,
	Down,
	Left,
	BoulderMovementByte2,
};

struct BookcaseTile{
	int tile_no = -1;
	std::string script_name;
	TextResourceId text_id;
	bool is_script;
};

struct TilesetData{
	TilesetId id;
	std::string name;
	std::shared_ptr<Blockset> blockset;
	const GraphicsAsset *tiles;
	std::shared_ptr<Collision> collision;
	short counters[16];
	int grass_tile;
	TilesetType type;
	std::pair<short, short> impassability_pairs[16];
	std::pair<short, short> impassability_pairs_water[16];
	BookcaseTile bookcase_tiles[8];

	TilesetData(
		const byte_t *,
		size_t &,
		size_t,
		const std::map<std::string, std::shared_ptr<Blockset>> &,
		const std::map<std::string, std::shared_ptr<Collision>> &,
		const std::map<std::string, const GraphicsAsset *> &
	);
	const BookcaseTile *get_bookcase_info(int tile);
};

struct BinaryMapData{
	std::string name;
	std::vector<byte_t> data;

	BinaryMapData(const byte_t *, size_t &, size_t);
};

struct TemporaryMapConnection{
	MapData *source;
	std::string destination;
	int direction;
	int local_pos, remote_pos;
	TemporaryMapConnection() = default;
	TemporaryMapConnection(const TemporaryMapConnection &) = delete;
	TemporaryMapConnection(TemporaryMapConnection &&other) : destination(std::move(other.destination)){
		this->source = other.source;
		this->direction = other.direction;
		this->local_pos = other.local_pos;
		this->remote_pos = other.remote_pos;
	}
};

struct MapConnection{
	Map destination = Map::Nowhere;
	int local_position;
	int remote_position;
	operator bool() const{
		return this->destination != Map::Nowhere;
	}
	bool operator!() const{
		return !(bool)*this;
	}
};

struct MapTextEntry{
	bool simple_text;
	TextResourceId text;
	std::string script;
};

struct MapData{
	Map map_id;
	int legacy_id;
	std::string name;
	int width, height;
	std::shared_ptr<TilesetData> tileset;
	std::shared_ptr<BinaryMapData> map_data;
	std::string on_load;
	std::string on_frame;
	MapConnection map_connections[4];
	int border_block;
	std::shared_ptr<std::vector<std::unique_ptr<MapObject>>> objects;
	std::vector<MapTextEntry> map_text;
	int warp_check;
	int warp_tiles[8];
	std::string map_script;
	AudioResourceId music;
	CppRed::VisibilityFlagId sprite_visibility_flags[32];

	MapData(
		Map map_id,
		BufferReader &buffer,
		const std::map<std::string, std::shared_ptr<TilesetData>> &tilesets,
		const std::map<std::string, std::shared_ptr<BinaryMapData>> &map_data,
		std::vector<TemporaryMapConnection> &tmcs,
		std::vector<std::pair<MapData *, std::string>> &map_objects
	);
	int get_block_at_map_position(const Point &) const;
	int get_partial_tile_at_actor_position(const Point &) const;
};

class MapObjectInstance{
	Point position;
	CppRed::Game *game = nullptr;
	CppRed::Actor *actor = nullptr;
	MapObject *full_object;
public:
	MapObjectInstance(MapObject &, CppRed::Game &);
	MapObjectInstance(const MapObjectInstance &) = default;
	const MapObject &get_object() const{
		return *this->full_object;
	}
	void activate(CppRed::Actor &activator);
	DEFINE_GETTER_SETTER(position)
	void set_actor(CppRed::Actor &actor){
		this->actor = &actor;
	}
	CppRed::Actor &get_actor() const{
		return *this->actor;
	}
};

class MapInstance{
	Map map;
	const MapData *data;
	MapStore *store;
	//Even bits: cell is occupied.
	//Odd bits: cell has a warp.
	std::vector<bool> occupation_bitmap;
	std::vector<MapObjectInstance> objects;
	ScriptStore::script_f on_load = nullptr;
	ScriptStore::script_f on_frame = nullptr;
	std::unique_ptr<Coroutine> coroutine;
	CppRed::Game *current_game = nullptr;
	bool in_script_function = false;

	void check_map_location(const Point &) const;
	int get_block_number(const Point &) const;
	void coroutine_entry_point();
	void resume_coroutine(CppRed::Game &game);
public:
	MapInstance(Map, MapStore &, CppRed::Game &);
	void set_cell_occupation(const Point &, bool);
	bool get_cell_occupation(const Point &) const;
	bool is_warp_tile(const Point &) const;
	auto get_objects() const{
		return make_range(this->objects);
	}
	auto get_objects(){
		return make_range(this->objects);
	}
	void update(CppRed::Game &game);
	void loaded(CppRed::Game &game);
	void pause();
	void stop();
	void last_chance_update(CppRed::Game &game);
	const MapData &get_map_data() const{
		return *this->data;
	}
};

class MapStore{
	std::vector<std::unique_ptr<MapData>> maps;
	std::vector<MapData *> maps_by_name;
	std::vector<MapData *> maps_by_legacy_id;
	std::vector<std::unique_ptr<MapInstance>> map_instances;

	typedef std::map<std::string, std::shared_ptr<Blockset>> blocksets_t;
	typedef std::map<std::string, std::shared_ptr<Collision>> collisions_t;
	typedef std::map<std::string, const GraphicsAsset *> graphics_map_t;
	typedef std::map<std::string, std::shared_ptr<TilesetData>> tilesets_t;
	typedef std::map<std::string, std::shared_ptr<BinaryMapData>> map_data_t;
	typedef std::map<std::string, std::shared_ptr<std::vector<std::unique_ptr<MapObject>>>> map_objects_t;
	static blocksets_t load_blocksets();
	static collisions_t load_collisions();
	static graphics_map_t load_graphics_map();
	static tilesets_t load_tilesets(const blocksets_t &, const collisions_t &, const graphics_map_t &);
	static map_data_t load_map_data();
	static TrainerClassesStore load_trainer_parties();
	map_objects_t load_objects(const graphics_map_t &graphics_map);
	void load_maps(const tilesets_t &, const map_data_t &, const graphics_map_t &);
	//void load_map_objects();
public:
	MapStore();
	const MapData &get_map_data(Map map) const;
	MapInstance &get_map_instance(Map map, CppRed::Game &);
	MapInstance *try_get_map_instance(Map map, CppRed::Game &);
	const MapInstance &get_map_instance(Map map, CppRed::Game &) const;
	const MapData &get_map_by_name(const std::string &) const;
	const MapData &get_map_by_legacy_id(int) const;
	const MapData *try_get_map_by_legacy_id(int) const;
	void release_map_instance(Map, CppRed::Game &);
	void stop();
};
