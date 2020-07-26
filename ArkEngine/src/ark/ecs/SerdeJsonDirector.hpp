#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/Color.hpp>

#include "ark/core/Core.hpp"
#include "ark/ecs/Director.hpp"
#include "ark/ecs/Meta.hpp"
#include "ark/ecs/Entity.hpp"

namespace ark
{
	class Entity;

	class SerdeJsonDirector : public Director {
	public:
		SerdeJsonDirector() = default;

		static inline std::string_view serviceSerializeName = "serialize";
		static inline std::string_view serviceDeserializeName = "deserialize";

		void init() override {}

		void serializeEntity(Entity& e);

		void deserializeEntity(Entity& e);
	};
}

namespace sf
{

	template <typename T>
	static void to_json(nlohmann::json& jsonObj, const Vector2<T>& vec)
	{
		jsonObj = nlohmann::json{ {"x", vec.x,}, {"y", vec.y} };
	}

	template <typename T>
	static void from_json(const nlohmann::json& jsonObj, Vector2<T>& vec)
	{
		vec.x = jsonObj.at("x").get<T>();
		vec.y = jsonObj.at("y").get<T>();
	}

	static void to_json(nlohmann::json& jsonObj, const Time& time)
	{
		jsonObj = nlohmann::json(time.asSeconds());
	}

	static void from_json(const nlohmann::json& jsonObj, Time& time)
	{
		time = sf::seconds(jsonObj.get<float>());
	}

	static void to_json(nlohmann::json& jsonObj, const Color& color)
	{
		jsonObj = nlohmann::json{ {"r", color.r}, {"g", color.g}, {"b", color.b}, {"a", color.a} };
	}

	static void from_json(const nlohmann::json& jsonObj, Color& color)
	{
		color.r = jsonObj.at("r").get<Uint8>();
		color.g = jsonObj.at("g").get<Uint8>();
		color.b = jsonObj.at("b").get<Uint8>();
		color.a = jsonObj.at("a").get<Uint8>();
	}
}

namespace ark
{

	using nlohmann::json;

	// TODO (json): serialize sf::Vector2f with json::adl_serializer or meta::register?
	template <typename Type>
	static json serialize_value(const void* pValue)
	{
		const Type& value = *static_cast<const Type*>(pValue);
		json jsonObj;
		meta::doForAllProperties<Type>([&value, &jsonObj](auto& property) {
			using PropType = meta::get_member_type<decltype(property)>;

			if constexpr (meta::isRegistered<PropType>()) {
				auto local = property.getCopy(value);
				jsonObj[property.getName()] = serialize_value<PropType>(&local);
			}
			else if constexpr (std::is_enum_v<PropType> /*&& is registered */) {
				auto propValue = property.getCopy(value);
				const auto& fields = meta::getEnumValues<PropType>();
				const char* fieldName = meta::getNameOfEnumValue<PropType>(propValue);
				jsonObj[property.getName()] = fieldName;
			}
			else if constexpr (std::is_same_v<char, PropType>) {
				std::string str;
				str.push_back(property.getCopy(value));
				jsonObj[property.getName()] = str;
			}
			else /* normal(convertible) value */ {
				jsonObj[property.getName()] = property.getCopy(value);
			}
		});
		return jsonObj;
	}

	template <typename Type>
	static void deserialize_value(Scene& s, Entity& e, const json& jsonObj, void* pValue)
	{
		Type& value = *static_cast<Type*>(pValue);
		meta::doForAllProperties<Type>([&value, &jsonObj, &e, &s](auto& property) {
			using PropType = meta::get_member_type<decltype(property)>;
			bool failure = true;

			if constexpr (meta::isRegistered<PropType>()) {
				PropType propValue;
				if (auto it = jsonObj.find(property.getName()); it != jsonObj.end()) {
					deserialize_value<PropType>(s, e, jsonObj.at(property.getName()), &propValue);
					property.set(value, propValue);
					failure = false;
				}
			}
			else if constexpr (std::is_enum_v<PropType>) {
				if (auto it = jsonObj.find(property.getName()); it != jsonObj.end()) {
					auto enumValueName = it->get<std::string>();
					auto memberValue = meta::getValueOfEnumName<PropType>(enumValueName.c_str());
					property.set(value, memberValue);
					failure = false;
				}
			}
			else if constexpr (std::is_same_v<char, PropType>) {
				if (auto it = jsonObj.find(property.getName()); it != jsonObj.end()) {
					auto&& memberValue = it->get<std::string>();
					property.set(value, memberValue[0]);
					failure = false;
				}
			}
			else /* normal(convertible) value*/ {
				if (auto it = jsonObj.find(property.getName()); it != jsonObj.end()) {
					auto&& memberValue = it->get<PropType>();
					property.set(value, memberValue);
					failure = false;
				}
			}
			if (failure) {
				auto mdata = ark::meta::getMetadata(typeid(Type));
				if (mdata)
					EngineLog(LogSource::Scene, LogLevel::Error,
						"failed to deser property (%s) on component (%s) on entity (%d)",
						property.getName(), mdata->name, e.getID());
				else
					EngineLog(LogSource::Scene, LogLevel::Error,
						"failed to deser property (%s) on entity (%s)",
						property.getName(), e.getID());
			}

		});
	}
}