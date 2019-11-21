
#include "System.hpp"
#include "Entity.hpp"
#include "EntityManager.hpp"
#include "Transform.hpp"
#include "RandomNumbers.hpp"

#include <imgui.h>
#include <libs/tinyformat.hpp>

#if 0
template <typename T, typename F, typename F2>
void treeWithSeparators(std::string_view label, T& range, F getLabel, F2 renderElem)
{
	static std::vector<bool> viewSeparators;

	if (ImGui::TreeNode(label.data())) {
		if (viewSeparators.size() != range.size())
			viewSeparators.resize(range.size());

		int index = 0;
		for (auto& element : range) {

			if (index == 0 && viewSeparators.at(index)) {
				ImGui::Separator();
			} else if (viewSeparators.at(index) && viewSeparators.at(index - 1) == false) {
				ImGui::Separator();
			}

			auto label = getLabel(element);
			if (ImGui::TreeNode(label.c_str())) {
				viewSeparators.at(index) = true;
				float indent_w = 10;
				ImGui::Indent(indent_w);
				
				renderElem(element);

				if(viewSeparators.at(index))
					ImGui::Separator();

				ImGui::Unindent(indent_w);
				ImGui::TreePop();
			} else {
				viewSeparators.at(index) = false;
			}
			index += 1;
		}
		ImGui::TreePop();
	}
}

void SystemManager::renderInspector()
{
	auto getLabel = [](auto& system) {
		return tfm::format("%s: E(%d) C(%d)", system->name.data(), system->entities.size(), system->componentNames.size());
	};

	auto renderElem = [](auto& system) {
		ImGui::TextUnformatted("Components:");
		for (std::string_view name : system->componentNames) {
			ImGui::BulletText(name.data());
		}

		ImGui::TextUnformatted("Entities:");
		for (Entity e : system->entities) {
			ImGui::BulletText(e.getName().c_str());
		}
	};

	treeWithSeparators("Systems:", systems, getLabel, renderElem);
}
#endif

void SystemManager::renderInspector()
{
	if (ImGui::TreeNode("Systems:")) {
		for (auto& system : systems) {
			std::string text = tfm::format("%s: E(%d) C(%d)", system->name.data(), system->entities.size(), system->componentNames.size());
			if (ImGui::TreeNodeEx(text.c_str(), ImGuiTreeNodeFlags_CollapsingHeader)) {
				float indent_w = 10;
				ImGui::Indent(indent_w);

				ImGui::TextUnformatted("Components:");
				for (std::string_view name : system->componentNames) {
					ImGui::BulletText(name.data());
				}

				ImGui::TextUnformatted("Entities:");
				for (Entity e : system->entities) {
					ImGui::BulletText(e.getName().c_str());
				}
				ImGui::Unindent(indent_w);
				//ImGui::TreePop() // don't pop a Collapsing Header
			}
		}
		ImGui::TreePop();
	}
}

void EntityManager::renderInspector()
{
	if (ImGui::TreeNode("Entities:")) {
		int i = 0;
		for (auto& entity : entities) {

			if (auto it = std::find(freeEntities.begin(), freeEntities.end(), i); it != freeEntities.end()) {
				//ImGui::BulletText("free_entity_%d", i);
				i += 1;
				continue;
			}
			ImGui::Separator();

			const std::vector<std::unique_ptr<Script>>* scripts = nullptr;
			if(entity.scriptsIndex != ArkInvalidIndex)
				scripts = &(scriptManager.getScripts(entity.scriptsIndex));
			int scriptNum = scripts ? scripts->size() : 0;

			std::string text = tfm::format("%s: C(%d) S(%d)", entity.name.c_str(), entity.components.size(), scriptNum);
			if (ImGui::TreeNode(text.c_str())) {
				float indent_w = 7;
				ImGui::Indent(indent_w);

				ImGui::TextUnformatted("Components:");
				for (auto& compData : entity.components) {
					auto type = componentManager.getTypeFromId(compData.id);
					ImGui::BulletText(Util::getNameOfType(type));
				}

				if (scripts) {
					ImGui::TextUnformatted("Scripts:");
					for (auto& script : *scripts) {
						ImGui::BulletText(script->name.data());
					}
				}

				ImGui::Unindent(indent_w);
				ImGui::TreePop();
			}
			i += 1;
		}
		ImGui::TreePop();
	}
	renderEditor();
}

