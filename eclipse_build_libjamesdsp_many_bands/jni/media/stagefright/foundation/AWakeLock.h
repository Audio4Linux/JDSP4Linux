/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef A_WAKELOCK_H_
#define A_WAKELOCK_H_

#include <media/stagefright/foundation/ABase.h>
#include <powermanager/IPowerManager.h>
#include <utils/RefBase.h>

namespace android {

class AWakeLock : public RefBase {

public:
    AWakeLock();

    // NOTE: acquire and release are not thread safe

    // returns true if wakelock was acquired
    bool acquire();
    void release(bool force = false);

    virtual ~AWakeLock();

private:
    sp<IPowerManager> mPowerManager;
    sp<IBinder>       mWakeLockToken;
    uint32_t          mWakeLockCount;

    class PMDeathRecipient : public IBinder::DeathRecipient {
    public:
        PMDeathRecipient(AWakeLock *wakeLock) : mWakeLock(wakeLock) {}
        virtual ~PMDeathRecipient() {}

        // IBinder::DeathRecipient
        virtual void binderDied(const wp<IBinder> &who);

    private:
        PMDeathRecipient(const PMDeathRecipient&);
        PMDeathRecipient& operator= (const PMDeathRecipient&);

        AWakeLock *mWakeLock;
    };

    const sp<PMDeathRecipient> mDeathRecipient;

    void clearPowerManager();

    DISALLOW_EVIL_CONSTRUCTORS(AWakeLock);
};

}  // namespace android

#endif  // A_WAKELOCK_H_
