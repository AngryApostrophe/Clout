#include <Windows.h>
#include <math.h>
#include <typeinfo>
#include <any>

#include <imgui.h>

#include "../Clout.h"
#include "../Helpers.h"
#include "../Comms.h"
#include "../Console.h"
#include "Probing.h"

//For image loading
#include "stb_image.h"

bool bProbingPageInitialized = false;

//Probe Op types
const std::vector<const char*> szProbeOpTypes = { "Boss Center", "Bore Center", "Pocket Center", "Web Center", "Single Axis" };

ProbeOperation	*Probing_ManualOp = 0;	//For manual operations (from the utilies menu), this is the one we're currently viewing/doing

//Common settings
	int iProbingSpeedFast = 300;
	int iProbingSpeedSlow = 10;
	

//Start a new manual probing operation from the utility menu.  This deletes any previous op and creates a new one
template <typename ProbeClass>	//This will carry the datatype of the new class we're going to create
ProbeClass* Probing_StartManualOp()
{
	if (Probing_ManualOp != 0)
	{
		delete Probing_ManualOp;
		Probing_ManualOp = 0;
	}

	Probing_ManualOp = (ProbeOperation*) new ProbeClass();

	return (ProbeClass*)Probing_ManualOp;
}
void ProbingEndManualOp()
{
	if (Probing_ManualOp != 0)
	{
		delete Probing_ManualOp;
		Probing_ManualOp = 0;
	}
}

//This simply creates a new probing operation, depending on type
ProbeOperation* Probing_InstantiateNewOp(int iOpType)
{
	ProbeOperation *Op = 0;

	if (iOpType == PROBE_OP_TYPE_BOSS)
		Op = (ProbeOperation*) new ProbeOperation_BossCenter();
	else if (iOpType == PROBE_OP_TYPE_BORE)
		Op = (ProbeOperation*) new ProbeOperation_BoreCenter();
	else if (iOpType == PROBE_OP_TYPE_WEB)
		Op = (ProbeOperation*) new ProbeOperation_WebCenter();
	else if (iOpType == PROBE_OP_TYPE_POCKET)
		Op = (ProbeOperation*) new ProbeOperation_PocketCenter();
	else if (iOpType == PROBE_OP_TYPE_SINGLEAXIS)
		Op = (ProbeOperation*) new ProbeOperation_SingleAxis();

	return Op;
}

void ProbeOperation::LoadPreviewImage(GLuint *img, const char *szFilename)
{
	if (img != 0 && *img == 0) //Make sure there's nothing already there
	{
		Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Loading image %s", szFilename);
		if (LoadTextureFromFile(szFilename, img) == 0)
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "^Error loading image!");
		}
	}
}
void ProbeOperation::ZeroWCS(bool x, bool y, bool z, float x_val, float y_val, float z_val)
{
	char szCmd[50];
	char szTemp[10];

	sprintf_s(szCmd, 50, "G10 L20 P%d", iWCSIndex - 1);

	if (x)
	{
		szTemp[0] = 0x0;

		if (x_val < -500.0f) //Random, but it's clearly still the default value
			strcpy_s(szTemp, 10, " X0");
		else
			sprintf_s(szTemp, 10, " X%0.1f", x_val);
		strcat_s(szCmd, 50, szTemp);
	}
	if (y)
	{
		szTemp[0] = 0x0;

		if (y_val < -500.0f) //Random, but it's clearly still the default value
			strcpy_s(szTemp, 10, " Y0");
		else
			sprintf_s(szTemp, 10, " Y%0.1f", y_val);
		strcat_s(szCmd, 50, szTemp);
	}
	if (z)
	{
		szTemp[0] = 0x0;

		if (z_val < -500.0f) //Random, but it's clearly still the default value
			strcpy_s(szTemp, 10, " Z0");
		else
			sprintf_s(szTemp, 10, " Z%0.1f", z_val);
		strcat_s(szCmd, 50, szTemp);
	}

	Comms_SendString(szCmd);
}

