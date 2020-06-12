/*************************************************************************
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
************************************************************************/
#ifndef _SGX_EA_H_
#define _SGX_EA_H_

#include "sgx_tcrypto.h"
#include "sgx_quote.h"
#include "sgx_tseal.h"

#define SGX_EA_NONCE_SIZE 16
#define AES_128_CMAC_SIZE 16
#define AES_128_GCM_IV_SIZE 16
#define AES_128_GCM_AAD_SIZE 16
#define AES_128_GCM_MAC_SIZE 16

#define SGX_EA_VERSION   1

typedef enum {
    EA_MSG0 = 1,    /**< This is message 0, it's sent from initiator to responder */
    EA_MSG0_RESP,   /**< This is response for message 0, it's sent from reponder, the message would include secure session id for initiator */
    EA_MSG1_REQ,    /**< This is message 1 request, sent from initiator to responder */
    EA_MSG1,		/**< This is message 1 response, sent from responder, the message would include responder_pubkey and collateral */
    EA_MSG2,		/**< This is message 2, sent from initiator to responder, the mssage would inclue initiator_pubkey and collateral */
    EA_MSG3,        /**< This is message 3, sent from responder to initiator, the message would include secure session establishment result */
    EA_MSG_SEC,     /**< This is secure message type */
    EA_MSG_GET_MK,  /**< This is for debug purpose to print session key */
    EA_MSG_CLOSE,
    EA_MSG_UNKNOWN
} sgx_ea_msg_type_t;

typedef enum {
    SGX_EA_ROLE_INITIATOR = 1,
    SGX_EA_ROLE_RESPONDER = 2
} sgx_ea_role_t;

// This enum defines session status
typedef enum {
    SGX_EA_SESSION_UNUSED = 1,   /**< This is session's initial status */
    SGX_EA_SESSION_INITED,      
    SGX_EA_SESSION_WAIT_FOR_MSG1,
    SGX_EA_SESSION_WAIT_FOR_MSG2,
    SGX_EA_SESSION_WAIT_FOR_MSG3,
    SGX_EA_SESSION_ESTABLISHED,
    SGX_EA_SESSION_UNEXPECTED,    
} sgx_ea_session_status_t;

#pragma pack(push,1)
typedef uint32_t sgx_ea_session_id_t;
#define SGX_EA_SESSION_INVALID_ID (sgx_ea_session_id_t)(-1)

/** 
 * This is nonce to be used in remote attestation. 
 * The remote attestation flow starts with Responder/Initiator sending nonce info to peer.
 **/
typedef struct sgx_ea_nonce {
    uint8_t data[SGX_EA_NONCE_SIZE];
} sgx_ea_nonce_t;

typedef struct aes_128_cmac {
    uint8_t data[AES_128_CMAC_SIZE];
} aes_128_cmac_t;

/**
 * This is message header for all types of message.
 **/ 
typedef struct sgx_ea_msg_header {
    uint8_t version;
    uint8_t type;
    uint32_t size; /**< This size doesn't input message header size */
} sgx_ea_msg_header_t;

/**
 * This is message 0 request sent from initiator to responder
 **/ 
typedef struct sgx_uea_msg0 {
    sgx_ea_msg_header_t header;
} sgx_uea_msg0_t;

/**
 * This is message 0  response sent from responder to initiator
 */ 
typedef struct sgx_uea_msg0_resp {
    sgx_ea_msg_header_t header;
    sgx_ea_session_id_t sessionid;  /**< This is session id allocated by responder for the initiator */
} sgx_uea_msg0_resp_t;

/**
 * This is message 1 request sent from initiator to responder
 */ 
typedef struct sgx_uea_msg1_req {
    sgx_ea_msg_header_t header;
    sgx_ea_session_id_t sessionid; /**< This is session id allocated by responder */
    sgx_ea_nonce_t nonce; /**< This is nonce generated by initiator (in trusted part), it's used to attest responder's identity */
} sgx_uea_msg1_req_t;;

/**
 * This is message 1 content for trusted part. It's generated by responder. 
 **/ 
typedef struct sgx_tea_msg1_content {
    sgx_ea_nonce_t nonce; 	/**< this nonce is generated in initiator enclave, responder copies it into the nonce here */
    sgx_ec256_public_t pubkey; /**< this is responder's EC public key */
    sgx_report_t report; /**< This is responder enclave's report, the report.data is SHA256(nonce || pubkey) */
} sgx_tea_msg1_content_t;

