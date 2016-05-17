//
// 2016-05-13, jjuiddong
// 
//	- first version
//		- 로그 출력, error 빨강, Topmost, Clear
//
// - ver 1.01
//		- display row line number
//
// - ver 1.02
//		- display hexadecimal
//
//

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/listctrl.h"

#include <string>
#include <fstream>
#include <mmsystem.h>
#include <sstream>
#include <iomanip> // setw()
using std::string;

string g_version = "ver 1.01";
__int64 g_oldFileSize = -1;
std::streampos g_oldPos = 0;


#pragma comment (lib, "winmm.lib")

class MyFrame;
class MyApp : public wxApp
{
public:
	virtual bool OnInit() wxOVERRIDE;
	virtual void OnIdle(wxIdleEvent& event);
	MyFrame *m_frame;
};

class MyFrame : public wxFrame
{
public:
	MyFrame(const wxString& title);
	void MainLoop();
	void ToggleTopMost();
	void OnQuit(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnDropFiles(wxDropFilesEvent& event);
	void OnContextMenu(wxContextMenuEvent& event);
	void OnMenuToggleTopMost(wxCommandEvent& event);
	void OnMenuToggleRowNum(wxCommandEvent& event);
	void OnMenuToggleHexadecimal(wxCommandEvent& event);
	void OnMenuToggleAutoscroll(wxCommandEvent& event);
	void OnMenuClear(wxCommandEvent& event);


private:
	wxListCtrl *m_listCtrl;
	std::string m_fileName;
	int m_maxLineCount = 500; // 화면에 출력할 최대 라인 수 (실행인자 값으로 설정 가능, 두 번째 인자)
	bool m_isReload;
	bool m_isTopMost;
	bool m_isRowNum;
	bool m_isHexadecimal;
	bool m_isAutoScroll;
	int m_rowNumber;
	wxDECLARE_EVENT_TABLE();
};
enum
{
	Minimal_Quit = wxID_EXIT,
	Minimal_About = wxID_ABOUT,
	MENU_TOGGLE_TOPMOST=10000,
	MENU_TOGGLE_ROWNUM,
	MENU_TOGGLE_HEXA,
	MENU_TOGGLE_AUTOSCROL,
	MENU_CLEAR,
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(Minimal_Quit, MyFrame::OnQuit)
EVT_SIZE(MyFrame::OnSize)
EVT_CONTEXT_MENU(MyFrame::OnContextMenu)
EVT_MENU(MENU_TOGGLE_TOPMOST, MyFrame::OnMenuToggleTopMost)
EVT_MENU(MENU_TOGGLE_ROWNUM, MyFrame::OnMenuToggleRowNum)
EVT_MENU(MENU_TOGGLE_HEXA, MyFrame::OnMenuToggleHexadecimal)
EVT_MENU(MENU_TOGGLE_AUTOSCROL, MyFrame::OnMenuToggleAutoscroll)
EVT_MENU(MENU_CLEAR, MyFrame::OnMenuClear)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
// 	if (__argc <= 1)
// 	{
// 		wxMessageBox(wxString::Format("command line input the filename \n"),
// 			"Log Printer",
// 			wxOK | wxICON_INFORMATION,
// 			NULL);
// 		return false;
// 	}
	
	m_frame = NULL;
	Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(MyApp::OnIdle));

	m_frame = new MyFrame("Log Printer");
	m_frame->Show(true);
	return true;
}

MyFrame::MyFrame(const wxString& title)
	: wxFrame(NULL, wxID_ANY, title + " - " + g_version, wxDefaultPosition, wxSize(500, 500))
	, m_isReload(true)
	, m_isTopMost(false)
	, m_isRowNum(true)
	, m_isHexadecimal(false)
	, m_isAutoScroll(true)
	, m_rowNumber(1)
{
	SetIcon(wxICON(sample));

	wxFont listfont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	m_listCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER);
	m_listCtrl->SetBackgroundColour(wxColour(0, 0, 0));
	m_listCtrl->SetTextColour(wxColour(255, 255, 255));

	m_listCtrl->InsertColumn(0, "data", 0, 480);
	sizer->Add(m_listCtrl, wxSizerFlags().Center());

	if (__argc > 1)
	{
		m_fileName = __argv[1];
	}

	if (__argc > 2)
	{
		m_maxLineCount = atoi(__argv[2]);
	}


	if (!m_fileName.empty())
		SetTitle(m_fileName + " - " + g_version);

	DragAcceptFiles(true);
	Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MyFrame::OnDropFiles), NULL, this);
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(true);
}


void MyFrame::ToggleTopMost()
{
	if (!m_isTopMost)
		SetWindowStyle(wxDEFAULT_FRAME_STYLE | wxSTAY_ON_TOP);
	else
		SetWindowStyle(wxDEFAULT_FRAME_STYLE);
	m_isTopMost = !m_isTopMost;
}

