#include <print>
#include <fstream>
#include <filesystem>

#include "CLI/CLI.hpp"

int main(int argc, char* argv[])
{
	struct PackerParms
	{
		std::filesystem::path input{"assets/data"};
		std::filesystem::path output{"assets/data.pack"};
	} params;

	CLI::App app{"Asset packer for DBC."};

	app.add_option("IN", params.input, "Input path for the packer (a directory).")->capture_default_str();
	app.add_option("OUT", params.output, "Output path for the packer (a file).")->capture_default_str();

	CLI11_PARSE(app, argc, argv);

	std::ofstream out_stream{params.output, std::ofstream::out};

	if (out_stream)
	{
		out_stream << "Test.";
	}

	return 0;
}