/*
 * Copyright 2010, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************************************************/
/*                                                               Date:11/2013 */
/*                                PRESENTATION                                */
/*                                                                            */
/*       Copyright 2013 TCL Communication Technology Holdings Limited.        */
/*                                                                            */
/* This material is company confidential, cannot be reproduced in any form    */
/* without the written permission of TCL Communication Technology Holdings    */
/* Limited.                                                                   */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/*  Author :  Tianjun Xu                                                      */
/*  Email  :  Tianjun.Xu@tcl-mobile.com                                       */
/*  Role   :  external/webkit                                                 */
/*  Reference documents :                                                     */
/* -------------------------------------------------------------------------- */
/*  Comments :                                                                */
/*  File     : Source/Webkit/android/WebSupport/WebRequest.h                  */
/*  Labels   :                                                                */
/* -------------------------------------------------------------------------- */
/* ========================================================================== */
/*     Modifications on Features list / Changes Request / Problems Report     */
/* -------------------------------------------------------------------------- */
/*    date   |        author        |         Key          |     comment      */
/* ----------|----------------------|----------------------|----------------- */
/* 11/13/2013|      Tianjun Xu      |        546141        |Baseline HTTP Us- */
/*           |                      |                      |er Agent String   */
/*           |                      |                      |Device ID         */
/* ----------|----------------------|----------------------|----------------- */
/******************************************************************************/

#ifndef WebRequest_h
#define WebRequest_h

#include "ChromiumIncludes.h"
#include <wtf/Vector.h>

class MessageLoop;

namespace android {

enum LoadState {
    Created,
    Started,
    Response,
    GotData,
    Cancelled,
    Finished,
    Deleted
};

class UrlInterceptResponse;
class WebFrame;
class WebRequestContext;
class WebResourceRequest;
class WebUrlLoaderClient;

// All methods in this class must be called on the io thread
class WebRequest : public net::URLRequest::Delegate, public base::RefCountedThreadSafe<WebRequest> {
public:
    WebRequest(WebUrlLoaderClient*, const WebResourceRequest&);

    // If this is an android specific url or the application wants to load
    // custom data, we load the data through an input stream.
    // Used for:
    // - file:///android_asset
    // - file:///android_res
    // - content://
    WebRequest(WebUrlLoaderClient*, const WebResourceRequest&, UrlInterceptResponse* intercept);

    // Optional, but if used has to be called before start
    void appendBytesToUpload(Vector<char>* data);
    void appendFileToUpload(const std::string& filename);

    void setRequestContext(WebRequestContext* context);
    void start();
    void cancel();
    void pauseLoad(bool pause);

    // From URLRequest::Delegate
    virtual void OnReceivedRedirect(net::URLRequest*, const GURL&, bool* deferRedirect);
    virtual void OnResponseStarted(net::URLRequest*);
    virtual void OnReadCompleted(net::URLRequest*, int bytesRead);
    virtual void OnAuthRequired(net::URLRequest*, net::AuthChallengeInfo*);
    virtual void OnSSLCertificateError(net::URLRequest* request, int cert_error, net::X509Certificate* cert);
    virtual void OnCertificateRequested(net::URLRequest* request, net::SSLCertRequestInfo* cert_request_info);

    // Methods called during a request by the UI code (via WebUrlLoaderClient).
    void setAuth(const string16& username, const string16& password);
    void cancelAuth();
    void followDeferredRedirect();
    void proceedSslCertError();
    void cancelSslCertError(int cert_error);
    void sslClientCert(EVP_PKEY* pkey, scoped_refptr<net::X509Certificate> chain);

    const std::string& getUrl() const;
    const std::string& getUserAgent() const;
    const std::string& getReferer() const;

    //[FEATURE]-Add-BEGIN by TCTNB.Tianjun Xu,11/14/2013,546141,
    //Baseline HTTP User Agent String Device ID
#ifdef FEATURE_ADD_HTTP_HEADER
    std::string GetAttDeviceID();
#endif
    //[FEATURE]-Add-END by TCTNB.Tianjun Xu

    void setSync(bool sync) { m_isSync = sync; }
private:
    void startReading();
    bool read(int* bytesRead);

    friend class base::RefCountedThreadSafe<WebRequest>;
    virtual ~WebRequest();
    void handleDataURL(GURL);
    void handleBrowserURL(GURL);
    void handleInterceptedURL();
    void finish(bool success);
    void updateLoadFlags(int& loadFlags);

    scoped_refptr<WebUrlLoaderClient> m_urlLoader;
    OwnPtr<net::URLRequest> m_request;
    scoped_refptr<net::IOBuffer> m_networkBuffer;
    scoped_ptr<UrlInterceptResponse> m_interceptResponse;
    std::string m_url;
    std::string m_userAgent;
    std::string m_referer;
    LoadState m_loadState;
    int m_authRequestCount;
    int m_cacheMode;
    ScopedRunnableMethodFactory<WebRequest> m_runnableFactory;
    bool m_wantToPause;
    bool m_isPaused;
    bool m_isSync;
#ifdef LOG_REQUESTS
    time_t m_startTime;
#endif
};

} // namespace android

#endif