void MyFrame::OnContextMenu(wxContextMenuEvent& event)
{
	wxPoint point = event.GetPosition();
	point = ScreenToClient(point);

	wxMenu menu;
	menu.AppendCheckItem(MENU_TOGGLE_TOPMOST, wxT("&Toggle TopMost"));
	menu.AppendCheckItem(MENU_TOGGLE_ROWNUM, wxT("&Toggle Row Number"));
	menu.AppendCheckItem(MENU_TOGGLE_HEXA, wxT("&Toggle Hexadecimal"));
	menu.AppendCheckItem(MENU_TOGGLE_AUTOSCROL, wxT("&Toggle AutoScroll"));
	menu.AppendCheckItem(MENU_CLEAR, wxT("&Clear"));
	menu.Check(MENU_TOGGLE_TOPMOST, m_isTopMost);
	menu.Check(MENU_TOGGLE_ROWNUM, m_isRowNum);
	menu.Check(MENU_TOGGLE_HEXA, m_isHexadecimal);
	menu.Check(MENU_TOGGLE_AUTOSCROL, m_isAutoScroll);
	PopupMenu(&menu, point);
}

void MyFrame::OnMenuToggleTopMost(wxCommandEvent& event)
{
	ToggleTopMost();	
}

void MyFrame::OnMenuToggleRowNum(wxCommandEvent& event)
{
	m_isRowNum = !m_isRowNum;
}

void MyFrame::OnMenuToggleHexadecimal(wxCommandEvent& event)
{
	m_isHexadecimal = !m_isHexadecimal;
}

void MyFrame::OnMenuToggleAutoscroll(wxCommandEvent& event)
{
	m_isAutoScroll = !m_isAutoScroll;
}

void MyFrame::OnMenuClear(wxCommandEvent& event)
{
	if (m_listCtrl)
		m_listCtrl->DeleteAllItems();
	m_rowNumber = 1;
}

void MyFrame::OnSize(wxSizeEvent& event)
{
	event.Skip();
	m_listCtrl->SetColumnWidth(0, event.GetSize().GetWidth()-20);
}

void MyFrame::OnDropFiles(wxDropFilesEvent& event)
{
	if (event.GetNumberOfFiles() > 0) {

		wxString* dropped = event.GetFiles();
		wxASSERT(dropped);
		m_fileName = *dropped;
		m_isReload = true;

		if (!m_fileName.empty())
			SetTitle(m_fileName + " - " + g_version);
	}
}

__int64 FileSize(std::string name)
{
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (!GetFileAttributesExA(name.c_str(), GetFileExInfoStandard, &fad))
		return -1; // error condition, could call GetLastError to find out more
	LARGE_INTEGER size;
	size.HighPart = fad.nFileSizeHigh;
	size.LowPart = fad.nFileSizeLow;
	return size.QuadPart;
}


// 파일 사이즈가 바뀌면, 파일 정보를 출력한다.
void MyFrame::MainLoop()
{
	using namespace std;

	if (m_fileName.empty())
		return;

	static int oldT = timeGetTime();
	const int curT = timeGetTime();
	if (curT - oldT < 100)
		return;
	oldT = curT;

	const __int64 curSize = FileSize(m_fileName);
	if (curSize <= 0)
		return;

	if (m_isReload || (curSize != g_oldFileSize))
	{
		ifstream ifs(m_fileName);
 		if (!ifs.is_open())
 			return;

		if (m_isReload || (curSize < g_oldFileSize))
		{
			g_oldPos = ifs.tellg();
			m_isReload = false;
		}
		else
		{
			ifs.seekg(g_oldPos, std::ios::beg);
		}

		while (1)
		{
			g_oldPos = ifs.tellg();

 			string line;
			if (m_isHexadecimal)
			{
				// todo : display hexadecimal
			}
			else
			{
	 			getline(ifs, line);
			}

			if (ifs.eof())
				break;

			if (!line.empty() && (line != "\r"))
			{
				stringstream ss;

				if (m_isRowNum)
				{
					ss << std::setw(5) << m_rowNumber << "    ";
					++m_rowNumber;
				}

				ss << line;

				const string str = ss.str();
				const int idx = m_listCtrl->InsertItem(m_listCtrl->GetItemCount(), str);
				
				if (m_isAutoScroll)
					m_listCtrl->EnsureVisible(idx);
				
				const int pos1 = str.find("error");
				const int pos2 = str.find("Error");
				if ((pos1 != string::npos) || (pos2 != string::npos))
				{
					m_listCtrl->SetItemTextColour(idx, wxColour(255, 0, 0));
				}

				if (m_listCtrl->GetItemCount() > m_maxLineCount)
					m_listCtrl->DeleteItem(0);
			}
		}

		g_oldFileSize = curSize;
	}
}

void MyApp::OnIdle(wxIdleEvent& event)
{	
	if (m_frame)
		m_frame->MainLoop();
	Sleep(10);
	event.RequestMore();
}
