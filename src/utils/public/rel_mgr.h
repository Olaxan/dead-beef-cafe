#pragma once

#include <concepts>

enum class RelationshipType
{
	OneToOne,
	OneToMany
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