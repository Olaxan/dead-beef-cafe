#include <print>
#include <fstream>
#include <filesystem>
#include <sstream>

#include "CLI/CLI.hpp"

#include "flatbuffers/flatbuffers.h"
#include <flatbuffers/idl.h>
#include <flatbuffers/flatbuffers.h>

#include "manifest_generated.h"
#include "asset_pack_generated.h"

#include <iso646.h>

using namespace flatbuffers;
using namespace DBC::Assets;

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	struct PackerParms
	{
		std::filesystem::path input{"assets/data/manifest.json"};
		std::filesystem::path output{"assets/data.pack"};
		std::string pack_name{"Assets"};
		bool export_schema{false};
		bool follow_symlinks{true};
	} params;

	CLI::App app{"Asset packer for DBC."};

	app.add_option("IN", params.input, "Input path for the packer (a manifest file).")->capture_default_str();
	app.add_option("OUT", params.output, "Output path for the packer (a packed file).")->capture_default_str();
	app.add_option("--name", params.pack_name, "The internal name of the resulting asset pack.")->capture_default_str();
	app.add_flag("-s,--follow-symlinks", params.follow_symlinks, "Whether to follow symlinks while recursing.")->capture_default_str();

	CLI11_PARSE(app, argc, argv);

	params.input.make_preferred();
	params.output.make_preferred();

	std::string json;
	bool ok = LoadFile(params.input.string().c_str(), false, &json);

	if (not ok)
	{
		std::println(std::cerr, "Failed to open file {} for reading.", params.input.string());
		return 1;
	}

	flatbuffers::Parser json_parser;
	flatbuffers::FlatBufferBuilder builder;
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

	auto create_pack_content = [&builder, &params](fs::path path, bool recurse = false) -> std::vector<flatbuffers::Offset<AssetDataItem>>
	{
		std::vector<flatbuffers::Offset<AssetDataItem>> out;

		auto opts = fs::directory_options::skip_permission_denied;
		if (params.follow_symlinks)
		{
			opts |= fs::directory_options::follow_directory_symlink;
		}

		if (not fs::is_directory(path))
		{
			std::cerr << path << " is not a directory.\n";
			return out;
		}

		for (auto&& dir_entry : fs::recursive_directory_iterator(path, opts))
		{
			std::string entry_path = dir_entry.path().string();
			std::string entry_name = dir_entry.path().filename().string();

			if (not dir_entry.is_regular_file())
			{
				std::println(std::cerr, "Skipping file '{}': not a regular file.", entry_path);
				continue;
			}

			std::ifstream file{dir_entry.path(), std::ios::binary | std::ios::ate};

			if (not file)
			{
				std::println(std::cerr, "Failed to open file '{}' for reading.", entry_path);
				continue;
			}

			const auto size = file.tellg();

			if (size < 0)
			{
				std::println(std::cerr, "Error reading '{}': got 0 bytes!", entry_path);
				continue;
			}

			file.seekg(0);
			std::vector<uint8_t> buffer(static_cast<std::size_t>(size));

			if (not file.read(reinterpret_cast<char*>(buffer.data()), size))
			{
				std::println(std::cerr, "Error reading '{}': failed to read file contents!", entry_path);
				continue;
			}

			auto offset_name = builder.CreateString(entry_name);
			auto offset_data = builder.CreateVector(buffer);
			auto item = CreateAssetDataItem(builder, offset_name, offset_data);
			
			std::println("Packed '{}' ({} bytes).", entry_name, buffer.size());
			
			out.emplace_back(std::move(item));
		}

		return out;
	};

	const Manifest* manifest = GetManifest(json_parser.builder_.GetBufferPointer());
	fs::path manifest_path = params.input.parent_path();

	auto asset_name_offset = builder.CreateString(params.pack_name);

	std::vector<flatbuffers::Offset<AssetDataCollection>> collections;

	auto pages = manifest->pages();
	for (std::size_t i = 0; i < pages->size(); ++i)
	{
		auto&& page = pages->Get(i);

		auto pack_name = page->name();
		auto pack_type = page->type();
		auto pack_tags = page->tags();
		auto pack_name_offset = builder.CreateString(pack_name);
		
		auto pack_path = fs::path{page->path()->str()};
		auto pack_recurse = page->recurse();
		
		auto real_path = pack_path.is_absolute() ? pack_path : manifest_path / pack_path;
		auto pack_content = create_pack_content(real_path, pack_recurse);

		std::vector<flatbuffers::Offset<flatbuffers::String>> pack_tag_offsets;
		if (pack_tags)
		{
			pack_tag_offsets.reserve(pack_tags->size());
			for (std::size_t tag_index = 0; tag_index < pack_tags->size(); ++tag_index)
			{
				pack_tag_offsets.emplace_back(builder.CreateString(pack_tags->Get(tag_index)->str()));
			}
		}

		auto pack_tags_offset = builder.CreateVector(pack_tag_offsets);
		auto pack_content_offset = builder.CreateVector(pack_content);
		auto collection_offset = CreateAssetDataCollection
		(
			builder,
			pack_name_offset,
			pack_type,
			pack_tags_offset,
			pack_content_offset
		);

		collections.emplace_back(collection_offset);

		std::println("Created asset collection '{}'.", pack_name->str());
	}

	auto collections_offset = builder.CreateVector(collections);
	auto assets = CreateAssetData(builder, asset_name_offset, collections_offset);

	builder.Finish(assets);
	
	std::ofstream out_stream{params.output, std::ofstream::out | std::ofstream::binary};

	if (not out_stream)
	{
		std::println(std::cerr, "Failed to open {} for writing.", params.output.string());
	}

	if (out_stream)
	{
		out_stream.write(
			reinterpret_cast<const char*>(builder.GetBufferPointer()),
			static_cast<std::streamsize>(builder.GetSize()));

		if (not out_stream)
		{
			std::println(std::cerr, "Failed to write {}.", params.output.string());
			return 1;
		}

		std::println("Wrote asset pack '{}' ({} bytes).", params.output.string(), builder.GetSize());
	}

	return 0;
}