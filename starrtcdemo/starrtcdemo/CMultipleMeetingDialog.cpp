// CMultipleMeetingDialog.cpp: 实现文件
//

#include "stdafx.h"
#include "starrtcdemo.h"
#include "CMultipleMeetingDialog.h"
#include "afxdialogex.h"
#include "HttpClient.h"
#include "json.h"
#include "CropType.h"
#include "CCreateLiveDialog.h"
enum MEETING_LIST_REPORT_NAME
{
	MEETING_NAME = 0,
	MEETING_ID,
	//MEETING_STATUS,
	MEETING_CREATER
};

// CMultipleMeetingDialog 对话框

IMPLEMENT_DYNAMIC(CMultipleMeetingDialog, CDialogEx)

CMultipleMeetingDialog::CMultipleMeetingDialog(CUserManager* pUserManager, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MULTIPLE_MEETING, pParent)
{
	m_pUserManager = pUserManager;
	CMeetingManager::addChatroomGetListListener(this);
	m_pMeetingManager = new CMeetingManager(m_pUserManager, this);
	m_pSoundManager = new CSoundManager(this);
	m_pCurrentProgram = NULL;
	m_nUpId = -1;
}

CMultipleMeetingDialog::~CMultipleMeetingDialog()
{
	m_bStop = true;
	m_bExit = true;
	if (m_pSoundManager != NULL)
	{
		delete m_pSoundManager;
		m_pSoundManager = NULL;
	}

	if (m_pMeetingManager != NULL)
	{
		delete m_pMeetingManager;
		m_pMeetingManager = NULL;
	}
	
	if (m_pDataShowView != NULL)
	{
		delete m_pDataShowView;
		m_pDataShowView = NULL;
	}
}

void CMultipleMeetingDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTCONTROL_MEETING_LIST, m_MeetingList);
	DDX_Control(pDX, IDC_STATIC__MEETING_SHOW_AEWA, m_ShowArea);
}


BEGIN_MESSAGE_MAP(CMultipleMeetingDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_MEETING_LISTBRUSH, &CMultipleMeetingDialog::OnBnClickedButtonMeetingListbrush)
	ON_BN_CLICKED(IDC_BUTTON_JOIN_MEETING, &CMultipleMeetingDialog::OnBnClickedButtonJoinMeeting)
	ON_NOTIFY(NM_CLICK, IDC_LISTCONTROL_MEETING_LIST, &CMultipleMeetingDialog::OnNMClickListcontrolMeetingList)
	ON_BN_CLICKED(IDC_BUTTON_CREATE_MEETING, &CMultipleMeetingDialog::OnBnClickedButtonCreateMeeting)
END_MESSAGE_MAP()


// CMultipleMeetingDialog 消息处理程序


void CMultipleMeetingDialog::OnBnClickedButtonMeetingListbrush()
{
	getMeetingList();
}


void CMultipleMeetingDialog::OnBnClickedButtonJoinMeeting()
{
	POSITION pos = m_MeetingList.GetFirstSelectedItemPosition();
	int nIndex = m_MeetingList.GetSelectionMark();
	if (nIndex >= 0)
	{
		CString str = m_MeetingList.GetItemText(nIndex, MEETING_ID);
		string strId = str.GetBuffer(0);
		if (strId.length() == 32)
		{
			/*CLiveDialog dlg(m_pUserManager, this);
			string strChannelId = strId.substr(0, 16);
			string strChatRoomId = strId.substr(16, 16);
			dlg.m_pStartRtcLive->setInfo(strChannelId, strChatRoomId);
			dlg.DoModal();*/
		}
		else
		{
			CString strMessage = "";
			strMessage.Format("err id %s", strId.c_str());
			AfxMessageBox(strMessage);
		}
	}
}


