#pragma once
#include "utility.h"
#include "TrainerData.h"
#include "GraphicsAsset.h"
#include "CppRed/Actor.h"
#include "CppRed/actor_ptr.h"
#ifndef HAVE_PCH
#include <string>
#endif

namespace CppRed{
class Actor;
class Game;
class Npc;
enum class IntegerVariableId;
}
struct MapData;
class MapStore;
class BaseTrainerParty;
enum class MapObjectFacingDirection;
enum class ItemId;
enum class SpeciesId;

enum class MapObjectType{
	EventDisp,
	Hidden,
	Item,
	Npc,
	Pokemon,
	Sign,
	Trainer,
	Warp,
};

class MapObject{
protected:
	int id;
	std::string name;
	Point position = {-1, -1};
	const MapData *map_data = nullptr;

	MapObject(BufferReader &);
public:
	MapObject(const std::string &name, const Point &position): name(name), position(position){}
	virtual ~MapObject() = 0;
	static std::pair<std::string, std::shared_ptr<std::vector<std::unique_ptr<MapObject>>>> create_vector(BufferReader &buffer, const std::function<std::unique_ptr<MapObject>(BufferReader &)> &);
	virtual const char *get_type_string() const = 0;
	virtual bool requires_actor() const = 0;
	virtual CppRed::actor_ptr<CppRed::Actor> create_actor(CppRed::Game &game, Renderer &renderer, Map map, MapObjectInstance &instance) const;
	virtual bool npcs_can_walk_over() const{
		return true;
	}
	virtual void activate(CppRed::Game &, CppRed::Actor &activator, CppRed::Actor *activatee){}
	virtual MapObjectType get_type() const = 0;
	virtual int get_legacy_id() const{
		return -1;
	}
	DEFINE_GETTER(position)
	DEFINE_GETTER(name)
	DEFINE_GETTER_SETTER(map_data)
};

class DialogingMapObject : public MapObject{
protected:
	DialogingMapObject(BufferReader &buffer): MapObject(buffer){}
	DialogingMapObject(const std::string &name, const Point &position): MapObject(name, position){}
public:
	virtual ~DialogingMapObject(){}
	virtual int get_text_index() const = 0;
	void activate(CppRed::Game &game, CppRed::Actor &activator, CppRed::Actor *activatee) override;
};

class EventDisp : public MapObject{
public:
	EventDisp(BufferReader &);
	EventDisp(const std::string &name, const Point &position): MapObject(name, position){}
	const char *get_type_string() const override{
		return "event_disp";
	}
	bool requires_actor() const override{
		return false;
	}
	MapObjectType get_type() const override{
		return MapObjectType::EventDisp;
	}
};

class Sign : public DialogingMapObject{
protected:
	int text_index;
public:
	Sign(BufferReader &);
	Sign(const std::string &name, const Point &position, int text_index):
		DialogingMapObject(name, position),
		text_index(text_index){}
	const char *get_type_string() const override{
		return "sign";
	}
	bool requires_actor() const override{
		return false;
	}
	int get_text_index() const override{
		return this->text_index;
	}
	MapObjectType get_type() const override{
		return MapObjectType::Sign;
	}
};

class HiddenObject : public MapObject{
protected:
	std::string script;
	std::string script_parameter;

public:
	HiddenObject(BufferReader &);
	HiddenObject(const std::string &name, const Point &position, const std::string &script, const std::string &script_parameter):
		MapObject(name, position),
		script(script),
		script_parameter(script_parameter){}
	const char *get_type_string() const override{
		return "hidden_object";
	}
	bool requires_actor() const override{
		return false;
	}
	MapObjectType get_type() const override{
		return MapObjectType::Hidden;
	}
	void activate(CppRed::Game &, CppRed::Actor &activator, CppRed::Actor *activatee) override;
};

struct WarpDestination{
	bool simple;
	const MapData *destination_map = nullptr;
	CppRed::IntegerVariableId variable;
	WarpDestination() = default;
	WarpDestination(const MapData &destination_map): simple(true), destination_map(&destination_map){}
	WarpDestination(CppRed::IntegerVariableId variable): simple(false), variable(variable){}
};

class MapWarp : public MapObject{
protected:
	int index;
	WarpDestination destination;
	int destination_warp_index;

public:
	MapWarp(BufferReader &, const MapStore &);
	MapWarp(const std::string &name, const Point &position, int index, const WarpDestination &destination, int destination_warp_index):
		MapObject(name, position),
		index(index),
		destination(destination),
		destination_warp_index(destination_warp_index){}
	const char *get_type_string() const override{
		return "map_warp";
	}
	bool requires_actor() const override{
		return false;
	}
	bool npcs_can_walk_over() const override{
		return false;
	}
	MapObjectType get_type() const override{
		return MapObjectType::Warp;
	}
	DEFINE_GETTER(index)
	DEFINE_GETTER(destination)
	DEFINE_GETTER(destination_warp_index)
};