int ProbeOperation::ProbingSuccessOrFail(char* s, DOUBLE_XYZ* xyz, bool bAbortOnFail)
{
	int iResult = 0;
	//Make sure we got something
	if (s == 0)
	{
		Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error reading response.  Aborted.");
		iState = PROBE_STATE_IDLE;

		return -1; //Abort
	}

	//Get the result
	char* c = strstr(s, "]");
	if (c == 0)
	{
		free(sProbeReplyMessage);
		sProbeReplyMessage = 0;
		Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Unexpected response from Carvera.  Aborted.");
		iState = PROBE_STATE_IDLE;

		return -1;
	}
	c--; //Move to the digit before

	//Success
	if (*c == '1')
	{
		//Save these values if requested
		if (xyz != 0)
			CommaStringTo3Doubles(sProbeReplyMessage + 5, &xyz->x, &xyz->y, &xyz->z);

		iResult = 1;
	}
	//Failed to find the edge
	else
	{
		//Sometimes we're hoping to NOT find an edge.  This is a safety feature

		if (bAbortOnFail)
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Failed to find the edge.  Aborted.");
			iState = PROBE_STATE_IDLE;
		}
	}

	free(sProbeReplyMessage);
	sProbeReplyMessage = 0;

	return iResult;
}

void ProbeOperation::BeginPopup()
{
	char sString[50]; //General purpose string

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	//Create the modal popup
		sprintf_s(sString, "Probe %s", szProbeOpTypes[bType]);
		ImGui::BeginPopupModal(sString, NULL, ImGuiWindowFlags_AlwaysAutoResize);
}

bool ProbeOperation::EndPopup()
{
	bool bRetVal = TRUE;
	char szString[50];

	ImGui::Dummy(ImVec2(0.0f, 15.0f)); //Extra empty space before the buttons

	//Run button
		sprintf_s(szString, "Run##%s", szWindowIdent);
		if (ImGui::Button(szString, ImVec2(120, 0)))
		{
			//ImGui::CloseCurrentPopup();
			iState = PROBE_STATE_START;
		}

	ImGui::SetItemDefaultFocus();
	ImGui::SameLine();

	//Cancel button
		sprintf_s(szString, "Cancel##%s", szWindowIdent);
		if (ImGui::Button(szString, ImVec2(120, 0)))
		{
			if (iState != PROBE_STATE_IDLE)
			{

				szString[0] = 0x18; //Abort command
				szString[1] = 0x0;
				Comms_SendString(szString);
			}

			ImGui::CloseCurrentPopup();
			bRetVal = false;	//The operation is done

			//TODO: If running, ask if they want to cancel the op before closing
		}


	//Close the window once we're done
	if (iState == PROBE_STATE_COMPLETE)
	{
		iState = PROBE_STATE_IDLE;
		ImGui::CloseCurrentPopup();

		bRetVal = false;	//The operation is done
	}

	ImGui::EndPopup();

	return bRetVal;
}



//Load the required images
void Probing_InitPage()
{	
	bProbingPageInitialized = TRUE;
}





void Probing_Draw() //This is called from inside the main draw code
{
	if (!bProbingPageInitialized)
		Probing_InitPage();

	if (ImGui::BeginTable("table_probing_ops", 2))// , ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH))
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Pocket center"))
		{
			ImGui::OpenPopup("Probe Pocket Center");
			Probing_StartManualOp<ProbeOperation_PocketCenter>();
		}

		ImVec2 sizeLeftButtons = ImGui::GetItemRectSize();

		ImGui::TableSetColumnIndex(1);

		if (ImGui::Button("Single Axis"))
		{
			ImGui::OpenPopup("Probe Single Axis");
			Probing_StartManualOp<ProbeOperation_SingleAxis>();
		}

		ImVec2 sizeRightButtons = ImGui::GetItemRectSize();

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Boss center", sizeLeftButtons))
		{
			ImGui::OpenPopup("Probe Boss Center");
			Probing_StartManualOp<ProbeOperation_BossCenter>();
		}

		ImGui::TableSetColumnIndex(1);

		ImGui::BeginDisabled();
		ImGui::Button("Int Corner", sizeRightButtons);
		ImGui::EndDisabled();


		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Bore Center", sizeLeftButtons))
		{
			ImGui::OpenPopup("Probe Bore Center");
			Probing_StartManualOp<ProbeOperation_BoreCenter>();
		}

		ImGui::TableSetColumnIndex(1);

		ImGui::BeginDisabled();
		ImGui::Button("Ext Corner", sizeRightButtons);
		ImGui::EndDisabled();
		
		
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		if (ImGui::Button("Web center", sizeLeftButtons))
		{
			ImGui::OpenPopup("Probe Web Center");
			Probing_StartManualOp<ProbeOperation_WebCenter>();
		}

		//These have to go inside the Table.  If it's called after EndTable() they won't appear
		if (Probing_ManualOp != 0)	//Do we have an popup operation running?
		{
			if (Probing_ManualOp->DrawPopup() == 0)	//Draw the popup.  If it's completed, delete the op
				ProbingEndManualOp();
		}

		ImGui::EndTable();
	}
}