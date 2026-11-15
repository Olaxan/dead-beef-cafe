#include <print>
#include <fstream>
#include <filesystem>

#include "CLI/CLI.hpp"

#include "flatbuffers/flatbuffers.h"
#include <flatbuffers/idl.h>
#include <flatbuffers/flatbuffers.h>

#include "manifest_generated.h"

#include <iso646.h>

using namespace flatbuffers;
using namespace DBC::Assets;

int main(int argc, char* argv[])
{
	struct PackerParms
	{
		std::filesystem::path input{"assets/data/manifest.json"};
		std::filesystem::path output{"assets/data.pack"};
		bool export_schema{false};
	} params;

	CLI::App app{"Asset packer for DBC."};

	app.add_option("IN", params.input, "Input path for the packer (a manifest file).")->capture_default_str();
	app.add_option("OUT", params.output, "Output path for the packer (a packed file).")->capture_default_str();

	CLI11_PARSE(app, argc, argv);

	std::string json;
	bool ok = LoadFile(params.input.string().c_str(), false, &json);

	if (not ok)
	{
		std::println(std::cerr, "Failed to open file {} for reading.", params.input.string());
		return 1;
	}

	flatbuffers::Parser json_parser;
	json_parser.opts.strict_json = true;

	if (not json_parser.Deserialize(Manifest::BinarySchema::data(), Manifest::BinarySchema::size()))
	{
		std::println(std::cerr, "Failed to load Manifest schema: {}.", json_parser.error_);
		return 1;
	}

	if (not json_parser.ParseJson(json.c_str()))
	{
		std::println(std::cerr, "Failed to parse input file: {}.", json_parser.error_);
		return 1;
	}

	const Manifest* manifest = GetManifest(json_parser.builder_.GetBufferPointer());
	
	std::ofstream out_stream{params.output, std::ofstream::out};

	if (not out_stream)
	{
		std::println(std::cerr, "Failed to open {} for writing.", params.output.string());
	}

	if (out_stream)
	{
		auto pages = manifest->pages();
		for (std::size_t i = 0; i < pages->size(); ++i)
		{
			out_stream << pages->Get(i)->name()->str() << "\n";
		}
	}

	return 0;
}