void EntityManager::renderEditor()
{
	//int windowHeight = 440;
	int windowHeight = 700;
	int windowWidth = 600;
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
	static bool _open = true;

	if (ImGui::Begin("Entity editor", &_open)) {

		// entity list
		static int selectedEntity = -1;
		int entityId = 0;
        ImGui::BeginChild("left_pane", ImVec2(150, 0), true);
		for (auto& entity : entities) {

			if (auto it = std::find(freeEntities.begin(), freeEntities.end(), entityId); it != freeEntities.end()) {
				if (selectedEntity == entityId)
					selectedEntity = -1;
				entityId += 1;
				continue;
			}
			if (ImGui::Selectable(entity.name.c_str(), selectedEntity == entityId)) {
				selectedEntity = entityId;
			}
			entityId += 1;
		}
		ImGui::EndChild();
		ImGui::SameLine();

		// selected entity editor
		ImGui::BeginChild("right_pane", ImVec2(0, 0), false);
		if (selectedEntity != -1) {

			// entity name at the top
			auto& entity = entities.at(selectedEntity);
			ImGui::Text("Entity: %s", entity.name.c_str());
			ImGui::Separator();

			// component editor
			ImGui::TextUnformatted("Components:");
			int componentWidgetId = 0;
			for (auto& compData : entity.components) {
				auto compType = componentManager.getTypeFromId(compData.id);
				ImGui::BulletText(Util::getNameOfType(compType));
				// remove component button
				auto typeNameSize = ImGui::GetItemRectSize();
				auto buttonTextSize = ImGui::CalcTextSize("remove component");
				auto width = ImGui::GetContentRegionAvailWidth();
				ImGui::SameLine(0, width - typeNameSize.x - buttonTextSize.x - 10);
				ImGui::PushID(&compData);
				if (ImGui::Button("remove component")) {
					Entity e;
					e.id = selectedEntity;
					e.manager = this;
					e.removeComponent(compType);
				}
				ImGui::PopID();

				componentManager.renderEditorOfComponent(&componentWidgetId, compData.id, compData.component);
				ImGui::Separator();
			}

			// add components list
			auto componentGetter = [](void* data, int index, const char** out_text) -> bool {
				auto& types = *static_cast<std::vector<std::type_index>*> (data);
				*out_text = Util::getNameOfType(types.at(index));
				return true; 
			};
			int componentItemIndex;
			auto& types = componentManager.getTypes();
			if (ImGui::Combo("add_component", &componentItemIndex, componentGetter, static_cast<void*>(&types), types.size())) {
				Entity e;
				e.id = selectedEntity;
				e.manager = this;
				e.addComponent(types.at(componentItemIndex));
			}

			// script editor
			ImGui::NewLine();
			ImGui::NewLine();
			if (entity.scriptsIndex != ArkInvalidIndex) {
				ImGui::TextUnformatted("Scripts:");
				auto& scripts = scriptManager.getScripts(entity.scriptsIndex);
				for (auto& script : scripts) {
					ImGui::BulletText(script->name.data());
					// remove script button
					ImGui::PushID(&script->type);
					if (ImGui::BeginPopupContextItem("remove_this_script_menu")) {
						std::string label = tfm::format("remove %s", Util::getNameOfType(script->type));
						if (ImGui::Button(label.c_str())) {
							Entity e;
							e.id = selectedEntity;
							e.manager = this;
							e.removeScript(script->type);
						}
						ImGui::EndPopup();
					}
					ImGui::PopID();
					// render_script_editor[script->type](script.get());
					ImGui::Separator();
				}
			}

			// add scripts list
			auto scriptGetter = [](void* data, int index, const char** out_text) mutable -> bool {
				auto& types = *static_cast<decltype(ScriptManager::factories)*>(data);
				auto it = types.begin();
				std::advance(it, index);
				*out_text = Util::getNameOfType(it->first);
				return true;
			};
			int scriptItemIndex;
			if (ImGui::Combo("add_script", &scriptItemIndex, scriptGetter, static_cast<void*>(&ScriptManager::factories), ScriptManager::factories.size())) {
				Entity e;
				e.id = selectedEntity;
				e.manager = this;
				auto it = ScriptManager::factories.begin();
				std::advance(it, scriptItemIndex);
				e.addScript(it->first);
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

void ArkAlign(std::string_view str, float widthPercentage)
{
	ImVec2 textSize = ImGui::CalcTextSize(str.data());
	float width = ImGui::GetContentRegionAvailWidth();
	ImGui::SameLine(0, width * widthPercentage - textSize.x);
	ImGui::SetNextItemWidth(-2);
}

void ArkSetFieldName(std::string_view name)
{
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(name.data());
	ArkAlign(name, 0.5);
}

bool ArkSelectable(const char* label, bool selected)
{
	return ImGui::Selectable(label, selected);
}

void ArkFocusHere(int i=-1)
{
	ImGui::SetKeyboardFocusHere(i);
}

std::unordered_map<std::type_index, ComponentManager::FieldFunc> ComponentManager::fieldRendererTable = {

	{ typeid(int), [](std::string_view name, const void* pField) {
		int field = *static_cast<const int*>(pField);
		ArkSetFieldName(name);
		if (ImGui::InputInt("", &field, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue)) {
			ArkFocusHere();
			return std::any{field};
		}
		return std::any{};
	} },

	{ typeid(float), [](std::string_view name, const void* pField) {
		float field = *static_cast<const float*>(pField);
		ArkSetFieldName(name);
		if (ImGui::InputFloat("", &field, 0, 0, 3, ImGuiInputTextFlags_EnterReturnsTrue))
			ArkFocusHere();
			return std::any{field};
		return std::any{};
	} },

	{ typeid(bool), [](std::string_view name, const void* pField) {
		bool field = *static_cast<const bool*>(pField);
		ArkSetFieldName(name);
		if (ImGui::Checkbox("", &field)) {
			ArkFocusHere();
			return std::any{field};
		}
		return std::any{};
	} }, 

	{ typeid(std::string), [](std::string_view name, const void* pField) {
		std::string field = *static_cast<const std::string*>(pField);
		char buff[50];
		field.copy(buff, field.size());
		buff[field.size()] = '\0';
		ArkSetFieldName(name);
		if (ImGui::InputText("", buff, field.size() + 1, ImGuiInputTextFlags_EnterReturnsTrue))
			ArkFocusHere();
			return std::any{std::string(buff, field.size())};
		return std::any{};
	} },

	{ typeid(sf::Vector2f), [](std::string_view name, const void* pField) {
		sf::Vector2f vec = *static_cast<const sf::Vector2f*>(pField);
		ArkSetFieldName(name);
		float v[2] = {vec.x, vec.y};
		if (ImGui::InputFloat2("", v, 2, ImGuiInputTextFlags_EnterReturnsTrue)) {
			ArkFocusHere();
			vec.x = v[0];
			vec.y = v[1];
			return std::any{vec};
		}
		return std::any{};
	} },

	{ typeid(sf::Color), [](std::string_view name, const void* pField) {
		sf::Color color = *static_cast<const sf::Color*>(pField);
		ArkSetFieldName(name);
		float v[4] = {
			static_cast<float>(color.r) / 255.f,
			static_cast<float>(color.g) / 255.f,
			static_cast<float>(color.b) / 255.f,
			static_cast<float>(color.a) / 255.f
		};

		if (ImGui::ColorEdit4("", v)) {
			color.r = static_cast<uint8_t>(v[0] * 255.f);
			color.g = static_cast<uint8_t>(v[1] * 255.f);
			color.b = static_cast<uint8_t>(v[2] * 255.f);
			color.a = static_cast<uint8_t>(v[3] * 255.f);
			return std::any{color};
		}
		return std::any{};
	} },

	{ typeid(sf::Time), [](std::string_view name, const void* pField) {
		sf::Time time = *static_cast<const sf::Time*>(pField);
		std::string label = std::string(name) + " (as seconds)";
		ArkSetFieldName(label);
		float sec = time.asSeconds();
		if (ImGui::InputFloat("", &sec, 0, 0, 3, ImGuiInputTextFlags_EnterReturnsTrue))
			ArkFocusHere();
			return std::any{sf::seconds(sec)};

		return std::any{};
	} }
};