/**
 * This is message 1 generated by responder
 **/ 
typedef struct sgx_uea_msg1 {
    sgx_ea_msg_header_t header;   
    sgx_tea_msg1_content_t msgbody;  /**< this is message 1 content */
    uint32_t quote_size;	/**< this is quote size */
    uint8_t quote[0];	    /**< this quote is for initiator to authenticate responder enclave's identity, using nonce sent from initiator */
} sgx_uea_msg1_t;

/**
 * This is message 2 content generated by initiator
 **/ 
typedef struct sgx_tea_msg2_content {
    sgx_ec256_public_t pubkey;   /**< This is initiator enclave's public key */
    sgx_report_t report; /**< This is initiator enclave's report. untrusted part would generate quote with it.
                          * report.data is SHA256(initiator.pubkey || responder.pubkey)
                          * */    
} sgx_tea_msg2_content_t;

/**
 * This is message 2 generated by initiator
 **/ 
typedef struct sgx_uea_msg2 {    
    sgx_ea_msg_header_t header;
    sgx_ea_session_id_t sessionid; /**< This is session id */
    sgx_tea_msg2_content_t msgbody; /**< This is message 2 content generated in initiator enclave */
    uint32_t quote_size; /**< This is quote size */
    uint8_t quote[0];   /**< This is quote */
} sgx_uea_msg2_t;

/**
 * This is message 3 content generated in responder
 **/ 
typedef struct sgx_tea_msg3_content {
    int result;     /**< This is secure session establishment result */
    aes_128_cmac_t mac; /**< mac is AES_128_CMAC(mk, result || initiator_pubkey || responder_pubkey)*/
} sgx_tea_msg3_content_t;

/**
 * This is message 3 generated by responder
 **/ 
typedef struct sgx_uea_msg3 {
    sgx_ea_msg_header_t header;
    sgx_tea_msg3_content_t msgbody;
} sgx_uea_msg3_t;

/**
 * This is for debug purpose, to notify responder to print session key. 
 * You need to build the project with DEBUG macro defined. You also need to modify enclave.edl to explicitly add ECALL interface declration to the EDL.
 **/ 
typedef struct sgx_uea_get_mk {
    sgx_ea_msg_header_t header;
    sgx_ea_session_id_t sessionid;
} sgx_uea_get_mk_t;

/**
 * This is encrypted message generated in enclave.
 **/ 
typedef struct sgx_tea_sec_msg {    
    sgx_aes_gcm_data_t aes_gcm_data;
} sgx_tea_sec_msg_t;

/**
 * This is encrypted message.
 **/ 
typedef struct sgx_ea_message_sec {
    sgx_ea_msg_header_t header;
    sgx_ea_session_id_t sessionid; /**< This is session id */
    sgx_tea_sec_msg_t sec_msg; /**< This is encrypted message */
} sgx_ea_msg_sec_t;

/**
 * This is message request to close secure session. 
 **/ 
typedef struct sgx_ea_message_close {
    sgx_ea_msg_header_t header;
    sgx_ea_session_id_t sessionid;  /**< This is session id */
} sgx_ea_msg_close_t;

typedef struct sgx_ea_message {
    uint32_t size;
    uint8_t msgbody[0];
} sgx_ea_raw_message_t; 

struct EARawMsg {
    uint32_t size;
    uint8_t * msgbody;

#ifdef __cplusplus
public:
    EARawMsg(uint32_t, uint8_t *);
    ~EARawMsg();
    EARawMsg(const EARawMsg &);
    EARawMsg& operator=(const EARawMsg &);
#endif 
};
#pragma pack(pop)

#ifdef __cplusplus
/**
 * This function initiates message header. 
 * Note: it only specifies message type according to input parameter. Caller needs to set header size with real message size.
 **/ 
inline void sgx_ea_init_msg_header(uint8_t msgtype, sgx_ea_msg_header_t *header)
{
    if (!header)
        return;

    if ((msgtype >= EA_MSG0) && (msgtype < EA_MSG_UNKNOWN)) {
        header->version = SGX_EA_VERSION;
        header->type = msgtype;
        header->size = 0;
    }
}
#endif

#endif
