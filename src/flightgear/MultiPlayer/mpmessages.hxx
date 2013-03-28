/**
 * @file mpmessages.hxx 
 * @author  Duncan McCreanor
 * 
 * @brief Message definitions for multiplayer communications
 *        within a multiplayer Flightgear
 * 
 * @note
 *      Each message used for multiplayer communications
 *      consists of a header and optionally a block of data. The combined
 *      header and data is sent as one IP packet.
 */

//
//  Written by Duncan McCreanor, started February 2003.
// duncan.mccreanor@airservicesaustralia.com
//
// Copyright (C) 2003  Airservices Australia
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef MPMESSAGES_H
#define MPMESSAGES_H

#define MPMESSAGES_HID "$Id: mpmessages.hxx,v 1.1.1.1 2007/06/12 10:10:24 oliver Exp $"





#include <vector>
#include <simgear/compiler.h>
#include <simgear/math/SGMath.hxx>
#include "tiny_xdr.hxx"

/** @brief magic value for messages */
const uint32_t MSG_MAGIC = 0x46474653;  // "FGFS"


/** @brief  protocol version */
const uint32_t PROTO_VER = 0x00010001;  // 1.1


#define CHAT_MSG_ID             1
#define RESET_DATA_ID           6
#define POS_DATA_ID             7

/** @brief XDR demands 4 byte alignment, but some compilers use8 byte alignment
 *         so it's safe to let the overall size of a network message be a 
 *         multiple of 8!
 */
#define MAX_CALLSIGN_LEN        8
#define MAX_CHAT_MSG_LEN        256
#define MAX_MODEL_NAME_LEN      96
#define MAX_PROPERTY_LEN        52


/** 
 * @struct T_MsgHdr
 * @brief Header for use with all messages sent 
 */
struct T_MsgHdr {
	
	/** @brief Magic Value */
    xdr_data_t  Magic;   
    
    /** @brief Protocol version */
    xdr_data_t  Version;           
    
    /** @brief Message identifier  */
    xdr_data_t  MsgId;     
    
    /** @brief Absolute length of message */
    xdr_data_t  MsgLen;    
    
    /** @brief Player's receiver address */
    xdr_data_t  ReplyAddress;   
    
    /** @brief Player's receiver port */
    xdr_data_t  ReplyPort;   
    
    /** @brief Callsign used by the player */
    char Callsign[MAX_CALLSIGN_LEN]; 
};


/** 
 * @struct T_ChatMsg
 * @brief Chat message 
 */
struct T_ChatMsg {
	
	/** @brief Text of chat message */
    char Text[MAX_CHAT_MSG_LEN];  
};


/** 
 * @struct T_PositionMsg
 * @brief Position Message
 */
struct T_PositionMsg {
	
	/** @brief  Name of the aircraft model */
    char Model[MAX_MODEL_NAME_LEN]; 

    /** @brief Time when this packet was generated */
    xdr_data2_t time;
	
	/** @brief Time when this packet was generated */
    xdr_data2_t lag;

    /** @brief Position wrt the earth centered frame */
    xdr_data2_t position[3];
	
	
    /** @brief Orientation wrt the earth centered frame, stored in the angle axis
     *         representation where the angle is coded into the axis length
	 */
    xdr_data_t orientation[3];

	/** @brief Linear velocity wrt the earth centered frame measured in
     *         the earth centered frame
	 */
    xdr_data_t linearVel[3];
	
    /** @brief Angular velocity wrt the earth centered frame measured in
     *          the earth centered frame
	 */
    xdr_data_t angularVel[3];

	/** @brief Linear acceleration wrt the earth centered frame measured in
     *         the earth centered frame
	 */
    xdr_data_t linearAccel[3];
	
    /** @brief Angular acceleration wrt the earth centered frame measured in
     *         the earth centered frame
	 */
    xdr_data_t angularAccel[3];
};


/** 
 * @struct T_PropertyMsg 
 *  @brief Property Message 
 */
struct T_PropertyMsg {
    xdr_data_t id;
    xdr_data_t value;
};


/**
 * @struct FGFloatPropertyData  
 * @brief Property Data 
 */
struct FGFloatPropertyData {
  unsigned id;
  float value;
};

/** @brief Position Message */
struct FGExternalMotionData {
	
  /** 
   * @brief Simulation time when this packet was generated 
   */
  double time;
  // the artificial lag the client should stay behind the average
  // simulation time to arrival time diference
  // FIXME: should be some 'per model' instead of 'per packet' property
  double lag;
  
  /** 
   * @brief The artificial lag the client should stay behind the average
   *        simulation time to arrival time diference
   * @todo  should be some 'per model' instead of 'per packet' property  double lag;
   *        Position wrt the earth centered frame  
   */
  SGVec3d position;
  
  /** @brief Orientation wrt the earth centered frame */
  SGQuatf orientation;
  
  /**
   * @brief Linear velocity wrt the earth centered frame measured in
   *        the earth centered frame
   */
  SGVec3f linearVel;
  
  /** 
   * @brief Angular velocity wrt the earth centered frame measured in the earth centered frame
   */
  SGVec3f angularVel;
  
  /** @brief Linear acceleration wrt the earth centered frame measured in the earth centered frame */
  SGVec3f linearAccel;
  
  /** @brief Angular acceleration wrt the earth centered frame measured in the earth centered frame */
  SGVec3f angularAccel;
  
  /** @brief The set of properties recieved for this timeslot */
  std::vector<FGFloatPropertyData> properties;
};

#endif
