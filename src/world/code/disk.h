#pragma once

#include <memory>

#include "device.h"
#include <print>

class File;
class FileSystem;

class Disk : public Device
{
public:

	Disk() = default;
	Disk(int32_t size) : size_(size) {}

	virtual void start_device() override {};
	virtual void shutdown_device() override { std::println("Shutdown disk."); };
	virtual void config_device(std::string_view cmd) override {};

	FileSystem& create_fs();

	void set_physical_size(int32_t new_size) { size_ = new_size; }

private:

	int32_t size_ = 0;
	std::unique_ptr<FileSystem> fs_{nullptr};
	
};