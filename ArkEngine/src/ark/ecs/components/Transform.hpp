#pragma once

#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/RenderStates.hpp>

#include "ark/core/Core.hpp"
#include "ark/ecs/Meta.hpp"
#include "ark/ecs/Component.hpp"

namespace ark {

	class Entity;

	struct ARK_ENGINE_API Transform final : public Component<Transform>, public sf::Transformable{

		using sf::Transformable::Transformable;
		operator const sf::Transform& () const { return this->getTransform(); }
		operator const sf::RenderStates& () const { return this->getTransform(); }

		Transform() = default;

		// don't actually use it, copy_ctor is defined so that the compiler dosen't 
		// scream when we instantiate deque<Transform> in the implementation
		Transform(const Transform& tx) {}
		Transform& operator=(const Transform& tx) {}

		Transform(Transform&& tx) noexcept
		{
			this->moveToThis(std::move(tx));
		}

		Transform& operator=(Transform&& tx) noexcept
		{
			if (&tx == this)
				return *this;
			this->removeFromParent();
			this->orphanChildren();
			this->moveToThis(std::move(tx));
			return *this;
		}

		~Transform()
		{
			orphanChildren();
			removeFromParent();
		}

		void addChild(Transform& child)
		{
			if (&child == this)
				return;
			if (child.m_parent == this)
				return;
			child.removeFromParent();
			child.m_parent = this;
			m_children.push_back(&child);
		}

		void removeChild(Transform& child)
		{
			if (&child == this)
				return;
			child.m_parent = nullptr;
			Util::erase(m_children, &child);
		}

		const std::vector<Transform*>& getChildren() { return m_children; }

		sf::Transform getWorldTransform()
		{
			if (m_parent)
				return this->getTransform() * m_parent->getWorldTransform();
			else
				return this->getTransform();
		}

		//int depth() { return m_depth; }


	private:

		void removeFromParent()
		{
			if (m_parent)
				m_parent->removeChild(*this);
		}

		void orphanChildren()
		{
			for (auto child : m_children)
				child->m_parent = nullptr;
		}

		void moveToThis(Transform&& tx)
		{
			this->m_parent = tx.m_parent;
			tx.removeFromParent();
			this->m_parent->addChild(*this);

			this->m_children = std::move(tx.m_children);
			for (auto child : m_children)
				child->m_parent = this;

			this->setOrigin(tx.getOrigin());
			this->setPosition(tx.getPosition());
			this->setRotation(tx.getRotation());
			this->setScale(tx.getScale());

			tx.setPosition({0.f, 0.f});
			tx.setRotation(0.f);
			tx.setScale({1.f, 1.f});
			tx.setOrigin({0.f, 0.f});
		}

		Transform* m_parent = nullptr;
		std::vector<Transform*> m_children{};
		//int m_depth;
	};
}

namespace ark::meta {
	template <> inline auto registerMembers<ark::Transform>()
	{
		using ark::Transform;
		return members(
			member("position", &Transform::getPosition, &Transform::setPosition),
			member("scale", &Transform::getScale, &Transform::setScale),
			member("roatation", &Transform::getRotation, &Transform::setRotation)
		);
	}
}