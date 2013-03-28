//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, U$
//
// Copyright (C) 2005-2010  Oliver Schroeder
//

//////////////////////////////////////////////////////////////////////
//
//  server for FlightGear
//
//////////////////////////////////////////////////////////////////////

#if !defined FG_SERVER_HXX
#define FG_SERVER_HXX

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <plib/netSocket.h>
#include <flightgear/MultiPlayer/mpmessages.hxx>
#include <flightgear/MultiPlayer/tiny_xdr.hxx>
#include <simgear/debug/logstream.hxx>
#include "daemon.hxx"
#include "fg_geometry.hxx"
#include "fg_player.hxx"
#include "fg_tracker.hxx"

using namespace std;

//////////////////////////////////////////////////////////////////////
/**
 * @class FG_SERVER
 * @brief The server class
 */
class FG_SERVER
{
public:
	
	
	/** @brief Internal Constants */
	enum FG_SERVER_CONSTANTS
	{
		// return values
		SUCCESS                 = 0,
		ERROR_COMMANDLINE       = 1,
		ERROR_CREATE_SOCKET     = 2,
		ERROR_COULDNT_BIND      = 3,
		ERROR_NOT_LISTENING     = 4,
		ERROR_COULDNT_LISTEN    = 5,
		
		// other constants
		MAX_PACKET_SIZE         = 1024,
		UPDATE_INACTIVE_PERIOD  = 1,
		MAX_TELNETS             = 5,
		RELAY_MAGIC             = 0x53464746    // GSGF
	};
	//////////////////////////////////////////////////
	//
	//  constructors
	//
	//////////////////////////////////////////////////
	FG_SERVER ();
	~FG_SERVER ();

	//////////////////////////////////////////////////
	//
	//  methods
	//
	//////////////////////////////////////////////////
	int   Init ();
	int   Loop ();
	void  Done ();

	void  PrepareInit ();
	void  SetDataPort ( int Port );
	void  SetTelnetPort ( int Port );
	void  SetPlayerExpires ( int Seconds );
	void  SetOutOfReach ( int OutOfReach );
	void  SetHub ( bool IamHUB );
	void  SetLoglevel ( int Loglevel );
	void  SetLogfile ( const std::string &LogfileName );
	void  SetServerName ( const std::string &ServerName );
	void  SetBindAddress ( const std::string &BindAddress );
	void  AddRelay ( const string & Server, int Port );
	void  AddCrossfeed ( const string & Server, int Port );
	int   AddTracker ( const string & Server, int Port, bool IsTracked );
	void  AddBlacklist  ( const string& FourDottedIP );
	void  CloseTracker ();
	#ifdef _MSC_VER
	void  TelnetReply(netSocket* NewTelnet);
	#endif
	int   check_keyboard();
  void  Show_Stats(void);
	void* HandleTelnet  (int Fd);
	pid_t m_TrackerPID; // tracker child pid

private:

	//////////////////////////////////////////////////
	/**
	 * @class mT_Relay
	 * @brief Type of list of relays
	 */
	class mT_Relay
	{
		public:
		string      Name;
		netAddress  Address;
	};
 
	//////////////////////////////////////////////////
	/**
	 * @class mT_ChatMsg
	 * @brief Chat messages from server to clients
	 * @todo Document the vars
	 */
	class mT_ChatMsg
	{
	public:
		
		/** 
		 * @brief -- todo -- 
		 * @param I int 
		 * @param M char
		 */
		mT_ChatMsg (int I, char* M) { Target=I; Msg=M; };
		
		/** @brief 0 = all */
		int   Target; // 0 = all
		
