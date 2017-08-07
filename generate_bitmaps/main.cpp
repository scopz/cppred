#include "../common/csv_parser.h"
#include <sstream>
#include <iostream>
#include <map>

class BitmapMember{
public:
	int offset;
	std::string name;
	std::string comment;
};

class Bitmap{
public:
	std::string name;
	std::vector<BitmapMember> members;

	void erase_skips(){
		std::vector<BitmapMember> temp;
		for (auto &m : this->members){
			if (m.name == "skip")
				continue;
			temp.push_back(m);
		}
		this->members = std::move(temp);
	}
};

unsigned to_unsigned(const std::string &s){
	std::stringstream stream(s);
	unsigned ret;
	if (!(stream >> ret))
		throw std::runtime_error("Can't convert \"" + s + "\" to integer.");
	return ret;
}

std::vector<Bitmap> parse_bitmaps_file(){
	std::map<std::string, Bitmap> bitmaps;

	const std::vector<std::string> order = { "struct", "member", "comment", };
	CsvParser csv("input/bitmaps.csv");

	auto rows = csv.row_count();
	for (size_t i = 0; i < rows; i++){
		auto columns = csv.get_ordered_row(i, order);
		auto it = bitmaps.find(columns[0]);
		if (it == bitmaps.end()){
			bitmaps[columns[0]] = Bitmap();
			it = bitmaps.find(columns[0]);
			it->second.name = columns[0];
		}
		BitmapMember member = {
			(int)it->second.members.size(),
			columns[1],
			columns[2],
		};

		it->second.members.push_back(member);
	}

	std::vector<Bitmap> ret;
	for (auto &kv : bitmaps){
		ret.push_back(kv.second);
		ret.back().erase_skips();
	}

	return ret;
}

void generate_header(const decltype(parse_bitmaps_file()) &bitmaps){
	std::ofstream file("output/bitmaps.h");

	file << "//This file is autogenerated. Do not edit.\n\n";

	for (auto &bitmap : bitmaps){
		auto struct_name = bitmap.name + "_struct";
		auto class_name = bitmap.name + "_wrapper";
		file << "struct " << struct_name << "{\n";
		for (auto &member : bitmap.members){
			file << "    //bit " << member.offset << "\n";
			if (member.comment.size())
				file << "    //" << member.comment << "\n";
			file << "    bool " << member.name << ";\n";
		}

		file <<
			"};\n"
			"\n"
			"class " << class_name << "{\n"
			"public:\n"
			"    typedef typename WrapperSelector<std::uint8_t, 1>::type member_type;\n"
			"    typedef typename member_type::callback_struct callback_struct;\n"
			"    static const size_t size = 1;\n"
			"private:\n"
			"    member_type wrapped_value;\n"
			"public:\n"
			"    " << class_name << "(void *memory, const callback_struct &callbacks): wrapped_value((char *)memory, callbacks){}\n"
			"    " << class_name << "(const " << class_name << " &) = default;\n"
			"    " << class_name << "(" << class_name << " &&) = delete;\n"
			"    void operator=(const " << class_name << " &) = delete;\n"
			"    void operator=(" << class_name << " &&) = delete;\n"
			"    operator " << struct_name << "() const;\n"
			"    void operator=(const " << struct_name << " &);\n"
			"    void clear(){\n"
			"        this->wrapped_value = 0;\n"
			"    }\n"
			"    byte_t get_raw_value() const{\n"
			"        return this->wrapped_value;\n"
			"    }\n"
			"    void set_raw_value(byte_t value){\n"
			"        this->wrapped_value = value;\n"
			"    }\n"
		;

		for (auto &member : bitmap.members)
			file <<
				"    bool get_" << member.name << "() const;\n"
				"    void set_" << member.name << "(bool);\n";
		
		file << "};\n\n";
	}
}

void generate_source(const decltype(parse_bitmaps_file()) &bitmaps){
	std::ofstream file("output/bitmaps.inl");
	
	file << "//This file is autogenerated. Do not edit.\n\n";

	for (auto &bitmap : bitmaps){
		auto struct_name = bitmap.name + "_struct";
		auto class_name = bitmap.name + "_wrapper";

		file <<
			class_name << "::operator " << struct_name << "() const{\n"
			"    byte_t val = this->wrapped_value;\n"
			"    " << struct_name << " ret;\n"
		;
		for (auto &member : bitmap.members)
			file << "    ret." << member.name << " = check_flag(val, (byte_t)(1 << " << member.offset << "));\n";
		file <<
			"    return ret;\n"
			"}\n"
			"\n"
			"void " << class_name << "::operator=(const " << struct_name << " &s){\n"
			"    byte_t val = 0;\n"
		;
		for (auto &member : bitmap.members)
			file << "    val |= !!s." << member.name << " << " << member.offset << ";\n";
		file <<
			"}\n"
			"\n"
		;
		for (auto &member : bitmap.members){
			file <<
				"bool " << class_name << "::get_" << member.name << "() const{\n"
				"    byte_t val = this->wrapped_value;\n"
				"    return check_flag(val, (byte_t)(1 << " << member.offset << "));\n"
				"}\n"
				"\n"
				"void " << class_name << "::set_" << member.name << "(bool v){\n"
				"    const byte_t mask = 1 << " << member.offset << ";\n"
				"    byte_t val = this->wrapped_value;\n"
				"    val &= ~mask;\n"
				"    val |= mask * v;\n"
				"    this->wrapped_value = val;\n"
				"}\n"
				"\n"
			;
		}
	}
}

void generate_files_for_rams(const decltype(parse_bitmaps_file()) &bitmaps){
	std::ofstream constructors("output/bitmaps_rams_constructors.inl");
	std::ofstream declarations("output/bitmaps_rams_declarations.inl");

	constructors << "//This file is autogenerated. Do not edit.\n\n";
	declarations << "//This file is autogenerated. Do not edit.\n\n";


	for (auto &bitmap : bitmaps){
		auto type_name = bitmap.name + "_t";
		auto class_name = bitmap.name + "_wrapper";
		auto constructor_name = "construct_" + type_name;
		constructors <<
			"std::unique_ptr<Type> " << constructor_name << "(){\n"
			"    return std::make_unique<PackedBitsWrapper>(\"" << class_name << "\");\n"
			"}\n"
			"\n"
		;

		declarations << "{ \"" << type_name << "\", " << constructor_name << " },\n";
	}
}

void generate_bitmaps(){
	auto bitmaps = parse_bitmaps_file();
	generate_header(bitmaps);
	generate_source(bitmaps);
	generate_files_for_rams(bitmaps);
}

int main(){
	try{
		generate_bitmaps();
	}catch (std::exception &e){
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return 0;
}
