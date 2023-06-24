// 2016-05-13, jjuiddong
// 
//	- first version
//		- print log, error Red color, TopMost, Clear
//	- ver 1.01
//		- display row line number
//	- ver 1.02
//		- display hexadecimal
//	- ver 1.03
//		- network send, recv
//	- ver 1.04
//		- optimize & refactoring
//		- maximum line = 500 -> 100
//		- stop menu
//		- logprinter_config.txt
//			- token = color
//				- logprinter_config.txt Sample
//					error = 1, 0, 0
//					Error = 1, 0, 0
//					Recv = 0, 1, 0
//	- ver 1.06, 2021-07-27
//		- ver 1.05 skip
//		- string missing bug fix
//	- ver 1.07, 2023-04-28
//		- multithreading, read file
//
#include "../../../Common/Common/common.h"
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/listctrl.h"
#include "../../../Common/Network/network.h"
#include <iomanip> // setw()

string g_version = "ver 1.07";

#pragma comment (lib, "winmm.lib")

class MyFrame;
class MyApp : public wxApp
{
public:
	virtual bool OnInit() wxOVERRIDE;
	virtual void OnIdle(wxIdleEvent& event);
	MyFrame *m_frame;
};


//---------------------------------------------------------------------------------
// file read task
class cFileReadTask : public common::cTask
{
public:
	cFileReadTask(MyFrame* myFrm)
		: cTask(0, "cFileReadTask")
		, m_frm(myFrm)
		, m_buffer(nullptr)
		, m_isReload(false)
		, m_curPos(0)
		, m_oldFileSize(-1)
		, m_oldPos(0)
	{
		m_buffer = new char[m_bufferSize];
	}
	virtual ~cFileReadTask()
	{
		SAFE_DELETEA(m_buffer);
	}

	virtual eRunResult Run(const double deltaSeconds) override;
	virtual void MessageProc(common::threadmsg::MSG msg
		, WPARAM wParam, LPARAM lParam, LPARAM added)  override;
	void ReadFile();
	void SplitString(OUT vector<string>& out);


public:
	MyFrame* m_frm;
	string m_fileName;
	bool m_isReload;
	__int64 m_oldFileSize; // read file size
	std::streampos m_oldPos; // read file pointer
	char* m_buffer;
	int m_bufferSize = 2048;
	int m_maxScrollSize = 10; // size = m_bufferSize * m_maxScrollSize
	int m_curPos; // current buffer pos(index)
};


class MyFrame : public wxFrame
{
public:
	MyFrame(const wxString& title);
	virtual ~MyFrame();
	void MainLoop();
	void OnQuit(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnDropFiles(wxDropFilesEvent& event);
	void OnContextMenu(wxContextMenuEvent& event);
	void OnMenuToggleTopMost(wxCommandEvent& event);
	void OnMenuToggleRowNum(wxCommandEvent& event);
	void OnMenuToggleHexadecimal(wxCommandEvent& event);
	void OnMenuToggleAutoscroll(wxCommandEvent& event);
	void OnMenuFastscroll(wxCommandEvent& event);
	void OnMenuToggleStop(wxCommandEvent& event);
	void OnMenuClear(wxCommandEvent& event);


public:
	void InputFromServer();
	void ToggleTopMost();
	void AddString(const common::Str512& str);
	void OutputString(const string &str, const int strSize);
	void InsertString(const common::Str512 &str);


public:
	enum class eState {
		Run, // running
		Terminate, // terminate process
	};

	enum class eInputType {
		NONE, // nothing~
		FILE, // file stream
		NETWORK // network stream 
	};

	eState m_state;
	wxListCtrl *m_listCtrl;
	string m_fileName;
	int m_maxLineCount = 100; // display maximum line count (commandline: logprinter.exe <> <line count>)
	bool m_isReload;
	bool m_isTopMost;
	bool m_isRowNum;
	bool m_isHexadecimal;
	bool m_isAutoScroll;
	bool m_isStop;
	int m_rowNumber;
	eInputType m_inputType;
	eInputType m_outputType;
	string m_ip;
	int m_port;
	int m_bindPort;
	int m_networkState; //-1:use not network, 0:before connect or bind, 1:after connect or bind
	int m_outputNetworkState; //-1:use not network, 0:before connect or bind, 1:after connect or bind
	double m_networkInitDelay; // initialize network after run program. seconds unit
	double m_outputNetworkInitDelay; // initialize network after run program. seconds unit
	network::cTCPServer m_svr;
	network::cTCPClient2 m_client;
	common::cTimer m_timer;
	struct sTextColor {
		string text;
		wxColour color;
	};
	vector<sTextColor> m_colors;
	common::cWQSemaphore m_wqSema;
	cFileReadTask* m_fileTask;
	common::CriticalSection m_cs;
	queue<string> m_strs; // string table
	int m_updateCnt = 3;

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
	MENU_TOGGLE_FASTSCROL,
	MENU_TOGGLE_STOP,
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
EVT_MENU(MENU_TOGGLE_FASTSCROL, MyFrame::OnMenuFastscroll)
EVT_MENU(MENU_TOGGLE_STOP, MyFrame::OnMenuToggleStop)
EVT_MENU(MENU_CLEAR, MyFrame::OnMenuClear)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
	m_frame = NULL;
	Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(MyApp::OnIdle));

