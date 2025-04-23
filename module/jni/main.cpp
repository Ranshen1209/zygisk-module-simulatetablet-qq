#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/system_properties.h>
#include <dlfcn.h>

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
    if (!dataDir) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "extractPackageNameFromDataDir: dataDir is null");
        return nullptr;
    }
    const char* lastSlash = strrchr(dataDir, '/');
    if (!lastSlash) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "extractPackageNameFromDataDir: '/' not found in dataDir '%s'", dataDir);
        return nullptr;
    }
    if (*(lastSlash + 1) == '\0') {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "extractPackageNameFromDataDir: dataDir '%s' ends with '/' or is empty after slash", dataDir);
        return nullptr;
    }

    size_t packageNameLen = strlen(lastSlash + 1);
    char* packageName = (char*)malloc(packageNameLen + 1);
    if (!packageName) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "extractPackageNameFromDataDir: Memory allocation failed");
        return nullptr;
    }
    strcpy(packageName, lastSlash + 1);
    return packageName;
}

static int (*original_system_property_get)(const char* name, char* value) = nullptr;

int hooked_system_property_get(const char* name, char* value) {
    if (name != nullptr && strcmp(name, "ro.build.characteristics") == 0) {
        strncpy(value, "tablet", PROP_VALUE_MAX - 1);
        value[PROP_VALUE_MAX - 1] = '\0';
        LOGI("Hooked __system_property_get: name='%s'. Returning 'tablet'.", name);
        return strlen("tablet");
    }

    if (original_system_property_get) {
        return original_system_property_get(name, value);
    } else {
        LOGE("Hooked __system_property_get: Original function pointer is null! Cannot call original for '%s'.", name ? name : "NULL");
        value[0] = '\0';
        return 0;
    }
}

class SimulateQQTabletModule : public zygisk::ModuleBase {
private:
    Api* api = nullptr;
    JNIEnv* env = nullptr;
    bool isTargetQQ = false;

    void simulateTabletDevice(const char *brand_c, const char *model_c, const char *manufacturer_c) {
        if (!env) {
            LOGE("simulateTabletDevice: JNIEnv is null!");
            return;
        }
        LOGI("Simulating device: Brand=%s, Model=%s, Manufacturer=%s", brand_c, model_c, manufacturer_c);

        jclass buildClass = env->FindClass("android/os/Build");
        if (!buildClass) {
            LOGE("simulateTabletDevice: android/os/Build class not found");
            if (env->ExceptionCheck()) env->ExceptionClear();
            return;
        }

        jfieldID manuField = env->GetStaticFieldID(buildClass, "MANUFACTURER", "Ljava/lang/String;");
        jfieldID brandField = env->GetStaticFieldID(buildClass, "BRAND", "Ljava/lang/String;");
        jfieldID modelField = env->GetStaticFieldID(buildClass, "MODEL", "Ljava/lang/String;");

        if (!manuField || !brandField || !modelField) {
            LOGE("simulateTabletDevice: Failed to get static String fields in android/os/Build");
            env->DeleteLocalRef(buildClass);
            if (env->ExceptionCheck()) env->ExceptionClear();
            return;
        }

        jstring manuJ = env->NewStringUTF(manufacturer_c);
        jstring brandJ = env->NewStringUTF(brand_c);
        jstring modelJ = env->NewStringUTF(model_c);

        if (!manuJ || !brandJ || !modelJ) {
            LOGE("simulateTabletDevice: Failed to create JNI strings for model info");
            if (manuJ) env->DeleteLocalRef(manuJ);
            if (brandJ) env->DeleteLocalRef(brandJ);
            if (modelJ) env->DeleteLocalRef(modelJ);
        } else {
            env->SetStaticObjectField(buildClass, manuField, manuJ);
            if (env->ExceptionCheck()) { LOGE("simulateTabletDevice: Exception setting MANUFACTURER"); env->ExceptionClear(); }

            env->SetStaticObjectField(buildClass, brandField, brandJ);
            if (env->ExceptionCheck()) { LOGE("simulateTabletDevice: Exception setting BRAND"); env->ExceptionClear(); }

            env->SetStaticObjectField(buildClass, modelField, modelJ);
            if (env->ExceptionCheck()) { LOGE("simulateTabletDevice: Exception setting MODEL"); env->ExceptionClear(); }

            env->DeleteLocalRef(manuJ);
            env->DeleteLocalRef(brandJ);
            env->DeleteLocalRef(modelJ);
            LOGI("simulateTabletDevice: Build fields modification attempt finished.");
        }

        env->DeleteLocalRef(buildClass);
        if (env->ExceptionCheck()) { LOGE("simulateTabletDevice: Latent exception detected after operations."); env->ExceptionClear(); }
    }

