#pragma once

#include "business_types.h"
#include "person_types.h"

#include "business_template.h"
#include "person_template.h"

#include <random>
#include <expected>
#include <exception>
#include <optional>

class ITagProvider;
class WorldGenerator
{
public:

	WorldGenerator() = default;

	void add_template(BusinessGenerationTemplate&& template);
	void add_template(PersonGenerationTemplate&& template);
	void add_locale(Locale locale);

	void configure();
	void generate();

	std::optional<BusinessId> generate_business(const ITagProvider& provider, const BusinessGenerationTemplate& template);
	std::optional<PersonId> generate_person(const ITagProvider& provider, const PersonGenerationTemplate& template);

protected:

	int32_t next_person_id_{0};
	int32_t next_business_id_{0};

	std::vector<Locale> locales_;
	std::vector<BusinessGenerationTemplate> business_templates_;
	std::vector<PersonGenerationTemplate> person_templates_;

	std::shared_ptr<WorldData> world_{};

};