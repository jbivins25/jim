#include "data.h"

#ifndef _WIN32
static void* threadEntry(void* p) {
        editorThread* t = (editorThread*)p;
        t->func(t->arg);

        return NULL;
}
#else
static DWORD WINAPI threadEntry(LPVOID p) {
        editorThread* t = (editorThread*)p;
        t->func(t->arg);

        return 0;
}
#endif

int editorThreadCreate(editorThreadFunc func, void* arg) {
	int slot = -1;

	THREAD_LOCK(T.threadLock);

	for (int i = 0; i < MAX_THREADS; i++) {
		if (T.slot[i].state != THREAD_UNUSED) continue;
		
		T.slot[i].func = func;
		T.slot[i].arg = arg;

		#ifndef _WIN32
		if (!pthread_create(&T.slot[i].handle, NULL, threadEntry, &T.slot[i])) slot = i;
		#else
		T.slot[i].handle = CreateThread(NULL, 0, threadEntry, &T.slot[i], 0, NULL);
		if (T.slot[i].handle != NULL) slot = i;
		#endif

		if (slot > -1) T.slot[i].state = THREAD_ACTIVE;
	}

	THREAD_UNLOCK(T.threadLock);

	return slot;
}

int editorDetachThread(editorThread* t) {
	THREAD_LOCK(T.threadLock);

	#ifndef _WIN32
	pthread_detach(t->handle);
	#else
	CloseHandle(t->handle);
	#endif

	THREAD_UNLOCK(T.threadLock);
	return 0;
}

int editorJoinThread() {
	THREAD_LOCK(T.threadLock);

	for (int i = 0; i < MAX_THREADS; i++) {
		if (T.slot[i].state != THREAD_FINISHED) continue;

		THREAD_UNLOCK(T.threadLock);

		#ifndef _WIN32
		pthread_join(T.slot[i].handle, NULL);
		#else
		WaitForSingleObject(T.slot[i].handle, INFINITE);
		CloseHandle(T.slot[i].handle);
		#endif

		THREAD_LOCK(T.threadLock);

		T.slot[i].state = THREAD_UNUSED;
	}

	THREAD_UNLOCK(T.threadLock);
	return 0;
}
