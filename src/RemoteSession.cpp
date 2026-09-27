#include "RemoteSession.h"
#include "DisplayGroupManager.h"
#include "MainWindow.h"
#include "QSSApp.h"
#include "main.h"
#include "log.h"

#include <random>

namespace
{
    std::string randomHex(int bytes)
    {
        static std::mutex mutex;
        static std::random_device device;
        static std::mt19937_64 generator(((uint64_t)device() << 32) ^ device());

        std::lock_guard<std::mutex> lock(mutex);

        static const char digits[] = "0123456789abcdef";
        std::string s;

        for(int i=0; i<bytes; i++)
        {
            unsigned int b = generator() & 0xff;
            s += digits[b >> 4];
            s += digits[b & 0xf];
        }

        return s;
    }

    QSSApplication * app()
    {
        return (QSSApplication *)g_app;
    }
}

RemoteSession::RemoteSession(WallController * controller) : controller_(controller)
{
    // lease changes can come from the HTTP threads; act on them on this one
    connect(this, SIGNAL(leaseChanged()), this, SLOT(applyLeaseChange()), Qt::QueuedConnection);
    connect(this, SIGNAL(leaseChanged()), this, SLOT(markDirty()), Qt::QueuedConnection);

    connect(&publishTimer_, SIGNAL(timeout()), this, SLOT(publishIfDirty()));
    publishTimer_.start(100);

    connect(&expiryTimer_, SIGNAL(timeout()), this, SLOT(expireStaleLease()));
    expiryTimer_.start(2000);
}

void RemoteSession::attach()
{
    connect(g_displayGroupManager.get(), SIGNAL(displayGroupChanged()), this, SLOT(markDirty()));
    connect(g_displayGroupManager->getOptions().get(), SIGNAL(updated()), this, SLOT(markDirty()));
    connect(g_mainWindow, SIGNAL(constrainAspectRatioChanged(bool)), this, SLOT(markDirty()));
    connect(app(), SIGNAL(asleepChanged(bool)), this, SLOT(markDirty()));
    connect(g_mainWindow, SIGNAL(takeControlRequested()), this, SLOT(revoke()));
}

bool RemoteSession::acquire(std::string label, bool force, json & lease, json & holder)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if(!secret_.empty() && !force)
        {
            holder = { { "id", id_ }, { "label", label_ } };
            return false;
        }

        secret_ = randomHex(16);
        id_ = randomHex(4);
        label_ = label.empty() ? "unnamed client" : label;
        lastSeen_ = std::chrono::steady_clock::now();

        lease = { { "lease", secret_ }, { "id", id_ }, { "label", label_ }, { "timeout", LEASE_TIMEOUT_SECONDS } };
    }

    put_flog(LOG_INFO, "remote control taken by %s", lease["label"].get<std::string>().c_str());

    emit leaseChanged();
    return true;
}

bool RemoteSession::release(std::string secret)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if(secret.empty() || secret != secret_)
        {
            return false;
        }

        secret_.clear();
    }

    put_flog(LOG_INFO, "remote control released");

    emit leaseChanged();
    return true;
}

bool RemoteSession::renew(std::string secret)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if(secret.empty() || secret != secret_)
    {
        return false;
    }

    lastSeen_ = std::chrono::steady_clock::now();
    return true;
}

void RemoteSession::revoke()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if(secret_.empty())
        {
            return;
        }

        secret_.clear();
    }

    put_flog(LOG_INFO, "remote control taken back at the control window");

    emit leaseChanged();
}

void RemoteSession::expireStaleLease()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if(secret_.empty() || std::chrono::steady_clock::now() - lastSeen_ < std::chrono::seconds(LEASE_TIMEOUT_SECONDS))
        {
            return;
        }

        secret_.clear();
    }

    put_flog(LOG_INFO, "remote control lapsed");

    emit leaseChanged();
}

json RemoteSession::holder()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if(secret_.empty())
    {
        return nullptr;
    }

    return { { "id", id_ }, { "label", label_ } };
}

void RemoteSession::applyLeaseChange()
{
    json h = holder();
    bool held = !h.is_null();

    // the controller runs its own idle timer, and the wall's restarts from zero
    // when control comes back; and someone at the control window has to take
    // control before their input counts - waking the wall included
    app()->setIdleTimerEnabled(!held);
    app()->setLocalInputWakes(!held);

    if(g_mainWindow != NULL)
    {
        g_mainWindow->setRemoteController(held ? QString::fromStdString(h["label"]) : QString());
    }
}

json RemoteSession::status()
{
    return {
        { "asleep", app()->isAsleep() },
        { "idleTimeout", app()->idleTimeoutSeconds() },
        { "controller", holder() }
    };
}

void RemoteSession::markDirty()
{
    dirty_ = true;
}

void RemoteSession::publishIfDirty()
{
    if(!dirty_)
    {
        return;
    }

    dirty_ = false;

    std::string snapshot = json({
        { "status", status() },
        { "windows", controller_->listWindows().body },
        { "options", controller_->getOptions().body }
    }).dump();

    std::lock_guard<std::mutex> lock(mutex_);

    if(snapshot != snapshot_)
    {
        snapshot_ = snapshot;
        version_++;
        snapshotChanged_.notify_all();
    }
}

bool RemoteSession::waitForSnapshot(uint64_t & version, std::string & snapshot, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(mutex_);

    snapshotChanged_.wait_for(lock, timeout, [&]() { return shutDown_ || (version_ > version && !snapshot_.empty()); });

    if(shutDown_ || version_ <= version || snapshot_.empty())
    {
        return false;
    }

    version = version_;
    snapshot = snapshot_;
    return true;
}

void RemoteSession::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);

    shutDown_ = true;
    snapshotChanged_.notify_all();
}

bool RemoteSession::isShutDown()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return shutDown_;
}