BOOL CMultipleMeetingDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	LONG lStyle;
	lStyle = GetWindowLong(m_MeetingList.m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK;
	lStyle |= LVS_REPORT;
	SetWindowLong(m_MeetingList.m_hWnd, GWL_STYLE, lStyle);

	DWORD dwStyle = m_MeetingList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;                                        //选中某行使整行高亮(LVS_REPORT)
	dwStyle |= LVS_EX_GRIDLINES;                                            //网格线(LVS_REPORT)
	//dwStyle |= LVS_EX_CHECKBOXES;                                            //CheckBox
	m_MeetingList.SetExtendedStyle(dwStyle);

	m_MeetingList.InsertColumn(MEETING_ID, _T("ID"), LVCFMT_LEFT, 210);
	m_MeetingList.InsertColumn(MEETING_NAME, _T("Name"), LVCFMT_LEFT, 120);
	m_MeetingList.InsertColumn(MEETING_CREATER, _T("Creator"), LVCFMT_LEFT, 80);
	//m_MeetingList.InsertColumn(MEETING_STATUS, _T("liveState"), LVCFMT_LEFT, 80);
	getMeetingList();

	CRect rect;
	::GetWindowRect(m_ShowArea, rect);
	CRect dlgRect;
	::GetWindowRect(this->m_hWnd, dlgRect);
	int left = rect.left - dlgRect.left - 7;
	int top = rect.top - dlgRect.top - 25;

	CRect showRect(left, top, left + rect.Width() - 5, top + rect.Height() - 15);

	m_pDataShowView = new CDataShowView();
	m_pDataShowView->setDrawRect(showRect);

	CPicControl *pPicControl = new CPicControl();
	pPicControl->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_BITMAP, showRect, this, WM_USER + 100);
	//mShowPicControlVector[i] = pPicControl;
	pPicControl->ShowWindow(SW_SHOW);
	dwStyle = ::GetWindowLong(pPicControl->GetSafeHwnd(), GWL_STYLE);
	::SetWindowLong(pPicControl->GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);

	m_pDataShowView->m_pPictureControlArr[0] = pPicControl;
	pPicControl->setInfo(this, m_pDataShowView);

	CRect rectClient = showRect;
	CRect rectChild(rectClient.right - (int)(rectClient.Width()*0.25), rectClient.top, rectClient.right, rectClient.bottom);

	for (int n = 0; n < 6; n++)
	{
		CPicControl *pPictureControl = new CPicControl();
		pPictureControl->setInfo(this, m_pDataShowView);
		rectChild.top = rectClient.top + (long)(n * rectClient.Height()*0.25);
		rectChild.bottom = rectClient.top + (long)((n + 1) * rectClient.Height()*0.25);
		pPictureControl->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_BITMAP, rectChild, this, WM_USER + 100 + n + 1);
		m_pDataShowView->m_pPictureControlArr[n + 1] = pPictureControl;
		DWORD dwStyle1 = ::GetWindowLong(pPictureControl->GetSafeHwnd(), GWL_STYLE);
		::SetWindowLong(pPictureControl->GetSafeHwnd(), GWL_STYLE, dwStyle1 | SS_NOTIFY);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CMultipleMeetingDialog::getMeetingList()
{
	CMeetingManager::getMeetingList(m_pUserManager);
	/*m_MeetingList.DeleteAllItems();

	CString stUrl = "";
	stUrl.Format(_T("%s/meeting/list?appid=%s"), m_pUserManager->m_ServiceParam.m_strRequestListAddr.c_str(), m_pUserManager->m_ServiceParam.m_strAgentId.c_str());

	int port = 9904;
	char* data = "";
	//std::string strVal = "";
	//std::string strErrInfo = "";
	CString strPara = _T("");
	CString strContent;

	CHttpClient httpClient;
	int nRet = httpClient.HttpPost(stUrl, strPara, strContent);

	//CString str;
	//str.Format(_T("%d  "), (int)nRet);
	//str += strContent;
	//AfxMessageBox(str);

	string str_json = strContent.GetBuffer(0);
	Json::Reader reader;
	Json::Value root;
	if (nRet == 0 && str_json != "" && reader.parse(str_json, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素   
	{
		std::cout << "========================[Dispatch]========================" << std::endl;
		if (root.isMember("status") && root["status"].asInt() == 1)
		{
			if (root.isMember("data"))
			{
				Json::Value data = root.get("data", "");
				int nSize = data.size();
				for (int i = 0; i < nSize; i++)
				{
					if (data[i].isMember("Name"))
					{
						m_MeetingList.InsertItem(i, data[i]["Name"].asCString());
					}

					if (data[i].isMember("ID"))
					{
						m_MeetingList.SetItemText(i, MEETING_ID, data[i]["ID"].asCString());
					}

					if (data[i].isMember("Creator"))
					{
						m_MeetingList.SetItemText(i, MEETING_CREATER, data[i]["Creator"].asCString());
					}
				}
			}

		}

	}*/
}

void CMultipleMeetingDialog::getLocalSoundData(char* pData, int nLength)
{
	if (m_pMeetingManager != NULL)
	{
		m_pMeetingManager->insertAudioRaw((uint8_t*)pData, nLength);
	}
}