class ObjectWithSprite : public DialogingMapObject{
protected:
	const GraphicsAsset *sprite;
	MapObjectFacingDirection facing_direction;
	bool wandering;
	int range;
	int text_index;
	int legacy_id;

public:
	ObjectWithSprite(BufferReader &buffer, const std::map<std::string, const GraphicsAsset *> &graphics_map);
	ObjectWithSprite(const std::string &name, const Point &position, const GraphicsAsset &sprite, MapObjectFacingDirection facing_direction, bool wandering, int range, int text_index):
		DialogingMapObject(name, position),
		sprite(&sprite),
		facing_direction(facing_direction),
		wandering(wandering),
		range(range),
		text_index(text_index){}
	virtual ~ObjectWithSprite() = 0;
	bool requires_actor() const override{
		return true;
	}
	int get_text_index() const override{
		return this->text_index;
	}
	void activate(CppRed::Game &game, CppRed::Actor &activator, CppRed::Actor *activatee) override;
	int get_legacy_id() const override{
		return this->legacy_id;
	}
};

inline ObjectWithSprite::~ObjectWithSprite(){}

class NpcMapObject : public ObjectWithSprite{
protected:
	void initialize_actor(CppRed::Npc &, Map, CppRed::Game &) const;
public:
	NpcMapObject(BufferReader &, const std::map<std::string, const GraphicsAsset *> &graphics_map);
	NpcMapObject(const char *name, const Point &position, const GraphicsAsset &sprite, MapObjectFacingDirection facing_direction, bool wandering, int range, int text_id):
		ObjectWithSprite(name, position, sprite, facing_direction, wandering, range, text_id){}
	virtual const char *get_type_string() const override{
		return "npc";
	}
	virtual CppRed::actor_ptr<CppRed::Actor> create_actor(CppRed::Game &game, Renderer &renderer, Map map, MapObjectInstance &instance) const override;
	virtual MapObjectType get_type() const override{
		return MapObjectType::Npc;
	}
};

class ItemMapObject : public ObjectWithSprite{
protected:
	ItemId item;

public:
	ItemMapObject(BufferReader &,
		const std::map<std::string, const GraphicsAsset *> &graphics_map,
		const std::map<std::string, ItemId> &items_map);
	ItemMapObject(const char *name, const Point &position, const GraphicsAsset &sprite, MapObjectFacingDirection facing_direction, bool wandering, int range, int text_id, ItemId item):
		ObjectWithSprite(name, position, sprite, facing_direction, wandering, range, text_id),
		item(item){}
	const char *get_type_string() const override{
		return "item";
	}
	CppRed::actor_ptr<CppRed::Actor> create_actor(CppRed::Game &game, Renderer &renderer, Map map, MapObjectInstance &instance) const override;
	MapObjectType get_type() const override{
		return MapObjectType::Item;
	}
};

class TrainerMapObject : public NpcMapObject{
protected:
	std::shared_ptr<TrainerClassData> trainer_class;
	int default_party;

public:
	TrainerMapObject(BufferReader &,
		const std::map<std::string, const GraphicsAsset *> &graphics_map,
		const TrainerClassesStore &);
	const char *get_type_string() const override{
		return "trainer";
	}
	CppRed::actor_ptr<CppRed::Actor> create_actor(CppRed::Game &game, Renderer &renderer, Map map, MapObjectInstance &instance) const override;
	MapObjectType get_type() const override{
		return MapObjectType::Trainer;
	}
};

class PokemonMapObject : public ObjectWithSprite{
protected:
	SpeciesId species;
	int level;

public:
	PokemonMapObject(BufferReader &, const std::map<std::string, const GraphicsAsset *> &graphics_map);
	PokemonMapObject(const char *name, const Point &position, const GraphicsAsset &sprite, MapObjectFacingDirection facing_direction, bool wandering, int range, int text_id, SpeciesId species, int level):
		ObjectWithSprite(name, position, sprite, facing_direction, wandering, range, text_id),
		species(species),
		level(level){}
	const char *get_type_string() const override{
		return "pokemon";
	}
	MapObjectType get_type() const override{
		return MapObjectType::Pokemon;
	}
};
