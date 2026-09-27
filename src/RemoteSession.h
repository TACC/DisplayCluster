#ifndef REMOTE_SESSION_H
#define REMOTE_SESSION_H

#include "WallController.h"
#include <QObject>
#include <QTimer>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>

// Who's controlling the wall remotely, and the stream of changes remote
// clients watch. Lives on the GUI thread, but its lease and event methods are
// safe to call from the HTTP threads.
//
// Control lease: only one client at a time may change the wall. A client takes
// control with acquire(), getting a secret (sent with each change) and a public
// id (which everyone sees, so a client can tell whether it's the holder). The
// lease lapses if the holder goes quiet for LEASE_TIMEOUT - any request carrying
// the secret, or an open event stream that passed it, keeps it alive. While a
// lease is held the control window is read-only and the wall's idle timer is
// paused (the controller runs its own); when it ends, both come back.
//
// Events: whenever the wall changes, a snapshot of it - status, windows,
// options - is published at most ten times a second, and only if it differs
// from the last one. So the screensaver's animation, which doesn't change the
// layout remote clients see (see WallController::listWindows()), sends nothing.
class RemoteSession : public QObject
{
    Q_OBJECT

    public:

        static const int LEASE_TIMEOUT_SECONDS = 30;

        RemoteSession(WallController * controller);

        // hooks up to the display group, options, screensaver and control
        // window, which must all exist by now
        void attach();

        // false (and the holder in `holder`) if someone else holds control
        // and force isn't set
        bool acquire(std::string label, bool force, json & lease, json & holder);
        bool release(std::string secret);

        // true if secret is the current lease, which it also renews
        bool renew(std::string secret);

        // the current holder as {id, label}, or null
        json holder();

        // {asleep, idleTimeout, controller}
        json status();

        // waits up to timeout for a snapshot newer than `version`; true with
        // the new one (and version updated), false on timeout or shutdown
        bool waitForSnapshot(uint64_t & version, std::string & snapshot, std::chrono::milliseconds timeout);

        // wakes up and ends every waitForSnapshot(), for shutdown
        void shutdown();
        bool isShutDown();

    public slots:

        // the control window's Take control
        void revoke();

    private slots:

        void markDirty();
        void publishIfDirty();
        void expireStaleLease();
        void applyLeaseChange();

    signals:

        void leaseChanged();

    private:

        WallController * controller_;

        std::mutex mutex_;
        std::condition_variable snapshotChanged_;

        std::string secret_;
        std::string id_;
        std::string label_;
        std::chrono::steady_clock::time_point lastSeen_;

        uint64_t version_ = 0;
        std::string snapshot_;
        bool shutDown_ = false;

        bool dirty_ = true;
        QTimer publishTimer_;
        QTimer expiryTimer_;
};

#endif
