#pragma once
#include "Data.h"
#include "Status.h"
#include "utility.h"
#include "../PokemonInfo.h"
#ifndef HAVE_PCH
#include <cstdint>
#endif

namespace CppRed{

class Pokemon{
	SpeciesId species;
	std::string nickname;
	int current_hp;
	int level;
	StatusCondition status;
	MoveId moves[4];
	std::uint16_t original_trainer_id;
	int experience;
	PokemonStats stat_experience;
	PokemonStats computed_stats;
	std::uint16_t individual_values;
	int pp[4];
	
	int get_iv(PokemonStats::StatId) const;
	int get_stat(PokemonStats::StatId, bool ignore_xp);
public:
	Pokemon():
		species(SpeciesId::None),
		current_hp(0),
		level(0),
		status(StatusCondition::Normal),
		original_trainer_id(0),
		experience(0),
		individual_values(0){}
	//Generates a random pokemon with the given species and level.
	Pokemon(SpeciesId species, int level, std::uint16_t original_trainer_id, XorShift128 &, const PokemonStats &input_stats = PokemonStats());
	Pokemon(const Pokemon &) = default;
	Pokemon(Pokemon &&) = default;
	int get_stat(PokemonStats::StatId stat){
		return this->get_stat(stat, false);
	}
	static int calculate_min_xp_to_reach_level(SpeciesId species, int level);
	static int calculate_level_at_xp(SpeciesId species, int xp);
	bool null() const{
		return this->species == SpeciesId::None;
	}
	DEFINE_GETTER(species)
	DEFINE_GETTER(level)
	DEFINE_GETTER(current_hp)
	DEFINE_GETTER_SETTER(nickname)
	int get_max_hp(){
		return this->get_stat(PokemonStats::StatId::Hp);
	}
	const char *get_display_name() const;
	StatusCondition2 get_status() const;
	void heal();
	const BasePokemonInfo &get_data() const;
};

class Party{
public:
	static const size_t max_party_size = 6;
private:
	std::vector<Pokemon> members;
public:
	Party() = default;
	bool add_pokemon(SpeciesId, int level, std::uint16_t original_trainer_id, XorShift128 &);
	bool add_pokemon(const Pokemon &);
	Pokemon &get_last_added_pokemon();
	void heal();
	int size() const{
		return (int)this->members.size();
	}
	Pokemon *get_first_usable_pokemon();
	iterator_range<std::vector<Pokemon>::iterator> iterate(){
		return {this->members.begin(), this->members.end()};
	}
	Pokemon &get(size_t i);
	const Pokemon &get(size_t i) const;
};

}
