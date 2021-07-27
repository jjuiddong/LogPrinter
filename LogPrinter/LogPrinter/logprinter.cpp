//
// 2016-05-13, jjuiddong
// 
//	- first version
//		- print log, error Red color, TopMost, Clear
//
// - ver 1.01
//		- display row line number
//
// - ver 1.02
//		- display hexadecimal
//
// - ver 1.03
//		- network send, recv
//
// - ver 1.04
//		- optimize & refactoring
//		- maximum line = 500 -> 100
//		- stop menu
//		- logprinter_config.txt
//			- token = color
//				- logprinter_config.txt Sample
//					error = 1, 0, 0
//					Error = 1, 0, 0
//					Recv = 0, 1, 0
//
// - ver 1.06, 2021-07-27
//	- ver 1.05 skip
//	- string missing bug fix
//
//
#include "../../../Common/Common/common.h"
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/listctrl.h"
#include "../../../Common/Network/network.h"
#include <iomanip> // setw()

string g_version = "ver 1.06";
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
	void OnQuit(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnDropFiles(wxDropFilesEvent& event);
	void OnContextMenu(wxContextMenuEvent& event);
	void OnMenuToggleTopMost(wxCommandEvent& event);
	void OnMenuToggleRowNum(wxCommandEvent& event);
	void OnMenuToggleHexadecimal(wxCommandEvent& event);
	void OnMenuToggleAutoscroll(wxCommandEvent& event);
	void OnMenuToggleStop(wxCommandEvent& event);
	void OnMenuClear(wxCommandEvent& event);


protected:
	void InputFromFile();
	void InputFromServer();
	void ToggleTopMost();
	void SplitString(OUT vector<string> &out);
	void OutputString(const common::Str512 &str);
	void InsertString(const common::Str512 &str);


private:
	wxListCtrl *m_listCtrl;
	string m_fileName;
	int m_maxLineCount = 100; // 화면에 출력할 최대 라인 수 (실행인자 값으로 설정 가능, 두 번째 인자)
	bool m_isReload;
	bool m_isTopMost;
	bool m_isRowNum;
	bool m_isHexadecimal;
	bool m_isAutoScroll;
	bool m_isStop;
	int m_rowNumber;

	struct eInputType {
		enum Enum {
			I_NONE, I_FILE, I_NETWORK
		};
	};
	eInputType::Enum m_inputType;
	eInputType::Enum m_outputType;
	string m_ip;
	int m_port;
	int m_bindPort;
	int m_networkState; //-1:use not network, 0:before connect or bind, 1:after connect or bind
	int m_outputNetworkState;//-1:use not network, 0:before connect or bind, 1:after connect or bind
	double m_networkInitDelay; // 프로그램이 실행 되고 난 뒤, 몇 초후에 네트워크를 초기화할지를 나타냄. seconds
	double m_outputNetworkInitDelay; // 프로그램이 실행 되고 난 뒤, 몇 초 후에 네트워크를 초기화할지를 나타냄. seconds
	char *m_buffer;
	int m_bufferSize;
	int m_curPos; // current buffer pos(index)
	network::cTCPServer m_svr;
	network::cTCPClient2 m_client;
	common::cTimer m_timer;
	struct sTextColor {
		string text;
		wxColour color;
	};
	vector<sTextColor> m_colors;
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
EVT_MENU(MENU_TOGGLE_STOP, MyFrame::OnMenuToggleStop)
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
	, m_isStop(false)
	, m_rowNumber(1)
	, m_inputType(eInputType::I_NONE)
	, m_outputType(eInputType::I_NONE)
	, m_networkState(-1)
	, m_outputNetworkState(-1)
	, m_networkInitDelay(0.3f)
	, m_outputNetworkInitDelay(0.4f)
	, m_svr(new network::cProtocol("LOGS"))
	, m_buffer(nullptr)
	, m_bufferSize(2048)
	, m_curPos(0)
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
					m_outputType = eInputType::I_NETWORK;
					m_bindPort = atoi(param.c_str());
					m_outputNetworkState = 0;
					break;

				case 1: // file=
					m_inputType = eInputType::I_FILE;
					m_fileName = param;
					break;

				case 2: // line=
					m_maxLineCount = atoi(param.c_str());
					break;

				case 3: // ip=
					m_ip = param;
					m_inputType = eInputType::I_NETWORK;
					m_networkState = 0;
					m_client.m_state = network::cTCPClient2::READYCONNECT;
					break;

				case 4: // port=
					m_port = atoi(param.c_str());
					m_inputType = eInputType::I_NETWORK;
					m_networkState = 0;
					break;

				case 5: // oplog
				{
					// yyymmdd_AMROperation.txt
					m_inputType = eInputType::I_FILE;
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

	m_buffer = new char[m_bufferSize];	

	DragAcceptFiles(true);
	Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MyFrame::OnDropFiles), NULL, this);
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(true);
	SAFE_DELETE(m_buffer);
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

