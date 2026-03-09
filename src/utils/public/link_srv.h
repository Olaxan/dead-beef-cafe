#pragma once
#pragma once

#include <set>
#include <print>
#include <coroutine>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class LinkServer;

class ILinkable
{
public:

	virtual void on_linked(LinkServer*, ILinkable*) = 0;
	virtual void on_unlinked(LinkServer*, ILinkable*) = 0;

};

class LinkServer
{
public:

	LinkServer() = default;
	LinkServer(LinkServer&) = delete;

	/* Adds a link between two nodes. */
	void link(ILinkable* first, ILinkable* second)
	{
		links_.insert(std::pair{first, second});
		links_.insert(std::pair{second, first});
		first->on_linked(this, second);
		second->on_linked(this, first);
	}

	/* Removes a specific link between two nodes. */
	void unlink(ILinkable* first, ILinkable* second)
	{
		// links_.erase(std::make_pair(first, second));
		// links_.erase(std::make_pair(second, first));
	}

	/* Removes a node entirely from the directory,
	and removes all links to and from it. */
	void purge(ILinkable* node)
	{
		links_.erase(node);
		auto range = links_.equal_range(node);
		for (auto it = range.first; it != range.second;)
		{
			if (it->second == node)
				it = links_.erase(it);
			else
				++it;
		}
	}

	void register_node(ILinkable* node)
	{
		nodes_.insert(node);
	}

	void unregister_node(ILinkable* node)
	{
		if (nodes_.erase(node)) { purge(node); }
	}

protected:

	std::unordered_multimap<ILinkable*, ILinkable*> links_{};
	std::unordered_set<ILinkable*> nodes_{};

};