    void applyDobbyHooks() {
        LOGI("Applying Dobby hooks for QQ process...");

        void *target_sym = DobbySymbolResolver(nullptr, "__system_property_get");

        if (target_sym) {
            LOGI("Found __system_property_get at address: %p", target_sym);

            int dobby_status = DobbyHook(
                    target_sym,
                    (void *)hooked_system_property_get,
                    (void **)&original_system_property_get
            );

            if (dobby_status == 0) {
                LOGI("Successfully hooked __system_property_get using Dobby.");
            } else {
                LOGE("Failed to hook __system_property_get using Dobby. Error code: %d", dobby_status);
                original_system_property_get = nullptr;
            }
        } else {
            LOGE("Failed to find the symbol '__system_property_get'. Hook cannot be applied.");
        }
    }

public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        LOGI("SimulateQQTabletModule loaded. PID: %d", getpid());
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        isTargetQQ = false;
        jstring appDataDirJ = args->app_data_dir;
        const char *appDataDirC = nullptr;
        char *packageName = nullptr;

        if (appDataDirJ != nullptr) {
            appDataDirC = env->GetStringUTFChars(appDataDirJ, nullptr);
            if (appDataDirC != nullptr) {
                packageName = extractPackageNameFromDataDir(appDataDirC);
                if (packageName != nullptr) {
                    if (strcmp(packageName, QQ_PACKAGE_NAME) == 0) {
                        LOGI("Pre-specialize: Target package QQ (%s) detected. Enabling FORCE_DENYLIST_UNMOUNT.", packageName);
                        isTargetQQ = true;
                        api->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);
                    }
                    free(packageName);
                    packageName = nullptr;
                }
                env->ReleaseStringUTFChars(appDataDirJ, appDataDirC);
            } else {
                LOGE("Pre-specialize: GetStringUTFChars failed for app_data_dir.");
                if(env->ExceptionCheck()) env->ExceptionClear();
            }
        } else {
            const char* niceNameC = nullptr;
            jstring niceNameJ = args->nice_name;
            if(niceNameJ) {
                niceNameC = env->GetStringUTFChars(niceNameJ, nullptr);
            }
            if (niceNameC) {
                if (strcmp(niceNameC, QQ_PACKAGE_NAME) == 0) {
                    LOGW("Pre-specialize: Matched QQ (%s) via nice_name (fallback). Enabling FORCE_DENYLIST_UNMOUNT.", niceNameC);
                    isTargetQQ = true;
                    api->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);
                }
                env->ReleaseStringUTFChars(niceNameJ, niceNameC);
            } else {
                LOGW("Pre-specialize: app_data_dir and nice_name are null or failed to read. Cannot reliably determine package.");
            }
        }

        if (!isTargetQQ) {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (isTargetQQ) {
            LOGI("Post-specialize: Executing hooks for QQ (PID: %d)", getpid());
            if (!this->env) {
                LOGE("Post-specialize: JNIEnv is null! Cannot apply hooks.");
                return;
            }
            simulateTabletDevice(QQ_TARGET_BRAND, QQ_TARGET_MODEL, QQ_TARGET_MANUFACTURER);
            applyDobbyHooks();
            LOGI("Post-specialize: QQ hooks applied.");
        }
    }
};

REGISTER_ZYGISK_MODULE(SimulateQQTabletModule)