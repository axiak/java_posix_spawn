#include <stdlib.h>
#include <jni.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <spawn.h>
#include <fcntl.h>
#include "spawnlib.h"

#define THROW_IO_EXCEPTION(cls, env) do { \
    cls = (*env)->FindClass(env, "java/io/IOException");    \
    (*env)->ThrowNew(env, cls, sys_errlist[errno]); \
} while (0)


char ** javaArrayToChar(JNIEnv * env, jobject array);
void inline releaseCharArray(JNIEnv * env, jobject javaArray, char ** cArray);
jobjectArray fdIntToObject(JNIEnv * env, int * fds, size_t length);
char ** createPrependedArgv(char * path, char * chdir, char ** argv, int length, int * fds);
void freePargv(char ** pargv);

/*
 * Class:     SpawnProcess
 * Method:    exec_process
 * Signature: ([Ljava/lang/String;[Ljava/lang/String;)Lrunutils/SpawnProcess$SpawnedProcess;
 */
JNIEXPORT jobject JNICALL Java_net_axiak_runutils_SpawnProcess_exec_1process
(JNIEnv * env, jclass clazz, jobjectArray cmdarray, jobjectArray envp, jstring chdir)
{
    int cpid, retval, length;
    jboolean iscopy;
    char ** argv = NULL, ** c_envp = NULL, ** prepended_argv = NULL, *tmp;
    jobjectArray ret = NULL;
    jstring program_name;
    jmethodID mid;
    jclass cls;
    char path[50] = "binrunner";
    int fds[3] = {1, 0, 2};
    int pipe_fd1[2], pipe_fd2[2], pipe_fd3[2];
    jobjectArray fdResult;

    if (pipe(pipe_fd1) != 0) {
        THROW_IO_EXCEPTION(cls, env);
        goto end;
    }
    if (pipe(pipe_fd2) != 0) {
        THROW_IO_EXCEPTION(cls, env);
        goto end;
    }
    if (pipe(pipe_fd3) != 0) {
        THROW_IO_EXCEPTION(cls, env);
        goto end;
    }

    length = (*env)->GetArrayLength(env, cmdarray);
    if (!length) {
        cls = (*env)->FindClass(env, "java/lang/IndexOutOfBoundsException");
        (*env)->ThrowNew(env, cls, "A non empty cmdarray is required.");
        goto end;
    }

    if ((argv = javaArrayToChar(env, cmdarray)) == NULL) {
        goto end;
    }
    program_name = (jstring)(*env)->GetObjectArrayElement(env, cmdarray, 0);
    if ((c_envp = javaArrayToChar(env, envp)) == NULL) {
        goto end;
    }

    /* Mapping for client to pipe. */
    fds[0] = pipe_fd1[0];
    fds[1] = pipe_fd2[1];
    fds[2] = pipe_fd3[1];

    /* Get the cwd */
    tmp = (char *)(*env)->GetStringUTFChars(env, chdir, &iscopy);

    prepended_argv = createPrependedArgv(path, tmp, argv, length, fds);

    /* This is the call to spawn! */
    retval = posix_spawnp(&cpid, path, NULL, NULL, prepended_argv, c_envp);

    (*env)->ReleaseStringChars(env, chdir, tmp);

    if (retval != 0) {
        THROW_IO_EXCEPTION(cls, env);
        goto end;
    }

    cls = (*env)->FindClass(env, "net/axiak/runutils/SpawnProcess$SpawnedProcess");
    if (cls == 0) {
        goto end;
    }

    /* Mapping for parent to pipe. */
    fds[0] = pipe_fd1[1];
    fds[1] = pipe_fd2[0];
    fds[2] = pipe_fd3[0];

    fdResult = fdIntToObject(env, fds, 3);
    if (fdResult == NULL) {
        goto end;
    }

    mid = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/lang/String;I[Ljava/io/FileDescriptor;)V");
    if (mid == 0) {
        goto end;
    }

    (*env)->ExceptionClear(env);
    ret = (*env)->NewObject(env, cls, mid, program_name, cpid, fdResult);

 end:
    /* Here we make sure we are good memory citizens. */
    freePargv(prepended_argv);
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
JNIEXPORT void JNICALL Java_net_axiak_runutils_SpawnProcess_killProcess
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
JNIEXPORT jint JNICALL Java_net_axiak_runutils_SpawnProcess_waitForProcess
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
    if (result == NULL) {
        return result;
    }

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


jobjectArray fdIntToObject(JNIEnv * env, int * fds, size_t length)
{
    jclass clazz = (*env)->FindClass(env, "java/io/FileDescriptor");
    jobjectArray result;
    jmethodID fdesc;
    jfieldID field_fd;
    int i;

    if (clazz == 0) {
        return NULL;
    }

    result = (*env)->NewObjectArray(env, length, clazz, NULL);
    if (result == NULL) {
        return NULL;
    }

    fdesc = (*env)->GetMethodID(env, clazz, "<init>", "()V");
    if (fdesc == 0) {
        return NULL;
    }

    field_fd = (*env)->GetFieldID(env, clazz, "fd", "I");
    if (field_fd == 0) {
        return NULL;
    }

    for (i = 0; i < length; i++) {
        jobject tmp = (*env)->NewObject(env, clazz, fdesc);
        (*env)->SetIntField(env, tmp, field_fd, fds[i]);
        (*env)->SetObjectArrayElement(env, result, i, tmp);
    }

    return result;
}

char ** createPrependedArgv(char * path, char * chdir, char ** argv, int length, int * fds)
{
    char ** pargv = (char **)malloc(sizeof(char *) * (length + 6));
    int i;

    if (pargv == NULL) {
        return NULL;
    }

    for (i = 1; i < (length + 6); i++) {
        pargv[i] = NULL;
    }
    pargv[0] = path;
    pargv[4] = chdir;


    /* Set fds */
    for (i = 0; i < 3; i++) {
        pargv[i + 1] = (char *)malloc(3 * fds[i]);
        if (pargv[i + 1] == NULL) {
            goto error;
        }
        sprintf(pargv[i + 1], "%d", fds[i]);
    }

    for (i = 0; i < length; i++) {
        pargv[i + 5] = argv[i];
    }

    return pargv;

 error:
    freePargv(pargv);
    return NULL;
}

void freePargv(char ** pargv)
{
    int i;
    if (pargv != NULL) {
        for (i = 1; i < 4; i++) {
            if (pargv[i] != NULL) {
                free(pargv[i]);
            }
        }
        free(pargv);
    }
}
