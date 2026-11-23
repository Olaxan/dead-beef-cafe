#pragma once

#include "country.h"

#include "business_types.h"
#include "person_types.h"

#include "business_template.h"
#include "person_template.h"

#include "tag_provider.h"

#include <random>
#include <expected>
#include <exception>
#include <optional>
#include <functional>
#include <memory>

struct WorldData;
class WorldGenerator
{
public:

	using WorldGeneratorLogger = std::function<void(std::string)>;

	WorldGenerator() = default;

	void add_template(BusinessGenerationTemplate&& temp);
	void add_template(PersonGenerationTemplate&& temp);
	void add_country(Country&& locale);

	void configure();
	void generate();

	std::optional<BusinessId> generate_business(const StringTagProvider& provider, const BusinessGenerationTemplate& temp);
	std::optional<PersonId> generate_person(const StringTagProvider& provider, const PersonGenerationTemplate& temp);

	void set_logger(WorldGeneratorLogger&& logger);

protected:

	void log(std::string line);

	int32_t next_person_id_{0};
	int32_t next_business_id_{0};

	WorldGeneratorLogger logger_{nullptr};

	std::vector<Country> countries_;
	std::vector<BusinessGenerationTemplate> business_templates_;
	std::vector<PersonGenerationTemplate> person_templates_;

	std::shared_ptr<WorldData> world_{};

};