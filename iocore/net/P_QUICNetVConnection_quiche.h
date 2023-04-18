/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

/****************************************************************************

  QUICNetVConnection_quiche.h

  This file implements an I/O Processor for network I/O.


 ****************************************************************************/
#pragma once

#include "tscore/ink_platform.h"
#include "P_Net.h"
#include "P_EventSystem.h"
#include "P_UnixNetVConnection.h"
#include "P_UnixNet.h"
#include "P_UDPNet.h"
#include "P_ALPNSupport.h"
#include "TLSBasicSupport.h"
#include "TLSSessionResumptionSupport.h"
#include "TLSSNISupport.h"
#include "TLSCertSwitchSupport.h"
#include "tscore/ink_apidefs.h"
#include "tscore/List.h"

#include "quic/QUICConnection.h"
#include "quic/QUICConnectionTable.h"
#include "quic/QUICContext.h"
#include "quic/QUICStreamManager.h"
#include "quic/QUICStreamManager_quiche.h"
#include <quiche.h>

class QUICPacketHandler;
class QUICResetTokenTable;
class QUICConnectionTable;

class QUICNetVConnection : public UnixNetVConnection,
                           public QUICConnection,
                           public RefCountObj,
                           public ALPNSupport,
                           public TLSSNISupport,
                           public TLSSessionResumptionSupport,
                           public TLSCertSwitchSupport,
                           public TLSBasicSupport
{
  using super = UnixNetVConnection; ///< Parent type.

public:
  QUICNetVConnection();
  ~QUICNetVConnection();
  void init(QUICVersion version, QUICConnectionId peer_cid, QUICConnectionId original_cid, UDPConnection *, QUICPacketHandler *);
  void init(QUICVersion version, QUICConnectionId peer_cid, QUICConnectionId original_cid, QUICConnectionId first_cid,
            QUICConnectionId retry_cid, UDPConnection *, quiche_conn *, QUICPacketHandler *, QUICConnectionTable *ctable, SSL *);

  // Event handlers
  int acceptEvent(int event, Event *e);
  int state_handshake(int event, Event *e);
  int state_established(int event, Event *e);

  // RefCountObj
  void free() override;

  // NetVConnection
  void set_local_addr() override;

  // NetEvent
  void free(EThread *t) override;

  // UnixNetVConnection
  void reenable(VIO *vio) override;
  VIO *do_io_read(Continuation *c, int64_t nbytes, MIOBuffer *buf) override;
  VIO *do_io_write(Continuation *c, int64_t nbytes, IOBufferReader *buf, bool owner = false) override;
  int connectUp(EThread *t, int fd) override;
  int64_t load_buffer_and_write(int64_t towrite, MIOBufferAccessor &buf, int64_t &total_written, int &needs) override;
  bool getSSLHandShakeComplete() const override;

  // NetEvent
  virtual void net_read_io(NetHandler *nh, EThread *lthread) override;

  // NetVConnection
  int populate_protocol(std::string_view *results, int n) const override;
  const char *protocol_contains(std::string_view tag) const override;
  const char *get_server_name() const override;
  bool support_sni() const override;

  // QUICConnection
  QUICStreamManager *stream_manager() override;
  void close_quic_connection(QUICConnectionErrorUPtr error) override;
  void reset_quic_connection() override;
  void handle_received_packet(UDPPacket *packet) override;
  void ping() override;

  // QUICConnection (QUICConnectionInfoProvider)
  QUICConnectionId peer_connection_id() const override;
  QUICConnectionId original_connection_id() const override;
  QUICConnectionId first_connection_id() const override;
  QUICConnectionId retry_source_connection_id() const override;
  QUICConnectionId initial_source_connection_id() const override;
  QUICConnectionId connection_id() const override;
  std::string_view cids() const override;
  const QUICFiveTuple five_tuple() const override;
  uint32_t pmtu() const override;
  NetVConnectionContext_t direction() const override;
  QUICVersion negotiated_version() const override;
  std::string_view negotiated_application_name() const override;
  bool is_closed() const override;
  bool is_at_anti_amplification_limit() const override;
  bool is_address_validation_completed() const override;
  bool is_handshake_completed() const override;
  bool has_keys_for(QUICPacketNumberSpace space) const override;

  // QUICConnection (QUICFrameHandler)
  std::vector<QUICFrameType> interests() override;
  QUICConnectionErrorUPtr handle_frame(QUICEncryptionLevel level, const QUICFrame &frame) override;

  // QUICNetVConnection
  int in_closed_queue = 0;

  bool shouldDestroy();
  void destroy(EThread *t);
  void remove_connection_ids();

  LINK(QUICNetVConnection, closed_link);
  SLINK(QUICNetVConnection, closed_alink);

protected:
  std::unique_ptr<QUICContext> _context;
  QUICPacketHandler *_packet_handler = nullptr;

  // TLSBasicSupport
  SSL *_get_ssl_object() const override;
  ssl_curve_id _get_tls_curve() const override;

  // TLSSNISupport
  void _fire_ssl_servername_event() override;

  // TLSSessionResumptionSupport
  const IpEndpoint &_getLocalEndpoint() override;

  // TLSCertSwitchSupport
  bool _isTryingRenegotiation() const override;
  shared_SSL_CTX _lookupContextByName(const std::string &servername, SSLCertContextType ctxType) override;
  shared_SSL_CTX _lookupContextByIP() override;

private:
  SSL *_ssl;
  QUICConfig::scoped_config _quic_config;

  QUICConnectionId _peer_quic_connection_id;      // dst cid in local
  QUICConnectionId _peer_old_quic_connection_id;  // dst previous cid in local
  QUICConnectionId _original_quic_connection_id;  // dst cid of initial packet from client
  QUICConnectionId _first_quic_connection_id;     // dst cid of initial packet from client that doesn't have retry token
  QUICConnectionId _retry_source_connection_id;   // src cid used for sending Retry packet
  QUICConnectionId _initial_source_connection_id; // src cid used for Initial packet
  QUICConnectionId _quic_connection_id;           // src cid in local

  UDPConnection *_udp_con      = nullptr;
  quiche_conn *_quiche_con     = nullptr;
  QUICConnectionTable *_ctable = nullptr;

  void _bindSSLObject();
  void _unbindSSLObject();

  void _schedule_packet_write_ready(bool delay = false);
  void _unschedule_packet_write_ready();
  void _close_packet_write_ready(Event *data);
  Event *_packet_write_ready = nullptr;

  void _handle_read_ready();
  void _handle_write_ready();
  void _handle_interval();

  void _switch_to_established_state();

  bool _handshake_completed = false;
  bool _application_started = false;
  void _start_application();

  QUICStreamManagerImpl *_stream_manager = nullptr;
  QUICApplicationMap *_application_map   = nullptr;
};

extern ClassAllocator<QUICNetVConnection> quicNetVCAllocator;
