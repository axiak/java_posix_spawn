/* vi: set sw=4 ts=4: */
#include <stdlib.h>
#include <jni.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <spawn.h>
#include <fcntl.h>
#include "jlinuxfork.h"

extern char ** environ;

#define THROW_IO_EXCEPTION(cls, env) do { \
    cls = (*env)->FindClass(env, "java/io/IOException"); \
    (*env)->ThrowNew(env, cls, sys_errlist[errno]); \
} while (0)

#define MAX_BUFFER_SIZE 131071

char ** javaArrayToChar(JNIEnv * env, jobject array);
void inline releaseCharArray(JNIEnv * env, jobject javaArray, char ** cArray);
jobjectArray fdIntToObject(JNIEnv * env, int * fds, size_t length);
char ** createPrependedArgv(char * path, char * chdir, char ** argv, int length, int * fds);
void freePargv(char ** pargv);
int isExecutable(char * path);
static void closeSafely(int fd);

/*
 * Class:     com_crunchtime_utils_runtime_SpawnedProcess
 * Method:    execProcess
 * Signature: ([Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/io/FileDescriptor;Ljava/io/FileDescriptor;Ljava/io/FileDescriptor;)I
 */
JNIEXPORT jint JNICALL Java_com_crunchtime_utils_runtime_SpawnedProcess_execProcess
(JNIEnv * env, jclass clazz, jobjectArray cmdarray, jobjectArray envp, jstring chdir,
 jstring jbinrunner, jobject stdin_fd, jobject stdout_fd, jobject stderr_fd)
{
    int cpid = -1, length, i, total_buffer_size = 0;
    jboolean iscopy;
    char ** argv = NULL, ** c_envp = NULL, ** prepended_argv = NULL, *tmp;
    jstring program_name;
    jfieldID fid;
    jclass cls;
    char *path;
    int fds[3] = {1, 0, 2};
    int pipe_fd1[2], pipe_fd2[2], pipe_fd3[2];
    jobjectArray fdResult;
    pipe_fd1[0] = pipe_fd2[1] = pipe_fd2[0] = pipe_fd2[1] = pipe_fd3[0] = pipe_fd3[1] = -1;

    path = (char *)(*env)->GetStringUTFChars(env, jbinrunner, &iscopy);
    if (path == NULL) {
        goto Catch;
    }

    if (pipe(pipe_fd1) != 0) {
        THROW_IO_EXCEPTION(cls, env);
        goto Catch;
    }
    if (pipe(pipe_fd2) != 0) {
        THROW_IO_EXCEPTION(cls, env);
        goto Catch;
    }
    if (pipe(pipe_fd3) != 0) {
        THROW_IO_EXCEPTION(cls, env);
        goto Catch;
    }

    length = (*env)->GetArrayLength(env, cmdarray);
    if (!length) {
        cls = (*env)->FindClass(env, "java/lang/IndexOutOfBoundsException");
        (*env)->ThrowNew(env, cls, "A non empty cmdarray is required.");
        goto Catch;
    }

    if ((argv = javaArrayToChar(env, cmdarray)) == NULL) {
        goto Catch;
    }
    program_name = (jstring)(*env)->GetObjectArrayElement(env, cmdarray, 0);
    if (envp == NULL) {
        c_envp = environ;
    } else if ((c_envp = javaArrayToChar(env, envp)) == NULL) {
        goto Catch;
    }

    /* Mapping for client to pipe. */
    fds[0] = pipe_fd1[0];
    fds[1] = pipe_fd2[1];
    fds[2] = pipe_fd3[1];

    /* Get the cwd */
    tmp = (char *)(*env)->GetStringUTFChars(env, chdir, &iscopy);

    prepended_argv = createPrependedArgv(path, tmp, argv, length, fds);

    if (prepended_argv == NULL) {
        goto Catch;
    }

    for (i = 0; prepended_argv[i] != NULL; ++i) {
        total_buffer_size += strlen(prepended_argv[i]) + 1;
    }
    for (i = 0; c_envp[i] != NULL; ++i) {
        total_buffer_size += strlen(c_envp[i]) + 1;
    }

    if (total_buffer_size > MAX_BUFFER_SIZE) {
        cls = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
        (*env)->ThrowNew(env, cls, "The environment and arguments combined require too much space.");
        goto Catch;
    }

    cpid = vfork();
    if (cpid == 0) {
        if (execve(path, prepended_argv, c_envp) == -1) {
            fprintf(stderr, "execve error: %s\n", strerror(errno));
        }
        _exit(-1);
    } else if (cpid < 0) {
        THROW_IO_EXCEPTION(cls, env);
        goto Catch;
    }

    (*env)->ReleaseStringChars(env, chdir, (const jchar *)tmp);

    /* Mapping for parent to pipe. */
    fds[0] = pipe_fd1[1];
    fds[1] = pipe_fd2[0];
    fds[2] = pipe_fd3[0];

    cls = (*env)->FindClass(env, "java/io/FileDescriptor");
    if (cls == 0) {
        goto Catch;
    }

    fid = (*env)->GetFieldID(env, cls, "fd", "I");

    (*env)->SetIntField(env, stdin_fd, fid, fds[0]);
    (*env)->SetIntField(env, stdout_fd, fid, fds[1]);
    (*env)->SetIntField(env, stderr_fd, fid, fds[2]);

    fdResult = fdIntToObject(env, fds, 3);
    if (fdResult == NULL) {
        goto Catch;
    }

    (*env)->ExceptionClear(env);
 Finally:
    closeSafely(pipe_fd1[0]);
    closeSafely(pipe_fd2[1]);
    closeSafely(pipe_fd3[1]);

    /* Here we make sure we are good memory citizens. */
    freePargv(prepended_argv);
    if (argv != NULL)
        releaseCharArray(env, cmdarray, argv);
    if (envp != NULL)
        releaseCharArray(env, envp, c_envp);
    if (path != NULL)
        (*env)->ReleaseStringChars(env, jbinrunner, (jchar *)path);
    return cpid;
 Catch:
    closeSafely(fds[0]);
    closeSafely(fds[1]);
    closeSafely(fds[2]);
    goto Finally;
}

