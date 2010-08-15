#include <stdlib.h>
#include <jni.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <spawn.h>

char ** javaArrayToChar(JNIEnv * env, jobject array);
void inline releaseCharArray(JNIEnv * env, jobject javaArray, char ** cArray);

/*
 * Class:     SpawnProcess
 * Method:    exec
 * Signature: ([Ljava/lang/String;[Ljava/lang/String;)LSpawnedProcess;
 */
JNIEXPORT jobject JNICALL Java_SpawnProcess_exec
  (JNIEnv * env, jclass clazz, jobjectArray cmdarray, jobjectArray envp)
{
    int i, cpid, retval, length;
    jboolean iscopy;
    char ** argv = NULL, ** c_envp = NULL;
    jobjectArray ret = NULL;
    jstring tmp;
    jstring program_name;
    jmethodID mid;
    jclass cls;
    char * path;

    length = (*env)->GetArrayLength(env, cmdarray);
    if (!length) {
        cls = (*env)->FindClass(env, "java/lang/IndexOutOfBoundsException");
        (*env)->ThrowNew(env, cls, "A non empty cmdarray is required.");
        goto end;
    }

    argv = javaArrayToChar(env, cmdarray);
    path = argv[0];
    program_name = (jstring)(*env)->GetObjectArrayElement(env, cmdarray, 0);
    c_envp = javaArrayToChar(env, envp);

    /* This is the call to spawn! */
    retval = posix_spawnp(&cpid, path, NULL, NULL, argv, c_envp);

    if (retval != 0) {
        cls = (*env)->FindClass(env, "java/io/IOException");
        (*env)->ThrowNew(env, cls, sys_errlist[errno]);
        goto end;
    }

    cls = (*env)->FindClass(env, "SpawnProcess$SpawnedProcess");
    if (cls == 0) {
        goto end;
    }

    mid = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/lang/String;I)V");
    if (mid == 0) {
        goto end;
    }

    (*env)->ExceptionClear(env);
    ret = (*env)->NewObject(env, cls, mid, program_name, cpid);

 end:
    /* Here we make sure we are good memory citizens. */
    if (argv != NULL)
        releaseCharArray(env, cmdarray, argv);
    if (envp != NULL)
        releaseCharArray(env, envp, c_envp);
    return ret;
}

/*
 * Class:     SpawnProcess
 * Method:    killProcess
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_SpawnProcess_killProcess
  (JNIEnv * env, jclass clazz, jint pid)
{
    kill(pid, 2);
    kill(pid, 5);
    kill(pid, 9);
}

/*
 * Class:     SpawnProcess
 * Method:    waitForProcess
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_SpawnProcess_waitForProcess
  (JNIEnv * env, jclass clazz, jint pid)
{
    /* Read http://www.opengroup.org/onlinepubs/000095399/functions/wait.html */
    int stat_loc;
    errno = 0;
    int retcode = waitpid(pid, &stat_loc, 0);
    if (WIFEXITED(stat_loc) == 0) {
        return -500;
    }
    return WEXITSTATUS(stat_loc);
}


char ** javaArrayToChar(JNIEnv * env, jobject array)
{
    int i, length = (*env)->GetArrayLength(env, array);
    char ** result = (char **)malloc(sizeof(char *) * (length + 1));
    jboolean iscopy;
    jstring tmp;

    result[length] = NULL;
    for (i = 0; i < length; i++) {
        tmp = (jstring)(*env)->GetObjectArrayElement(env, array, i);
        result[i] = (char *)(*env)->GetStringUTFChars(env, tmp, &iscopy);
    }
    return result;
}

void inline releaseCharArray(JNIEnv * env, jobject javaArray, char ** cArray)
{
    int i, length =  (*env)->GetArrayLength(env, javaArray);

    for (i = 0; i < length; i++) {
        (*env)->ReleaseStringChars(env,
                                   (jstring)(*env)->GetObjectArrayElement(env, javaArray, i),
                                   (jchar *)cArray[i]);
    }
    free(cArray);
}
