#ifndef THREAD_H
#define THREAD_H

struct thread_arg {
    render_global *global;
    int32_t cam_id;
    size_t width;
    size_t height;
    bool schain;
    volatile bool exit;
};

HANDLE thread_create(uint32_t, render_global *, size_t, size_t);
void thread_stop(HANDLE, uint32_t);

#endif  /* !THREAD_H */
