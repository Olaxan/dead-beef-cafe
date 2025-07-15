#include "world_gui.h"

#include "host.h"

#include "immapp/immapp.h"
#include "imgui_md_wrapper.h"

void WorldGui::DrawHostGui(Host &host)
{
	if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Applications"))
        {
            ImGui::MenuItem("Console");
			ImGui::MenuItem("Calculator");
			ImGui::MenuItem("Logger");
			ImGui::MenuItem("Notepad");
            ImGui::EndMenu();
        }
		if (ImGui::BeginMenu("Files"))
		{
			ImGui::MenuItem("help.txt");
			ImGui::MenuItem("readme.txt");
			ImGui::MenuItem("links.txt");
			ImGui::EndMenu();
		}
        ImGui::EndMainMenuBar();
    }

	bool browser_open = true;
	if (ImGui::Begin("File Browser", &browser_open, ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::MenuItem("New...");
				ImGui::MenuItem("Open...");
				ImGui::MenuItem("Close");
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Finder"))
			{
				ImGui::MenuItem("Find on disk");
				ImGui::MenuItem("Find on web");
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		ImGui::Text(host.get_hostname().c_str());
	}
	ImGui::End();
}