	m_frame = new MyFrame("Log Printer");
	m_frame->Show(true);
	return true;
}

MyFrame::MyFrame(const wxString& title)
	: wxFrame(NULL, wxID_ANY, title + " - " + g_version, wxDefaultPosition, wxSize(500, 500))
	, m_state(eState::Run)
	, m_isReload(true)
	, m_isTopMost(false)
	, m_isRowNum(true)
	, m_isHexadecimal(false)
	, m_isAutoScroll(true)
	, m_isStop(false)
	, m_rowNumber(1)
	, m_inputType(eInputType::NONE)
	, m_outputType(eInputType::NONE)
	, m_networkState(-1)
	, m_outputNetworkState(-1)
	, m_networkInitDelay(0.3f)
	, m_outputNetworkInitDelay(0.4f)
	, m_svr(new network::cProtocol("LOGS"))
	, m_fileTask(nullptr)
{
	SetIcon(wxICON(sample));

	m_timer.Create();

	wxFont listfont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	m_listCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER);
	m_listCtrl->SetBackgroundColour(wxColour(0, 0, 0));
	m_listCtrl->SetTextColour(wxColour(255, 255, 255));

	m_listCtrl->InsertColumn(0, "data", 0, 480);
	sizer->Add(m_listCtrl, wxSizerFlags().Center());

	// command line = "file=aaa line=100 ip=192.168.0.1 port=100 binport=1000 oplog"
	// show command line help
	m_listCtrl->InsertItem(0, "commandline -> file=filename.txt line=100 ip=192.168.0.1 port=100 binport=1000 oplog");
	m_listCtrl->InsertItem(1, "logprinter_config.txt, string = color");

	vector<string> args;
	for (int i = 0; i < __argc; ++i)
		args.push_back(__argv[i]);

	int argIdx = 1;
	while(__argc > argIdx)
	{
		const string cmd[] = {"bindport=", "file=", "line=", "ip=", "port=", "oplog"};
		const int cmdSize = sizeof(cmd) / sizeof(string);
		for (int i = 0; i < cmdSize; ++i)
		{
			const int pos = string(args[argIdx]).find(cmd[i]);
			if (pos != string::npos)
			{
				const string param = string(args[argIdx]).substr(pos+ cmd[i].length());
				switch (i)
				{
				case 0: // bindport=
					m_outputType = eInputType::NETWORK;
					m_bindPort = atoi(param.c_str());
					m_outputNetworkState = 0;
					break;

				case 1: // file=
					m_inputType = eInputType::FILE;
					m_fileName = param;
					break;

				case 2: // line=
					m_maxLineCount = atoi(param.c_str());
					break;

				case 3: // ip=
					m_ip = param;
					m_inputType = eInputType::NETWORK;
					m_networkState = 0;
					m_client.m_state = network::cTCPClient2::READYCONNECT;
					break;

				case 4: // port=
					m_port = atoi(param.c_str());
					m_inputType = eInputType::NETWORK;
					m_networkState = 0;
					break;

				case 5: // oplog
				{
					// yyymmdd_AMROperation.txt
					m_inputType = eInputType::FILE;
					m_fileName = common::GetCurrentDateTime4() + "_AMROperation.log";
					stringstream ss;
					ss << "oplog = " << m_fileName;
					m_listCtrl->InsertItem(m_listCtrl->GetItemCount(), ss.str().c_str());
				}
				break;
				}

				break; // find command, and search next argument
			}
		}

		++argIdx;
	}

	if (!m_fileName.empty())
		SetTitle(m_fileName + " - " + g_version);

	// read logprinter_config.txt to load text:color
	common::cConfig config("logprinter_config.txt");
	for (auto &option : config.m_options)
	{
		sTextColor tColor;
		const common::Vector3 color = config.GetVector3(option.first);

		tColor.text = option.first;
		tColor.color = wxColour((unsigned char)(color.x * 255)
			, (unsigned char)(color.y * 255)
			, (unsigned char)(color.z * 255)
		);
		m_colors.push_back(tColor);
	}

	// default color
	if (m_colors.empty())
	{
		sTextColor tColor;
		tColor.text = "error";
		tColor.color = wxColour(255, 0, 0);
		m_colors.push_back(tColor);

		tColor.text = "Error";
		tColor.color = wxColour(255, 0, 0);
		m_colors.push_back(tColor);
	}

	m_fileTask = new cFileReadTask(this);
	if (!m_fileName.empty())
		m_fileTask->MessageProc(common::threadmsg::TASK_MSG, 0, 0, 0);
	m_wqSema.PushTask(m_fileTask);

	DragAcceptFiles(true);
	Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MyFrame::OnDropFiles), NULL, this);
}

