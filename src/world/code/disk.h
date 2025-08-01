#pragma once

#include <memory>

#include "device.h"
#include "filesystem.h"

#include <print>

class File;
class FileSystem;

class Disk : public Device
{
public:

	Disk() = default;
	Disk(int32_t size) : size_(size) {}

	virtual void config_device(std::string_view cmd) override {};
	virtual std::string get_device_id() const override { return "Physical Drive"; }
	virtual std::string get_driver_id() const override { return "disk"; }

	FileSystem& create_fs();
	FileSystem* get_fs() { return fs_.get(); }

	int32_t get_physical_size() const { return size_; }
	void set_physical_size(int32_t new_size) { size_ = new_size; }

private:

	int32_t size_ = 0;
	std::unique_ptr<FileSystem> fs_{nullptr};
	
};