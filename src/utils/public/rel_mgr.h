#pragma once

#include <concepts>
#include <unordered_map>
#include <unordered_set>

enum class RelationshipType
{
	OneToOne,
	OneToMany,
    ManyToMany
};

template <RelationshipType type, typename A, typename B>
struct RelationshipPolicy;

template <typename A, typename B>
struct RelationshipPolicy<RelationshipType::OneToOne, A, B> 
{
	std::unordered_map<A, B> forward;
	std::unordered_map<A, B> reverse;

public:

	void relate(const A& a, const B& b)
    {
        unrelate_all(a);
        if (reverse.contains(b))
            forward.erase(reverse[b]);

        forward[a] = b;
        reverse[b] = a;
    }

    void unrelate(const A& a, const B& b)
    {
        if (is_related(a, b))
        {
            forward.erase(a);
            reverse.erase(b);
        }
    }

    void unrelate_all(const A& a)
    {
        if (forward.contains(a))
        {
            auto b = forward[a];
            forward.erase(a);
            reverse.erase(b);
        }
    }

    bool is_related(const A& a, const B& b) const
    {
        return forward.contains(a) && forward.at(a) == b;
    }

    std::optional<B> get_related(const A& a) const
    {
        if (forward.contains(a)) return forward.at(a);
        return std::nullopt;
    }	
};

template <typename A, typename B>
struct RelationshipPolicy<RelationshipType::OneToMany, A, B> 
{

    std::unordered_map<A, std::unordered_set<B>> forward;
    std::unordered_map<B, A> reverse;

public:

    void relate(const A& a, const B& b)
    {
        if (reverse.contains(b))
            forward[reverse[b]].erase(b);

        forward[a].insert(b);
        reverse[b] = a;
    }

    void unrelate(const A& a, const B& b)
    {
        if (is_related(a, b))
        {
            forward[a].erase(b);
            reverse.erase(b);
        }
    }

    void unrelate_all(const A& a)
    {
        if (!forward.contains(a)) return;

        for (auto& b : forward[a])
            reverse.erase(b);

        forward.erase(a);
    }

    bool is_related(const A& a, const B& b) const
    {
        return forward.contains(a) && forward.at(a).contains(b);
    }

    const std::unordered_set<B>& get_all_related(const A& a) const
    {
        static const std::unordered_set<B> empty;
        if (!forward.contains(a)) return empty;
        return forward.at(a);
    }

};

template <typename A, typename B>
struct RelationshipPolicy<RelationshipType::ManyToMany, A, B>
{
    std::unordered_map<A, std::unordered_set<B>> forward;
    std::unordered_map<B, std::unordered_set<A>> reverse;

private:

    void unrelate_one_way(const A& a, const B& b)
    {
        auto fit = forward.find(a);
        if (fit == forward.end() || fit->second.erase(b) == 0) return;

        auto rit = reverse.find(b);
        if (rit != reverse.end())
        {
            rit->second.erase(a);
            if (rit->second.empty())
                reverse.erase(rit);
        }

        if (fit->second.empty())
            forward.erase(fit);
    }

public:

    void relate(const A& a, const B& b)
    {
        forward[a].insert(b);
        reverse[b].insert(a);
        if constexpr (std::same_as<A, B>)
        {
            forward[b].insert(a);
            reverse[a].insert(b);
        }
    }

    void unrelate(const A& a, const B& b)
    {
        unrelate_one_way(a, b);
        if constexpr (std::same_as<A, B>)
        {
            unrelate_one_way(b, a);
        }
    }

    void unrelate_all(const A& a)
    {
        const auto related = get_all_related(a);
        for (const B& b : related)
            unrelate(a, b);
    }

    bool is_related(const A& a, const B& b) const
    {
        auto fit = forward.find(a);
        if (fit != forward.end() && fit->second.contains(b))
            return true;

        if constexpr (std::same_as<A, B>)
        {
            auto rit = reverse.find(a);
            return rit != reverse.end() && rit->second.contains(b);
        }

        return false;
    }

    std::unordered_set<B> get_all_related(const A& a) const
    {
        std::unordered_set<B> related;

        if (auto it = forward.find(a); it != forward.end())
            related.insert(it->second.begin(), it->second.end());

        if constexpr (std::same_as<A, B>)
        {
            if (auto it = reverse.find(a); it != reverse.end())
                related.insert(it->second.begin(), it->second.end());
        }

        return related;
    }
};

template<typename P, typename A>
concept HasGetAll = requires(P p, A a)
{
    { p.get_all_related(a) };
};


template<RelationshipType Type, typename A, typename B>
class RelationManager
{
	using Policy = RelationshipPolicy<Type, A, B>;

public:

	void relate(const A& a, const B& b)
    {
    	policy.relate(a, b);
    }

    void unrelate(const A& a, const B& b)
    {
        policy.unrelate(a, b);
    }

    void unrelate_all(const A& a)
    {
        policy.unrelate_all(a);
    }

    bool is_related(const A& a, const B& b) const
    {
        return policy.is_related(a, b);
    }

	auto get_all_related(const A& a) const requires HasGetAll<Policy, A>
    {
        return policy.get_all_related(a);
    }

protected:

	typename Policy policy;

};