void MyFrame::OnMenuToggleStop(wxCommandEvent& event)
{
	m_isStop = !m_isStop;
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
	m_listCtrl->SetColumnWidth(0, event.GetSize().GetWidth() - 20);
}

void MyFrame::OnDropFiles(wxDropFilesEvent& event)
{
	if (event.GetNumberOfFiles() > 0) {

		wxString* dropped = event.GetFiles();
		wxASSERT(dropped);
		m_fileName = *dropped;
		m_isReload = true;
		m_inputType = eInputType::I_FILE;

		if (!m_fileName.empty())
		{
			std::stringstream ss;
			if (m_outputType == eInputType::I_NONE)
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

			SetTitle(ss.str());
			//SetTitle(m_fileName + " - " + g_version);
		}
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
	static double oldT = m_timer.GetSeconds();
	const double curT = m_timer.GetSeconds();
	const double deltaT = curT - oldT;
	if (deltaT < 0.1f)
		return;
	oldT = curT;

	if (m_isStop)
		return;

	if ((m_inputType == eInputType::I_NETWORK) && (m_networkState == 0))// network input
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

	if ((m_outputType == eInputType::I_NETWORK) && (m_outputNetworkState == 0))// network output
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
	case eInputType::I_NONE: break;
	case eInputType::I_FILE: InputFromFile(); break;
	case eInputType::I_NETWORK: InputFromServer(); break;
	default: assert(0); break;
	}
}


// input from file
void MyFrame::InputFromFile()
{
	using namespace std;

	const __int64 curSize = FileSize(m_fileName);
	if (curSize <= 0)
		return;

	if (!m_isReload && (curSize == g_oldFileSize))
		return; // nothing to do

	ifstream ifs(m_fileName, ios::binary);
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

		common::Str512 line;
		if (m_isHexadecimal)
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
			OutputString(line);

			g_oldPos += i;
		}
		else
		{
			const int remainSize = m_bufferSize - m_curPos;
			const __int64 oldFileSize = max((__int64)0, g_oldFileSize);
			const int cpySize = (int)min((__int64)remainSize, curSize - oldFileSize);
			if (cpySize <= 0)
				break;

			ifs.read(m_buffer + m_curPos, cpySize);
			const size_t extracted = ifs.gcount(); // actual read byte size
			g_oldPos = ifs.tellg(); // update current file pos
			m_curPos += extracted;
			g_oldFileSize = oldFileSize + extracted;

			vector<string> strs;
			SplitString(strs);
			for (auto &str : strs)
				OutputString(str);
			if (strs.empty() || (0 == extracted))
				break;
		}

		if (!m_isHexadecimal && ifs.eof())
			break;
		if (m_isHexadecimal)
			break;
	}

	g_oldFileSize = curSize;	
}


// output string to listctrl
void MyFrame::OutputString(const common::Str512 &line)
{
	if (line.empty() || (line == "\r"))
		return;

	common::Str512 showText;

	if (m_isRowNum)
	{
		sprintf_s(showText.m_str, "%5d: ", m_rowNumber);
		++m_rowNumber;
	}

	// 파일이 너무크면, 화면에 모두 출력하지 않는다. lineNumber는 증가시킨다.
	//if (curSize - g_oldPos > 3000)
	//	continue;

	showText += line;
	InsertString(showText);

	if (m_outputType == eInputType::I_NETWORK) // send network
	{
		if (m_svr.IsConnect())
		{
			m_svr.m_sendQueue.Push(0, m_svr.m_protocol, (const BYTE*)line.c_str(), line.size());
			m_svr.m_sendQueue.SendBroadcast(m_svr.m_sessions);
		}
	}
}


// split m_buffer with delimeter new line
// return split string
void MyFrame::SplitString(OUT vector<string> &out)
{
	common::Str512 tok;
	common::WStr512 tok2;
	int start = 0;
	int i = 0;
	while (m_curPos > i)
	{
		const char c = m_buffer[i];
		if (('\r' != c) && ('\n' != c))
		{
			++i;
			continue;
		}

		if (i - start >= 1)
		{
			int m = 0;
			for (int k = start; k < i; ++k)
				tok.m_str[m++] = m_buffer[k];

			// convert utf-8 unicode string and then convert ansi string
			tok2 = tok.wstrUTF8();
			out.push_back(tok2.str().c_str());
			tok.clear();
		}

		++i;
		start = i;
	}

	if (start == m_curPos)
	{
		m_curPos = 0; // clear buffer
	}
	else if (0 != start)
	{
		// move data to front
		int m = 0;
		int k = start;
		while(k < m_curPos)
			m_buffer[m++] = m_buffer[k++];
		m_curPos = m_curPos - start; // curPos indicate next write pos (+1)
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
	Sleep(10);
	event.RequestMore();
}
