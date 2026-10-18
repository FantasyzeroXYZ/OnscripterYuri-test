/* -*- C++ -*-
 * 
 *  onscripter_main.cpp -- main function of ONScripter
 *
 *  Copyright (c) 2001-2018 Ogapee. All rights reserved.
 *            (c) 2014-2019 jh10001 <jh10001@live.cn>
 *            (c) 2022-2023 yurisizuku <https://github.com/YuriSizuku>
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ONScripter.h"
#include "ONSInput.h"
#include "Utils.h"
#include "gbk2utf16.h"
#include "sjis2utf16.h"
#include "version.h"
#include "stdlib.h"

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>

ONScripter ons;
Coding2UTF16 *coding2utf16 = NULL;
std::string g_stdoutpath = "stdout.txt";
std::string g_stderrpath = "stderr.txt";

#if defined(IOS)
#import <Foundation/NSArray.h>
#import <UIKit/UIKit.h>
#import "DataCopier.h"
#import "DataDownloader.h"
#import "ScriptSelector.h"
#import "MoviePlayer.h"
#endif

#if defined(ANDROID)
#include <unistd.h>
#endif

#if defined(WINRT)
#include "ScriptSelector.h"
#endif

void optionHelp()
{
    printf( "Usage: onsyuri [option ...]\n" );
    printf( "  -h, --help\t\tshow this help and exit\n");
    printf( "  -v, --version\t\tshow the version information and exit\n\n");
    
    printf( " load options: \n");
    printf( "  -f, --font file\tset a TTF font file\n");
    printf( "  -r, --root path\tset the root path to the archives\n");
    printf( "      --save-dir\tset save dir\n");
    printf( "      --debug:1\t\tprint debug info\n");
    printf( "      --enc:[gbk|sjis|utf8]\tdefine the encoding of script\n\n");

    printf( " render options: \n");
    printf( "      --window\t\tstart in windowed mode\n");
    printf( "      --width 1280\tforce window width\n");
    printf( "      --height 720\tforce window height\n");
    printf( "      --fullscreen\tstart in fullscreen mode (alt+f or alt+enter)\n");
    printf( "      --fullscreen2\tstart in fullscreen mode with stretch (f10 toggle stretch)\n");
    printf( "      --sharpness 3.1 \t use gles to make image sharp\n");
    printf( "      --no-video\tdo not decode video\n");
    printf( "      --no-vsync\tturn off vsync\n\n");
    
    printf( " other options: \n");
    printf( "      --cdaudio\t\tuse CD audio if available\n");
    printf( "      --cdnumber no\tchoose the CD-ROM drive number\n");
    printf( "      --registry file\tset a registry file\n");
    printf( "      --dll file\tset a dll file\n");
    printf( "      --enable-wheeldown-advance\tadvance the text on mouse wheel down\n");
    printf( "      --disable-rescale\tdo not rescale the images in the archives\n");
    printf( "      --force-button-shortcut\tignore useescspc and getenter command\n");
    printf( "      --render-font-outline\trender the outline of a text instead of casting a shadow\n");
    printf( "      --edit\t\tenable online modification of the volume and variables when 'z' is pressed\n");
    printf( "      --key-exe file\tset a file (*.EXE) that includes a key table\n");
    printf( "      --fontcache\tcache default font\n");
    exit(0);
}

void optionVersion()
{
    printf("Written by Ogapee <ogapee@aqua.dti2.ne.jp>\n\n");
    printf("Copyright (c) 2001-2018 Ogapee.\n\
                (c) 2014-2018 jh10001<jh10001@live.cn>\n\
                (c) 2022-2023 yurisizuku <https://github.com/YuriSizuku>\n");
    printf("This is free software; see the source for copying conditions.\n");
    exit(0);
}

#if defined(ANDROID)
extern "C"
{
#include <jni.h>
#include <android/log.h>
#include <errno.h>
#include <stdint.h>
static JavaVM *jniVM = NULL;
static jobject JavaONScripter = NULL;
static jmethodID JavaPlayVideo = NULL;
static jmethodID JavaGetFD = NULL;
static jmethodID JavaMkdir = NULL;
static jmethodID JavaBridgeEvent = NULL; // [ONS-BRIDGE] extraction event upcall
static jmethodID JavaShowInput = NULL;   // [ONS-INPUT] showInputDialog(String,String,int)

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    jniVM = vm;
    return JNI_VERSION_1_2;
};

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved)
{
    jniVM = vm;
};

#ifndef SDL_JAVA_PACKAGE_PATH
#error You have to define SDL_JAVA_PACKAGE_PATH to your package path with dots replaced with underscores, for example "com_example_SanAngeles"
#endif
#define JAVA_EXPORT_NAME2(name,package) Java_##package##_##name
#define JAVA_EXPORT_NAME1(name,package) JAVA_EXPORT_NAME2(name,package)
#define JAVA_EXPORT_NAME(name) JAVA_EXPORT_NAME1(name,SDL_JAVA_PACKAGE_PATH)

JNIEXPORT jint JNICALL JAVA_EXPORT_NAME(ONScripter_nativeInitJavaCallbacks) (JNIEnv * jniEnv, jobject thiz)
{
    JavaONScripter = jniEnv->NewGlobalRef(thiz);
    jclass JavaONScripterClass = jniEnv->GetObjectClass(JavaONScripter);
    JavaPlayVideo = jniEnv->GetMethodID(JavaONScripterClass, "playVideo", "([B)V");
    JavaGetFD = jniEnv->GetMethodID(JavaONScripterClass, "getFD", "([BI)I");
    JavaMkdir = jniEnv->GetMethodID(JavaONScripterClass, "mkdir", "([B)I");
    JavaBridgeEvent = jniEnv->GetMethodID(JavaONScripterClass, "onBridgeEvent", "([B)V"); // [ONS-BRIDGE]
    JavaShowInput = jniEnv->GetMethodID(JavaONScripterClass, "showInputDialog",
                                        "(Ljava/lang/String;Ljava/lang/String;I)V"); // [ONS-INPUT]
    return 0;
}

JNIEXPORT jint JNICALL 
JAVA_EXPORT_NAME(ONScripter_nativeGetWidth) ( JNIEnv*  env, jobject thiz )
{
    return ons.getWidth();
}

JNIEXPORT jint JNICALL
JAVA_EXPORT_NAME(ONScripter_nativeGetHeight) ( JNIEnv*  env, jobject thiz )
{
    return ons.getHeight();
}

} // extern "C" — SDL JNI entry points above

// ============================================================================
// [ONS-BRIDGE] Android extraction bridge
// - EmitAndroid: JSON event (dialogue/sound) -> Java onBridgeEvent(byte[])
// - JNI pull wrappers around the wave byte capture / file read exports,
//   so the host can fetch voice bytes for replay/Anki export (same
//   three-stage semantics as the web host).
// ============================================================================
namespace onsbridge {

void EmitAndroid(const char* type, const std::string& payload)
{
    if (!jniVM || !JavaONScripter || !JavaBridgeEvent || !type) return;

    JNIEnv * jniEnv = NULL;
    jniVM->AttachCurrentThread(&jniEnv, NULL);
    if (!jniEnv){
        __android_log_print(ANDROID_LOG_ERROR, "ONS", "EmitAndroid: AttachCurrentThread() failed");
        return;
    }

    std::string json = std::string("{\"type\":\"") + type + "\",\"payload\":" + payload + "}";
    jsize len = (jsize)json.size();
    jbyteArray jba = jniEnv->NewByteArray(len);
    if (jba){
        jniEnv->SetByteArrayRegion(jba, 0, len, (const jbyte*)json.c_str());
        jniEnv->CallVoidMethod(JavaONScripter, JavaBridgeEvent, jba);
        jniEnv->DeleteLocalRef(jba);
    }
    if (jniEnv->ExceptionCheck()) jniEnv->ExceptionClear();
}

} // namespace onsbridge

// ============================================================================
// [ONS-INPUT] Android implementation of ONSInput.h.
// JNI -> ONScripter.java#showInputDialog -> AlertDialog + EditText.
// The SDL thread blocks here until the Java dialog calls nativeOnInputResult().
// Web/desktop implementations live in ons_input_web.cpp / ONSInput.cpp.
// ============================================================================
#if defined(ANDROID)

namespace onsbridge {

std::mutex g_inputMutex;
std::condition_variable g_inputCv;
bool g_inputDone = false;
bool g_inputCancelled = false;
std::string g_inputText;

} // namespace onsbridge

extern "C" int ons_host_request_input(const char* prompt,
                                      const char* initial,
                                      int maxlen,
                                      int widechar,
                                      char* out,
                                      int out_size)
{
    (void)widechar;
    if (!out || out_size <= 0) return 0;
    out[0] = '\0';
    if (!jniVM || !JavaONScripter || !JavaShowInput) return 0;

    __android_log_print(ANDROID_LOG_INFO, "ONS-INPUT",
                        "request maxlen=%d", maxlen);

    JNIEnv* jniEnv = NULL;
    jniVM->AttachCurrentThread(&jniEnv, NULL);
    if (!jniEnv) {
        __android_log_print(ANDROID_LOG_ERROR, "ONS",
                            "ons_host_request_input: AttachCurrentThread() failed");
        return 0;
    }

    jstring jprompt = jniEnv->NewStringUTF(prompt ? prompt : "");
    jstring jinitial = jniEnv->NewStringUTF(initial ? initial : "");
    if (!jprompt || !jinitial) {
        if (jprompt) jniEnv->DeleteLocalRef(jprompt);
        if (jinitial) jniEnv->DeleteLocalRef(jinitial);
        return 0;
    }

    {
        std::lock_guard<std::mutex> lk(onsbridge::g_inputMutex);
        onsbridge::g_inputDone = false;
        onsbridge::g_inputCancelled = false;
        onsbridge::g_inputText = initial ? initial : "";
    }

    jniEnv->CallVoidMethod(JavaONScripter, JavaShowInput, jprompt, jinitial,
                           (jint)maxlen);
    jniEnv->DeleteLocalRef(jprompt);
    jniEnv->DeleteLocalRef(jinitial);
    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
        return 0;
    }

    std::unique_lock<std::mutex> lk(onsbridge::g_inputMutex);
    if (!onsbridge::g_inputCv.wait_for(lk, std::chrono::seconds(60),
                                       [] { return onsbridge::g_inputDone; })) {
        __android_log_print(ANDROID_LOG_WARN, "ONS",
                            "ons_host_request_input: timeout waiting for host dialog");
        return 0;
    }

    std::strncpy(out, onsbridge::g_inputText.c_str(), (size_t)out_size - 1);
    out[out_size - 1] = '\0';
    __android_log_print(ANDROID_LOG_INFO, "ONS-INPUT",
                        "result confirmed=%d len=%zu",
                        onsbridge::g_inputCancelled ? 0 : 1,
                        std::strlen(out));
    return onsbridge::g_inputCancelled ? 0 : 1;
}

extern "C" JNIEXPORT void JNICALL
JAVA_EXPORT_NAME(ONScripter_nativeOnInputResult)(JNIEnv* env, jobject,
                                                 jstring text)
{
    std::lock_guard<std::mutex> lk(onsbridge::g_inputMutex);
    onsbridge::g_inputCancelled = (text == nullptr);
    if (text) {
        const char* s = env->GetStringUTFChars(text, nullptr);
        onsbridge::g_inputText = s ? s : "";
        if (s) env->ReleaseStringUTFChars(text, s);
    }
    onsbridge::g_inputDone = true;
    onsbridge::g_inputCv.notify_all();
}
#endif // ANDROID

extern "C" {
// byte-capture / file-read exports implemented in ONScripter_sound.cpp /
// ONScripter_command.cpp
int ons_wave_size();
void* ons_wave_buf();
const char* ons_wave_name();
void ons_wave_drop();
int ons_read_file( const char* name, int name_len, int* out_size );
void ons_free_buf( int ptr );

static jbyteArray newByteArrayFromNative(JNIEnv * env, const void* data, jint len)
{
    if (!env || !data || len <= 0) return NULL;
    jbyteArray jba = env->NewByteArray(len);
    if (jba) env->SetByteArrayRegion(jba, 0, len, (const jbyte*)data);
    return jba;
}

JNIEXPORT jint JNICALL
JAVA_EXPORT_NAME(ONScripter_nativeOnsWaveSize) ( JNIEnv* env, jobject thiz )
{
    return ons_wave_size();
}

JNIEXPORT jbyteArray JNICALL
JAVA_EXPORT_NAME(ONScripter_nativeOnsWaveName) ( JNIEnv* env, jobject thiz )
{
    const char* nm = ons_wave_name();
    if (!nm || !nm[0]) return NULL;
    return newByteArrayFromNative(env, nm, (jint)strlen(nm));
}

JNIEXPORT jbyteArray JNICALL
JAVA_EXPORT_NAME(ONScripter_nativeOnsWaveBytes) ( JNIEnv* env, jobject thiz )
{
    jint sz = (jint)ons_wave_size();
    void* buf = ons_wave_buf();
    if (sz <= 0 || !buf) return NULL;
    return newByteArrayFromNative(env, buf, sz);
}

JNIEXPORT void JNICALL
JAVA_EXPORT_NAME(ONScripter_nativeOnsWaveDrop) ( JNIEnv* env, jobject thiz )
{
    ons_wave_drop();
}

// read file bytes by storage name (archive-aware, for replay/export of a
// previously paired voice after the live capture buffer was replaced)
JNIEXPORT jbyteArray JNICALL
JAVA_EXPORT_NAME(ONScripter_nativeOnsReadFile) ( JNIEnv* env, jobject thiz, jbyteArray jname )
{
    if (!env || !jname) return NULL;
    jsize len = env->GetArrayLength(jname);
    if (len <= 0 || len > 1024) return NULL;
    char name[1025];
    env->GetByteArrayRegion(jname, 0, len, (jbyte*)name);
    name[len] = 0;

    int size = 0;
    int ptr = ons_read_file( name, len, &size );
    if (!ptr || size <= 0) return NULL;
    jbyteArray jba = newByteArrayFromNative(env, (const void*)(intptr_t)ptr, (jint)size);
    ons_free_buf( ptr );
    return jba;
}

void playVideoAndroid(const char *path)
{
    JNIEnv * jniEnv = NULL;
    jniVM->AttachCurrentThread(&jniEnv, NULL);

    if (!jniEnv){
        __android_log_print(ANDROID_LOG_ERROR, "ONS", "ONScripter::playVideoAndroid: Java VM AttachCurrentThread() failed");
        return;
    }

    jbyte *jb = new jbyte[strlen(path)];
    for (int i=0;i<strlen(path);i++) jb[i] = path[i];
    jbyteArray jba = jniEnv->NewByteArray(strlen(path));
    jniEnv->SetByteArrayRegion(jba, 0, strlen(path), jb);
    jniEnv->CallVoidMethod(JavaONScripter, JavaPlayVideo, jba);
    jniEnv->DeleteLocalRef(jba);
    delete[] jb;
}

#include <sys/stat.h>
int stat_ons(const char *path, struct stat * statbuf){
    int res = stat(path, statbuf);
    if(res!=-1 ) return res;

    JNIEnv * jniEnv = NULL;
    jniVM->AttachCurrentThread(&jniEnv, NULL);
    jbyte *jb = new jbyte[strlen(path)];
    for (int i=0;i<strlen(path);i++) jb[i] = path[i];
    jbyteArray jba = jniEnv->NewByteArray(strlen(path));
    jniEnv->SetByteArrayRegion(jba, 0, strlen(path), jb);
    int fd = jniEnv->CallIntMethod(JavaONScripter, JavaGetFD, jba, 0);
    jniEnv->DeleteLocalRef(jba);
    delete[] jb;
    res = fstat(fd, statbuf);
    close(fd);
    return res;
}

#undef fopen
FILE *fopen_ons(const char *path, const char *mode)
{
    int mode2 = 0;
    if (mode[0] == 'w') mode2 = 1;

    FILE *fp = fopen(path, mode);
    // __android_log_print(ANDROID_LOG_INFO, "## onsyuri",  "fopen_ons %s, %s, %p", path, mode, fp);

    if (fp) return fp;
    
    JNIEnv * jniEnv = NULL;
    jniVM->AttachCurrentThread(&jniEnv, NULL);

    if (!jniEnv){
        __android_log_print(ANDROID_LOG_ERROR, "ONS", "ONScripter::getFD: Java VM AttachCurrentThread() failed");
        return NULL;
    }

    jbyte *jb = new jbyte[strlen(path)];
    for (int i=0;i<strlen(path);i++) jb[i] = path[i];
    jbyteArray jba = jniEnv->NewByteArray(strlen(path));
    jniEnv->SetByteArrayRegion(jba, 0, strlen(path), jb);
    int fd = jniEnv->CallIntMethod(JavaONScripter, JavaGetFD, jba, mode2 );
    jniEnv->DeleteLocalRef(jba);
    delete[] jb;

    return fdopen(fd, mode);
}

#undef mkdir
extern int mkdir(const char *pathname, mode_t mode);
int mkdir_ons(const char *path, mode_t mode)
{
    if (mkdir(path, mode) == 0 || errno != EACCES) return 0;

    JNIEnv * jniEnv = NULL;
    jniVM->AttachCurrentThread(&jniEnv, NULL);

    if (!jniEnv){
        __android_log_print(ANDROID_LOG_ERROR, "ONS", "ONScripter::mkdir: Java VM AttachCurrentThread() failed");
        return -1;
    }

    jbyte *jb = new jbyte[strlen(path)];
    for (int i=0;i<strlen(path);i++) jb[i] = path[i];
    jbyteArray jba = jniEnv->NewByteArray(strlen(path));
    jniEnv->SetByteArrayRegion(jba, 0, strlen(path), jb);
    int ret = jniEnv->CallIntMethod(JavaONScripter, JavaMkdir, jba);
    jniEnv->DeleteLocalRef(jba);
    delete[] jb;

    return ret;
}
}
#endif

#if defined(IOS)
extern "C" void playVideoIOS(const char *filename, bool click_flag, bool loop_flag)
{
    NSString *str = [[NSString alloc] initWithUTF8String:filename];
    id obj = [MoviePlayer alloc];
    [[obj init] play:str click : click_flag loop : loop_flag];
    [obj release];
}
#endif

#if defined(WEB)
#include <emscripten.h>

#undef fopen

/*
* fetch the file from server beforce fopen for lazyload
* value: g_onsyuri_module, g_onsyuri_index, g_onsyuri_filemap
* function: fetch_file
*/
FILE *fopen_ons(const char *path, const char *mode)
{
    // printf("## fopen_ons %s, ", path);
    static int use_lazyload = -1;
    if(use_lazyload==-1)
    {
        use_lazyload = EM_ASM_INT(
            // check enviroment
            if (!g_onsyuri_module) return 0;
            if (!g_onsyuri_index) return 0;
            if (!fetch_file) return 0;

            // check lazyload flag with filemap
            if (!g_onsyuri_filemap) return 0;
            if (Object.keys(g_onsyuri_filemap).length==0) return 0;
            return 1;
        );
    }
    
    FILE *fp = fopen(path, mode);
    if(!fp && use_lazyload==1 && (strcmp(mode, "r") || strcmp(mode, "rb")))
    {   
        int ret = 0;
        ret = EM_ASM_INT( // path is combined after --gamedir
            var path = g_onsyuri_module.UTF8ToString($0);
            var key = path.toLowerCase(); // pay attention to the case sensitive of web
            if(!g_onsyuri_filemap[key]) return 0;
            if(g_onsyuri_filemap[key].loaded) return 2;
            try {
                fetch_file(g_onsyuri_module.FS, key, g_onsyuri_filemap);
                return 1;
            } 
            catch(e) { 
                console.log("## fopen_ons web lazyload ${path} error: ", e);
                return 0;
            }, path);
        switch (ret)
        {
            case 0: 
            {
                // printf(" not found in g_onsyuri_filemap !\n");
                return fp; // use fopen to check for other file mount
                break;
            }
            case 1:
            {
                // printf(" waiting for fetch...\n");
                break;
            }
            case 2:
            {
                // printf(" already loaded!\n");
                return fp;
                break;
            }
        }

        while(!EM_ASM_INT(
            var path = g_onsyuri_module.UTF8ToString($0);
            var key = path.toLowerCase();
            return g_onsyuri_filemap[key].loaded;, path))
        {
            SDL_Delay(5); // wait for async function
        }
        fp = fopen(path, mode); // reload after fetch
    }
    return fp;
}

