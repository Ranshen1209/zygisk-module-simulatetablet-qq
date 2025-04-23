#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include "zygisk.hpp"
#include "Dobby/include/dobby.h"

#define LOG_TAG "SimulateTabletQQ"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#define QQ_PACKAGE_NAME "com.tencent.mobileqq"
#define QQ_TARGET_BRAND "Xiaomi"
#define QQ_TARGET_MODEL "23046RP50C"
#define QQ_TARGET_MANUFACTURER "Xiaomi"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;
using zygisk::Option;

static char* extractPackageNameFromDataDir(const char* dataDir) {
    if (!dataDir) return nullptr;

    const char* lastSlash = strrchr(dataDir, '/');
    if (!lastSlash) {
        LOGW("extractPackageNameFromDataDir: No '/' found in dataDir '%s'", dataDir);
        return nullptr;
    }

    size_t packageNameLen = strlen(lastSlash + 1);
    if (packageNameLen == 0) {
        LOGW("extractPackageNameFromDataDir: dataDir '%s' ends with '/'", dataDir);
        return nullptr;
    }

    char* packageName = (char*)malloc(packageNameLen + 1);
    if (!packageName) {
        LOGE("extractPackageNameFromDataDir: Memory allocation failed");
        return nullptr;
    }

    strcpy(packageName, lastSlash + 1);
    packageName[packageNameLen] = '\0';
    return packageName;
}

typedef int (*t_system_property_get)(const char *name, char *value);
static t_system_property_get orig_system_property_get = nullptr;

int my_system_property_get(const char *name, char *value) {
    if (name != nullptr && strcmp(name, "ro.build.characteristics") == 0) {
        LOGI("Hooked __system_property_get: Returning 'tablet' for 'ro.build.characteristics'");
        strcpy(value, "tablet");
        return strlen(value);
    }

    if (orig_system_property_get) {
        return orig_system_property_get(name, value);
    } else {
        LOGE("Hooked __system_property_get: Original function is null");
        value[0] = '\0';
        return 0;
    }
}

class SimulateQQTabletModule : public zygisk::ModuleBase {
private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
    bool isTargetApp = false;

    void simulateTabletDevice(const char *brand, const char *model, const char *manufacturer) {
        LOGI("Simulating device: Brand=%s, Model=%s, Manufacturer=%s", brand, model, manufacturer);

        jclass buildClass = env->FindClass("android/os/Build");
        if (!buildClass) {
            LOGE("simulateTabletDevice: Failed to find android.os.Build class");
            if (env->ExceptionCheck()) env->ExceptionClear();
            return;
        }

        jfieldID manuField = env->GetStaticFieldID(buildClass, "MANUFACTURER", "Ljava/lang/String;");
        jfieldID brandField = env->GetStaticFieldID(buildClass, "BRAND", "Ljava/lang/String;");
        jfieldID modelField = env->GetStaticFieldID(buildClass, "MODEL", "Ljava/lang/String;");

        if (!manuField || !brandField || !modelField) {
            LOGE("simulateTabletDevice: Failed to get Build class fields");
            env->DeleteLocalRef(buildClass);
            if (env->ExceptionCheck()) env->ExceptionClear();
            return;
        }

        jstring manuJ = env->NewStringUTF(manufacturer);
        jstring brandJ = env->NewStringUTF(brand);
        jstring modelJ = env->NewStringUTF(model);

        if (!manuJ || !brandJ || !modelJ) {
            LOGE("simulateTabletDevice: Failed to create JNI strings");
            if (manuJ) env->DeleteLocalRef(manuJ);
            if (brandJ) env->DeleteLocalRef(brandJ);
            if (modelJ) env->DeleteLocalRef(modelJ);
        } else {
            env->SetStaticObjectField(buildClass, manuField, manuJ);
            env->SetStaticObjectField(buildClass, brandField, brandJ);
            env->SetStaticObjectField(buildClass, modelField, modelJ);
            LOGI("simulateTabletDevice: Build fields set successfully");
            env->DeleteLocalRef(manuJ);
            env->DeleteLocalRef(brandJ);
            env->DeleteLocalRef(modelJ);
        }

        env->DeleteLocalRef(buildClass);
        if (env->ExceptionCheck()) {
            LOGE("simulateTabletDevice: JNI exception occurred");
            env->ExceptionClear();
        }
    }

    void installSystemPropertyHook() {
        LOGI("Installing hook for __system_property_get");

        void *target_addr = dlsym(RTLD_DEFAULT, "__system_property_get");
        if (target_addr) {
            LOGI("Found __system_property_get at address: %p", target_addr);
            int ret = DobbyHook(
                    target_addr,
                    (dobby_dummy_func_t)my_system_property_get,
                    (dobby_dummy_func_t *)&orig_system_property_get
            );
            if (ret == 0) {
                LOGI("DobbyHook for __system_property_get installed successfully");
            } else {
                LOGE("DobbyHook failed with error code: %d", ret);
                orig_system_property_get = nullptr;
            }
        } else {
            LOGE("Failed to find __system_property_get using dlsym");
        }
    }

public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        this->isTargetApp = false;
        LOGI("SimulateQQTablet module loaded, Zygote PID: %d", getpid());
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        this->isTargetApp = false;

        jstring appDataDirJ = args->app_data_dir;
        const char *appDataDirC = nullptr;
        char *packageName = nullptr;

        if (appDataDirJ) {
            appDataDirC = env->GetStringUTFChars(appDataDirJ, nullptr);
            if (appDataDirC) {
                packageName = extractPackageNameFromDataDir(appDataDirC);
                if (packageName) {
                    if (strcmp(packageName, QQ_PACKAGE_NAME) == 0) {
                        LOGI("Pre-specialize: Matched QQ (%s) from app_data_dir", packageName);
                        api->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);
                        this->isTargetApp = true;
                    }
                    free(packageName);
                }
                env->ReleaseStringUTFChars(appDataDirJ, appDataDirC);
            } else {
                LOGE("Pre-specialize: Failed to get app_data_dir string");
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
        }

        if (!this->isTargetApp && args->nice_name) {
            const char *niceNameC = env->GetStringUTFChars(args->nice_name, nullptr);
            if (niceNameC) {
                if (strstr(niceNameC, QQ_PACKAGE_NAME) != nullptr) {
                    LOGI("Pre-specialize: Matched QQ (%s) from nice_name", niceNameC);
                    api->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);
                    this->isTargetApp = true;
                }
                env->ReleaseStringUTFChars(args->nice_name, niceNameC);
            }
        }

        if (!this->isTargetApp) {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (this->isTargetApp) {
            LOGI("Post-specialize: Processing QQ, PID: %d", getpid());
            simulateTabletDevice(QQ_TARGET_BRAND, QQ_TARGET_MODEL, QQ_TARGET_MANUFACTURER);
            installSystemPropertyHook();
            LOGI("Post-specialize: QQ processing completed");
        }
    }
};

REGISTER_ZYGISK_MODULE(SimulateQQTabletModule)