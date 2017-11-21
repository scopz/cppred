#include "utility.h"
#include "../common/sha1.h"
#include "../common/csv_parser.h"
#include "../common/base64.h"
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cctype>
#include <iomanip>
#include "../FreeImage/Source/ZLib/zlib.h"

const char * const generated_file_warning = "//This file is autogenerated. Do not edit.\n";

static bool to_unsigned_internal(unsigned &dst, const std::string &s){
	std::stringstream stream(s);
	return !!(stream >> dst);
}

static bool to_int_internal(int &dst, const std::string &s){
	std::stringstream stream(s);
	return !!(stream >> dst);
}

unsigned to_unsigned(const std::string &s){
	unsigned ret;
	if (!to_unsigned_internal(ret, s))
		throw std::runtime_error("Can't convert \"" + s + "\" to integer.");
	return ret;
}

int to_int(const std::string &s){
	int ret;
	if (!to_int_internal(ret, s))
		throw std::runtime_error("Can't convert \"" + s + "\" to integer.");
	return ret;
}

bool hex_no_prefix_to_unsigned_internal(unsigned &dst, const std::string &s){
	std::stringstream stream;
	stream << std::hex << s;
	return !!(stream >> dst);
}

unsigned hex_no_prefix_to_unsigned(const std::string &s){
	unsigned ret;
	if (!hex_no_prefix_to_unsigned_internal(ret, s))
		throw std::runtime_error((std::string)"Can't convert \"" + s + "\" from hex to integer.");
	return ret;
}

std::string hash_buffer(const void *data , size_t size, const char *date_string){
	SHA1 sha1;
	sha1.Input(data, size);
	return sha1.ToString();
}

std::string hash_file(const std::string &path, const char *date_string){
	std::ifstream file(path, std::ios::binary);
	if (!file)
		throw std::runtime_error("hash_file(): File not found: " + path);

	file.seekg(0, std::ios::end);
	std::vector<unsigned char> data((size_t)file.tellg());
	file.seekg(0);
	file.read((char *)&data[0], data.size());

	SHA1 sha1;
	sha1.Input(&data[0], data.size());
	for (auto s = date_string; *s; s++)
		sha1.Input(*s);
	return sha1.ToString();
}

std::string hash_files(const std::vector<std::string> &files, const char *date_string){
	SHA1 sha1;
	for (auto &path : files){
		std::ifstream file(path, std::ios::binary);
		if (!file)
			throw std::runtime_error("hash_files(): File not found: " + path);

		file.seekg(0, std::ios::end);
		std::vector<unsigned char> data((size_t)file.tellg());
		file.seekg(0);
		file.read((char *)&data[0], data.size());
		sha1.Input(&data[0], data.size());
	}

	for (auto s = date_string; *s; s++)
		sha1.Input(*s);

	return sha1.ToString();
}

bool check_for_known_hash(const known_hashes_t &known_hashes, const std::string &key, const std::string &value){
	auto it = known_hashes.find(key);
	if (it == known_hashes.end())
		return false;
	return it->second == value;
}

bool is_hex(char c){
	return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

unsigned to_unsigned_default(const std::string &s, unsigned def){
	unsigned ret;
	if (!to_unsigned_internal(ret, s))
		ret = def;
	return ret;
}

unsigned hex_no_prefix_to_unsigned_default(const std::string &s, unsigned def){
	unsigned ret;
	if (!hex_no_prefix_to_unsigned_internal(ret, s))
		ret = def;
	return ret;
}

bool case_insensitive(const std::string &a, const char *b){
	for (auto c : a){
		if (!*b)
			return false;
		if (tolower(c) != tolower(*b))
			return false;
		b++;
	}
	return !*b;
}

bool to_bool(const std::string &s){
	unsigned value;
	if (to_unsigned_internal(value, s))
		return !!value;
	if (case_insensitive(s, "true"))
		return true;
	if (case_insensitive(s, "false"))
		return false;
	throw std::runtime_error("Can't convert \"" + s + "\" to a bool.");
}

const char *bool_to_string(bool value){
	return value ? "true" : "false";
}

void write_buffer_to_stream(std::ostream &stream, const std::vector<std::uint8_t> &buffer){
	stream << "{\n";
	bool new_line = true;
	int line_width = 0;
	auto size = buffer.size();
	for (size_t i = 0; i < size; i++){
		if (new_line){
			stream << "   ";
			line_width += 3;
			new_line = false;
		}
		stream << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i] << std::dec << ",";
		line_width += 6;
		if (line_width >= 80){
			line_width = 0;
			new_line = true;
			stream << std::endl;
		}
	}
	if (!new_line)
		stream << std::endl;
	stream << "}";
}

void write_varint(std::vector<std::uint8_t> &dst, std::uint32_t n){
	do{
		auto m = n & 0x7F;
		n >>= 7;
		m |= (1 << 7) * !!n;
		dst.push_back(m);
	}while (n);
}

void write_ascii_string(std::vector<std::uint8_t> &dst, const std::string &s){
	for (auto c : s){
		if (!c)
			break;
		dst.push_back(c);
	}
	dst.push_back(0);
}

data_map_t read_data_csv(const char *path){
	static const std::vector<std::string> order = { "name", "data", };

	CsvParser csv(path);
	auto rows = csv.row_count();

	std::map<std::string, std::shared_ptr<std::vector<byte_t>>> ret;
	for (size_t i = 0; i < rows; i++){
		auto columns = csv.get_ordered_row(i, order);
		ret[std::move(columns[0])] = std::make_shared<std::vector<byte_t>>(base64_decode(columns[1]));
	}
	return ret;
}

void write_data_csv(const char *path, const data_map_t &map){
	std::ofstream file(path);
	file << "name,data\n";
	for (auto &kv : map)
		file << kv.first << "," << base64_encode(*kv.second) << std::endl;
}

std::vector<byte_t> compress_memory_DEFLATE(std::vector<byte_t> &in_data){
	std::vector<byte_t> ret(in_data.size() * 2);

	z_stream stream;
	stream.zalloc = nullptr;
	stream.zfree = nullptr;
	stream.next_in = &in_data[0];
	stream.avail_in = in_data.size();
	stream.next_out = &ret[0];
	stream.avail_out = ret.size();

	deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
	size_t bytes_written = 0;

	while (stream.avail_in){
		int res = deflate(&stream, Z_NO_FLUSH);
		assert(res == Z_OK);
		if (!stream.avail_out){
			stream.avail_out = ret.size();
			auto n = ret.size();
			ret.resize(n * 2);
			bytes_written = n;
			stream.next_out = &ret[n];
		}
	}

	int deflate_res = Z_OK;
	while (deflate_res == Z_OK){
		if (!stream.avail_out){
			stream.avail_out = ret.size();
			auto n = ret.size();
			ret.resize(n * 2);
			bytes_written = n;
			stream.next_out = &ret[n];
		}
		deflate_res = deflate(&stream, Z_FINISH);
	}

	assert(deflate_res == Z_STREAM_END);
	bytes_written += ret.size() - bytes_written - stream.avail_out;
	ret.resize(bytes_written);
	deflateEnd(&stream);
	return ret;
}

void write_buffer_to_header_and_source(std::ostream &header, std::ostream &source, const std::vector<byte_t> &data, const char *array_name){
	header << "extern const byte_t " << array_name << "[" << data.size() << "];\n"
		"static const size_t " << array_name << "_size = " << data.size() << ";\n";

	source << "extern const byte_t " << array_name << "[" << data.size() << "] = ";
	write_buffer_to_stream(source, data);
	source << ";\n";
}
