#include "Maps.h"
#include "utility.h"
#include "../common/csv_parser.h"

Maps::Maps(const char *maps_path, const char *map_data_path, const Tilesets &tilesets){
	static const std::vector<std::string> order = { "name", "tileset", "width", "height", "map_data", "script", "objects", };
	
	auto maps_data = read_data_csv(map_data_path);

	CsvParser csv(maps_path);
	auto rows = csv.row_count();

	for (size_t i = 0; i < rows; i++){
		auto columns = csv.get_ordered_row(i, order);
		this->maps.emplace_back(new Map(columns, tilesets, maps_data));
		auto back = this->maps.back();
		this->map[back->get_name()] = back;
	}
}

Map::Map(const std::vector<std::string> &columns, const Tilesets &tilesets, const data_map_t &maps_data){
	this->name = columns[0];
	this->tileset = tilesets.get(columns[1]);
	this->width = to_unsigned(columns[2]);
	this->height = to_unsigned(columns[3]);
	auto it = maps_data.find(columns[4]);
	if (it == maps_data.end())
		throw std::runtime_error("Error: Map \"" + this->name + "\" references unknown map data \"" + columns[4] + "\"");
	this->map_data = it->second;
}

std::shared_ptr<Map> Maps::get(const std::string &name){
	auto it = this->map.find(name);
	if (it == this->map.end())
		throw std::runtime_error("Error: Invalid map \"" + name + "\"");
	return it->second;
}

void Map::render_to_file(const char *path){
	const auto block_size = 4;
	const auto block_pixel_size = block_size * Tile::size;
	auto w = this->width * block_size;
	auto h = this->height * block_size;
	std::vector<byte_t> final_tiles(w * h);
	auto &blockset = this->tileset->get_blockset();
	for (unsigned y = 0; y < this->height; y++){
		for (unsigned x = 0; x < this->width; x++){
			auto base_dst = &final_tiles[x * block_size + y * block_size * w];
			auto block_id = (*this->map_data)[x + y * this->width];
			auto base_block = &blockset[block_size * block_size * block_id];
			for (unsigned y2 = 0; y2 < block_size; y2++)
				for (unsigned x2 = 0; x2 < block_size; x2++)
					base_dst[x2 + y2 * w] = *(base_block++);
		}
	}
	auto pixel_w = w * Tile::size;
	auto pixel_h = h * Tile::size;
	std::vector<pixel> pixel_data(pixel_w * pixel_h);
	auto &tiles = this->tileset->get_tiles().tiles;
	for (unsigned y = 0; y < h; y++){
		for (unsigned x = 0; x < w; x++){
			auto src = final_tiles[x + y * w];
			if (src >= tiles.size())
				throw std::runtime_error("Map::render_to_file(): Inconsistent source data.");
			auto pixels = tiles[src].pixels;
			auto dst = &pixel_data[x * Tile::size + y * Tile::size * pixel_w];
			for (unsigned y2 = 0; y2 < Tile::size; y2++)
				for (unsigned x2 = 0; x2 < Tile::size; x2++)
					dst[x2 + y2 * pixel_w] = *(pixels++);
		}
	}

	save_png(path, &pixel_data[0], pixel_w, pixel_h);
}
