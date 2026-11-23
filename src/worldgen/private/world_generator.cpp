#include "world_generator.h"

#include "world_data.h"
#include "tag_provider.h"
#include "business.h"
#include "person.h"

#include "business_template.h"

void WorldGenerator::add_template(BusinessGenerationTemplate&& temp)
{
	business_templates_.emplace_back(std::move(temp));
}

void WorldGenerator::add_country(Country&& locale)
{
	countries_.emplace_back(std::move(locale));
}

void WorldGenerator::configure()
{
	add_template(BusinessGenerationTemplate{
		.tag = "business_garage",
		.num_employees_range = std::make_pair(1, 2),
		.num_instances_per_country_range = std::make_pair(5, 10),
		.roles = {
			BusinessRole::Owner
		}
	});

	add_template(PersonGenerationTemplate{
		.tag = "person_hacker"
	});
}

void WorldGenerator::generate()
{
	const auto& corps = world_->corps;
	const auto& people = world_->people;

	for (auto&& country : countries_)
	{
		for (auto&& temp : business_templates_)
		{
			std::size_t instances = random_from_range(temp.num_instances_per_country_range);

			for (std::size_t i = 0; i < instances; ++i)
			{
				generate_business(country, temp);
			}
		}

		for (auto&& temp : person_templates_)
		{
			std::size_t instances = random_from_range(temp.num_instances_per_country_range);

			for (std::size_t i = 0; i < instances; ++i)
			{
				generate_person(country, temp);
			}
		}
	}
}

std::optional<BusinessId> WorldGenerator::generate_business(const StringTagProvider& provider, const BusinessGenerationTemplate& temp)
{
	const auto& corps = world_->corps;

	auto fill_roles = [&](const std::vector<BusinessRole>& roles) -> std::vector<BusinessRolePair>
	{
		std::vector<BusinessRolePair> out;

		for (auto&& role : roles)
		{
			if (auto opt_person = generate_person(provider); opt_person.has_value())
			{
				out.emplace_back(*opt_person, role);
			}
		}

		return out;
	};

	Business out
	{
		.name = provider.get_by_tag("business_name").value_or("<INVALID BUSINESS NAME>"),
		.roles = fill_roles(template.roles),
		.num_employees = random_from_range(template.num_employees_range),
		.type = BusinessType::Computing
	};

	int32_t out_id = next_business_id_++
	if (corps.try_emplace(out_id, std::move(out)))
	{
		return BusinessId{.id = out_id};
	}

	return std::nullopt;
}

void WorldGenerator::set_logger(WorldGeneratorLogger&& logger)
{
	logger_ = std::move(logger);
}

void WorldGenerator::log(std::string line)
{
	if (logger_)
	{
		logger_(line);
	}
}

std::optional<PersonId> WorldGenerator::generate_person(const StringTagProvider& provider, const PersonGenerationTemplate& temp)
{
	auto&& people = world_->people;

	Person out
	{
		.forename = provider.get_by_tag("given_name").value_or("Anonymous"),
		.surname = provider.get_by_tag("family_name").value_or("")
	};

	auto out_id = PersonId{ .id = ++next_person_id_ };
	if (auto [it, success] = people.try_emplace(out_id, std::move(out)); success)
	{
		return out_id;
	}

	return std::nullopt;
}
