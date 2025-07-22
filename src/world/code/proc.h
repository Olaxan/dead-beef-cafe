#pragma once

#include "task.h"

#include <iostream>

using ProcessTask = Task<int32_t, std::suspend_always>;

struct Proc
{

	Proc(int32_t pid, OS* owner, std::istream& in_stream, std::ostream& out_stream)
		: pid(pid), in_stream(in_stream), out_stream(out_stream), owning_os(owner) { }

	Proc(int32_t pid, OS* owner)
		: Proc(pid, owner, std::cin, std::cout) { }

	int32_t pid{0};
	std::istream& in_stream;
	std::ostream& out_stream;
	OS* owning_os{nullptr};

	std::string read()
	{
		std::string temp{};
		std::getline(in_stream, temp);
		return temp; //RVO?
	}

	template<typename ...Args>
	void write(std::string&& out_str, Args&& ...args)
	{
		std::println(out_stream, std::forward<Args>(args));
	}
};