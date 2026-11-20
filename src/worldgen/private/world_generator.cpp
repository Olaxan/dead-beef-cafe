#include "world_generator.h"

#include "world_data.h"
#include "tag_provider.h"
#include "business.h"
#include "person.h"

#include "business_template.h"

void WorldGenerator::add_template(BusinessGenerationTemplate&& template)
{
	business_templates_.emplace_back(std::move(template));
}

void WorldGenerator::configure()
{
	add_template(BusinessGenerationTemplate{
		.tag = "business_garage",
		.num_employees_range = std::make_pair(1, 2),
		.num_instances_per_country_range = std::make_pair(5, 10)
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
	auto&& corps = world_->corps;
	auto&& people = world_->people;

	for (auto&& locale : locales_)
	{
		for (const auto& template : business_templates_)
		{
			std::size_t instances = random_from_range(template.num_instances_per_country_range);

			for (std::size_t i = 0; i < instances; ++i)
			{
				generate_corporation(locale, template);
			}
		}

		for (const auto&& template : person_templates_)
		{
			std::size_t instances = random_from_range(template.num_instances_per_country_range);

			for (std::size_t i = 0; i < instances; ++i)
			{
				generate_person(locale, template);
			}
		}
	}
}

std::optional<BusinessId> WorldGenerator::generate_business(const ITagProvider& provider, const BusinessGenerationTemplate& template)
{
	auto&& corps = world_->corps;

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

std::optional<PersonId> WorldGenerator::generate_person(const ITagProvider& provider)
{
	auto&& people = world_->people;

	Person out
	{
		.forename = provider.get_by_tag("given_name").value_or("Anonymous"),
		.surname = provider.get_by_tag("family_name").value_or("")
	}

	int32_t out_id = next_person_id_++
	if (people.try_emplace(out_id, std::move(out)))
	{
		return PersonId{.id = out_id};
	}

	return std::nullopt;
}