extern "C" void playVideoWeb(const char *path, bool click_flag, bool loop_flag)
{
    EM_ASM(
        var path = g_onsyuri_module.UTF8ToString($0);
        playVideo(path, $1, $2);
    ,path,click_flag,loop_flag);

    while(EM_ASM_INT(return g_onsyuri_module.wait_video;))
    {
        SDL_Delay(5);
    }
}
#endif

void parseOption(int argc, char *argv[]) {
    while (argc > 0) {
        if ( argv[0][0] == '-' ){
            // version, help
            if ( !strcmp( argv[0]+1, "h" ) || !strcmp( argv[0]+1, "-help" ) ){
                optionHelp();
            }
            else if ( !strcmp( argv[0]+1, "v" ) || !strcmp( argv[0]+1, "-version" ) ){
                optionVersion();
            }

            // load options
            else if ( !strcmp( argv[0]+1, "f" ) || !strcmp( argv[0]+1, "-font" ) ){
                argc--;
                argv++;
                ons.setFontFile(argv[0]);
            }
            else if ( !strcmp( argv[0]+1, "r" ) || !strcmp( argv[0]+1, "-root" ) ){
                argc--;
                argv++;
                ons.setArchivePath(argv[0]);
            }
            else if ( !strcmp(argv[0]+1, "-save-dir") ){
                argc--;
                argv++;
                ons.setSaveDir(argv[0]);
            }
            else if (!strcmp(argv[0]+1, "-debug:1")){
                ons.setDebugLevel(1);
            }
            else if (!strcmp(argv[0]+1, "-enc:sjis")){
                if(!coding2utf16) coding2utf16 = new SJIS2UTF16();
            }
            else if (!strcmp(argv[0]+1, "-enc:gbk")){
                if(!coding2utf16) coding2utf16 = new GBK2UTF16();
            }
            else if (!strcmp(argv[0]+1, "-enc:utf8")){
                if(!coding2utf16) coding2utf16 = new GBK2UTF16();
                coding2utf16->force_utf8 = true;
            }

            // render options
            else if ( !strcmp( argv[0]+1, "-window" ) ){
                ons.setWindowMode();
            }
            else if ( !strcmp( argv[0]+1, "-width" ) ){
                argc--;
                argv++;
                ons.setWindowWidth(atoi(argv[0]));
            }
            else if ( !strcmp( argv[0]+1, "-height" ) ){
                argc--;
                argv++;
                ons.setWindowHeight(atoi(argv[0]));
            }
            else if ( !strcmp( argv[0]+1, "-fullscreen" ) ){
                ons.setFullscreenMode(1);
            }
            else if ( !strcmp( argv[0]+1, "-fullscreen2" ) ){
                ons.setFullscreenMode(2);
            }
            else if ( !strcmp( argv[0]+1, "-sharpness" ) ){
                argc--;
                argv++;
                ons.setSharpness(atof(argv[0]));
            }
            else if (!strcmp(argv[0]+1, "-no-video")){
			    ons.setVideoOff();
			}
            else if (!strcmp(argv[0]+1, "-no-vsync")){
			    ons.setVsyncOff();
			}

            // other options
            else if ( !strcmp( argv[0]+1, "-cdaudio" ) ){
                ons.enableCDAudio();
            }
            else if ( !strcmp( argv[0]+1, "-cdnumber" ) ){
                argc--;
                argv++;
                ons.setCDNumber(atoi(argv[0]));
            }
            else if ( !strcmp( argv[0]+1, "-registry" ) ){
                argc--;
                argv++;
                ons.setRegistryFile(argv[0]);
            }
            else if ( !strcmp( argv[0]+1, "-dll" ) ){
                argc--;
                argv++;
                ons.setDLLFile(argv[0]);
            }
            else if ( !strcmp( argv[0]+1, "-force-button-shortcut" ) ){
                ons.enableButtonShortCut();
            }
            else if ( !strcmp( argv[0]+1, "-enable-wheeldown-advance" ) ){
                ons.enableWheelDownAdvance();
            }
            else if ( !strcmp( argv[0]+1, "-disable-rescale" ) ){
                ons.disableRescale();
            }
            else if ( !strcmp( argv[0]+1, "-render-font-outline" ) ){
                ons.renderFontOutline();
            }
            else if ( !strcmp( argv[0]+1, "-edit" ) ){
                ons.enableEdit();
            }
            else if ( !strcmp( argv[0]+1, "-key-exe" ) ){
                argc--;
                argv++;
                ons.setKeyEXE(argv[0]);
            }
            else if (!strcmp(argv[0]+1, "-fontcache")){
                ons.setFontCache();
            }
            else{
                utils::printInfo(" unknown option %s\n", argv[0]);
            }
        }
        else{
            optionHelp();
        }
        argc--;
        argv++;
    }
}

