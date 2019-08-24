// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/renderer_host/electron_render_message_filter.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "chrome/browser/predictors/preconnect_manager.h"
#include "components/network_hints/common/network_hints_common.h"
#include "components/network_hints/common/network_hints_messages.h"
#include "shell/browser/api/atom_api_session.h"
#include "shell/common/native_mate_converters/gurl_converter.h"

using content::BrowserThread;

namespace {

const uint32_t kRenderFilteredMessageClasses[] = {
    NetworkHintsMsgStart,
};

void EmitPreconnect(electron::api::Session* session,
                    const GURL& url,
                    bool allow_credentials) {
  session->Emit("preconnect", url, allow_credentials);
}

}  // namespace

ElectronRenderMessageFilter::ElectronRenderMessageFilter(
    electron::api::Session* session)
    : BrowserMessageFilter(kRenderFilteredMessageClasses,
                           base::size(kRenderFilteredMessageClasses)),
      session_(session) {}

ElectronRenderMessageFilter::~ElectronRenderMessageFilter() {}

bool ElectronRenderMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ElectronRenderMessageFilter, message)
    IPC_MESSAGE_HANDLER(NetworkHintsMsg_Preconnect, OnPreconnect)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void ElectronRenderMessageFilter::OnPreconnect(int render_frame_id,
                                               const GURL& url,
                                               bool allow_credentials,
                                               int count) {
  if (count < 1) {
    LOG(WARNING) << "NetworkHintsMsg_Preconnect IPC with invalid count: "
                 << count;
    return;
  }

  if (!url.is_valid() || !url.has_host() || !url.has_scheme() ||
      !url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  base::PostTask(
      FROM_HERE, {BrowserThread::UI},
      base::BindOnce(&EmitPreconnect, session_, url, allow_credentials));
}

namespace predictors {

PreconnectRequest::PreconnectRequest(
    const GURL& origin,
    int num_sockets,
    const net::NetworkIsolationKey& network_isolation_key)
    : origin(origin), num_sockets(num_sockets) {
  DCHECK_GE(num_sockets, 0);
}

}  // namespace predictors
