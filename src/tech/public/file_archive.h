#pragma once

#include "file_archive_types.h"

#include "uid64.h"
#include "proc_types.h"
#include "tag_mgr.h"
#include "tag_provider.h"
#include "archive_base.h"
#include "datasource_base.h"

#include <variant>
#include <optional>
#include <unordered_map>
#include <string_view>


class FileArchive : public IArchiveBase, ITagProvider<ArHandle>
{
public:

	FileArchive();

	void add_datasource(std::shared_ptr<IDatasourceBase> source) override;

	ArHandle add_file(std::string content, std::vector<std::string>&& tags);
	ArHandle add_program(ProcessFn&& func, std::vector<std::string>&& tags);

	std::optional<ArHandle> get_by_tag(std::string tag) const override;
	std::optional<ArHandle> get_by_tags(std::vector<std::string> tags, RequireTagMode mode = RequireTagMode::All) const override;

	std::optional<ArHandle> find_by_digest(std::string_view data) const;
	
	std::optional<std::string_view> try_get_bytes(ArHandle h) const;

	std::optional<ProcessFn> try_get_exec(std::string_view data) const;
	std::optional<ProcessFn> try_get_exec(ArHandle handle) const;

	std::vector<std::string> get_all_program_tags() const;
	std::vector<ArHandle> get_all_programs() const;

protected:

protected:

	int64_t next_h_{1};

	std::vector<std::shared_ptr<IDatasourceBase>> sources_;

	TagManager<ArHandle> tag_lookup_;
	std::unordered_map<std::size_t, ArHandle> hash_lookup_;
	std::unordered_map<ArHandle, ProcessFn> execs_;
	std::unordered_map<ArHandle, ArPage> pages_;

};