/*
 * Class:     SpawnedProcess
 * Method:    killProcess
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_crunchtime_utils_runtime_SpawnedProcess_killProcess
  (JNIEnv * env, jclass clazz, jint pid)
{
    kill(pid, 2);
    kill(pid, 5);
    kill(pid, 9);
}

/*
 * Class:     SpawnedProcess
 * Method:    waitForProcess
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_crunchtime_utils_runtime_SpawnedProcess_waitForProcess
  (JNIEnv * env, jclass clazz, jint pid)
{
    /* Read http://www.opengroup.org/onlinepubs/000095399/functions/wait.html */
    int stat_loc;
    errno = 0;
    while(waitpid(pid, &stat_loc, 0) < 0) {
        switch (errno) {
            case ECHILD:
                return 0;
            case EINTR:
                break;
            default:
                return -1;
        }
    }

    if (WIFEXITED(stat_loc)) {
        return WEXITSTATUS(stat_loc);
    } else if (WIFSIGNALED(stat_loc)) {
        return 0x80 + WTERMSIG(stat_loc);
    } else {
        return stat_loc;
    }
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
    if (cArray == NULL)
        return;

    for (i = 0; i < length; i++) {
        if (cArray[i] != NULL)
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

int _isAbsPathExecutable(char * absolute_path)
{
	struct stat filestat;
	return stat(absolute_path, &filestat) == 0
		&& filestat.st_mode & S_IXUSR;
}


int isExecutable(char * path)
{
	char *path_list, *path_copy, *ptr, *buffer;
	int executable = 0;

	if (_isAbsPathExecutable(path)) {
		return 1;
	}

	path_list = getenv("PATH");
	if (!path_list) {
		path_list = "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin";
	}
	path_copy = (char *)malloc(strlen(path_list) + 1);
	strcpy (path_copy, path_list);
	buffer = (char *)malloc(strlen(path_list) + strlen(path) + 2);

	ptr = strtok(path_copy, ":");
	while (ptr != NULL) {
		strcpy(buffer, ptr);
		strcat(buffer, "/");
		strcat(buffer, path);
		if (_isAbsPathExecutable(buffer)) {
			executable = 1;
			goto isExecutableEnd;
		}
		ptr = strtok(NULL, ":");
	}
isExecutableEnd:
	free(buffer);
	free(path_copy);
	return executable;
}

static void closeSafely(int fd)
{
    if (fd != -1) {
        close(fd);
    }
}

/*
Local Variables:
c-file-style: "linux"
c-basic-offset: 4
tab-width: 4
End:
*/