		/** @brief Pointer to the message */
		char* Msg;
	};
	//////////////////////////////////////////////////
	//
	//  private variables
	//  
	//////////////////////////////////////////////////
	typedef std::vector<FG_Player>              mT_PlayerList;
	typedef mT_PlayerList::iterator           mT_PlayerListIt;
	typedef std::list<mT_Relay>               mT_RelayList;
	typedef mT_RelayList::iterator            mT_RelayListIt;
	typedef std::list<mT_ChatMsg>             mT_MessageList;
	typedef std::list<mT_ChatMsg>::iterator   mT_MessageIt;
	typedef std::map<uint32_t,bool>           mT_BlackList;
	typedef std::map<uint32_t,bool>::iterator mT_BlackListIt;
	typedef std::map<uint32_t,string>         mT_IP2RelayNames;
	typedef std::map<uint32_t,string>::iterator mT_RelayMapIt;
	bool              m_Initialized;
	bool              m_ReinitData;
	bool              m_ReinitTelnet;
	bool              m_Listening;
	int               m_ListenPort;
	int               m_TelnetPort;
	int               m_TrackingPort;
	int               m_PlayerExpires;
	int               m_PlayerIsOutOfReach;
	sgDebugPriority   m_Loglevel;
	ofstream          m_LogFile;
	string            m_LogFileName;
	string            m_BindAddress;
	int               m_NumCurrentClients;
	int               m_NumMaxClients;
	int16_t           m_ProtoMinorVersion;
	int16_t           m_ProtoMajorVersion;
	bool              m_IsParent;
	bool              m_IsTracked;
	int               m_MaxClientID;
	string            m_ServerName;
	string            m_TrackerServer;
	netSocket*        m_DataSocket;
	netSocket*        m_TelnetSocket;
	mT_PlayerList     m_PlayerList;
	mT_RelayList      m_RelayList;
	mT_IP2RelayNames  m_RelayMap;
	mT_RelayList      m_CrossfeedList;
	mT_MessageList    m_MessageList;
	int               m_ipcid;
	int               m_childpid;
	FG_TRACKER*       m_Tracker;
	mT_BlackList      m_BlackList;
	bool              m_IamHUB;
	pthread_mutex_t   m_PlayerMutex;
    time_t            m_UpdateSecs;
	//////////////////////////////////////////////////
	//
	//  statistics
	//
	//////////////////////////////////////////////////
	int               m_PacketsReceived; // rw data packet received
	int               m_BlackRejected;  // in black list
	int               m_PacketsInvalid; // invalid packet
	int               m_UnknownRelay; // unknown relay
	int               m_RelayMagic; // known relay packet
	int               m_PositionData;   // position data packet
  int               m_NotPosData;     // packet NOT pos data
	int               m_TelnetReceived;
  int               mT_PacketsReceived, mT_BlackRejected, mT_PacketsInvalid;
  int               mT_UnknownRelay, mT_PositionData, mT_TelnetReceived;
  int               mT_RelayMagic, mT_NotPosData;
  int               m_CrossFeedFailed, m_CrossFeedSent;
  int               mT_CrossFeedFailed, mT_CrossFeedSent;
  int               m_TrackerConnect, m_TrackerDisconnect,m_TrackerPostion;

	//////////////////////////////////////////////////
	//
	//  private methods
	//  
	//////////////////////////////////////////////////
	void  AddClient     ( const netAddress& Sender, char* Msg);
	void  AddBadClient  ( const netAddress& Sender, string &ErrorMsg,
			bool IsLocal );
	bool  IsBlackListed ( const netAddress& SenderAddress );
	bool  IsKnownRelay ( const netAddress& SenderAddress );
	bool  PacketIsValid ( int Bytes, T_MsgHdr *MsgHdr,
			const netAddress &SenderAddress );
	void  HandlePacket  ( char *sMsg, int Bytes,
			const netAddress &SenderAdress );
	int   UpdateTracker ( string callsign, string passwd, string modelname,
			time_t time, int type );
	void  DropClient ( mT_PlayerListIt& CurrentPlayer );
	void  CreateChatMessage ( int ID, string Msg );
	void  SendChatMessages ( mT_PlayerListIt& CurrentPlayer );
	void  DeleteMessageQueue ();
	bool  IsInRange ( mT_Relay& Relay,  FG_Player& SendingPlayer );
	void  SendToCrossfeed ( char * Msg, int Bytes, const netAddress& SenderAddress );
	void  SendToRelays ( char* Msg, int Bytes, mT_PlayerListIt& SendingPlayer );
	int   SenderIsKnown ( const string& SenderCallsign, const netAddress &SenderAddress);
}; // FG_SERVER

typedef struct st_telnet
{
	FG_SERVER* Instance;
	int        Fd;
} st_telnet;
#endif

// vim: ts=2:sw=2:sts=0