void CMultipleMeetingDialog::querySoundData(char** pData, int* nLength)
{
	if (m_pMeetingManager != NULL)
	{
		m_pMeetingManager->querySoundData((uint8_t**)pData, nLength);
	}
}
/**
 * 查询聊天室列表回调
 */
int CMultipleMeetingDialog::chatroomQueryAllListOK(list<ChatroomInfo>& listData)
{
	m_MeetingList.DeleteAllItems();
	list<ChatroomInfo>::iterator iter = listData.begin();
	int i = 0;
	for (; iter != listData.end(); iter++)
	{
		m_MeetingList.InsertItem(i, (char*)(*iter).m_strName.c_str());
		m_MeetingList.SetItemText(i, MEETING_ID, (char*)(*iter).m_strRoomId.c_str());
		m_MeetingList.SetItemText(i, MEETING_CREATER, (char*)(*iter).m_strCreaterId.c_str());
	}
	return 0;
}
/**
 * 聊天室成员数变化
 * @param number
 */
void CMultipleMeetingDialog::onMembersUpdated(int number)
{
}

/**
 * 自己被踢出聊天室
 */
void CMultipleMeetingDialog::onSelfKicked()
{
}

/**
 * 自己被踢出聊天室
 */
void CMultipleMeetingDialog::onSelfMuted(int seconds)
{
}

/**
 * 聊天室已关闭
 */
void CMultipleMeetingDialog::onClosed()
{
}

/**
 * 收到消息
 * @param message
 */
void CMultipleMeetingDialog::onReceivedMessage(CIMMessage* pMessage)
{
}

/**
 * 收到私信消息
 * @param message
 */
void CMultipleMeetingDialog::onReceivePrivateMessage(CIMMessage* pMessage)
{
}

int CMultipleMeetingDialog::uploaderAdd(char* upUserId, int upId)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->addUpId(upId);
		m_pDataShowView->setShowPictures();
	}
	return 0;
}
int CMultipleMeetingDialog::uploaderRemove(char* upUserId, int upId)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeUpUser(upId);
		m_pDataShowView->setShowPictures();
	}
	return 0;
}
int CMultipleMeetingDialog::getRealtimeData(int upId, uint8_t* data, int len)
{
	return 0;
}
int CMultipleMeetingDialog::getVideoRaw(int upId, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->drawPic(FMT_YUV420P, upId, w, h, videoData, videoDataLen);
	}
	return 0;
}
int CMultipleMeetingDialog::deleteChannel(char* channelId)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUpUser();
		m_pDataShowView->setShowPictures();
	}
	return 0;
}
int CMultipleMeetingDialog::stopOK()
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUpUser();
		m_pDataShowView->setShowPictures();
	}
	return 0;
}
int CMultipleMeetingDialog::srcError(char* errString)
{
	CString strErr;
	strErr.Format("live error errmsg:%s", errString);
	AfxMessageBox(strErr);
	return 0;
}

void CMultipleMeetingDialog::addUpId()
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->addUpId(m_nUpId);
		m_pDataShowView->setShowPictures();
	}
}

void CMultipleMeetingDialog::insertVideoRaw(uint8_t* videoData, int dataLen, int isBig)
{
	if (m_pMeetingManager != NULL)
	{
		m_pMeetingManager->insertVideoRaw(videoData, dataLen, isBig);
	}
}

int CMultipleMeetingDialog::cropVideoRawNV12(int w, int h, uint8_t* videoData, int dataLen, int yuvProcessPlan, int rotation, int needMirror, uint8_t* outVideoDataBig, uint8_t* outVideoDataSmall)
{
	int ret = 0;
	if (m_pMeetingManager != NULL)
	{
		ret = m_pMeetingManager->cropVideoRawNV12(w, h, videoData, dataLen, yuvProcessPlan, 0, 0, outVideoDataBig, outVideoDataSmall);
	}
	return ret;
}
void CMultipleMeetingDialog::drawPic(YUV_TYPE type, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->drawPic(type, m_nUpId, w, h, videoData, videoDataLen);
	}
}