int main(int argc, char *argv[])
{
#if defined(PSP)
    ons.disableRescale();
    ons.enableButtonShortCut();
    SetupCallbacks();
#elif defined(WINRT)
    {
        ScriptSelector ss;
        ons.setArchivePath(ss.selectedPath.c_str());
    }
    ons.disableRescale();
#elif defined(ANDROID)
    ons.enableButtonShortCut();
#endif

#if defined(IOS)
#if defined(HAVE_CONTENTS)
    if ([[[DataCopier alloc] init] copy]) exit(-1);
#endif

    // scripts and archives are stored under /Library/Caches
    NSArray* cpaths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString* cpath = [[cpaths objectAtIndex : 0] stringByAppendingPathComponent:@"ONS"];
    char filename[256];
    strcpy(filename, [cpath UTF8String]);
    ons.setArchivePath(filename);

    // output files are stored under /Documents
    NSArray* dpaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* dpath = [[dpaths objectAtIndex : 0] stringByAppendingPathComponent:@"ONS"];
    strcpy(filename, [dpath UTF8String]);
    ons.setSaveDir(filename);

#if defined(ZIP_URL)
    if ([[[DataDownloader alloc] init] download]) exit(-1);
#endif

#if defined(USE_SELECTOR)
    // scripts and archives are stored under /Library/Caches
    cpath = [[[ScriptSelector alloc] initWithStyle:UITableViewStylePlain] select];
    strcpy(filename, [cpath UTF8String]);
    ons.setArchivePath(filename);

    // output files are stored under /Documents
    dpath = [[dpaths objectAtIndex : 0] stringByAppendingPathComponent:[cpath lastPathComponent]];
    NSFileManager *fm = [NSFileManager defaultManager];
    [fm createDirectoryAtPath : dpath withIntermediateDirectories : YES attributes : nil error : nil];
    strcpy(filename, [dpath UTF8String]);
    ons.setSaveDir(filename);
#endif

#if defined(RENDER_FONT_OUTLINE)
    ons.renderFontOutline();
#endif
#endif

    // ----------------------------------------
    // Parse options
    argv++;
    parseOption(argc - 1, argv);

    utils::printInfo("ONScripter Yuri %s, (Jh%s,Ons%s,NSC%d.%02d)\n", 
        ONS_YURI_VERSION, ONS_JH_VERSION, 
        ONS_VERSION, NSC_VERSION / 100, NSC_VERSION % 100);

    const char *argfilename = "ons_args";
    FILE *fp = NULL;
    if (ons.getArchivePath()) {
        size_t len = strlen(ons.getArchivePath()) + strlen(argfilename) + 1;
        char *full_path = new char[len];
        sprintf(full_path, "%s%s", ons.getArchivePath(), argfilename);
        fp = fopen(full_path, "r");
        delete[] full_path;
    }
    else fp = fopen(argfilename, "r");
    if (fp) {
        char **args = new char*[16];
        int argn = 0;
        args[argn] = new char[64];
        while (argn < 16 && (fscanf(fp, "%s", args[argn]) > 0)) {
            ++argn;
            if (argn < 16) args[argn] = new char[64];
        }
        parseOption(argn, args);
        for (int i = 0; i < argn; ++i) delete[] args[i];
        delete[] args;
    }

    if (coding2utf16 == NULL) coding2utf16 = new GBK2UTF16();

    // ----------------------------------------
    // Run ONScripter
    if (ons.openScript()) exit(-1);
    if (ons.init()) exit(-1);
#if defined(WEB)
    EM_ASM(
        self.postMessage("onsinit");
    );
#endif
    ons.executeLabel();
    exit(0);
}