MyFrame::~MyFrame()
{
	m_state = eState::Terminate;
	m_wqSema.Clear();
	m_fileTask = nullptr;
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
	menu.AppendCheckItem(MENU_TOGGLE_FASTSCROL, wxT("&Fast Scroll"));
	menu.AppendCheckItem(MENU_TOGGLE_STOP, wxT("&Stop"));
	menu.AppendCheckItem(MENU_CLEAR, wxT("&Clear"));
	menu.Check(MENU_TOGGLE_TOPMOST, m_isTopMost);
	menu.Check(MENU_TOGGLE_ROWNUM, m_isRowNum);
	menu.Check(MENU_TOGGLE_HEXA, m_isHexadecimal);
	menu.Check(MENU_TOGGLE_AUTOSCROL, m_isAutoScroll);
	menu.Check(MENU_TOGGLE_STOP, m_isStop);
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

void MyFrame::OnMenuFastscroll(wxCommandEvent& event)
{
	m_updateCnt = 100;
	m_fileTask->m_maxScrollSize = 1;
}

void MyFrame::OnMenuToggleStop(wxCommandEvent& event)
{
	m_isStop = !m_isStop;
}

void MyFrame::OnMenuClear(wxCommandEvent& event)
{
	if (m_listCtrl)
		m_listCtrl->DeleteAllItems();
	m_rowNumber = 1;

	{
		common::AutoCSLock cs(m_cs);
		while (!m_strs.empty())
			m_strs.pop();
	}
}

void MyFrame::OnSize(wxSizeEvent& event)
{
	event.Skip();
	m_listCtrl->SetColumnWidth(0, event.GetSize().GetWidth() - 20);
}

void MyFrame::OnDropFiles(wxDropFilesEvent& event)
{
	if (event.GetNumberOfFiles() <= 0)
		return;

	wxString* dropped = event.GetFiles();
	wxASSERT(dropped);
	m_fileName = dropped->c_str();
	m_isReload = true;
	m_inputType = eInputType::FILE;

	if (m_fileName.empty())
		return;

	std::stringstream ss;
	if (m_outputType == eInputType::NONE)
	{
		ss << "file=" << m_fileName << " - " << g_version;
	}
	else
	{
		if (m_svr.IsConnect())
		{
			ss << "file=" << m_fileName << " bindport=" << m_bindPort << " - " << g_version;
		}
		else
		{
			ss << "file=" << m_fileName << " - " << g_version;
		}
	}

	// send reload message
	if (eInputType::FILE == m_inputType)
		if (m_fileTask)
			m_fileTask->MessageProc(common::threadmsg::TASK_MSG, 0, 0, 0);

	SetTitle(ss.str());
	//SetTitle(m_fileName + " - " + g_version);
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


// mainloop, every call 0.1 second
void MyFrame::MainLoop()
{
	static double oldT = m_timer.GetSeconds();
	const double curT = m_timer.GetSeconds();
	const double deltaT = curT - oldT;
	//if (deltaT < 0.1f)
	//	return;
	//oldT = curT;

	if (m_isStop)
		return;

	if ((m_inputType == eInputType::NETWORK) && (m_networkState == 0))// network input
	{
		m_networkInitDelay -= deltaT;
		if (m_networkInitDelay < 0)
		{
			m_listCtrl->InsertItem(m_listCtrl->GetItemCount()
				, common::format("try connect server %s %d", m_ip.c_str(), m_port));
			if (m_client.Init(m_ip, m_port, 256, 256))
			{
				m_listCtrl->InsertItem(m_listCtrl->GetItemCount(), "success connect");

				std::stringstream ss;
				ss << "ip=" << m_ip << " port=" << m_port << " - " << g_version;
				SetTitle(ss.str());
			}
			else
			{
				m_listCtrl->InsertItem(m_listCtrl->GetItemCount(), "fail connect");

				std::stringstream ss;
				ss << "Fail Connect ip=" << m_ip << " port=" << m_port << " - " << g_version;
				SetTitle(ss.str());
			}
			m_networkState = 1; // network init 
		}
	}

	if ((m_outputType == eInputType::NETWORK) && (m_outputNetworkState == 0))// network output
	{
		m_outputNetworkInitDelay -= deltaT;
		if (m_outputNetworkInitDelay < 0)
		{
			m_listCtrl->InsertItem(m_listCtrl->GetItemCount()
				, common::format("try bind server %d", m_bindPort));
			if (m_svr.Init(m_bindPort, 256, 256))
			{
				m_listCtrl->InsertItem(m_listCtrl->GetItemCount(), "success bind");

				std::stringstream ss;
				ss << "file=[" << m_fileName << "] bindport=" << m_bindPort << " - " << g_version;
				SetTitle(ss.str());
			}
			else
			{
				m_listCtrl->InsertItem(m_listCtrl->GetItemCount(), "fail bind");

				std::stringstream ss;
				ss << "file=[" << m_fileName << "] fail bindport=" << m_bindPort << " - " << g_version;
				SetTitle(ss.str());
			}
			m_outputNetworkState = 1; // network init 
		}
	}

	switch (m_inputType)
	{
	case eInputType::NONE: break;
	case eInputType::FILE:
	{
		common::AutoCSLock cs(m_cs);
		int cnt = 0;
		while (!m_strs.empty() && (cnt++ < m_updateCnt))
		{
			OutputString(m_strs.front(), (int)m_strs.size());
			m_strs.pop();
		}
		if ((m_updateCnt >= 10) && (m_strs.size() < 10))
			m_updateCnt = 3;
	}
	break;
	case eInputType::NETWORK: InputFromServer(); break;
	default: assert(0); break;
	}
}


// add string
void MyFrame::AddString(const common::Str512& str)
{
	common::AutoCSLock cs(m_cs);
	m_strs.push(str.c_str());
}


// output string to listctrl
void MyFrame::OutputString(const string &line, const int strSize)
{
	if (line.empty() || (line == "\r"))
		return;

	// remove list item
	{
		const int itemCount = m_listCtrl->GetItemCount();
		const int remainSize = m_maxLineCount - itemCount;
		const int rmSize = (strSize > remainSize) ? (strSize - remainSize) : 0;
		int size = min(itemCount, rmSize);
		if (size >= (int)(m_maxLineCount * 0.65f))
			m_listCtrl->DeleteAllItems();
	}

	common::Str512 showText;
	if (m_isRowNum)
	{
		sprintf_s(showText.m_str, "%5d: ", m_rowNumber);
		++m_rowNumber;
	}

	showText += line;
	InsertString(showText);

	if (m_outputType == eInputType::NETWORK) // send network
	{
		if (m_svr.IsConnect())
		{
			m_svr.m_sendQueue.Push(0, m_svr.m_protocol, (const BYTE*)line.c_str(), line.size());
			m_svr.m_sendQueue.SendBroadcast(m_svr.m_sessions);
		}
	}
}


// input from server
void MyFrame::InputFromServer()
{
	using namespace std;

	if (!m_client.IsReadyConnect() && !m_client.IsConnect())
	{
		static bool showError = false;
		if (!showError)
		{
			InsertString("Client Disconnect");
			showError = true;
		}
		return;
	}

	network::sSockBuffer packet;
	while (m_client.m_recvQueue.Front(packet))
	{
		common::Str512 showText;

		string line;
		for (int i = 0; i < packet.actualLen; ++i)
			line += (char)packet.buffer[i];

		if (m_isRowNum)
		{
			sprintf_s(showText.m_str, "%5d: ", m_rowNumber);
			++m_rowNumber;
		}

		showText += line;
		InsertString(showText);

		m_client.m_recvQueue.Pop();
	}	
}


// Insert String to ListCtrl
void MyFrame::InsertString(const common::Str512 &str)
{
	const int idx = m_listCtrl->InsertItem(m_listCtrl->GetItemCount(), str.c_str());

	if (m_isAutoScroll)
		m_listCtrl->EnsureVisible(idx);

	wxColour color(255, 255, 255);
	for (auto &c : m_colors)
	{
		if (str.find(c.text.c_str()))
		{
			color = c.color;
			break;
		}
	}

	m_listCtrl->SetItemTextColour(idx, color);

	if (m_listCtrl->GetItemCount() > m_maxLineCount)
		m_listCtrl->DeleteItem(0);
}


void MyApp::OnIdle(wxIdleEvent& event)
{	
	if (m_frame)
		m_frame->MainLoop();
	Sleep(1);
	event.RequestMore();
}


//---------------------------------------------------------------------------------
// file read task
// 
// task running
common::cTask::eRunResult cFileReadTask::Run(const double deltaSeconds)
{
	while (MyFrame::eState::Run == m_frm->m_state)
	{
		ReadFile();
		Sleep(10);
	}
	return eRunResult::End;
}


// message process
void cFileReadTask::MessageProc(common::threadmsg::MSG msg
	, WPARAM wParam, LPARAM lParam, LPARAM added) 
{
	using namespace common::threadmsg;
	switch (msg)
	{
	case TASK_MSG:
	{
		if (0 == wParam) // reload?
		{
			m_isReload = true;
			m_fileName = m_frm->m_fileName;
		}
	}
	break;
	}
}


// input from file
void cFileReadTask::ReadFile()
{
	using namespace std;
	if (m_fileName.empty())
		return; // nothing to do

	const __int64 fileSize = FileSize(m_fileName);
	if (fileSize <= 0)
		return;

	if (!m_isReload && (fileSize == m_oldFileSize))
		return; // nothing to do

	ifstream ifs(m_fileName, ios::binary);
	if (!ifs.is_open())
		return;

	if (m_isReload || (fileSize < m_oldFileSize))
	{
		m_oldPos = ifs.tellg();
		m_isReload = false;
		m_oldFileSize = 0;
	}
	else
	{
		ifs.seekg(m_oldPos, std::ios::beg);
	}

	while (1)
	{
		m_oldPos = ifs.tellg();

		common::Str512 line;
		if (m_frm->m_isHexadecimal)
		{
			char buffer[64];
			ZeroMemory(buffer, sizeof(buffer));
			ifs.read(buffer, sizeof(buffer) - 1);

			int i = 0;
			while (1)
			{
				if (!buffer[i])
					break;

				char hex[3] = { NULL, NULL, NULL };
				sprintf_s(hex, "%2X", buffer[i]);
				line.m_str[i * 2] = hex[0];
				line.m_str[i * 2 + 1] = hex[1];
				++i;
			}

			// todo: all hexacode parsing
			m_frm->AddString(line);

			m_oldPos += i;
			m_oldFileSize = fileSize;
		}
		else
		{
			const int remainSize = m_bufferSize - m_curPos;
			const __int64 oldFileSize = max((__int64)0, m_oldFileSize);
			const int cpySize = (int)min((__int64)remainSize, fileSize - oldFileSize);
			if (cpySize <= 0)
				break;

			ifs.read(m_buffer + m_curPos, cpySize);
			const size_t extracted = ifs.gcount(); // actual read byte size
			if (extracted == 0)
				break; // read error
			m_oldPos = ifs.tellg(); // update current file pos
			m_curPos += extracted;
			m_oldFileSize = oldFileSize + extracted;

			if (abs(fileSize - oldFileSize) > (m_bufferSize * m_maxScrollSize))
			{
				m_curPos = 0;
				continue; // too much scroll, ignore data
			}
			else
			{
				m_maxScrollSize = 10; // recovery
			}

			vector<string> strs;
			SplitString(strs);

			for (auto& str : strs)
				m_frm->AddString(str);

			break; // to fast response
		}

		if (!m_frm->m_isHexadecimal && ifs.eof())
			break;
		if (m_frm->m_isHexadecimal)
			break;
	}
}


// split m_buffer with delimeter new line
// return split string
void cFileReadTask::SplitString(OUT vector<string>& out)
{
	static common::String<char, 2049> tok;
	static common::String<wchar_t, 2049> tok2;
	int start = 0;
	int i = 0;
	while (m_curPos > i)
	{
		const char c = m_buffer[i];
		if (('\r' != c) && ('\n' != c))
		{
			if (i - start < 200)
			{
				++i;
				continue;
			}
		}

		if (i - start >= 1)
		{
			int m = 0;
			for (int k = start; k < i; ++k)
				if ((int)tok.SIZE > m)
					tok.m_str[m++] = m_buffer[k];
			if (('\r' != c) && ('\n' != c))
				tok.m_str[m++] = c;
			tok.m_str[m] = NULL;

			// convert utf-8 unicode string and then convert ansi string
			tok2 = tok.wstrUTF8();
			out.push_back(tok2.str().c_str());
			tok.clear();
		}

		++i;
		start = i;
	}

	if (0 != start)
	{
		// move data to front
		int m = 0;
		int k = start;
		while (k < m_curPos)
			m_buffer[m++] = m_buffer[k++];
		m_curPos = m; // curPos indicate next write pos
	}
}