void CMultipleMeetingDialog::liveExit(void* pParam)
{
	//CDataShowView* pDataShowView = (CDataShowView*)pParam;

	if (m_pDataShowView != NULL)
	{
		if (m_pMeetingManager != NULL)
		{
			m_pMeetingManager->closeLive();
		}

		stopGetData();
		if (m_pSoundManager != NULL)
		{
			m_pSoundManager->stopGetSoundData();
		}
		m_pDataShowView->removeAllUpUser();
		m_pDataShowView->setShowPictures();
	}
}
void CMultipleMeetingDialog::changeStreamConfig(void* pParam, int upid)
{
	//CDataShowView* pDataShowView = (CDataShowView*)pParam;

	if (m_pMeetingManager != NULL && upid < UPID_MAX_SIZE - 1)
	{
		if (upid == -1)
		{
			m_pDataShowView->resetStreamConfig(false);
		}
		else if (m_pDataShowView->m_configArr[upid] != 2)
		{
			for (int i = 0; i < UPID_MAX_SIZE - 1; i++)
			{
				if (m_pDataShowView->m_configArr[i] == 2)
				{
					m_pDataShowView->m_configArr[i] = 1;	
					break;
				}
			}
			m_pDataShowView->m_configArr[upid] = 2;
		}
		m_pMeetingManager->setStreamConfig(m_pDataShowView->m_configArr, UPID_MAX_SIZE);
		m_pDataShowView->setShowPictures();
	}
}

void CMultipleMeetingDialog::closeCurrentLive(void* pParam)
{
	if (m_pDataShowView != NULL)
	{
		if (m_pMeetingManager != NULL)
		{
			m_pMeetingManager->closeLive();
		}

		stopGetData();
		if (m_pSoundManager != NULL)
		{
			m_pSoundManager->stopGetSoundData();
		}
		m_pDataShowView->removeAllUpUser();
		m_pDataShowView->setShowPictures();
	}
}

void CMultipleMeetingDialog::startFaceFeature(void* pParam)
{
}

void CMultipleMeetingDialog::stopFaceFeature(void* pParam)
{
}

void CMultipleMeetingDialog::OnNMClickListcontrolMeetingList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUpUser();
		m_pDataShowView->setShowPictures();
	}
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopGetSoundData();
	}

	*pResult = 0;
	int nItem = -1;
	if (pNMItemActivate != NULL)
	{
		nItem = pNMItemActivate->iItem;
		if (nItem >= 0)
		{
			CString strId = m_MeetingList.GetItemText(nItem, MEETING_ID);
			CString strName = m_MeetingList.GetItemText(nItem, MEETING_NAME);
			CString strCreater = m_MeetingList.GetItemText(nItem, MEETING_CREATER);
			if (m_pCurrentProgram == NULL)
			{
				m_pCurrentProgram = new CLiveProgram();
			}
			m_pCurrentProgram->m_strId = strId;
			m_pCurrentProgram->m_strName = strName;
			m_pCurrentProgram->m_strCreator = strCreater;
			if (m_pMeetingManager != NULL)
			{
				if (strId.GetLength() == 32)
				{
					string strChannelId = strId.Left(16);
					string strChatRoomId = strId.Right(16);
					m_pDataShowView->resetStreamConfig(false);
					bool bRet = m_pMeetingManager->join(strChatRoomId, strChannelId, m_pDataShowView->m_configArr, UPID_MAX_SIZE);
					if (!bRet)
					{
						AfxMessageBox("申请加入失败！");
					}
					else
					{
						startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
						if (m_pSoundManager != NULL)
						{
							m_pSoundManager->startGetSoundData(true);
						}
					}
				}			
			}
		}
	}
}


void CMultipleMeetingDialog::OnBnClickedButtonCreateMeeting()
{
	CString strName = "";
	bool bPublic = false;
	CHATROOM_TYPE chatRoomType = CHATROOM_TYPE::CHATROOM_TYPE_PUBLIC;
	MEETING_TYPE channelType = MEETING_TYPE::MEETING_TYPE_LOGIN_SPECIFY;
	CCreateLiveDialog dlg(m_pUserManager);
	if (dlg.DoModal() == IDOK)
	{
		strName = dlg.m_strLiveName;
		bPublic = dlg.m_bPublic;
	}
	else
	{
		return;
	}

	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopGetSoundData();
	}
	m_pDataShowView->removeAllUpUser();
	m_pDataShowView->setShowPictures();

	if (m_pMeetingManager != NULL)
	{
		m_pDataShowView->resetStreamConfig(false);
		bool bRet = m_pMeetingManager->createAndJoin(strName.GetBuffer(0), chatRoomType, channelType, m_pDataShowView->m_configArr, UPID_MAX_SIZE);
		if (bRet)
		{
			startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
			if (m_pSoundManager != NULL)
			{
				m_pSoundManager->startGetSoundData(true);
			}
		}
		else
		{
			AfxMessageBox("创建会议失败 !");
		}
	}
}
