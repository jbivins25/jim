#include "data.h"

#ifndef _WIN32
static void* threadEntry(void* p) {
        editorThread* t = (editorThread*)p;
        t->func(t->arg);

	THREAD_LOCK(T.threadLock);
	t->state = THREAD_FINISHED;
	THREAD_UNLOCK(T.threadLock);
        return NULL;
}
#else
static DWORD WINAPI threadEntry(LPVOID p) {
        editorThread* t = (editorThread*)p;
        t->func(t->arg);

	THREAD_LOCK(T.threadLock);
	t->state = THREAD_FINISHED;
	THREAD_UNLOCK(T.threadLock);
        return 0;
}
#endif

int editorThreadCreate(editorThreadFunc func, void* arg) {
	int slot = -1;

	THREAD_LOCK(T.threadLock);

	for (int i = 0; i < MAX_THREADS; i++) {
		if (T.slot[i].state != THREAD_UNUSED) continue;
		

		*((int*)arg) = i; //Thread args, whether custom designed or the default ThreadArgs type, must guarantee that the first member is an int for the thread to hold an id
		T.slot[i].func = func;
		T.slot[i].arg = arg;
		T.slot[i].state = THREAD_ACTIVE;
		T.slot[i].writeEnabled = 1;

		#ifndef _WIN32
		if (!pthread_create(&T.slot[i].handle, NULL, threadEntry, &T.slot[i])) slot = i;
		#else
		T.slot[i].handle = CreateThread(NULL, 0, threadEntry, &T.slot[i], 0, NULL);
		if (T.slot[i].handle != NULL) slot = i;
		#endif

		if (slot == -1) T.slot[i].state = THREAD_UNUSED;
		break;
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

	t->state = THREAD_UNUSED;

	THREAD_UNLOCK(T.threadLock);
	return 0;
}

int editorJoinThread() {
	THREAD_LOCK(T.threadLock);

	for (int i = 0; i < MAX_THREADS; i++) {
		if (T.slot[i].state != THREAD_FINISHED) continue;

		#ifndef _WIN32
		pthread_join(T.slot[i].handle, NULL);
		#else
		WaitForSingleObject(T.slot[i].handle, INFINITE);
		CloseHandle(T.slot[i].handle);
		#endif

		T.slot[i].state = THREAD_UNUSED;
	}

	THREAD_UNLOCK(T.threadLock);
	return 0;
}

void editorThreadLinkWindow(int slot) {
	if (slot < 0 || slot >= MAX_THREADS) return;
	if (!E.win.active) return;
	THREAD_LOCK(T.threadLock);

	T.slot[slot].windowId = E.win.uniqueId;
	E.win.slot = slot;

	THREAD_UNLOCK(T.threadLock);
}
