#pragma once

#include <memory>

class WorldData;
class WorldBuilder
{
public:

	WorldBuilder(std::shared_ptr<WorldData> data);

protected:

	std::shared_ptr<WorldData> data_{};
};