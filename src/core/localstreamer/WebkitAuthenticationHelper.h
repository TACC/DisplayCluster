/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#ifndef WEBKITAUTHENTICATIONHELPER_H
#define WEBKITAUTHENTICATIONHELPER_H

#include <QObject>
#include <QUrl>
class QNetworkReply;
class QAuthenticator;
class QWebView;

/**
 * Handle HTTP authentication requests for a QWebView.
 *
 * The WebkitAuthenticationHelper class intercepts HTTP authentication requests
 * an displays a simple html login page.
 *
 * It offers a replacement to the system dialog box when QWebView is used
 * without a window.
 *
 * @usage Create one WebkitAuthenticationHelper instance for each
 * QWebView that needs to support HTTP authentication.
 */
class WebkitAuthenticationHelper : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param webView The QWebView for which to add authentication handling.
     */
    WebkitAuthenticationHelper(QWebView& webView);

protected slots:
    /**
     * @defgroup Internal Callbacks
     * These slots are accessed internally by the Javascript only but cannot be private,
     * otherwise the compiler ignores them at compilation time.
     *  @{
     */
    void loginFormInputChanged(const QString &inputName, const QString &inputValue);
    void loginFormSubmitted();
    /** @} */ // Internal Callbacks

private slots:
    void handleAuthenticationRequest(QNetworkReply*, QAuthenticator *authenticator);
    void errorPageFinishedLoading(const bool ok);

private:
    void displayLoginPage();
    void registerLoginFormCallbacks();
    void sendCredentials(QAuthenticator *authenticator) const;
    QString readQrcFile(const QString &filename);

    QWebView& webView_;

    bool userHasInputNewCredentials_;
    QString username_;
    QString password_;
};

#endif // WEBKITAUTHENTICATIONHELPER_H
